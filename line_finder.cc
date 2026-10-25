#include "line_finder.h"

#include <ctype.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <sstream>
#include "cross_hand_summarizer.h"
#include "deal.h"
#include "glog/logging.h"
#include "interface_dds.h"
#include "omp_compat.h"

void FuzzyBoardState::SetHands(const std::vector<uint64_t> &hands,
                               uint64_t unknown_cards) {
  known_hands_ = hands;
  unknown_cards_ = unknown_cards;
}

BoardState FuzzyBoardState::Randomize(bool deal_missing_cards) {
  std::vector<uint64_t> hands = known_hands_;
  if (unknown_cards_ == 0) {
    BoardState bs;
    bs.SetHands(hands);
    return bs;
  }
  if (deal_missing_cards) {
    hands = DealMissingCards(known_hands_, all_hand_reqs_);
  } else {
    int max_cards = 0;
    for (uint64_t h : hands) {
      max_cards = std::max(max_cards, CountCardsInHand(h));
    }
    int unk_cnt = CountCardsInHand(unknown_cards_);
    int hand1_known = CountCardsInHand(hands[1]);
    int hand3_known = CountCardsInHand(hands[3]);
    int target_cards = max_cards;
    if (target_cards == 0 ||
        (hand1_known + hand3_known + unk_cnt) != 2 * target_cards) {
      target_cards = (hand1_known + hand3_known + unk_cnt) / 2;
    }
    int hand1_needed = std::max(0, target_cards - hand1_known);
    std::vector<std::shared_ptr<HandRequirements>> active_reqs1;
    std::vector<std::shared_ptr<HandRequirements>> active_reqs3;
    for (const auto &r : hand_reqs1_) {
      if (r && (target_cards == 13 || r->AppliesToRemainingHand())) {
        active_reqs1.push_back(r);
      }
    }
    for (const auto &r : hand_reqs3_) {
      if (r && (target_cards == 13 || r->AppliesToRemainingHand())) {
        active_reqs3.push_back(r);
      }
    }
    auto [hand1, hand3] =
        DealTwoRequiredHands(unknown_cards_, active_reqs1, active_reqs3,
                             hand1_needed, hands[1], hands[3]);

    hands[1] = hands[1] | hand1;
    hands[3] = hands[3] | hand3;
  }

  BoardState bs;
  bs.SetHands(hands);
  VLOG(1) << "Have hands: "
          << HandDebugString(hands[0], true) << " // "
          << HandDebugString(hands[1], true) << " // "
          << HandDebugString(hands[2], true) << " // "
          << HandDebugString(hands[3], true);
  return bs;
}

// Plays a card on `bs` matching `card_str` (e.g. "SA", "Cx", "D2").
// Returns true if a legal card in `bs.ValidCardsToPlay()` was matched and
// played, or false if the board cannot follow this play.
bool PlayReplayCardOnBoard(const std::string &card_str, BoardState &bs) {
  if (card_str.size() < 2) return false;
  std::vector<CardType> valid = bs.ValidCardsToPlay();
  if (valid.empty()) return false;
  if (card_str == "__" || card_str == "R_") {
    for (CardType v : valid) {
      auto [vs, _] = Card::DecodeCard(v);
      if (vs != bs.GetTrump()) {
        bs.PlayCard(bs.CurPlayer(), v);
        return true;
      }
    }
    bs.PlayCard(bs.CurPlayer(), valid.front());
    return true;
  }
  Suit suit = Card::ParseSuit(static_cast<char>(std::toupper(card_str[0])));
  if (suit == NOTRUMP) return false;
  if (card_str.size() == 3 && card_str[1] == '<') {
    std::string bound_str(1, static_cast<char>(std::toupper(card_str[0])));
    bound_str += static_cast<char>(std::toupper(card_str[2]));
    CardType bound_card = Card::ParseStringCard(bound_str);
    if (bound_card == 0) return false;
    auto [_, bound_rank] = Card::DecodeCard(bound_card);
    for (CardType v : valid) {
      auto [vs, vr] = Card::DecodeCard(v);
      if (vs == suit && vr < bound_rank) {
        bs.PlayCard(bs.CurPlayer(), v);
        return true;
      }
    }
    return false;
  }
  char rch = card_str[1];
  if (card_str.size() >= 3 && card_str[1] == '1' && card_str[2] == '0') {
    rch = 'T';
  }
  bool is_any_in_suit = (rch == '_');
  bool is_spot_wildcard = (rch == 'x');
  bool is_honor_wildcard = (rch == 'X');
  CardType exact_card = 0;
  if (!is_any_in_suit && !is_spot_wildcard && !is_honor_wildcard) {
    std::string norm_str(1, static_cast<char>(std::toupper(card_str[0])));
    norm_str += static_cast<char>(std::toupper(rch));
    exact_card = Card::ParseStringCard(norm_str);
  }

  if (exact_card != 0) {
    for (CardType v : valid) {
      if (v == exact_card) {
        bs.PlayCard(bs.CurPlayer(), v);
        return true;
      }
    }
    auto [ex_suit, ex_rank] = Card::DecodeCard(exact_card);
    // Only fall back to another spot card if the requested card was itself a
    // spot card (2..9) chosen from Unknown Cards to stand in for 'x'.
    if (ex_rank <= 9) {
      is_spot_wildcard = true;
    } else {
      return false;
    }
  }

  // Pick the lowest matching card of `suit` in `valid` (ValidCardsToPlay
  // iterates suits and ranks 2..14 in ascending order).
  for (CardType v : valid) {
    auto [vs, vr] = Card::DecodeCard(v);
    if (vs != suit) continue;
    if (is_any_in_suit) {
      bs.PlayCard(bs.CurPlayer(), v);
      return true;
    }
    if (is_spot_wildcard && vr <= 9) {
      bs.PlayCard(bs.CurPlayer(), v);
      return true;
    }
    if (is_honor_wildcard && vr >= 10) {
      bs.PlayCard(bs.CurPlayer(), v);
      return true;
    }
  }
  return false;
}

std::vector<BoardState> LineFinder::SetupBoards(
    Suit trump, int leader, std::set<int> &board_offsets,
    const std::vector<CardType> &cur_trick, bool deal_missing_cards) {
  std::vector<BoardState> boards;
  initial_boards_.clear();

  if (!predealt_boards_.empty() &&
      static_cast<int>(predealt_boards_.size()) == num_iters_) {
    initial_boards_ = predealt_boards_;
    for (int i = 0; i < num_iters_; i++) {
      BoardState sim = predealt_boards_[i];
      sim.set_board_id(static_cast<uint32_t>(i + 1));
      sim.set_check_card_play(false);
      sim.SetTrump(trump);

      bool valid_for_branch =
          branch_board_ids_.empty() || (branch_board_ids_.count(i) > 0);
      for (size_t t_idx = 0;
           valid_for_branch && t_idx < replay_tricks_.size(); ++t_idx) {
        std::vector<std::string> board_cards;
        if (t_idx < replay_board_tricks_.size()) {
          auto bt_it = replay_board_tricks_[t_idx].find(i);
          if (bt_it != replay_board_tricks_[t_idx].end()) {
            std::istringstream iss(bt_it->second);
            std::string tok;
            while (iss >> tok) {
              board_cards.push_back(tok);
            }
          }
        }
        const auto &trick_cards =
            (board_cards.size() == 4) ? board_cards : replay_tricks_[t_idx];
        if (trick_cards.size() != 4) {
          valid_for_branch = false;
          break;
        }
        for (const std::string &c_str : trick_cards) {
          if (!PlayReplayCardOnBoard(c_str, sim)) {
            valid_for_branch = false;
            break;
          }
        }
      }

      if (valid_for_branch && sim.CurPlayer() != leader) {
        valid_for_branch = false;
      }

      std::vector<uint64_t> rem_hands(4, 0);
      for (int p = 0; p < 4; ++p) {
        rem_hands[p] = sim.GetHand(p) & ~sim.played_cards();
      }
      if (valid_for_branch) {
        for (const auto &req : fuzzy_state_.hand_reqs1()) {
          uint64_t check_h1 =
              (req && !req->AppliesToRemainingHand()) ? sim.GetHand(1)
                                                      : rem_hands[1];
          if (req && !req->MeetsRequirement(check_h1)) {
            valid_for_branch = false;
            break;
          }
        }
      }
      if (valid_for_branch) {
        for (const auto &req : fuzzy_state_.hand_reqs3()) {
          uint64_t check_h3 =
              (req && !req->AppliesToRemainingHand()) ? sim.GetHand(3)
                                                      : rem_hands[3];
          if (req && !req->MeetsRequirement(check_h3)) {
            valid_for_branch = false;
            break;
          }
        }
      }

      BoardState b;
      if (valid_for_branch) {
        b.SetHands(rem_hands);
        b.set_board_id(static_cast<uint32_t>(i + 1));
        b.set_check_card_play(false);
        b.SetTrump(trump);
        b.SetLead(leader);
        for (CardType c : cur_trick) {
          if (!PlayReplayCardOnBoard(Card::PrintCard(c), b)) {
            valid_for_branch = false;
            break;
          }
        }
      }

      if (!valid_for_branch) {
        // Populate a valid placeholder board with the expected card count so
        // boards_[i] is well-formed, but omit `i` from `board_offsets` so it
        // cannot be covered along this branch. Do not enforce hand_reqs on
        // this unused placeholder board.
        FuzzyBoardState placeholder_fuzzy(fuzzy_state_.known_hands(),
                                          fuzzy_state_.unknown_cards());
        b = placeholder_fuzzy.Randomize(deal_missing_cards);
        b.set_board_id(static_cast<uint32_t>(i + 1));
        b.set_check_card_play(false);
        b.SetTrump(trump);
        b.SetLead(leader);
        for (CardType c : cur_trick) {
          b.PlayCard(b.CurPlayer(), c);
        }
      } else {
        board_offsets.insert(i);
      }
      boards.emplace_back(b);
    }
    return boards;
  }

  for (int i = 0; i < num_iters_; i++) {
    BoardState b = fuzzy_state_.Randomize(deal_missing_cards);
    b.set_board_id(static_cast<uint32_t>(i + 1));
    b.set_check_card_play(false);
    b.SetTrump(trump);
    b.SetLead(leader);
    initial_boards_.emplace_back(b);
    for (CardType c : cur_trick) {
      b.PlayCard(b.CurPlayer(), c);
    }
    boards.emplace_back(b);
    board_offsets.insert(i);
    LOG(INFO) << "Complete board " << i << ": " << b.DebugString();
  }
  return boards;
}


std::set<std::string> LineFinder::FindBestLead(
    int min_tricks,
    const std::vector<uint64_t> &hands, uint64_t unknown_cards,
    int leader, Suit trump) {
  CHECK_EQ(4, hands.size());
  fuzzy_state_.SetHands(hands, unknown_cards);
  std::set<int> board_offsets;
  std::vector<CardType> cur_trick;
  std::vector<BoardState> boards =
      SetupBoards(trump, leader, board_offsets, cur_trick,
                  /*deal_missing_cards=*/true);
  std::map<std::string, std::set<int>> lead_to_boards;
  std::map<int, int> board_tricks;
  size_t max_outcome = 0;
  for (unsigned int i = 0; i < boards.size(); i++) {
    std::vector<std::string> choices =
        FindBestCardsToPlay(boards[i], min_tricks);
    if (choices.empty()) {
      LOG(INFO) << "Could not solve board " << i;
      continue;
    }
    for (const std::string &lead : choices) {
      auto &s = lead_to_boards[lead];
      s.insert(i);
      max_outcome = std::max(s.size(), max_outcome);
    }
  }
  std::set<std::string> ans;
  std::multimap<int, std::string> sorted_leads;
  for (const auto &[lead, offsets] : lead_to_boards) {
    sorted_leads.insert(std::make_pair(boards.size() - offsets.size(), lead));
  }
  for (const auto &[_count, lead] : sorted_leads) {
    std::set<int> offsets = lead_to_boards[lead];
    bool is_best = offsets.size() == max_outcome;
    std::string off_str = StrJoin(offsets, ",");
    if (off_str.size() > 80) {
      off_str.resize(80);
      off_str += "...";
    }
    LOG(INFO) << (is_best ? "*" : " ")
              << "Lead " << lead << " (" << offsets.size()
              << ") " << off_str;
    if (is_best) {
      ans.insert(lead);
    }
  }
  return ans;
}

std::multiset<int> LineFinder::FindTricksWon(
    const std::vector<uint64_t> &hands, uint64_t unknown_cards,
    int leader, Suit trump, bool deal_missing_cards) {
  CHECK_EQ(4, hands.size());
  fuzzy_state_.SetHands(hands, unknown_cards);
  std::set<int> board_offsets;
  std::vector<CardType> cur_trick;
  std::vector<BoardState> boards =
      SetupBoards(trump, leader, board_offsets, cur_trick,
                  deal_missing_cards);
  std::multiset<int> tricks_won;
  for (unsigned int i = 0; i < boards.size(); i++) {
    int cards_left =
        CardsInHand(boards[i].GetHand(0), boards[i].played_cards()).size();
    int mini_max = FindOptimalTricks(boards[i]);
    tricks_won.insert(leader % 2 == 0 ? mini_max : cards_left - mini_max);
  }
  return tricks_won;
}

/* static */
std::string LineFinder::TricksWonDebugString(const std::multiset<int> &tricks) {
  int sum = 0;
  std::map<int, int> counts;
  for (int x : tricks) {
    sum += (x);
    counts[(x)]++;
  }
  double avg = sum / static_cast<double>(tricks.size());
  std::string ans = "Average: " + std::to_string(avg) + "\n";
  for (const auto &[t, n] : counts) {
    ans += "  " + std::to_string(t) + " tricks: " + std::to_string(n) + "\n";
  }
  return ans;
}

/* static */
int LineFinder::CountAtLeast(const std::multiset<int> &data, int thresh) {
  int ans = 0;
  for (int x : data) {
    if (x >= thresh) {
      ans++;
    }
  }
  return ans;
}

static std::atomic<uint64_t> get_line_from_trick_calls{0};
static std::atomic<uint64_t> g_play_trick_calls{0};
static std::atomic<uint64_t> g_memo_hits{0};
static std::atomic<uint64_t> g_memo_misses{0};
static std::atomic<uint64_t> g_memo_evictions{0};

namespace {

size_t NonStateExtraBranches(const ConditionalLine &cl) {
  const auto &conds = cl.conditions();
  bool is_state = !conds.empty() && conds[0].first &&
                  conds[0].first->IsStateCondition();
  size_t extra = (!is_state && conds.size() > 1) ? (conds.size() - 1) : 0;
  for (const auto &[_cond, child] : conds) {
    if (child) {
      extra += NonStateExtraBranches(*child);
    }
  }
  return extra;
}

std::mutex g_status_mutex;
std::string g_solve_status = "Idle";
// Progress tree of the search currently running, if any; the status reported
// while it is set is derived from it rather than from g_solve_status.
std::shared_ptr<SearchProgress> g_active_progress;
std::atomic<int> g_best_covered{0};
std::atomic<int> g_total_boards{0};
std::atomic<bool> g_stop_requested{false};
LineFinder::ProgressCallback g_progress_cb;
std::atomic<bool> g_has_progress_cb{false};
std::atomic<uint32_t> g_progress_tick{0};
std::atomic<int64_t> g_last_progress_notify_ms{0};

int64_t NowSteadyMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

std::string FormatSearchProgressStatus(double frac) {
  int pct = static_cast<int>(std::floor(frac * 100.0));
  return "Searching play trees: ~" + std::to_string(pct) +
         "% done (best coverage: " + std::to_string(g_best_covered.load()) +
         "/" + std::to_string(g_total_boards.load()) + " deals, " +
         std::to_string(get_line_from_trick_calls.load()) + " branches)...";
}

void MaybeNotifyProgress(bool force = false) {
  if (!g_has_progress_cb.load(std::memory_order_relaxed)) return;
  if (!force) {
    if (omp_get_thread_num() != 0) return;
    int64_t now_ms = NowSteadyMs();
    int64_t prev_ms =
        g_last_progress_notify_ms.load(std::memory_order_relaxed);
    if (now_ms - prev_ms < 100) return;
    if (!g_last_progress_notify_ms.compare_exchange_strong(
            prev_ms, now_ms, std::memory_order_relaxed)) {
      return;
    }
  } else {
    g_last_progress_notify_ms.store(NowSteadyMs(), std::memory_order_relaxed);
  }
  LineFinder::ProgressCallback cb;
  std::shared_ptr<SearchProgress> progress;
  std::string base_status;
  {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    cb = g_progress_cb;
    progress = g_active_progress;
    if (!progress) {
      base_status = g_solve_status;
    }
  }
  if (!cb) return;
  if (!progress) {
    cb(base_status, -1.0);
    return;
  }
  double frac = progress->Fraction();
  cb(FormatSearchProgressStatus(frac), frac);
}
}  // namespace

void LineFinder::SetProgressCallback(ProgressCallback cb) {
  std::lock_guard<std::mutex> lock(g_status_mutex);
  g_has_progress_cb.store(static_cast<bool>(cb), std::memory_order_relaxed);
  g_progress_cb = std::move(cb);
  g_last_progress_notify_ms.store(0, std::memory_order_relaxed);
}

void LineFinder::RequestStop() {
  g_stop_requested.store(true, std::memory_order_relaxed);
}

void LineFinder::ClearStop() {
  g_stop_requested.store(false, std::memory_order_relaxed);
}

bool LineFinder::IsStopRequested() {
  if (g_has_progress_cb.load(std::memory_order_relaxed)) {
    uint32_t tick =
        g_progress_tick.fetch_add(1, std::memory_order_relaxed) + 1;
    if ((tick & 511u) == 0u) {
      MaybeNotifyProgress(/*force=*/false);
    }
  }
  return g_stop_requested.load(std::memory_order_relaxed);
}

std::string LineFinder::GetSolveStatus() {
  std::shared_ptr<SearchProgress> progress;
  {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    if (!g_active_progress) {
      return g_solve_status;
    }
    progress = g_active_progress;
  }
  return FormatSearchProgressStatus(progress->Fraction());
}

void LineFinder::SetSolveStatus(const std::string &status) {
  {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_solve_status = status;
  }
  MaybeNotifyProgress(/*force=*/true);
}

double LineFinder::GetSolveProgress() {
  std::shared_ptr<SearchProgress> progress;
  {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    progress = g_active_progress;
  }
  return progress ? progress->Fraction() : -1.0;
}
bool WonEnoughTricks(int mini_max, int min_tricks, int leader,
                     int cards_left);

namespace {

void SetActiveProgress(std::shared_ptr<SearchProgress> progress) {
  std::lock_guard<std::mutex> lock(g_status_mutex);
  g_active_progress = std::move(progress);
}

// Reuses a cached initial OptimalPlayTree from an unforced Solve() when a
// forced 1-card declarer/dummy lead is requested on the same deals.
std::shared_ptr<OptimalPlayTree> DerivePlayTreeForForcedLead(
    const OptimalPlayTree &base_tree, const BoardState &base_board,
    const BoardState &cur_board, const std::string &forced_lead_prefix,
    std::pair<Suit, int> lead_decoded, int min_tricks, SolverCache *cache) {
  int tr = base_tree.CardsLeft();
  if (base_tree.MiniMax() < min_tricks) {
    return std::make_shared<OptimalPlayTree>(0, base_board, tr, min_tricks);
  }
  std::vector<const OptimalLine *> matching;
  bool has_same_suit = false;
  for (const auto &l : base_tree.GetLines()) {
    if (l.size() >= 3 && l.compare(0, 3, forced_lead_prefix) == 0) {
      matching.push_back(&l);
    } else if (l.size() >= 2) {
      auto c_dec = Card::DecodeCard(Card::ParseStringCard(l.substr(0, 2)));
      if (c_dec.first == lead_decoded.first) {
        has_same_suit = true;
      }
    }
  }
  if (!matching.empty()) {
    auto tree = std::make_shared<OptimalPlayTree>(
        min_tricks, base_board, tr, min_tricks);
    for (const OptimalLine *l : matching) {
      tree->AddLineWithChild(*l, base_tree.TryGetDeepened(*l));
    }
    return tree;
  }
  if (!has_same_suit) {
    return std::make_shared<OptimalPlayTree>(0, base_board, tr, min_tricks);
  }
  return OptimalPlayTree::FindOptimalPlayTree(cur_board, min_tricks, cache);
}

using Trick1CardPair = std::pair<std::string, std::string>;

Trick1CardPair GetPrimaryTrick1Play(const ConditionalLine &line) {
  std::map<Trick1CardPair, size_t> counts;
  for (const auto &[_cond, child] : line.conditions()) {
    if (!child || child->do_first().Empty()) continue;
    const GeneralizedTrick &gt = child->do_first();
    Trick1CardPair pair{gt.demi1.my_card, gt.demi2.my_card};
    counts[pair] += child->CoveredBoards();
  }
  Trick1CardPair best{"", ""};
  size_t best_count = 0;
  for (const auto &[pair, count] : counts) {
    if (count > best_count) {
      best_count = count;
      best = pair;
    }
  }
  return best;
}

// Tied top-level lines can be the same line of play described with different
// branching (say, splitting Trick 1 on a defender's card when an unsplit or
// coarser Trick-1 condition covers the exact same boards, or varying
// declarer's card only on a minor defender-card sub-branch). Groups lines by
// their dominant Trick-1 declarer play (my1, my2), keeps only the first line
// with the fewest Trick-1 conditions for each dominant Trick-1 play, and drops
// any lines that prescribe identical declarer plays on every covered board.
void DeduplicateTopLevelLines(std::vector<ConditionalLine> *lines,
                              bool collapse_same_t1 = true) {
  std::map<Trick1CardPair, size_t> min_conds_for_t1;
  if (collapse_same_t1) {
    for (const ConditionalLine &line : *lines) {
      Trick1CardPair t1 = GetPrimaryTrick1Play(line);
      if (!t1.first.empty() || !t1.second.empty()) {
        auto it = min_conds_for_t1.find(t1);
        size_t n = line.conditions().size();
        if (it == min_conds_for_t1.end() || n < it->second) {
          min_conds_for_t1[std::move(t1)] = n;
        }
      }
    }
  }
  std::vector<ConditionalLine> distinct;
  std::set<Trick1CardPair> seen_t1;
  std::vector<std::map<int, std::string>> seen_sig;
  for (ConditionalLine &line : *lines) {
    if (collapse_same_t1) {
      Trick1CardPair t1 = GetPrimaryTrick1Play(line);
      if (!t1.first.empty() || !t1.second.empty()) {
        if (line.conditions().size() > min_conds_for_t1.at(t1) ||
            !seen_t1.insert(std::move(t1)).second) {
          continue;
        }
      }
    }
    std::map<int, std::string> sig = line.DeclarerPlaySignature();
    if (std::find(seen_sig.begin(), seen_sig.end(), sig) == seen_sig.end()) {
      seen_sig.push_back(std::move(sig));
      distinct.push_back(std::move(line));
    }
  }
  *lines = std::move(distinct);
}

void RescoreTopLinesAdversarial(
    const std::vector<BoardState> &boards,
    const std::set<int> &board_offsets,
    const std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    int min_tricks, int num_threads, std::vector<ConditionalLine> *lines) {
  if (lines->empty() || board_offsets.empty()) {
    return;
  }
  std::vector<int> all_boards(board_offsets.begin(), board_offsets.end());
  std::vector<uint8_t> makeable_at_root(all_boards.size(), 1);
  std::vector<int> active_boards;
  active_boards.reserve(all_boards.size());
  for (size_t i = 0; i < all_boards.size(); ++i) {
    int b = all_boards[i];
    auto it = board_trees.find(b);
    if (it != board_trees.end() && it->second != nullptr) {
      const auto &pt = it->second;
      if (!WonEnoughTricks(pt->MiniMax(), min_tricks,
                           pt->MyBoard()->CurPlayer(), pt->CardsLeft())) {
        makeable_at_root[i] = 0;
        continue;
      }
    }
    active_boards.push_back(b);
  }
  size_t num_boards = active_boards.size();
  size_t num_all = all_boards.size();
  size_t num_lines = lines->size();
  for (size_t l_i = 0; l_i < num_lines; ++l_i) {
    (*lines)[l_i].set_min_tricks(min_tricks);
    (*lines)[l_i].DeepCloneChildren();
  }
  {
    std::vector<std::vector<ConditionalLine::UnmakeableBoardTrace>>
        unmakeable_traces(
            num_all,
            std::vector<ConditionalLine::UnmakeableBoardTrace>(num_lines));
    ParallelForDynamic(num_all, num_threads, [&](size_t u_i) {
      if (LineFinder::IsStopRequested()) return;
      int b = all_boards[u_i];
      bool in_step1_set = (makeable_at_root[u_i] != 0);
      ConditionalLine::Step1DefenderCardCache step1_cache;
      for (size_t l_i = 0; l_i < num_lines; ++l_i) {
        (*lines)[l_i].TraceUnmakeableBoard(boards[b], b, in_step1_set,
                                           &step1_cache,
                                           &unmakeable_traces[u_i][l_i]);
      }
    });
    for (size_t l_i = 0; l_i < num_lines; ++l_i) {
      std::unordered_map<const ConditionalLine *, size_t> opp1_counts;
      std::unordered_map<const ConditionalLine *, size_t> opp2_counts;
      std::unordered_map<const ConditionalLine *, std::array<size_t, 4>>
          winner_counts;
      for (size_t u_i = 0; u_i < num_all; ++u_i) {
        const auto &tr = unmakeable_traces[u_i][l_i];
        for (const ConditionalLine *node : tr.opp1_nodes) {
          ++opp1_counts[node];
        }
        for (const ConditionalLine *node : tr.opp2_nodes) {
          ++opp2_counts[node];
        }
        for (const auto &[node, w] : tr.winner_nodes) {
          if (w >= 0 && w < 4) {
            ++winner_counts[node][w];
          }
        }
      }
      if (!opp1_counts.empty() || !opp2_counts.empty() ||
          !winner_counts.empty()) {
        (*lines)[l_i].ApplyUnmakeableBoardCounts(opp1_counts, opp2_counts,
                                                 winner_counts);
      }
    }
  }
  for (size_t l_i = 0; l_i < num_lines; ++l_i) {
    if (LineFinder::IsStopRequested()) break;
    (*lines)[l_i].CollapseUniformInSuitSplits();
  }

  std::vector<ConditionalLine::DefenderCardsCache> def_caches(num_boards);
  std::vector<std::vector<uint8_t>> board_line_ok(
      num_boards, std::vector<uint8_t>(num_lines, 0));
  std::vector<std::vector<ConditionalLine::BoardPlayTrace>> board_line_trace(
      num_boards, std::vector<ConditionalLine::BoardPlayTrace>(num_lines));

  ParallelForDynamic(num_boards, num_threads, [&](size_t b_i) {
    if (LineFinder::IsStopRequested()) return;
    int b = active_boards[b_i];
    for (size_t l_i = 0; l_i < num_lines; ++l_i) {
      ConditionalLine::BoardPlayTrace trace =
          (*lines)[l_i].SimulateBoard(boards[b], b, /*adversarial=*/true,
                                      &def_caches[b_i]);
      if (trace.final_ns_tricks >= min_tricks && trace.leaf != nullptr) {
        board_line_ok[b_i][l_i] = 1;
      }
      board_line_trace[b_i][l_i] = std::move(trace);
    }
  });

  for (size_t l_i = 0; l_i < num_lines; ++l_i) {
    std::unordered_map<const ConditionalLine *, std::set<int>> leaf_boards;
    std::unordered_map<const ConditionalLine *, std::map<int, std::string>>
        node_board_tricks;
    std::map<int, ConditionalLine::BoardPlayTrace> line_traces;
    for (size_t b_i = 0; b_i < num_boards; ++b_i) {
      int b = active_boards[b_i];
      auto &tr = board_line_trace[b_i][l_i];
      if (board_line_ok[b_i][l_i]) {
        leaf_boards[tr.leaf].insert(b);
        for (const auto &step : tr.steps) {
          if (step.node != nullptr && step.unmatched_seat < 0) {
            node_board_tricks[step.node][b] = step.trick_str;
          }
        }
      }
      line_traces.emplace(b, std::move(tr));
    }
    (*lines)[l_i].ApplyRescoredBoards(leaf_boards, node_board_tricks);
    (*lines)[l_i].set_board_traces(std::move(line_traces));
  }

  std::stable_sort(lines->begin(), lines->end(),
                   [](const ConditionalLine &a, const ConditionalLine &b) {
                     if (a.CoveredFinalBoards() != b.CoveredFinalBoards()) {
                       return a.CoveredFinalBoards() > b.CoveredFinalBoards();
                     }
                     if (a.conditions().size() != b.conditions().size()) {
                       return a.conditions().size() < b.conditions().size();
                     }
                     size_t extra_a = a.ExtraBranches();
                     size_t extra_b = b.ExtraBranches();
                     if (extra_a != extra_b) {
                       return extra_a < extra_b;
                     }
                     return a.step1_covered_boards() >
                            b.step1_covered_boards();
                   });
}

void PopulateDealSummaries(const std::vector<BoardState> &boards,
                           const std::set<int> &board_offsets,
                           int num_iters, int min_tricks,
                           std::vector<ConditionalLine> *lines) {
  int active_total = board_offsets.empty()
                         ? num_iters
                         : static_cast<int>(board_offsets.size());
  CrossHandDealSetSummarizer summarizer;
  for (size_t line_idx = 0; line_idx < lines->size(); ++line_idx) {
    if (LineFinder::IsStopRequested()) break;
    auto &line = (*lines)[line_idx];
    line.set_total_boards(active_total);
    line.set_min_tricks(min_tricks);
    if (!boards.empty()) {
      int sample_idx = board_offsets.empty() ? 0 : *board_offsets.begin();
      if (sample_idx >= 0 && sample_idx < static_cast<int>(boards.size())) {
        line.set_sample_board(boards[sample_idx]);
      } else {
        line.set_sample_board(boards[0]);
      }
      std::map<int, ConditionalLine::BoardPlayTrace> traces =
          line.board_traces();
      ConditionalLine::DefenderCardsCache def_cache;
      bool traces_updated = false;
      for (int b_idx = 0; b_idx < static_cast<int>(boards.size()); ++b_idx) {
        if (!board_offsets.empty() && board_offsets.count(b_idx) == 0) {
          continue;
        }
        if (traces.find(b_idx) == traces.end()) {
          traces.emplace(b_idx, line.SimulateBoard(boards[b_idx], b_idx,
                                                   line.has_rescored_boards(),
                                                   &def_cache));
          traces_updated = true;
        }
      }
      if (traces_updated) {
        line.set_board_traces(std::move(traces));
      }
    }
    std::set<int> covered = line.CoveredBoardsSet();
    std::vector<PairedDeal> covered_deals;
    covered_deals.reserve(covered.size());
    for (int b_idx : covered) {
      if (b_idx >= 0 && b_idx < static_cast<int>(boards.size())) {
        covered_deals.emplace_back(boards[b_idx].GetHand(1),
                                   boards[b_idx].GetHand(3));
      }
    }

    std::vector<PairedDeal> failure_deals;
    for (int b_idx = 0; b_idx < static_cast<int>(boards.size()); ++b_idx) {
      if (!board_offsets.empty() && board_offsets.count(b_idx) == 0) {
        continue;
      }
      if (covered.find(b_idx) == covered.end()) {
        failure_deals.emplace_back(boards[b_idx].GetHand(1),
                                   boards[b_idx].GetHand(3));
      }
    }

    if (!covered_deals.empty() && !LineFinder::IsStopRequested()) {
      LineFinder::SetSolveStatus("Summarizing deal invariants (line " +
                                 std::to_string(line_idx + 1) + "/" +
                                 std::to_string(lines->size()) + ")...");
      try {
        line.set_summary(summarizer.Summarize(
            covered_deals, "hand 1", "hand 3", &line, &failure_deals));
      } catch (const std::exception &e) {
        LOG(WARNING) << "Failed to summarize line: " << e.what();
      }
    }

    if (!failure_deals.empty() && !LineFinder::IsStopRequested()) {
      LineFinder::SetSolveStatus("Summarizing failure invariants (line " +
                                 std::to_string(line_idx + 1) + "/" +
                                 std::to_string(lines->size()) + ")...");
      try {
        line.set_failure_summary(summarizer.Summarize(
            failure_deals, "hand 1", "hand 3", &line, &covered_deals,
            /*is_failure_mode=*/true));
      } catch (const std::exception &e) {
        LOG(WARNING) << "Failed to summarize failure line: " << e.what();
      }
    }
  }
}

}  // namespace

std::vector<ConditionalLine> LineFinder::Solve(
    int min_tricks,
    const std::vector<uint64_t> &hands, uint64_t unknown_cards,
    int leader, Suit trump, const std::vector<CardType> &cur_trick,
    int num_threads) {
  auto start_solve = std::chrono::high_resolution_clock::now();
  CHECK_EQ(4, hands.size());
  ClearStop();
  int max_dds_threads = EnsureDDSInitialized();
  int effective_threads = std::clamp(num_threads, 1, max_dds_threads);
  get_line_from_trick_calls = 0;
  g_play_trick_calls = 0;
  g_memo_hits = 0;
  g_memo_misses = 0;
  g_memo_evictions = 0;
  g_best_covered = 0;
  g_total_boards = num_iters_;
  SetSolveStatus("Generating " + std::to_string(num_iters_) +
                 " random deals...");
  LOG(INFO) << "Solving hands: \n\n"
            << "          N: " << HandDebugString(hands[0], true) << "\n\n"
            << " W: " << HandDebugString(hands[1], true)
            << " E: " << HandDebugString(hands[3], true)
            << "[" << HandDebugString(unknown_cards, true) << "]\n\n"
            << "          S: " << HandDebugString(hands[2], true)
            << "\n\nGoal: " << min_tricks << " tricks\n"
            << "Hands simulated: " << num_iters_
            << " (threads: " << effective_threads << ")\n\n";
  bool can_reuse_deals =
      replay_tricks_.empty() && branch_board_ids_.empty() &&
      predealt_boards_.empty() && !cur_trick.empty() &&
      base_boards_.size() == static_cast<size_t>(num_iters_) &&
      hands == base_hands_ && unknown_cards == base_unknown_cards_ &&
      trump == base_trump_ && leader == base_leader_;

  std::set<int> board_offsets;
  std::vector<BoardState> boards;
  if (can_reuse_deals) {
    initial_boards_ = base_boards_;
    boards = base_boards_;
    for (size_t i = 0; i < boards.size(); ++i) {
      board_offsets.insert(static_cast<int>(i));
      for (CardType ct : cur_trick) {
        boards[i].PlayCard(boards[i].CurPlayer(), ct);
      }
    }
    if (!solver_cache_) {
      solver_cache_ = std::make_shared<SolverCache>();
    }
    if (!solve_memo_) {
      solve_memo_ = std::make_unique<SolveMemo>();
    }
  } else {
    fuzzy_state_.SetHands(hands, unknown_cards);
    boards = SetupBoards(trump, leader, board_offsets, cur_trick);
    solve_memo_ = std::make_unique<SolveMemo>();
    if (!solver_cache_ || (predealt_boards_.empty() && trump != base_trump_)) {
      solver_cache_ = std::make_shared<SolverCache>();
    }
  }
  boards_ = boards;
  board_offsets_ = board_offsets;
  int active_total = board_offsets.empty()
                         ? num_iters_
                         : static_cast<int>(board_offsets.size());
  g_total_boards = active_total;

  SolverCache &cache = *solver_cache_;
  auto start_init_trees = std::chrono::high_resolution_clock::now();
  std::map<int, std::shared_ptr<OptimalPlayTree>> board_trees;
  std::vector<std::shared_ptr<OptimalPlayTree>> init_trees(boards.size());

  bool can_derive_init_trees =
      can_reuse_deals && cur_trick.size() == 1 && leader % 2 == 0 &&
      min_tricks == base_min_tricks_ && min_tricks >= 1 &&
      base_init_trees_.size() == boards.size();
  std::string forced_lead_prefix =
      can_derive_init_trees ? (Card::PrintCard(cur_trick[0]) + " ") : "";
  std::pair<Suit, int> lead_decoded =
      can_derive_init_trees ? Card::DecodeCard(cur_trick[0])
                            : std::make_pair(NOTRUMP, -1);

  std::atomic<int> init_done{0};
  ParallelForDynamic(boards.size(), effective_threads, [&](size_t i) {
    if (IsStopRequested() || board_offsets.count(static_cast<int>(i)) == 0) {
      ++init_done;
      return;
    }
    if (can_derive_init_trees && base_init_trees_[i] != nullptr) {
      init_trees[i] = DerivePlayTreeForForcedLead(
          *base_init_trees_[i], base_boards_[i], boards[i],
          forced_lead_prefix, lead_decoded, min_tricks, &cache);
    } else {
      init_trees[i] = OptimalPlayTree::FindOptimalPlayTree(
          boards[i], min_tricks, &cache);
    }
    int done = ++init_done;
    if (!IsStopRequested()) {
      SetSolveStatus("Solving initial double-dummy trees (" +
                     std::to_string(done) + "/" +
                     std::to_string(boards.size()) + " deals)...");
    }
    VLOG(1) << "Finished computing tree for board " << i
            << " with mini_max " << init_trees[i]->MiniMax();
  });
  if (IsStopRequested()) {
    SetSolveStatus("Solve stopped.");
    return {};
  }
  if (cur_trick.empty() && replay_tricks_.empty() &&
      branch_board_ids_.empty() && predealt_boards_.empty()) {
    base_boards_ = boards;
    base_init_trees_ = init_trees;
    base_hands_ = hands;
    base_unknown_cards_ = unknown_cards;
    base_trump_ = trump;
    base_leader_ = leader;
    base_min_tricks_ = min_tricks;
  }
  for (size_t i = 0; i < boards.size(); i++) {
    if (init_trees[i]) {
      board_trees[static_cast<int>(i)] = init_trees[i];
    }
  }
  auto end_init_trees = std::chrono::high_resolution_clock::now();
  double init_trees_ms = std::chrono::duration<double, std::milli>(
      end_init_trees - start_init_trees).count();

  SetSolveStatus("Searching play trees across " +
                 std::to_string(active_total) + " deals...");
  auto progress = std::make_shared<SearchProgress>();
  SetActiveProgress(progress);
  std::vector<ConditionalLine> ans;
  ParallelRegion(effective_threads, [&]() {
    ans = SolveHelper(min_tricks, cache, board_trees,
                      board_offsets, leader, 0, progress->root());
  });
  SetActiveProgress(nullptr);
#ifdef __EMSCRIPTEN__
  solve_memo_.reset();
#endif
  if (IsStopRequested()) {
    SetSolveStatus("Solve stopped.");
    return {};
  }
  auto end_solve = std::chrono::high_resolution_clock::now();
  double search_trees_ms = std::chrono::duration<double, std::milli>(
      end_solve - end_init_trees).count();
  solve_ms_ = std::chrono::duration<double, std::milli>(
      end_solve - start_solve).count();
  LOG(INFO) << "TIMING: init_trees_ms=" << init_trees_ms
            << " search_trees_ms=" << search_trees_ms
            << " (play_trick_calls=" << g_play_trick_calls.load()
            << " cache_hits=" << cache.tree_hits.load()
            << " cache_misses=" << cache.tree_misses.load() << ")"
            << " memo_hits=" << g_memo_hits.load()
            << " memo_misses=" << g_memo_misses.load()
            << " memo_evictions=" << g_memo_evictions.load()
            << " total_solve_ms=" << solve_ms_
            << " get_line_from_trick_calls="
            << get_line_from_trick_calls.load();

  if (ans.empty() && !cur_trick.empty() && !board_offsets.empty()) {
    GeneralizedTrickFinder gen_trick_finder(trump);
    for (int b : board_offsets) {
      auto tree0 = OptimalPlayTree::FindOptimalPlayTree(
          boards[b], /*min_tricks=*/0, &cache, /*deepen_levels=*/0);
      for (const OptimalLine &trick : tree0->GetLines()) {
        gen_trick_finder.RecordOptimalTrick(leader, trick, b);
      }
    }
    GeneralizedTricksVector gen_tricks =
        gen_trick_finder.GetGeneralizedTricks(1);
    if (!gen_tricks.empty()) {
      ConditionalLine fail_line;
      fail_line.SetAction(gen_tricks[0]);
      ans.push_back(std::move(fail_line));
    }
  }

  DeduplicateTopLevelLines(&ans, !adversarial_rescore_ && top_k_lines_ <= 1);

  rescore_ms_ = 0.0;
  if (adversarial_rescore_ && !ans.empty()) {
    SetSolveStatus("Re-scoring top " + std::to_string(ans.size()) +
                   " lines against adversarial defense...");
    auto start_rescore = std::chrono::high_resolution_clock::now();
    RescoreTopLinesAdversarial(boards, board_offsets, board_trees, min_tricks,
                               effective_threads, &ans);
    auto end_rescore = std::chrono::high_resolution_clock::now();
    rescore_ms_ = std::chrono::duration<double, std::milli>(
                      end_rescore - start_rescore)
                      .count();
    LOG(INFO) << "TIMING: rescore_ms=" << rescore_ms_
              << " top_cov=" << ans[0].CoveredFinalBoards()
              << " (step1=" << ans[0].step1_covered_boards() << ")";
  }
  if (!keep_all_top_k_ && !ans.empty() &&
      (adversarial_rescore_ || top_k_lines_ > 1)) {
    int best_cov = ans[0].CoveredFinalBoards();
    size_t keep_n = 1;
    while (keep_n < ans.size() &&
           ans[keep_n].CoveredFinalBoards() == best_cov) {
      ++keep_n;
    }
    ans.resize(keep_n);
    DeduplicateTopLevelLines(&ans, /*collapse_same_t1=*/true);
  }

  auto start_invariant = std::chrono::high_resolution_clock::now();
  PopulateDealSummaries(boards, board_offsets, num_iters_, min_tricks, &ans);
  auto end_invariant = std::chrono::high_resolution_clock::now();
  invariant_ms_ = std::chrono::duration<double, std::milli>(
      end_invariant - start_invariant).count();
  LOG(INFO) << "TIMING: invariant_ms=" << invariant_ms_
            << " ans.size()=" << ans.size();

  SetSolveStatus("Building decision tree...");
  LOG(INFO) << "Solve finished with get_line_from_trick_calls "
            << get_line_from_trick_calls.load();
  return ans;
}

int LineFinder::TopKForLevel(int lvl, size_t num_boards) const {
  if (lvl <= 0) {
    return top_k_lines_;
  }
  if (top_k_lines_ <= 1 || top_k_lvl1_ <= 1 || num_boards < 4) {
    return 1;
  }
  if (lvl == 1) {
    return top_k_lvl1_;
  }
  return std::min(2, top_k_lvl1_);
}

namespace {

// Sorts candidate lines by CoveredFinalBoards() descending then ExtraBranches()
// ascending, deduplicates identical ExtraBranches() counts to the front when
// truncating to `limit`, and ensures the minimum-ExtraBranches() candidate is
// retained in the last slot before resizing to `limit`.
void FinalizeTopKLines(std::vector<ConditionalLine> *lines, int limit) {
  std::stable_sort(
      lines->begin(), lines->end(),
      [](const ConditionalLine &a, const ConditionalLine &b) {
        if (a.CoveredFinalBoards() != b.CoveredFinalBoards()) {
          return a.CoveredFinalBoards() > b.CoveredFinalBoards();
        }
        return a.ExtraBranches() < b.ExtraBranches();
      });
  if (static_cast<int>(lines->size()) > limit && limit > 0) {
    std::set<size_t> seen_nseb;
    std::stable_partition(
        lines->begin(), lines->end(),
        [&](const ConditionalLine &l) {
          size_t nseb = NonStateExtraBranches(l);
          return nseb == 0 || seen_nseb.insert(nseb).second;
        });
    if (limit > 1) {
      size_t min_nseb_idx = 0;
      size_t min_nseb = NonStateExtraBranches((*lines)[0]);
      for (size_t k = 1; k < lines->size(); ++k) {
        size_t nseb = NonStateExtraBranches((*lines)[k]);
        if (nseb < min_nseb) {
          min_nseb = nseb;
          min_nseb_idx = k;
        }
      }
      if (min_nseb_idx >= static_cast<size_t>(limit)) {
        std::swap((*lines)[limit - 1], (*lines)[min_nseb_idx]);
      }
    }
    lines->resize(limit);
  }
}

struct SolveMemoKeyBuilder {
  std::string key;
  Suit raw_prev_suit = NOTRUMP;
  uint16_t opp_suit_bits = 0;
  uint16_t disc_or = 0;
  uint16_t disc_and = 0;
  size_t suit_key_pos = 0;

  void Init(int leader, int min_tricks, Suit prev_suit, int top_k,
            size_t num_boards, bool allow_state_split = true) {
    raw_prev_suit = prev_suit;
    opp_suit_bits =
        (raw_prev_suit >= 0 && raw_prev_suit < 4)
            ? static_cast<uint16_t>((1u << (1 * 4 + raw_prev_suit)) |
                                    (1u << (3 * 4 + raw_prev_suit)))
            : 0;
    disc_or = 0;
    disc_and = opp_suit_bits;
    key.reserve(4 + num_boards * 14);
    uint8_t leader_byte = static_cast<uint8_t>(leader & 0x0f) |
                          (allow_state_split ? 0x10u : 0u);
    key.push_back(static_cast<char>(leader_byte));
    key.push_back(static_cast<char>(std::max(min_tricks, 0)));
    suit_key_pos = key.size();
    key.push_back(static_cast<char>(NOTRUMP));
    if (top_k > 1) {
      key.push_back(static_cast<char>(std::min(top_k, 127)));
    }
  }

  void AddBoard(int board, uint64_t played, uint16_t disc) {
    uint32_t bid = static_cast<uint32_t>(board);
    disc_or |= (disc & opp_suit_bits);
    disc_and &= (disc & opp_suit_bits);
    key.append(reinterpret_cast<const char *>(&bid), sizeof(bid));
    key.append(reinterpret_cast<const char *>(&played), sizeof(played));
    key.append(reinterpret_cast<const char *>(&disc), sizeof(disc));
  }

  std::string Finish() {
    if ((disc_or ^ disc_and) != 0) {
      key[suit_key_pos] = static_cast<char>(raw_prev_suit);
    }
    return std::move(key);
  }
};

// Assembles candidate ConditionalLines from per-branch alternative lists:
// 1. Primary line choosing index 0 in every branch.
// 2. When `include_alts` is true: single-branch substitutions (choosing `alt`
//    in branch `idx` and 0 in all others) that reduce NonStateExtraBranches()
//    at `lvl > 0`, plus a composite `min_nseb_line` choosing the minimum
//    NonStateExtraBranches() alternative in every branch when at least
//    `min_diff_branches` branches have a strictly lower count.
template <typename BuildLineFn>
std::vector<ConditionalLine> AssembleBranchLines(
    const std::vector<std::vector<std::shared_ptr<ConditionalLine>>> &branches,
    bool include_alts, int lvl, size_t min_diff_branches,
    BuildLineFn build_line) {
  std::vector<ConditionalLine> out;
  std::vector<size_t> picks(branches.size(), 0);
  out.push_back(build_line(picks));
  if (!include_alts) {
    return out;
  }
  std::vector<size_t> min_picks(branches.size(), 0);
  size_t diff_branches = 0;
  for (size_t idx = 0; idx < branches.size(); ++idx) {
    if (branches[idx].empty()) continue;
    size_t base_nseb = NonStateExtraBranches(*branches[idx][0]);
    size_t min_nseb = base_nseb;
    for (size_t alt = 1; alt < branches[idx].size(); ++alt) {
      size_t nseb = NonStateExtraBranches(*branches[idx][alt]);
      if (nseb < min_nseb) {
        min_nseb = nseb;
        min_picks[idx] = alt;
      }
      if (lvl > 0 && base_nseb > 0 && nseb >= base_nseb) {
        continue;
      }
      picks[idx] = alt;
      out.push_back(build_line(picks));
      picks[idx] = 0;
    }
    if (min_picks[idx] > 0) {
      ++diff_branches;
    }
  }
  if (branches.size() > 1 && diff_branches >= min_diff_branches) {
    out.push_back(build_line(min_picks));
  }
  return out;
}

}  // namespace

std::vector<ConditionalLine> LineFinder::GetLinesFromTrick(
    const GeneralizedTrick &gen_trick,
    int min_tricks,
    SolverCache &cache,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress,
    int min_covered, int top_k) {
  SearchProgress::Completer progress_completer(progress);
  size_t num_trick_boards = gen_trick.NumBoards();
  if (IsStopRequested() ||
      min_covered > static_cast<int>(num_trick_boards)) {
    return {};
  }
  if (top_k < 0) {
    top_k = TopKForLevel(lvl + 1, num_trick_boards);
  }
  ++get_line_from_trick_calls;
  VLOG(2) << "Pursuing line satisfying boards " << num_trick_boards;

  struct WinnerBranch {
    int next_leader = 0;
    std::set<int> boards;
    std::set<std::string> winning_cards;
    std::vector<const GeneralizedTrick::BoardTrickEntry *> entries;
  };
  std::array<WinnerBranch, 4> by_winner;
  for (int w = 0; w < 4; ++w) {
    by_winner[w].next_leader = w;
  }
  for (const auto &entry : gen_trick.board_entries) {
    WinnerBranch &wb = by_winner[entry.winner];
    wb.boards.insert(wb.boards.end(), entry.board);
    wb.winning_cards.emplace(entry.win_card);
    wb.entries.push_back(&entry);
  }
  std::vector<WinnerBranch> entries;
  entries.reserve(4);
  for (int w = 0; w < 4; ++w) {
    if (!by_winner[w].boards.empty()) {
      entries.push_back(std::move(by_winner[w]));
    }
  }
  if (min_covered > 0 && entries.size() > 1) {
    std::stable_sort(entries.begin(), entries.end(),
                     [](const WinnerBranch &a, const WinnerBranch &b) {
                       return a.boards.size() < b.boards.size();
                     });
  }

  std::vector<double> branch_weights;
  if (progress != nullptr && lvl < 2) {
    branch_weights.reserve(entries.size());
    for (const auto &wb : entries) {
      branch_weights.push_back(wb.boards.size());
    }
  }
  std::vector<SearchProgress::Node *> branch_progress =
      (progress != nullptr && lvl < 2)
          ? SearchProgress::AddChildren(progress, branch_weights)
          : std::vector<SearchProgress::Node *>(entries.size(), nullptr);
  int total_trick_boards = static_cast<int>(num_trick_boards);
  int max_losses = (min_covered > 0) ? (total_trick_boards - min_covered)
                                     : total_trick_boards;
  std::vector<std::vector<ConditionalLine>> branch_results(entries.size());
  int child_top_k = top_k;

  auto eval_branch = [&](size_t idx, int branch_min_cov) {
    const WinnerBranch &wb = entries[idx];
    if (IsStopRequested() || wb.entries.empty() ||
        static_cast<int>(wb.boards.size()) < branch_min_cov) {
      SearchProgress::Complete(branch_progress[idx]);
      return;
    }
    int next_leader = wb.next_leader;
    int next_lvl_top_k = TopKForLevel(lvl + 1, wb.boards.size());
    int branch_top_k =
        (child_top_k <= 1 || wb.boards.size() < 4)
            ? 1
            : std::min(child_top_k, next_lvl_top_k);
    int trick_won = (next_leader % 2 == 0) ? 1 : 0;
    int next_min_tricks = min_tricks - trick_won;
    int cards_left = board_trees.at(wb.entries[0]->board)->CardsLeft();
    if (cards_left <= 1) {
      SearchProgress::Complete(branch_progress[idx]);
      if (next_min_tricks <= 0) {
        branch_results[idx] = {ConditionalLine(wb.boards)};
      }
      return;
    }

    Suit next_prev_suit = wb.entries[0]->trick_str.empty()
                              ? NOTRUMP
                              : Card::ParseSuit(wb.entries[0]->trick_str[0]);
    std::string memo_key;
    if (solve_memo_ != nullptr) {
      SolveMemoKeyBuilder kb;
      kb.Init(next_leader, next_min_tricks, next_prev_suit, branch_top_k,
              wb.entries.size(), /*allow_state_split=*/true);
      for (const auto *e : wb.entries) {
        const BoardState *bs_ptr = board_trees.at(e->board)->MyBoard();
        kb.AddBoard(e->board, bs_ptr->played_cards() | e->trick_mask,
                    bs_ptr->discarded_suits_mask() | e->discard_mask);
      }
      memo_key = kb.Finish();
      if (solve_memo_->Lookup(memo_key, branch_min_cov,
                              &branch_results[idx])) {
        g_memo_hits.fetch_add(1, std::memory_order_relaxed);
        SearchProgress::Complete(branch_progress[idx]);
        return;
      }
      g_memo_misses.fetch_add(1, std::memory_order_relaxed);
    }

    std::vector<std::shared_ptr<OptimalPlayTree>> played_trees(
        wb.entries.size());
    if (omp_in_parallel() && wb.entries.size() > 1) {
      std::vector<size_t> pending_entries;
      pending_entries.reserve(wb.entries.size());
      for (size_t e_idx = 0; e_idx < wb.entries.size(); ++e_idx) {
        if (IsStopRequested()) break;
        const auto *entry = wb.entries[e_idx];
        const auto &bt = board_trees.at(entry->board);
        auto deepened = bt->TryGetDeepened(entry->trick_str);
        if (deepened) {
          played_trees[e_idx] = std::move(deepened);
        } else {
          pending_entries.push_back(e_idx);
        }
      }
      ParallelTaskBatch(pending_entries.size(), [&](size_t p_idx) {
        if (IsStopRequested()) return;
        size_t e_idx = pending_entries[p_idx];
        const auto *ent = wb.entries[e_idx];
        played_trees[e_idx] =
            board_trees.at(ent->board)->PlayTrick(ent->trick_str, &cache);
      });
    } else {
      for (size_t e_idx = 0; e_idx < wb.entries.size(); ++e_idx) {
        if (IsStopRequested()) break;
        const auto *entry = wb.entries[e_idx];
        played_trees[e_idx] =
            board_trees.at(entry->board)->PlayTrick(entry->trick_str, &cache);
      }
    }
    if (IsStopRequested()) {
      SearchProgress::Complete(branch_progress[idx]);
      return;
    }
    g_play_trick_calls.fetch_add(wb.entries.size(),
                                 std::memory_order_relaxed);
    std::map<int, std::shared_ptr<OptimalPlayTree>> after_play_trees;
    for (size_t e_idx = 0; e_idx < wb.entries.size(); ++e_idx) {
      CHECK(played_trees[e_idx]);
      after_play_trees.emplace_hint(after_play_trees.end(),
                                    wb.entries[e_idx]->board,
                                    std::move(played_trees[e_idx]));
    }
    branch_results[idx] = SolveHelperUncached(
        next_min_tricks, cache, after_play_trees, wb.boards, next_leader,
        lvl + 1, branch_progress[idx], branch_min_cov, branch_top_k,
        next_prev_suit, /*state_depth=*/0);
    if (solve_memo_ != nullptr && !IsStopRequested()) {
      solve_memo_->Insert(std::move(memo_key), branch_min_cov,
                          branch_results[idx]);
    }
  };

  if (omp_in_parallel() && entries.size() > 1 && min_covered <= 0 && lvl <= 2) {
    ParallelTaskBatch(entries.size(), [&](size_t idx) {
      int branch_size = static_cast<int>(entries[idx].boards.size());
      int branch_min_cov =
          (min_covered > 0) ? std::max(0, branch_size - max_losses) : 0;
      eval_branch(idx, branch_min_cov);
    });
  } else {
    int rem_losses = max_losses;
    bool failed_low = false;
    for (size_t idx = 0; idx < entries.size(); ++idx) {
      if (IsStopRequested()) {
        failed_low = true;
        for (size_t rem = idx; rem < entries.size(); ++rem) {
          SearchProgress::Complete(branch_progress[rem]);
        }
        break;
      }
      int branch_size = static_cast<int>(entries[idx].boards.size());
      int branch_min_cov =
          (min_covered > 0) ? std::max(0, branch_size - rem_losses) : 0;
      eval_branch(idx, branch_min_cov);
      int branch_cov = branch_results[idx].empty()
                           ? 0
                           : branch_results[idx][0].CoveredFinalBoards();
      if (branch_min_cov > 0 && branch_cov < branch_min_cov) {
        failed_low = true;
        for (size_t rem = idx + 1; rem < entries.size(); ++rem) {
          SearchProgress::Complete(branch_progress[rem]);
        }
        break;
      }
      rem_losses -= (branch_size - branch_cov);
    }
    if (failed_low) {
      return {};
    }
  }
  if (IsStopRequested()) {
    return {};
  }

  std::vector<std::vector<std::shared_ptr<ConditionalLine>>> shared_branches(
      entries.size());
  for (size_t idx = 0; idx < entries.size(); ++idx) {
    for (auto &cl : branch_results[idx]) {
      if (cl.CoveredFinalBoards() > 0) {
        shared_branches[idx].push_back(
            std::make_shared<ConditionalLine>(std::move(cl)));
      }
    }
  }

  std::vector<ConditionalLine> candidates = AssembleBranchLines(
      shared_branches, child_top_k > 1, lvl, /*min_diff_branches=*/1,
      [&](const std::vector<size_t> &picks) {
        ConditionalLine line;
        line.SetAction(gen_trick);
        for (size_t idx = 0; idx < entries.size(); ++idx) {
          if (picks[idx] < shared_branches[idx].size()) {
            line.AddCondition(
                new WinnerPlayCondition(entries[idx].next_leader,
                                        entries[idx].winning_cards),
                shared_branches[idx][picks[idx]]);
          }
        }
        return line;
      });

  int primary_cov = candidates[0].CoveredFinalBoards();
  if ((min_covered > 0 && primary_cov < min_covered) || primary_cov <= 0) {
    return {};
  }
  if (child_top_k > 1 && candidates.size() > 1) {
    int alt_slack = std::max(2, primary_cov / 4);
    int alt_floor = std::max(1, primary_cov - alt_slack);
    candidates.erase(
        std::remove_if(candidates.begin() + 1, candidates.end(),
                       [alt_floor](const ConditionalLine &cl) {
                         return cl.CoveredFinalBoards() < alt_floor;
                       }),
        candidates.end());
    FinalizeTopKLines(&candidates, child_top_k);
  }
  return candidates;
}

namespace {

// Opponent patterns that carry no information: "__" (any card) and "R_" (any
// discard).
bool IsUninformativeOppPattern(const std::string &p) {
  return p == "__" || p == "R_";
}

// True if, on every board, neither opponent follows suit or ruffs and partner
// does not ruff, so the trick splits no deals apart: cashing a trump when the
// opponents are void is the typical case.
bool AnyOpponentPlaysTrump(const GeneralizedTrick &t, Suit trump_suit) {
  if (trump_suit == NOTRUMP || t.leader % 2 != 0 ||
      t.demi1.my_card.empty() || t.demi2.my_card.empty()) {
    return false;
  }
  uint64_t my_mask = Card::ParseStringCard(t.demi1.my_card) |
                     Card::ParseStringCard(t.demi2.my_card);
  for (const auto &e : t.board_entries) {
    if (CardsInSuit(e.trick_mask & ~my_mask, trump_suit) > 0) {
      return true;
    }
  }
  return false;
}

// True if, on every board, neither opponent follows suit or ruffs and partner
// does not ruff, so the trick splits no deals apart: cashing a trump when the
// opponents are void is the typical case.
bool IsUninformativeTrick(const GeneralizedTrick &t, Suit trump_suit) {
  bool partner_ruffs =
      trump_suit != NOTRUMP &&
      Card::ParseSuit(t.demi1.my_card[0]) != trump_suit &&
      Card::ParseSuit(t.demi2.my_card[0]) == trump_suit;
  return !partner_ruffs && IsUninformativeOppPattern(t.demi1.opp_card) &&
         IsUninformativeOppPattern(t.demi2.opp_card) &&
         !AnyOpponentPlaysTrump(t, trump_suit);
}

bool TrickDrawsTrump(const GeneralizedTrick &t, Suit trump_suit) {
  if (trump_suit == NOTRUMP || t.leader % 2 != 0 ||
      t.demi1.my_card.empty()) {
    return false;
  }
  if (Card::ParseSuit(t.demi1.my_card[0]) != trump_suit) {
    return false;
  }
  return !IsUninformativeTrick(t, trump_suit);
}

bool GroupDrawsTrump(const std::vector<GeneralizedTrick> &group,
                     Suit trump_suit) {
  for (const GeneralizedTrick &t : group) {
    if (TrickDrawsTrump(t, trump_suit)) return true;
  }
  return false;
}

// Relative cost estimate of evaluating a trick, for progress reporting. The
// sub-search grows with the deals it has to handle; a trick that leaves every
// deal in play (no winner or defender-card split) faces nearly the same
// problem as its parent one card shorter, so it is weighted up further.
double TrickProgressWeight(const GeneralizedTrick &t, Suit trump_suit) {
  constexpr double kUninformativeTrickFactor = 4.0;
  double weight = t.NumBoards();
  if (t.leader % 2 == 0 && IsUninformativeTrick(t, trump_suit)) {
    weight *= kUninformativeTrickFactor;
  }
  return weight;
}

// Returns declarer's "null move" leads at this node: cards whose lead, on
// every board, neither opponent ever follows suit or ruffs (so every opponent
// pattern is "__" or "R_") and which partner does not ruff. Cashing a trump
// when the opponents are void is the typical case. Such a trick gains no
// information about the layout and merely postpones the real decision.
std::set<std::string> FindNullMoveLeads(
    const std::vector<std::vector<GeneralizedTrick>> &groupings,
    Suit trump_suit) {
  std::set<std::string> uninformative;
  std::set<std::string> informative;
  for (const auto &group : groupings) {
    for (const GeneralizedTrick &t : group) {
      if (t.leader % 2 != 0) {
        continue;
      }
      if (IsUninformativeTrick(t, trump_suit)) {
        uninformative.insert(t.demi1.my_card);
      } else {
        informative.insert(t.demi1.my_card);
      }
    }
  }
  std::set<std::string> ans;
  for (const std::string &lead : uninformative) {
    if (informative.count(lead) == 0) {
      ans.insert(lead);
    }
  }
  return ans;
}

Trick1CardPair GetUniformDeclarerPlay(
    const std::vector<GeneralizedTrick> &group) {
  if (group.empty()) {
    return {"", ""};
  }
  Trick1CardPair p{group[0].demi1.my_card, group[0].demi2.my_card};
  for (size_t k = 1; k < group.size(); ++k) {
    if (group[k].demi1.my_card != p.first ||
        group[k].demi2.my_card != p.second) {
      return {"", ""};
    }
  }
  return p;
}

int EncodeShortCardId(const std::string &c) {
  if (c.size() >= 2) {
    return (static_cast<unsigned char>(c[0]) << 8) |
           static_cast<unsigned char>(c[1]);
  }
  return c.empty() ? 0 : static_cast<unsigned char>(c[0]);
}

bool SameConcreteTricksOnSubset(const GeneralizedTrick &sup,
                                const GeneralizedTrick &sub) {
  if (sup.board_entries.empty() || sub.board_entries.empty()) {
    return false;
  }
  size_t i = 0;
  for (const auto &sub_e : sub.board_entries) {
    while (i < sup.board_entries.size() &&
           sup.board_entries[i].board < sub_e.board) {
      ++i;
    }
    if (i >= sup.board_entries.size() ||
        sup.board_entries[i].board != sub_e.board ||
        sup.board_entries[i].trick_mask != sub_e.trick_mask) {
      return false;
    }
    ++i;
  }
  return true;
}

}  // namespace

std::vector<ConditionalLine> LineFinder::GetLinesFromGroups(
    const std::vector<std::vector<GeneralizedTrick>> &groupings,
    int min_tricks,
    SolverCache &cache,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress,
    int min_covered, int top_k, int *out_unsplit_ub) {
  VLOG(2) << "GetLinesFromGroups " << groupings.size();
  int group_num = 0;
  for (const auto &grp : groupings) {
    VLOG(2) << "See group " << group_num++ << ":";
    for (const auto &trick : grp) {
      VLOG(2) << "   " << trick.ToString();
    }
  }

  std::vector<int> group_possible_boards(groupings.size(), 0);
  // keys[i][k] identifies groupings[i][k] in saved_work; weights[i][k] is
  // its estimated share of the search for progress reporting.
  std::vector<std::vector<std::string>> keys(groupings.size());
  std::vector<std::vector<size_t>> group_trick_ids(groupings.size());
  std::vector<Trick1CardPair> group_uniform_t1(groupings.size());
  std::vector<bool> group_draws_trump(groupings.size(), false);
  std::vector<std::vector<double>> weights(
      progress != nullptr ? groupings.size() : 0);
  Suit trump_suit = board_offsets.empty()
                        ? NOTRUMP
                        : board_trees.at(*board_offsets.begin())
                              ->MyBoard()->GetTrump();

  std::unordered_map<std::string, size_t> key_to_uid;
  std::vector<const GeneralizedTrick *> unique_tricks;
  int max_board_id = board_offsets.empty() ? 0 : *board_offsets.rbegin();
  for (size_t i = 0; i < groupings.size(); ++i) {
    group_uniform_t1[i] = GetUniformDeclarerPlay(groupings[i]);
    group_draws_trump[i] = GroupDrawsTrump(groupings[i], trump_suit);
    keys[i].reserve(groupings[i].size());
    group_trick_ids[i].reserve(groupings[i].size());
    if (progress != nullptr) {
      weights[i].reserve(groupings[i].size());
    }
    for (const auto &trick : groupings[i]) {
      group_possible_boards[i] += trick.NumBoards();
      std::string k_str = trick.ToString(true);
      auto [it, inserted] =
          key_to_uid.emplace(k_str, unique_tricks.size());
      if (inserted) {
        unique_tricks.push_back(&trick);
        if (!trick.demi2.boards_vec.empty()) {
          max_board_id = std::max(max_board_id, trick.demi2.boards_vec.back());
        } else if (!trick.boards.empty()) {
          max_board_id = std::max(max_board_id, *trick.boards.rbegin());
        }
      }
      group_trick_ids[i].push_back(it->second);
      keys[i].push_back(std::move(k_str));
      if (progress != nullptr) {
        weights[i].push_back(TrickProgressWeight(trick, trump_suit));
      }
    }
  }

  size_t num_words = static_cast<size_t>(max_board_id / 64) + 1;
  size_t num_unique = unique_tricks.size();
  std::vector<uint64_t> u_masks(num_unique * num_words, 0);
  std::vector<int> u_my1(num_unique, 0);
  std::vector<int> u_my2(num_unique, 0);
  std::vector<int> u_upper_bound(num_unique, 0);
  std::vector<uint8_t> u_evaluated(num_unique, 0);
  for (size_t u = 0; u < num_unique; ++u) {
    const GeneralizedTrick &t = *unique_tricks[u];
    u_my1[u] = EncodeShortCardId(t.demi1.my_card);
    u_my2[u] = EncodeShortCardId(t.demi2.my_card);
    u_upper_bound[u] = static_cast<int>(t.NumBoards());
    uint64_t *m = &u_masks[u * num_words];
    if (!t.demi2.boards_vec.empty()) {
      for (int b : t.demi2.boards_vec) {
        if (b >= 0) m[b / 64] |= (1ULL << (b % 64));
      }
    } else {
      for (int b : t.boards) {
        if (b >= 0) m[b / 64] |= (1ULL << (b % 64));
      }
    }
  }

  if (top_k < 0) {
    top_k = TopKForLevel(lvl, board_offsets.size());
  }
  int child_top_k =
      (top_k <= 1 || top_k_lvl1_ <= 1 || board_offsets.size() < 4)
          ? 1
          : ((lvl == 0 && groupings.size() == 1)
                 ? std::min(top_k, top_k_lvl1_)
                 : std::min(top_k, TopKForLevel(lvl, board_offsets.size())));
  struct TopCandidate {
    int covered = 0;
    size_t num_conds = 0;
    size_t extra_branches = 0;
    size_t non_state_eb = 0;
    bool draws_trump = false;
    std::map<int, std::string> sig;
    ConditionalLine line;
  };
  std::vector<TopCandidate> top_candidates;
  std::map<Trick1CardPair, std::pair<int, size_t>> best_t1_cov_conds;
  std::unordered_map<std::string, std::shared_ptr<ConditionalLine>> saved_work;
  std::unordered_map<std::string,
                     std::vector<std::shared_ptr<ConditionalLine>>>
      saved_work_alts;
  std::unordered_map<std::string, int> saved_covered;
  int most_boards_covered = 0;
  int known_best_cov = 0;
  size_t known_best_idx = groupings.size();

  auto competitive_window_floor = [&]() -> int {
    if (top_k > 1 && most_boards_covered > 0 &&
        (lvl > 0 || !keep_all_top_k_) &&
        most_boards_covered >= min_covered) {
      int slack = std::max(2, most_boards_covered / 6);
      return std::max(1, most_boards_covered - slack);
    }
    return std::max(1, min_covered);
  };

  auto top_k_prune_floor = [&](size_t group_size, bool draws_trump) -> int {
    int floor_cov = competitive_window_floor();
    if ((lvl >= 1 || (!keep_all_top_k_ && leader % 2 != 0)) &&
        group_size > 1 && most_boards_covered > 0) {
      floor_cov = std::max(floor_cov, most_boards_covered + 1);
    }
    if (lvl >= 1 && !top_candidates.empty()) {
      size_t min_nseb = top_candidates[0].non_state_eb;
      int min_nseb_cov = top_candidates[0].covered;
      size_t min_nseb_conds = top_candidates[0].num_conds;
      size_t min_nseb_eb = top_candidates[0].extra_branches;
      bool min_nseb_dt = top_candidates[0].draws_trump;
      for (const auto &tc : top_candidates) {
        if (tc.non_state_eb < min_nseb) {
          min_nseb = tc.non_state_eb;
          min_nseb_cov = tc.covered;
          min_nseb_conds = tc.num_conds;
          min_nseb_eb = tc.extra_branches;
          min_nseb_dt = tc.draws_trump;
        }
      }
      if (min_nseb == 0 && min_nseb_eb > 0) {
        bool can_win_nseb0_tie =
            (group_size < min_nseb_conds) ||
            (group_size == min_nseb_conds && draws_trump && !min_nseb_dt);
        int eb0_floor =
            can_win_nseb0_tie ? min_nseb_cov : (min_nseb_cov + 1);
        floor_cov = std::max(floor_cov, eb0_floor);
      }
    }
    if (static_cast<int>(top_candidates.size()) >= top_k && top_k > 0) {
      int full_cov = top_candidates.back().covered;
      bool can_win_tie =
          (lvl == 0) || (group_size < top_candidates.back().num_conds) ||
          (group_size == top_candidates.back().num_conds && draws_trump &&
           !top_candidates.back().draws_trump);
      int regular_floor = can_win_tie ? full_cov : (full_cov + 1);
      if (top_k > 1 && group_size == 1) {
        size_t min_nseb = top_candidates[0].non_state_eb;
        int min_nseb_cov = top_candidates[0].covered;
        bool min_nseb_dt = top_candidates[0].draws_trump;
        for (const auto &tc : top_candidates) {
          if (tc.non_state_eb < min_nseb) {
            min_nseb = tc.non_state_eb;
            min_nseb_cov = tc.covered;
            min_nseb_dt = tc.draws_trump;
          }
        }
        if (min_nseb > 0) {
          return floor_cov;
        }
        if (draws_trump && !min_nseb_dt) {
          regular_floor = std::min(regular_floor, min_nseb_cov);
        }
      }
      floor_cov = std::max(floor_cov, regular_floor);
    }
    return floor_cov;
  };

  auto is_better_meta = [](int cov_a, size_t conds_a, size_t eb_a, bool dt_a,
                           int cov_b, size_t conds_b, size_t eb_b, bool dt_b) {
    if (cov_a != cov_b) return cov_a > cov_b;
    if (conds_a != conds_b) return conds_a < conds_b;
    if (eb_a != eb_b) return eb_a < eb_b;
    return dt_a > dt_b;
  };

  auto add_top_candidate = [&](const ConditionalLine &cand,
                               const Trick1CardPair &t1,
                               size_t group_conds,
                               bool draws_trump) -> bool {
    int cov = cand.CoveredFinalBoards();
    int min_floor = competitive_window_floor();
    if (cov < min_floor) {
      return false;
    }
    if (!t1.first.empty()) {
      auto it = best_t1_cov_conds.find(t1);
      if (it == best_t1_cov_conds.end() || cov > it->second.first) {
        best_t1_cov_conds[t1] = {cov, group_conds};
      } else if (cov == it->second.first && group_conds < it->second.second) {
        it->second.second = group_conds;
      }
    }
    std::map<int, std::string> sig =
        (top_k > 1) ? cand.DeclarerPlaySignature()
                    : std::map<int, std::string>{};
    size_t conds = cand.conditions().size();
    size_t extra_br = (top_k > 1) ? cand.ExtraBranches() : 0;
    size_t nseb = (top_k > 1) ? NonStateExtraBranches(cand) : 0;
    bool changed = false;
    bool found_sig = false;
    size_t min_existing_nseb =
        top_candidates.empty() ? 0 : top_candidates[0].non_state_eb;
    int min_nseb_cov = top_candidates.empty() ? 0 : top_candidates[0].covered;
    size_t min_nseb_conds =
        top_candidates.empty() ? 0 : top_candidates[0].num_conds;
    size_t min_nseb_eb =
        top_candidates.empty() ? 0 : top_candidates[0].extra_branches;
    bool min_nseb_dt =
        top_candidates.empty() ? false : top_candidates[0].draws_trump;
    for (auto &tc : top_candidates) {
      if (tc.non_state_eb < min_existing_nseb) {
        min_existing_nseb = tc.non_state_eb;
        min_nseb_cov = tc.covered;
        min_nseb_conds = tc.num_conds;
        min_nseb_eb = tc.extra_branches;
        min_nseb_dt = tc.draws_trump;
      }
      if (top_k > 1 && tc.sig == sig) {
        found_sig = true;
        if (is_better_meta(cov, conds, extra_br, draws_trump, tc.covered,
                           tc.num_conds, tc.extra_branches, tc.draws_trump)) {
          tc.covered = cov;
          tc.num_conds = conds;
          tc.extra_branches = extra_br;
          tc.non_state_eb = nseb;
          tc.draws_trump = draws_trump;
          tc.line = cand;
          changed = true;
        }
        break;
      }
    }
    if (!found_sig) {
      if (static_cast<int>(top_candidates.size()) < top_k ||
          is_better_meta(cov, conds, extra_br, draws_trump,
                         top_candidates.back().covered,
                         top_candidates.back().num_conds,
                         top_candidates.back().extra_branches,
                         top_candidates.back().draws_trump) ||
          (top_k > 1 &&
           (nseb < min_existing_nseb ||
            (nseb == min_existing_nseb &&
             is_better_meta(cov, conds, extra_br, draws_trump, min_nseb_cov,
                            min_nseb_conds, min_nseb_eb, min_nseb_dt))))) {
        top_candidates.push_back(TopCandidate{
            cov, conds, extra_br, nseb, draws_trump, std::move(sig), cand});
        changed = true;
      }
    }
    if (changed) {
      std::stable_sort(
          top_candidates.begin(), top_candidates.end(),
          [&](const TopCandidate &a, const TopCandidate &b) {
            return is_better_meta(a.covered, a.num_conds, a.extra_branches,
                                  a.draws_trump, b.covered, b.num_conds,
                                  b.extra_branches, b.draws_trump);
          });
      if (static_cast<int>(top_candidates.size()) > top_k) {
        if (lvl > 0 && top_k > 1) {
          std::set<size_t> seen_nseb;
          std::stable_partition(
              top_candidates.begin(), top_candidates.end(),
              [&](const TopCandidate &tc) {
                return tc.non_state_eb == 0 ||
                       seen_nseb.insert(tc.non_state_eb).second;
              });
        }
        if (top_k > 1) {
          size_t min_nseb_idx = 0;
          size_t min_cand_nseb = top_candidates[0].non_state_eb;
          for (size_t k = 1; k < top_candidates.size(); ++k) {
            if (top_candidates[k].non_state_eb < min_cand_nseb) {
              min_cand_nseb = top_candidates[k].non_state_eb;
              min_nseb_idx = k;
            }
          }
          if (min_nseb_idx >= static_cast<size_t>(top_k)) {
            std::swap(top_candidates[top_k - 1], top_candidates[min_nseb_idx]);
          }
        }
        top_candidates.resize(top_k);
      }
      most_boards_covered = top_candidates.front().covered;
    }
    return changed;
  };

  auto req_group_cov = [&](size_t j) -> int {
    int req = top_k_prune_floor(groupings[j].size(), group_draws_trump[j]);
    if (!group_uniform_t1[j].first.empty()) {
      auto it = best_t1_cov_conds.find(group_uniform_t1[j]);
      if (it != best_t1_cov_conds.end()) {
        int c_t1 = it->second.first;
        size_t min_conds = it->second.second;
        req = std::max(
            req, (groupings[j].size() >= min_conds) ? (c_t1 + 1) : c_t1);
      }
    } else if (lvl > 0 && most_boards_covered > 0) {
      req = std::max(req, most_boards_covered);
    }
    if (top_k <= 1 && known_best_cov > 0) {
      bool skip_known_tie =
          lvl > 0 && j != known_best_idx &&
          !(group_draws_trump[j] && groupings[j].size() == 1 &&
            (known_best_idx >= groupings.size() ||
             !group_draws_trump[known_best_idx]));
      req = std::max(req,
                     skip_known_tie ? (known_best_cov + 1) : known_best_cov);
    }
    return req;
  };
  auto is_pruned = [&](size_t j) {
    return group_possible_boards[j] < req_group_cov(j);
  };
  // Estimated size of each batch the groups after `i` may still need: for
  // every group that can beat the best coverage so far, the weight of its
  // tricks that are neither cached nor part of the batch being launched.
  // Groups that can cover the most deals come first, since they outlast the
  // rising bar longest; the estimate is an upper bound either way.
  auto remaining_batches = [&](size_t i,
                               const std::set<std::string> *in_batch) {
    if (progress == nullptr) {
      return std::vector<double>{};
    }
    std::vector<std::pair<int, double>> possible_and_size;
    std::set<std::string> counted;
    for (size_t j = i + 1; j < groupings.size(); ++j) {
      if (is_pruned(j)) continue;
      double size = 0.0;
      for (size_t k = 0; k < keys[j].size(); ++k) {
        const std::string &key = keys[j][k];
        if (saved_work.count(key) == 0 &&
            (in_batch == nullptr || in_batch->count(key) == 0) &&
            counted.insert(key).second) {
          size += weights[j][k];
        }
      }
      if (size > 0.0) {
        possible_and_size.emplace_back(group_possible_boards[j], size);
      }
    }
    std::stable_sort(possible_and_size.begin(), possible_and_size.end(),
                     [](const auto &a, const auto &b) {
                       return a.first > b.first;
                     });
    std::vector<double> sizes;
    sizes.reserve(possible_and_size.size());
    for (const auto &[_possible, size] : possible_and_size) {
      sizes.push_back(size);
    }
    return sizes;
  };
  // Evaluates `batch` (tricks not yet in saved_work) as one progress batch,
  // in parallel when inside the OpenMP region.
  auto run_batch = [&](
      size_t i,
      std::vector<std::pair<std::string, const GeneralizedTrick *>> &batch,
      const std::set<std::string> &batch_seen) {
    std::stable_sort(
        batch.begin(), batch.end(),
        [](const auto &a, const auto &b) {
          return a.second->NumBoards() > b.second->NumBoards();
        });
    std::vector<int> batch_req_cov(batch.size(), 1);
    int max_poss_sentinel = static_cast<int>(board_offsets.size()) + 1;
    for (size_t b_idx = 0; b_idx < batch.size(); ++b_idx) {
      const std::string &key = batch[b_idx].first;
      size_t uid = key_to_uid.at(key);
      int trick_ub = u_upper_bound[uid];
      int min_req = max_poss_sentinel;
      for (size_t j = i; j < groupings.size(); ++j) {
        if (is_pruned(j)) continue;
        for (size_t k = 0; k < keys[j].size(); ++k) {
          if (keys[j][k] == key) {
            int other_poss = group_possible_boards[j] - trick_ub;
            int needed = std::max(1, req_group_cov(j) - other_poss);
            min_req = std::min(min_req, needed);
            break;
          }
        }
      }
      batch_req_cov[b_idx] = (min_req == max_poss_sentinel) ? 1 : min_req;
    }
    std::vector<SearchProgress::Node *> trick_progress(batch.size(), nullptr);
    if (progress != nullptr) {
      std::vector<double> batch_weights;
      batch_weights.reserve(batch.size());
      for (const auto &[_key, trick] : batch) {
        batch_weights.push_back(TrickProgressWeight(*trick, trump_suit));
      }
      trick_progress = SearchProgress::BeginBatch(
          progress, batch_weights, remaining_batches(i, &batch_seen));
    }
    std::vector<std::shared_ptr<ConditionalLine>> batch_results(batch.size());
    std::vector<std::vector<std::shared_ptr<ConditionalLine>>>
        batch_alt_results(batch.size());
    std::vector<int> batch_covered(batch.size(), 0);
    auto evaluate = [&](size_t b_idx) {
      if (IsStopRequested()) {
        SearchProgress::Complete(trick_progress[b_idx]);
        batch_results[b_idx] = nullptr;
        batch_covered[b_idx] = 0;
        return;
      }
      int req_cov = batch_req_cov[b_idx];
      std::vector<ConditionalLine> lines = GetLinesFromTrick(
          *batch[b_idx].second, min_tricks, cache, board_trees,
          board_offsets, leader, lvl, trick_progress[b_idx], req_cov,
          child_top_k);
      int covered = lines.empty() ? 0 : lines[0].CoveredFinalBoards();
      if (covered >= req_cov) {
        batch_results[b_idx] =
            std::make_shared<ConditionalLine>(std::move(lines[0]));
        batch_covered[b_idx] = covered;
        int alt_slack = std::max(2, covered / 4);
        int alt_floor = std::max(1, covered - alt_slack);
        for (size_t m = 1; m < lines.size(); ++m) {
          if (lines[m].CoveredFinalBoards() >= alt_floor) {
            batch_alt_results[b_idx].push_back(
                std::make_shared<ConditionalLine>(std::move(lines[m])));
          }
        }
      } else {
        batch_results[b_idx] = nullptr;
        batch_covered[b_idx] = std::max(0, req_cov - 1);
      }
      if (lvl == 0 && covered >= req_cov) {
        // A single first trick is itself a complete line, so the deals it
        // makes are a floor on the final coverage worth reporting right
        // away: the top-level batch can run for most of the solve.
        int seen = g_best_covered.load();
        while (covered > seen &&
               !g_best_covered.compare_exchange_weak(seen, covered)) {
        }
      }
    };
    ParallelTaskBatch(batch.size(), [&](size_t b_idx) {
      evaluate(b_idx);
    });
    for (size_t b_idx = 0; b_idx < batch.size(); ++b_idx) {
      const std::string &key = batch[b_idx].first;
      saved_covered[key] = batch_covered[b_idx];
      saved_work[key] = std::move(batch_results[b_idx]);
      if (!batch_alt_results[b_idx].empty()) {
        saved_work_alts[key] = std::move(batch_alt_results[b_idx]);
      }
      size_t uid = key_to_uid.at(key);
      u_evaluated[uid] = 1;
      u_upper_bound[uid] = batch_covered[b_idx];
    }

    // Tighten upper bounds on unevaluated tricks using evaluated subsets:
    // if B(v_1), ..., B(v_k) are disjoint subsets of B(u) with identical
    // concrete tricks on each board, then
    // cov(u) <= sum_r cov(v_r) + |B(u) \ union_r B(v_r)|.
    std::vector<size_t> eval_with_losses;
    for (size_t v = 0; v < num_unique; ++v) {
      if (u_evaluated[v] &&
          u_upper_bound[v] < static_cast<int>(unique_tricks[v]->NumBoards())) {
        eval_with_losses.push_back(v);
      }
    }
    if (!eval_with_losses.empty()) {
      std::sort(eval_with_losses.begin(), eval_with_losses.end(),
                [&](size_t a, size_t b) {
                  int loss_a = static_cast<int>(unique_tricks[a]->NumBoards()) -
                               u_upper_bound[a];
                  int loss_b = static_cast<int>(unique_tricks[b]->NumBoards()) -
                               u_upper_bound[b];
                  return loss_a > loss_b;
                });
      std::vector<uint64_t> rem_mask(num_words);
      for (size_t u = 0; u < num_unique; ++u) {
        if (u_evaluated[u]) continue;
        const uint64_t *mu = &u_masks[u * num_words];
        bool any_matched = false;
        int bound_sum = 0;
        for (size_t w = 0; w < num_words; ++w) {
          rem_mask[w] = mu[w];
        }
        for (size_t v : eval_with_losses) {
          if (u_my1[v] != u_my1[u] || u_my2[v] != u_my2[u]) continue;
          if (unique_tricks[v]->NumBoards() > unique_tricks[u]->NumBoards()) {
            continue;
          }
          const uint64_t *mv = &u_masks[v * num_words];
          bool is_sub = true;
          for (size_t w = 0; w < num_words; ++w) {
            if ((mv[w] & ~rem_mask[w]) != 0) {
              is_sub = false;
              break;
            }
          }
          if (!is_sub) continue;
          if (!SameConcreteTricksOnSubset(*unique_tricks[u],
                                          *unique_tricks[v])) {
            continue;
          }
          for (size_t w = 0; w < num_words; ++w) {
            rem_mask[w] &= ~mv[w];
          }
          bound_sum += u_upper_bound[v];
          any_matched = true;
        }
        if (any_matched) {
          int rem_boards = 0;
          for (size_t w = 0; w < num_words; ++w) {
            rem_boards += PopCount64(rem_mask[w]);
          }
          u_upper_bound[u] = std::min(u_upper_bound[u], bound_sum + rem_boards);
        }
      }
    }

    for (size_t j = i; j < groupings.size(); ++j) {
      int updated_possible = 0;
      bool all_known = true;
      for (size_t k = 0; k < groupings[j].size(); ++k) {
        size_t uid = group_trick_ids[j][k];
        updated_possible += u_upper_bound[uid];
        if (!u_evaluated[uid] || !saved_work.at(keys[j][k])) {
          all_known = false;
        }
      }
      group_possible_boards[j] = updated_possible;
      if (all_known && updated_possible >= req_group_cov(j) &&
          updated_possible > known_best_cov) {
        known_best_cov = updated_possible;
        known_best_idx = j;
      }
    }
    if (progress != nullptr) {
      SearchProgress::EndBatch(progress, remaining_batches(i, nullptr));
    }
  };

  for (unsigned int i = 0; i < groupings.size(); i++) {
    if (IsStopRequested()) {
      break;
    }
    const std::vector<GeneralizedTrick> &group = groupings[i];
    VLOG(2) << "Trying group " << i << " with tricks " << group.size();
    int most_possible_boards = group_possible_boards[i];
    VLOG(2) << "lvl " << lvl << " most_possible_boards " << most_possible_boards
            << " vs " << most_boards_covered;
    if (is_pruned(i)) {
      VLOG(2) << "Skipping, it can only cover " << most_possible_boards
              << " vs " << most_boards_covered;
      continue;
    }

    if (lvl == 0) {
      g_best_covered = std::max(most_boards_covered, known_best_cov);
    }

    std::set<Trick1CardPair> batch_t1_seen;
    auto add_group_to_batch =
        [&](size_t j,
            std::vector<std::pair<std::string, const GeneralizedTrick *>> &b,
            std::set<std::string> &seen) {
          if (!group_uniform_t1[j].first.empty()) {
            batch_t1_seen.insert(group_uniform_t1[j]);
          }
          for (size_t k = 0; k < groupings[j].size(); ++k) {
            const std::string &key = keys[j][k];
            if (saved_work.count(key) == 0 && seen.count(key) == 0) {
              seen.insert(key);
              b.emplace_back(key, &groupings[j][k]);
            }
          }
        };

    bool wide_top_batch =
        omp_in_parallel() &&
        (lvl == 0 && i == 0 && leader % 2 == 0 && board_offsets.size() >= 30);

    std::vector<std::pair<std::string, const GeneralizedTrick *>> batch;
    std::set<std::string> batch_seen;
    add_group_to_batch(i, batch, batch_seen);
    bool has_uncomputed = !batch.empty();

    if (omp_in_parallel() && has_uncomputed && lvl <= 7) {
      if (wide_top_batch) {
        int max_possible = group_possible_boards[i];
        for (size_t j = i; j < groupings.size(); ++j) {
          max_possible = std::max(max_possible, group_possible_boards[j]);
        }
        int thresh = max_possible - (max_possible / 20);
        for (size_t j = i + 1; j < groupings.size(); ++j) {
          if (!is_pruned(j) && group_possible_boards[j] >= thresh &&
              groupings[j].size() == 1 &&
              !group_uniform_t1[j].first.empty() &&
              batch_t1_seen.count(group_uniform_t1[j]) == 0) {
            add_group_to_batch(j, batch, batch_seen);
          }
        }
      } else {
        int max_possible_so_far = group_possible_boards[i];
        size_t best_j = i;
        for (size_t j = i + 1; j < groupings.size(); ++j) {
          if (!is_pruned(j) &&
              group_possible_boards[j] > max_possible_so_far) {
            max_possible_so_far = group_possible_boards[j];
            best_j = j;
          }
        }
        if (best_j != i) {
          add_group_to_batch(best_j, batch, batch_seen);
        }
        if (lvl == 0 && i >= 1) {
          size_t max_batch = 16;
          for (size_t j = i + 1;
               j < groupings.size() && batch.size() < max_batch; ++j) {
            if (!is_pruned(j) && groupings[j].size() == 1 &&
                !group_uniform_t1[j].first.empty() &&
                batch_t1_seen.count(group_uniform_t1[j]) == 0) {
              add_group_to_batch(j, batch, batch_seen);
            }
          }
        }
      }
    }
    if (has_uncomputed) {
      run_batch(i, batch, batch_seen);
    }
    if (IsStopRequested()) {
      break;
    }

    int sum_covered = 0;
    bool group_failed_low = false;
    for (size_t k = 0; k < group.size(); ++k) {
      if (!saved_work.at(keys[i][k])) {
        group_failed_low = true;
        break;
      }
      sum_covered += saved_covered.at(keys[i][k]);
    }
    if (group_failed_low || sum_covered < req_group_cov(i)) {
      continue;
    }

    // Make candidate lines for this group
    std::vector<std::vector<std::shared_ptr<ConditionalLine>>> group_branches(
        group.size());
    for (size_t k = 0; k < group.size(); ++k) {
      const std::string &t_str = keys[i][k];
      group_branches[k].push_back(saved_work.at(t_str));
      auto alt_it = saved_work_alts.find(t_str);
      if (alt_it != saved_work_alts.end()) {
        group_branches[k].insert(group_branches[k].end(),
                                 alt_it->second.begin(), alt_it->second.end());
      }
    }
    std::vector<ConditionalLine> group_lines = AssembleBranchLines(
        group_branches, child_top_k > 1, lvl, /*min_diff_branches=*/1,
        [&](const std::vector<size_t> &picks) {
          ConditionalLine cl;
          for (size_t m = 0; m < group.size(); ++m) {
            cl.AddCondition(new TrickPlayCondition(keys[i][m]),
                            group_branches[m][picks[m]]);
          }
          return cl;
        });
    bool changed = false;
    for (size_t g_idx = 0; g_idx < group_lines.size(); ++g_idx) {
      const Trick1CardPair &t1 =
          (g_idx == 0) ? group_uniform_t1[i] : Trick1CardPair{"", ""};
      if (add_top_candidate(group_lines[g_idx], t1, group.size(),
                            group_draws_trump[i])) {
        changed = true;
      }
    }
    if (lvl == 0) {
      g_best_covered = std::max(most_boards_covered, known_best_cov);
    }
    if (changed && progress != nullptr) {
      SearchProgress::SetRemaining(progress, remaining_batches(i, nullptr));
    }
  }
  if (out_unsplit_ub != nullptr) {
    if (most_boards_covered >= std::max(1, min_covered)) {
      *out_unsplit_ub = most_boards_covered;
    } else {
      int max_poss = 0;
      for (int p : group_possible_boards) {
        max_poss = std::max(max_poss, p);
      }
      *out_unsplit_ub =
          (min_covered > 0) ? std::min(min_covered - 1, max_poss) : max_poss;
    }
  }
  if (progress != nullptr) {
    SearchProgress::SetRemaining(progress, {});
  }
  if (most_boards_covered < min_covered) {
    return {};
  }
  std::vector<ConditionalLine> ans;
  ans.reserve(top_candidates.size());
  for (auto &tc : top_candidates) {
    ans.push_back(std::move(tc.line));
  }
  VLOG(2) << "Returning lines " << ans.size() << " at lvl " << lvl;
  return ans;
}

using std::string;

bool WonEnoughTricks(int mini_max, int min_tricks, int leader,
                     int cards_left) {
  if (leader % 2 == 0) {
    return mini_max >= min_tricks;
  } else {
    return (cards_left - mini_max >= min_tricks);
  }
}

void LineFinder::TryBranchOnGoodCards(
    int min_tricks,
    SolverCache &cache_in,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &solvable_boards,
    int cards_left, int leader, int lvl,
    std::vector<ConditionalLine> &ans,
    SearchProgress::Node *progress,
    int min_covered, int top_k,
    Suit prev_suit_led, int state_depth,
    int unsplit_ub) {
  if (top_k < 0) {
    top_k = TopKForLevel(lvl, solvable_boards.size());
  }
  int solvable_n = static_cast<int>(solvable_boards.size());
  int best_covered = ans.empty() ? 0 : ans[0].CoveredFinalBoards();
  bool has_unsplit_play = (best_covered > 0 || unsplit_ub != 0);
  int min_endgame_unit = (cards_left <= 5) ? 1 : 2;
  int split_min_gain =
      has_unsplit_play ? std::max(min_endgame_unit, solvable_n / 30) : 1;
  int min_branch_cov =
      has_unsplit_play ? std::max(min_endgame_unit, solvable_n / 15) : 1;
  int min_possible_target = std::max(best_covered + 1, min_covered);
  if (IsStopRequested() || lvl <= 0 || leader % 2 != 0 || state_depth >= 4 ||
      cards_left <= 0 || solvable_n < min_possible_target || solvable_n < 2) {
    return;
  }
  // The split searches count like one more batch of the trick groups the
  // node has already run.
  SearchProgress::Node *split_progress =
      progress != nullptr
          ? SearchProgress::AddChildren(progress,
                                        {SearchProgress::kBatchDecay})[0]
          : nullptr;
  SearchProgress::Completer split_completer(split_progress);

  int child_top_k =
      (top_k <= 1 || top_k_lvl1_ <= 1 || solvable_boards.size() < 4)
          ? 1
          : std::min(top_k, TopKForLevel(lvl + 1, solvable_boards.size()));

  auto adopt_split_results = [&](std::vector<ConditionalLine> &true_lines,
                                 std::vector<ConditionalLine> &false_lines,
                                 int total_cov,
                                 auto make_cond) {
    best_covered = total_cov;
    std::vector<std::vector<std::shared_ptr<ConditionalLine>>> split_branches(
        2);
    for (auto &tl : true_lines) {
      if (tl.CoveredFinalBoards() > 0) {
        split_branches[0].push_back(
            std::make_shared<ConditionalLine>(std::move(tl)));
      }
    }
    for (auto &fl : false_lines) {
      if (fl.CoveredFinalBoards() > 0) {
        split_branches[1].push_back(
            std::make_shared<ConditionalLine>(std::move(fl)));
      }
    }
    int alt_slack = std::max(2, total_cov / 4);
    int alt_floor = std::max(1, total_cov - alt_slack);
    std::vector<ConditionalLine> new_splits = AssembleBranchLines(
        split_branches, top_k > 1, /*lvl=*/1, /*min_diff_branches=*/2,
        [&](const std::vector<size_t> &picks) {
          ConditionalLine sl;
          sl.AddCondition(make_cond(true), split_branches[0][picks[0]]);
          sl.AddCondition(make_cond(false), split_branches[1][picks[1]]);
          return sl;
        });
    if (new_splits.size() > 1) {
      new_splits.erase(
          std::remove_if(new_splits.begin() + 1, new_splits.end(),
                         [alt_floor](const ConditionalLine &sl) {
                           return sl.CoveredFinalBoards() < alt_floor;
                         }),
          new_splits.end());
    }
    if (top_k > 1 && !ans.empty() && !new_splits.empty()) {
      size_t min_new_nseb = NonStateExtraBranches(new_splits[0]);
      for (size_t k = 1; k < new_splits.size(); ++k) {
        min_new_nseb =
            std::min(min_new_nseb, NonStateExtraBranches(new_splits[k]));
      }
      size_t min_old_idx = 0;
      size_t min_old_nseb = NonStateExtraBranches(ans[0]);
      for (size_t k = 1; k < ans.size(); ++k) {
        size_t nseb = NonStateExtraBranches(ans[k]);
        if (nseb < min_old_nseb) {
          min_old_nseb = nseb;
          min_old_idx = k;
        }
      }
      if (min_old_nseb < min_new_nseb) {
        if (min_old_idx != 0) {
          ans[0] = std::move(ans[min_old_idx]);
        }
        ans.resize(1);
      } else {
        ans.clear();
      }
    } else {
      ans.clear();
    }
    for (auto &sl : new_splits) {
      ans.push_back(std::move(sl));
    }
    FinalizeTopKLines(&ans, top_k);
  };

  const BoardState *first_bs =
      board_trees.at(*solvable_boards.begin())->MyBoard();
  Suit trump_suit = first_bs->GetTrump();
  uint64_t mask = ~first_bs->played_cards();
  uint64_t decl_cards = (first_bs->GetHand(0) | first_bs->GetHand(2)) & mask;
  uint64_t leader_cards = first_bs->GetHand(leader) & mask;

  std::set<int> disc_boards[2];
  std::set<int> no_disc_boards[2];
  int any_disc_count = 0;
  int both_disc_count = 0;
  if (prev_suit_led >= 0 && prev_suit_led < 4) {
    for (int b : solvable_boards) {
      const BoardState &bs = *board_trees.at(b)->MyBoard();
      bool d1 = bs.HasPlayerDiscardedOnSuit(1, prev_suit_led);
      bool d3 = bs.HasPlayerDiscardedOnSuit(3, prev_suit_led);
      if (d1 || d3) ++any_disc_count;
      if (d1 && d3) ++both_disc_count;
      if (d1) {
        disc_boards[0].insert(disc_boards[0].end(), b);
      } else {
        no_disc_boards[0].insert(no_disc_boards[0].end(), b);
      }
      if (d3) {
        disc_boards[1].insert(disc_boards[1].end(), b);
      } else {
        no_disc_boards[1].insert(no_disc_boards[1].end(), b);
      }
    }
  }

  CardType top_cards[4] = {};
  std::set<int> good_boards[4];
  std::set<int> not_good_boards[4];
  int max_good_boards_size = 0;
  for (int suit = 0; suit < 4; ++suit) {
    uint16_t suit_bits = (decl_cards >> (suit * 16)) & 0xffff;
    uint16_t leader_suit_bits = (leader_cards >> (suit * 16)) & 0xffff;
    if (suit_bits == 0 || leader_suit_bits == 0) continue;
    int top_val = 31 - __builtin_clz(suit_bits);
    CardType top_card = Card::GetSuitedCard(static_cast<Suit>(suit), top_val);
    top_cards[suit] = top_card;
    IsCardGoodCondition cond_true(top_card, true);
    for (int b : solvable_boards) {
      if (cond_true.Evaluate(*board_trees.at(b)->MyBoard())) {
        good_boards[suit].insert(good_boards[suit].end(), b);
      } else {
        not_good_boards[suit].insert(not_good_boards[suit].end(), b);
      }
    }
    if (!good_boards[suit].empty() && !not_good_boards[suit].empty()) {
      max_good_boards_size = std::max(
          max_good_boards_size, static_cast<int>(good_boards[suit].size()));
    }
  }

  for (int suit = 0; suit < 4; ++suit) {
    if (IsStopRequested() || best_covered >= solvable_n) {
      break;
    }
    CardType top_card = top_cards[suit];
    if (top_card == 0) continue;
    if (static_cast<int>(good_boards[suit].size()) < min_branch_cov ||
        static_cast<int>(not_good_boards[suit].size()) < min_branch_cov) {
      continue;
    }
    int cur_target = std::max(best_covered + split_min_gain, min_covered);
    if (cur_target > solvable_n) {
      continue;
    }
    int false_ub = static_cast<int>(not_good_boards[suit].size());
    if (unsplit_ub >= 0) {
      int other_good_max = 0;
      for (int s2 = 0; s2 < 4; ++s2) {
        if (s2 != suit && !not_good_boards[s2].empty()) {
          other_good_max = std::max(
              other_good_max, static_cast<int>(good_boards[s2].size()));
        }
      }
      int ub_slack = solvable_n / 30;
      int extra_cap = std::max(any_disc_count, other_good_max);
      false_ub = std::min(false_ub, unsplit_ub + ub_slack + extra_cap);
    }
    if (static_cast<int>(good_boards[suit].size()) + false_ub < cur_target) {
      continue;
    }
    int req_true = std::max(min_branch_cov, cur_target - false_ub);
    Suit suit_enum = static_cast<Suit>(suit);
    GeneralizedTrickFinder cash_finder(trump_suit);
    for (int b : good_boards[suit]) {
      const OptimalLines &lines = board_trees.at(b)->GetLines();
      for (const OptimalLine &trick : lines) {
        if (!trick.empty() && Card::ParseSuit(trick[0]) == suit_enum) {
          cash_finder.RecordOptimalTrick(leader, trick, b);
        }
      }
    }
    GeneralizedTricksVector raw_tricks =
        cash_finder.GetGeneralizedTricks(500, /*populate_boards_set=*/false);
    GeneralizedTricksVector cashing_tricks;
    cashing_tricks.reserve(raw_tricks.size());
    for (auto &gt : raw_tricks) {
      if (gt.leader != leader || gt.demi1.my_card.empty() ||
          Card::ParseSuit(gt.demi1.my_card[0]) != suit_enum ||
          gt.board_entries.empty()) {
        continue;
      }
      bool all_cash_good = true;
      for (const auto &e : gt.board_entries) {
        if (e.winner % 2 != 0 || e.win_card[0] == '\0' ||
            Card::ParseSuit(e.win_card[0]) != suit_enum) {
          all_cash_good = false;
          break;
        }
        CardType win_c = Card::ParseStringCard(e.win_card);
        if (!IsCardGoodCondition(win_c, true)
                 .Evaluate(*board_trees.at(e.board)->MyBoard())) {
          all_cash_good = false;
          break;
        }
      }
      if (all_cash_good) {
        cashing_tricks.push_back(std::move(gt));
      }
    }
    if (cashing_tricks.empty()) {
      continue;
    }
    std::vector<std::vector<GeneralizedTrick>> all_groups =
        GroupTricks(std::move(cashing_tricks));
    std::vector<std::vector<GeneralizedTrick>> cashing_groups;
    for (auto &g : all_groups) {
      if (g.size() == 1 && static_cast<int>(g[0].NumBoards()) >= req_true) {
        cashing_groups.push_back(std::move(g));
      }
    }
    if (cashing_groups.empty()) {
      continue;
    }
    std::stable_sort(cashing_groups.begin(), cashing_groups.end(),
                     [](const std::vector<GeneralizedTrick> &a,
                        const std::vector<GeneralizedTrick> &b) {
                       return a[0].NumBoards() > b[0].NumBoards();
                     });
    std::vector<SearchProgress::Node *> half_progress =
        split_progress != nullptr
            ? SearchProgress::AddChildren(
                  split_progress,
                  {static_cast<double>(good_boards[suit].size()),
                   static_cast<double>(not_good_boards[suit].size())})
            : std::vector<SearchProgress::Node *>{nullptr, nullptr};
    std::vector<ConditionalLine> true_lines = GetLinesFromGroups(
        cashing_groups, min_tricks, cache_in, board_trees, good_boards[suit],
        leader, lvl + 1, half_progress[0], req_true, child_top_k);
    SearchProgress::Complete(half_progress[0]);
    int true_cov = true_lines.empty() ? 0 : true_lines[0].CoveredFinalBoards();
    if (true_lines.empty() || true_cov < req_true) {
      SearchProgress::Complete(half_progress[1]);
      continue;
    }
    int req_false = std::max(min_branch_cov, cur_target - true_cov);
    std::vector<ConditionalLine> false_lines = SolveHelper(
        min_tricks, cache_in, board_trees, not_good_boards[suit], leader,
        lvl + 1, half_progress[1], req_false, child_top_k, prev_suit_led,
        state_depth + 1);
    int false_cov =
        false_lines.empty() ? 0 : false_lines[0].CoveredFinalBoards();
    if (false_lines.empty() || false_cov < req_false) {
      continue;
    }
    int total_cov = true_cov + false_cov;
    if (total_cov >= cur_target) {
      adopt_split_results(
          true_lines, false_lines, total_cov,
          [top_card](bool is_good) -> PlayCondition * {
            return new IsCardGoodCondition(top_card, is_good);
          });
    }
  }

  if (prev_suit_led >= 0 && prev_suit_led < 4 && any_disc_count > 0) {
    int disc_min_branch_cov = (lvl <= 5) ? 1 : min_branch_cov;
    int disc_split_min_gain = (lvl <= 5) ? 1 : split_min_gain;
    int order[2] = {0, 1};
    if (disc_boards[1].size() > disc_boards[0].size()) {
      std::swap(order[0], order[1]);
    }
    bool first_disc_searched = false;
    for (int ord = 0; ord < 2; ++ord) {
      if (IsStopRequested() || best_covered >= solvable_n) {
        break;
      }
      int p_idx = order[ord];
      int player = (p_idx == 0) ? 1 : 3;
      const std::set<int> &true_b = disc_boards[p_idx];
      const std::set<int> &false_b = no_disc_boards[p_idx];
      if (static_cast<int>(true_b.size()) < disc_min_branch_cov ||
          static_cast<int>(false_b.size()) < disc_min_branch_cov) {
        continue;
      }
      int cur_target =
          std::max(best_covered + disc_split_min_gain, min_covered);
      if (cur_target > solvable_n) {
        continue;
      }
      bool skip_nested_disc = (both_disc_count == 0 && first_disc_searched);
      int false_ub = static_cast<int>(false_b.size());
      if (unsplit_ub >= 0) {
        int ub_slack = solvable_n / 30;
        int true_n = static_cast<int>(true_b.size());
        int other_only_disc = skip_nested_disc ? 0 : (any_disc_count - true_n);
        int extra_split_cap =
            std::max(other_only_disc, max_good_boards_size);
        false_ub = std::min(false_ub, unsplit_ub + ub_slack + extra_split_cap);
      }
      if (static_cast<int>(true_b.size()) + false_ub < cur_target) {
        continue;
      }
      first_disc_searched = true;
      int req_true = std::max(disc_min_branch_cov, cur_target - false_ub);
      Suit true_prev_suit =
          (both_disc_count == 0 ||
           both_disc_count == static_cast<int>(true_b.size()))
              ? NOTRUMP
              : prev_suit_led;
      Suit false_prev_suit = skip_nested_disc ? NOTRUMP : prev_suit_led;
      std::vector<SearchProgress::Node *> half_progress =
          split_progress != nullptr
              ? SearchProgress::AddChildren(
                    split_progress,
                    {static_cast<double>(true_b.size()),
                     static_cast<double>(false_b.size())})
              : std::vector<SearchProgress::Node *>{nullptr, nullptr};
      std::vector<ConditionalLine> true_lines = SolveHelper(
          min_tricks, cache_in, board_trees, true_b, leader, lvl + 1,
          half_progress[0], req_true, child_top_k, true_prev_suit,
          state_depth + 1);
      int true_cov =
          true_lines.empty() ? 0 : true_lines[0].CoveredFinalBoards();
      if (true_lines.empty() || true_cov < req_true) {
        SearchProgress::Complete(half_progress[1]);
        continue;
      }
      int req_false = std::max(disc_min_branch_cov, cur_target - true_cov);
      std::vector<ConditionalLine> false_lines = SolveHelper(
          min_tricks, cache_in, board_trees, false_b, leader, lvl + 1,
          half_progress[1], req_false, child_top_k, false_prev_suit,
          state_depth + 1);
      int false_cov =
          false_lines.empty() ? 0 : false_lines[0].CoveredFinalBoards();
      if (false_lines.empty() || false_cov < req_false) {
        continue;
      }
      int total_cov = true_cov + false_cov;
      if (total_cov >= cur_target) {
        Suit s = prev_suit_led;
        adopt_split_results(
            true_lines, false_lines, total_cov,
            [player, s](bool discarded) -> PlayCondition * {
              return new PlayerDiscardedCondition(player, s, discarded);
            });
      }
    }
  }
}

bool LineFinder::SolveMemo::Lookup(
    const std::string &key, int min_covered,
    std::vector<ConditionalLine> *out) {
  size_t shard = std::hash<std::string>{}(key) & (kMemoShards - 1);
  std::lock_guard<std::mutex> lock(mu[shard]);
  auto it = shards[shard].find(key);
  if (it == shards[shard].end()) {
    return false;
  }
  if (it->second.exact) {
    *out = it->second.lines;
    return true;
  }
  if (min_covered >= it->second.fail_low_bound) {
    out->clear();
    return true;
  }
  return false;
}

void LineFinder::SolveMemo::Insert(
    std::string key, int min_covered,
    const std::vector<ConditionalLine> &res) {
  int achieved = res.empty() ? 0 : res[0].CoveredFinalBoards();
  bool is_exact = (min_covered <= 1) || (achieved >= min_covered);
  if (!is_exact && achieved >= min_covered) {
    return;
  }
  size_t shard = std::hash<std::string>{}(key) & (kMemoShards - 1);
  std::lock_guard<std::mutex> lock(mu[shard]);
  auto it = shards[shard].find(key);
  if (it == shards[shard].end()) {
    if (ring[shard].size() < kMaxMemoPerShard) {
      ring[shard].push_back(key);
    } else {
      size_t &cur = cursor[shard];
      shards[shard].erase(ring[shard][cur]);
      ring[shard][cur] = key;
      cur = (cur + 1) % kMaxMemoPerShard;
      g_memo_evictions.fetch_add(1, std::memory_order_relaxed);
    }
    MemoEntry entry;
    if (is_exact) {
      entry.lines = res;
      entry.exact_min_cov = std::max(0, min_covered);
      entry.exact = true;
    } else {
      entry.fail_low_bound = min_covered;
      entry.exact = false;
    }
    shards[shard].emplace(std::move(key), std::move(entry));
  } else if (!it->second.exact) {
    if (is_exact) {
      it->second.lines = res;
      it->second.exact_min_cov = std::max(0, min_covered);
      it->second.exact = true;
    } else if (min_covered < it->second.fail_low_bound) {
      it->second.fail_low_bound = min_covered;
    }
  } else if (is_exact && res.size() > it->second.lines.size()) {
    it->second.lines = res;
    it->second.exact_min_cov =
        std::min(it->second.exact_min_cov, std::max(0, min_covered));
  }
}

std::vector<ConditionalLine> LineFinder::SolveHelper(
    int min_tricks,
    SolverCache &cache_in,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress,
    int min_covered, int top_k,
    Suit prev_suit_led, int state_depth) {
  if (IsStopRequested() || board_offsets.empty() ||
      static_cast<int>(board_offsets.size()) < min_covered) {
    SearchProgress::Complete(progress);
    return std::vector<ConditionalLine>();
  }
  const std::shared_ptr<OptimalPlayTree> &first_tree =
      board_trees.at(*board_offsets.begin());
  CHECK(first_tree);
  int cards_left = first_tree->CardsLeft();
  if (cards_left == 0) {
    SearchProgress::Complete(progress);
    if (min_tricks > 0) {
      return std::vector<ConditionalLine>();
    }
    return std::vector<ConditionalLine>{ConditionalLine(board_offsets)};
  }
  int cur_top_k =
      (top_k >= 0) ? top_k : TopKForLevel(lvl, board_offsets.size());
  bool use_memo = (lvl > 0 && solve_memo_ != nullptr);
  SolveMemoKeyBuilder kb;
  if (use_memo) {
    kb.Init(leader, min_tricks, prev_suit_led, cur_top_k, board_offsets.size(),
            /*allow_state_split=*/(state_depth < 4));
  }
  std::set<int> solvable_boards;
  for (int b : board_offsets) {
    const std::shared_ptr<OptimalPlayTree> &play_tree = board_trees.at(b);
    CHECK(play_tree);
    int mini_max = play_tree->MiniMax();
    int cur_player = play_tree->MyBoard()->CurPlayer();
    if (WonEnoughTricks(mini_max, min_tricks, cur_player, cards_left)) {
      solvable_boards.insert(solvable_boards.end(), b);
      if (use_memo) {
        const BoardState *bs_ptr = play_tree->MyBoard();
        kb.AddBoard(b, bs_ptr->played_cards(), bs_ptr->discarded_suits_mask());
      }
    }
  }
  if (solvable_boards.empty() ||
      static_cast<int>(solvable_boards.size()) < min_covered) {
    SearchProgress::Complete(progress);
    return std::vector<ConditionalLine>();
  }
  if (!use_memo) {
    return SolveHelperUncached(min_tricks, cache_in, board_trees,
                               solvable_boards, leader, lvl, progress,
                               min_covered, cur_top_k, prev_suit_led,
                               state_depth);
  }
  std::string key = kb.Finish();
  std::vector<ConditionalLine> cached_lines;
  if (solve_memo_->Lookup(key, min_covered, &cached_lines)) {
    g_memo_hits.fetch_add(1, std::memory_order_relaxed);
    SearchProgress::Complete(progress);
    return cached_lines;
  }
  g_memo_misses.fetch_add(1, std::memory_order_relaxed);
  std::vector<ConditionalLine> res = SolveHelperUncached(
      min_tricks, cache_in, board_trees, solvable_boards, leader, lvl,
      progress, min_covered, cur_top_k, prev_suit_led, state_depth);
  if (!IsStopRequested()) {
    solve_memo_->Insert(std::move(key), min_covered, res);
  }
  return res;
}

std::vector<ConditionalLine> LineFinder::SolveHelperUncached(
    int min_tricks,
    SolverCache &cache_in,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress,
    int min_covered, int top_k,
    Suit prev_suit_led, int state_depth) {
  SearchProgress::Completer progress_completer(progress);
  CHECK_LT(lvl, 30);
  if (lvl < 3) {
    VLOG(1) << "SolveHelper lvl " << lvl << " to win " << min_tricks
            << " on boards " << board_trees.size()
            << " (" << StrJoin(board_offsets, ", ") << ")";
  }
  if (board_offsets.empty()) {
    return std::vector<ConditionalLine>();
  }
  if (top_k < 0) {
    top_k = TopKForLevel(lvl, board_offsets.size());
  }
  const std::shared_ptr<OptimalPlayTree> &first_tree =
      board_trees.at(*board_offsets.begin());
  Suit trump_suit = first_tree->MyBoard()->GetTrump();
  int cards_left = first_tree->CardsLeft();
  GeneralizedTrickFinder gen_trick_finder(trump_suit);
  for (int b : board_offsets) {
    const std::shared_ptr<OptimalPlayTree> &play_tree = board_trees.at(b);
    const OptimalLines &lines = play_tree->GetLines();
    for (const OptimalLine &trick : lines) {
      VLOG(3) << "    " << trick;
      gen_trick_finder.RecordOptimalTrick(leader, trick, b);
    }
  }
  unsigned int kMaxTricks = 500;
  GeneralizedTricksVector gen_tricks =
      gen_trick_finder.GetGeneralizedTricks(kMaxTricks,
                                            /*populate_boards_set=*/false);
  bool hit_max_tricks = (gen_tricks.size() == kMaxTricks);
  if (hit_max_tricks) {
    LOG(INFO) << "Hit kMaxTricks " << kMaxTricks;
  }
  if (gen_tricks.empty()) {
    if (lvl < 3) {
      VLOG(1) << "Giving up";
    }
    return std::vector<ConditionalLine>();
  }
  std::vector<std::vector<GeneralizedTrick>> groupings =
      GroupTricks(std::move(gen_tricks));

  auto group_total_boards = [](const std::vector<GeneralizedTrick> &g) {
    size_t sum = 0;
    for (const auto &t : g) sum += t.NumBoards();
    return sum;
  };
  std::stable_sort(
      groupings.begin(), groupings.end(),
      [&](const std::vector<GeneralizedTrick> &a,
          const std::vector<GeneralizedTrick> &b) {
        size_t tot_a = group_total_boards(a);
        size_t tot_b = group_total_boards(b);
        if (tot_a != tot_b) return tot_a > tot_b;
        if (a.size() != b.size()) return a.size() < b.size();
        bool dt_a = GroupDrawsTrump(a, trump_suit);
        bool dt_b = GroupDrawsTrump(b, trump_suit);
        return dt_a > dt_b;
      });

  // Null-move leads are double-dummy optimal on nearly every board, so they
  // always carry the largest upper bound and would otherwise be searched
  // first and win every coverage tie. Searching them last keeps them as
  // candidates (a squeeze or entry play may genuinely need them) but lets an
  // informative trick with equal coverage be preferred.
  std::set<std::string> null_leads = FindNullMoveLeads(groupings, trump_suit);
  if (!null_leads.empty()) {
    std::stable_partition(
        groupings.begin(), groupings.end(),
        [&null_leads](const std::vector<GeneralizedTrick> &g) {
          return g.empty() || g[0].leader % 2 != 0 ||
                 null_leads.count(g[0].demi1.my_card) == 0;
        });
  }

  VLOG(2) << "See groupings " << groupings.size();

  int raw_unsplit_ub = -1;
  std::vector<ConditionalLine> ans =
      GetLinesFromGroups(groupings, min_tricks, cache_in, board_trees,
                         board_offsets, leader, lvl, progress, min_covered,
                         top_k, hit_max_tricks ? nullptr : &raw_unsplit_ub);
  TryBranchOnGoodCards(min_tricks, cache_in, board_trees, board_offsets,
                       cards_left, leader, lvl, ans, progress, min_covered,
                       top_k, prev_suit_led, state_depth, raw_unsplit_ub);
  return ans;
}
