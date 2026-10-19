#include "line_finder.h"

#include <ctype.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
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

void PlayTrick(const std::string &trick, BoardState &bs) {
  return bs.PlayTrick(trick);
}

// Plays a card on `bs` matching `card_str` (e.g. "SA", "Cx", "D2").
// Returns true if a legal card in `bs.ValidCardsToPlay()` was matched and
// played, or false if the board cannot follow this play.
bool PlayReplayCardOnBoard(const std::string &card_str, BoardState &bs) {
  if (card_str.size() < 2) return false;
  Suit suit = Card::ParseSuit(static_cast<char>(std::toupper(card_str[0])));
  if (suit == NOTRUMP) return false;
  char rch = card_str[1];
  if (card_str.size() >= 3 && card_str[1] == '1' && card_str[2] == '0') {
    rch = 'T';
  }
  bool is_spot_wildcard = (rch == 'x' || rch == '_');
  bool is_honor_wildcard = (rch == 'X');
  CardType exact_card = 0;
  if (!is_spot_wildcard && !is_honor_wildcard) {
    std::string norm_str(1, static_cast<char>(std::toupper(card_str[0])));
    norm_str += static_cast<char>(std::toupper(rch));
    exact_card = Card::ParseStringCard(norm_str);
  }

  std::vector<CardType> valid = bs.ValidCardsToPlay();
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

      bool valid_for_branch = true;
      for (const auto &trick_cards : replay_tricks_) {
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
        if (!valid_for_branch) break;
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

namespace {
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

void MaybeNotifyProgress() {
  if (!g_has_progress_cb.load(std::memory_order_relaxed)) return;
  LineFinder::ProgressCallback cb;
  {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    cb = g_progress_cb;
  }
  if (cb) {
    cb(LineFinder::GetSolveStatus(), LineFinder::GetSolveProgress());
  }
}
}  // namespace

static std::atomic<uint64_t> get_line_from_trick_calls{0};
static std::atomic<uint64_t> g_play_trick_calls{0};
static std::atomic<uint64_t> g_memo_hits{0};
static std::atomic<uint64_t> g_memo_misses{0};
static std::atomic<uint64_t> g_memo_evictions{0};

void LineFinder::SetProgressCallback(ProgressCallback cb) {
  std::lock_guard<std::mutex> lock(g_status_mutex);
  g_has_progress_cb.store(static_cast<bool>(cb), std::memory_order_relaxed);
  g_progress_cb = std::move(cb);
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
    if ((tick & 63u) == 0u) {
      MaybeNotifyProgress();
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
  int pct = static_cast<int>(std::floor(progress->Fraction() * 100.0));
  return "Searching play trees: ~" + std::to_string(pct) +
         "% done (best coverage: " + std::to_string(g_best_covered.load()) +
         "/" + std::to_string(g_total_boards.load()) + " deals, " +
         std::to_string(get_line_from_trick_calls.load()) + " branches)...";
}

void LineFinder::SetSolveStatus(const std::string &status) {
  {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_solve_status = status;
  }
  MaybeNotifyProgress();
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
  bool has_higher_same_suit = false;
  for (const auto &l : base_tree.GetLines()) {
    if (l.size() >= 3 && l.compare(0, 3, forced_lead_prefix) == 0) {
      matching.push_back(&l);
    } else if (l.size() >= 2) {
      auto c_dec = Card::DecodeCard(Card::ParseStringCard(l.substr(0, 2)));
      if (c_dec.first == lead_decoded.first &&
          c_dec.second > lead_decoded.second) {
        has_higher_same_suit = true;
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
  if (!has_higher_same_suit) {
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
  std::vector<std::vector<uint8_t>> board_line_ok(
      num_boards, std::vector<uint8_t>(num_lines, 0));
  std::vector<std::vector<ConditionalLine::RescoreBoardTrace>> board_line_trace(
      num_boards, std::vector<ConditionalLine::RescoreBoardTrace>(num_lines));

  ParallelForDynamic(num_boards, num_threads, [&](size_t b_i) {
    if (LineFinder::IsStopRequested()) return;
    int b = active_boards[b_i];
    ConditionalLine::DefenderCardsCache def_cache;
    for (size_t l_i = 0; l_i < num_lines; ++l_i) {
      ConditionalLine::RescoreBoardTrace trace;
      int tricks = (*lines)[l_i].ResultsOnHandAdversarial(
          boards[b], &def_cache, &trace);
      if (tricks >= min_tricks && trace.leaf != nullptr) {
        board_line_ok[b_i][l_i] = 1;
        board_line_trace[b_i][l_i] = std::move(trace);
      }
    }
  });

  for (size_t l_i = 0; l_i < num_lines; ++l_i) {
    std::unordered_map<const ConditionalLine *, std::set<int>> leaf_boards;
    std::unordered_map<const ConditionalLine *, std::map<int, std::string>>
        node_board_tricks;
    for (size_t b_i = 0; b_i < num_boards; ++b_i) {
      if (!board_line_ok[b_i][l_i]) continue;
      int b = active_boards[b_i];
      const auto &tr = board_line_trace[b_i][l_i];
      leaf_boards[tr.leaf].insert(b);
      for (const auto &[node_ptr, trick_str] : tr.node_tricks) {
        node_board_tricks[node_ptr][b] = trick_str;
      }
    }
    (*lines)[l_i].ApplyRescoredBoards(leaf_boards, node_board_tricks);
  }

  std::stable_sort(lines->begin(), lines->end(),
                   [](const ConditionalLine &a, const ConditionalLine &b) {
                     if (a.CoveredFinalBoards() != b.CoveredFinalBoards()) {
                       return a.CoveredFinalBoards() > b.CoveredFinalBoards();
                     }
                     if (a.step1_covered_boards() != b.step1_covered_boards()) {
                       return a.step1_covered_boards() >
                              b.step1_covered_boards();
                     }
                     return a.conditions().size() < b.conditions().size();
                   });
}

void PopulateDealSummaries(const std::vector<BoardState> &boards,
                           int num_iters, int min_tricks,
                           std::vector<ConditionalLine> *lines) {
  CrossHandDealSetSummarizer summarizer;
  for (size_t line_idx = 0; line_idx < lines->size(); ++line_idx) {
    if (LineFinder::IsStopRequested()) break;
    auto &line = (*lines)[line_idx];
    line.set_total_boards(num_iters);
    line.set_min_tricks(min_tricks);
    if (!boards.empty()) {
      line.set_sample_board(boards[0]);
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

std::set<std::string> ExtractWinningCardsForLeader(
    const GeneralizedTrick &gen_trick, int next_leader,
    const std::set<int> &board_offsets) {
  std::set<std::string> winning_cards;
  int card_idx = (next_leader - gen_trick.leader + 4) % 4;
  for (int b : board_offsets) {
    const std::string &trick_str = gen_trick.GetTrickForBoard(b);
    size_t pos = 0;
    for (int c = 0; c < card_idx; ++c) {
      while (pos < trick_str.size() && trick_str[pos] == ' ') ++pos;
      while (pos < trick_str.size() && trick_str[pos] != ' ') ++pos;
    }
    while (pos < trick_str.size() && trick_str[pos] == ' ') ++pos;
    size_t end = pos;
    while (end < trick_str.size() && trick_str[end] != ' ') ++end;
    if (end > pos) {
      winning_cards.emplace(trick_str, pos, end - pos);
    }
  }
  return winning_cards;
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
      replay_tricks_.empty() && predealt_boards_.empty() &&
      !cur_trick.empty() &&
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
  if (cur_trick.empty() && replay_tricks_.empty() && predealt_boards_.empty()) {
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
                 std::to_string(boards.size()) + " deals...");
  auto progress = std::make_shared<SearchProgress>();
  SetActiveProgress(progress);
  std::vector<ConditionalLine> ans;
  ParallelRegion(effective_threads, [&]() {
    ans = SolveHelper(min_tricks, cache, board_trees,
                      board_offsets, leader, 0, progress->root());
  });
  SetActiveProgress(nullptr);
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

  DeduplicateTopLevelLines(&ans, top_k_lines_ <= 1);

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
    if (!keep_all_top_k_) {
      int best_step2 = ans[0].CoveredFinalBoards();
      size_t keep_n = 1;
      while (keep_n < ans.size() &&
             ans[keep_n].CoveredFinalBoards() == best_step2) {
        ++keep_n;
      }
      ans.resize(keep_n);
      DeduplicateTopLevelLines(&ans, /*collapse_same_t1=*/true);
    }
  } else if (!keep_all_top_k_ && top_k_lines_ > 1 && !ans.empty()) {
    int best_step1 = ans[0].CoveredFinalBoards();
    size_t keep_n = 1;
    while (keep_n < ans.size() &&
           ans[keep_n].CoveredFinalBoards() == best_step1) {
      ++keep_n;
    }
    ans.resize(keep_n);
    DeduplicateTopLevelLines(&ans, /*collapse_same_t1=*/true);
  }

  auto start_invariant = std::chrono::high_resolution_clock::now();
  PopulateDealSummaries(boards, num_iters_, min_tricks, &ans);
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

std::string PrintBoardToTrick(const std::map<int, std::string> &arg) {
  std::string ans;
  for (const auto &[b, trick] : arg) {
    ans += std::to_string(b) + " -> " + trick;// + "; ";
  }
  return ans;
}

ConditionalLine LineFinder::GetLineFromTrick(
    const GeneralizedTrick &gen_trick,
    int min_tricks,
    SolverCache &cache,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress,
    int min_covered) {
  std::vector<ConditionalLine> res = GetLinesFromTrick(
      gen_trick, min_tricks, cache, board_trees, board_offsets, leader, lvl,
      progress, min_covered);
  if (res.empty()) {
    return ConditionalLine();
  }
  return std::move(res[0]);
}

std::vector<ConditionalLine> LineFinder::GetLinesFromTrick(
    const GeneralizedTrick &gen_trick,
    int min_tricks,
    SolverCache &cache,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress,
    int min_covered) {
  SearchProgress::Completer progress_completer(progress);
  size_t num_trick_boards = gen_trick.NumBoards();
  if (IsStopRequested() ||
      min_covered > static_cast<int>(num_trick_boards)) {
    return {};
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
  int child_top_k = (lvl == 0) ? top_k_lvl1_ : 1;

  auto eval_branch = [&](size_t idx, int branch_min_cov) {
    const WinnerBranch &wb = entries[idx];
    if (IsStopRequested() || wb.entries.empty() ||
        static_cast<int>(wb.boards.size()) < branch_min_cov) {
      SearchProgress::Complete(branch_progress[idx]);
      return;
    }
    int next_leader = wb.next_leader;
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

    std::string memo_key;
    if (solve_memo_ != nullptr) {
      memo_key.reserve(3 + wb.entries.size() * 12);
      memo_key.push_back(static_cast<char>(next_leader));
      memo_key.push_back(static_cast<char>(std::max(next_min_tricks, 0)));
      if (child_top_k > 1) {
        memo_key.push_back(static_cast<char>(std::min(child_top_k, 127)));
      }
      for (const auto *e : wb.entries) {
        uint32_t bid = static_cast<uint32_t>(e->board);
        uint64_t played =
            board_trees.at(e->board)->MyBoard()->played_cards() |
            e->trick_mask;
        memo_key.append(reinterpret_cast<const char *>(&bid), sizeof(bid));
        memo_key.append(reinterpret_cast<const char *>(&played),
                        sizeof(played));
      }
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
        lvl + 1, branch_progress[idx], branch_min_cov);
    if (solve_memo_ != nullptr && !IsStopRequested()) {
      solve_memo_->Insert(std::move(memo_key), branch_min_cov,
                          branch_results[idx], child_top_k);
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

  auto build_line = [&](size_t alt_branch_idx, size_t alt_choice) {
    ConditionalLine line;
    line.SetAction(gen_trick);
    for (size_t idx = 0; idx < entries.size(); ++idx) {
      size_t pick = (idx == alt_branch_idx) ? alt_choice : 0;
      if (pick < shared_branches[idx].size()) {
        line.AddCondition(
            new WinnerPlayCondition(entries[idx].next_leader,
                                    entries[idx].winning_cards),
            shared_branches[idx][pick]);
      }
    }
    return line;
  };

  std::vector<ConditionalLine> out_lines;
  ConditionalLine primary = build_line(0, 0);
  if (min_covered > 0 && primary.CoveredFinalBoards() < min_covered) {
    return {};
  }
  if (primary.CoveredFinalBoards() > 0) {
    out_lines.push_back(std::move(primary));
  }

  if (child_top_k > 1) {
    for (size_t idx = 0; idx < entries.size(); ++idx) {
      for (size_t alt = 1; alt < shared_branches[idx].size(); ++alt) {
        ConditionalLine alt_line = build_line(idx, alt);
        if (alt_line.CoveredFinalBoards() > 0 &&
            (min_covered <= 0 ||
             alt_line.CoveredFinalBoards() >= min_covered)) {
          out_lines.push_back(std::move(alt_line));
        }
      }
    }
    std::stable_sort(
        out_lines.begin(), out_lines.end(),
        [](const ConditionalLine &a, const ConditionalLine &b) {
          return a.CoveredFinalBoards() > b.CoveredFinalBoards();
        });
  }
  return out_lines;
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
bool IsUninformativeTrick(const GeneralizedTrick &t, Suit trump_suit) {
  bool partner_ruffs =
      trump_suit != NOTRUMP &&
      Card::ParseSuit(t.demi1.my_card[0]) != trump_suit &&
      Card::ParseSuit(t.demi2.my_card[0]) == trump_suit;
  return !partner_ruffs && IsUninformativeOppPattern(t.demi1.opp_card) &&
         IsUninformativeOppPattern(t.demi2.opp_card);
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
    int min_covered) {
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

  int top_k = (lvl == 0) ? top_k_lines_ : ((lvl == 1) ? top_k_lvl1_ : 1);
  struct TopCandidate {
    int covered = 0;
    size_t num_conds = 0;
    std::map<int, std::string> sig;
    ConditionalLine line;
  };
  std::vector<TopCandidate> top_candidates;
  std::map<Trick1CardPair, std::pair<int, size_t>> best_t1_cov_conds;

  std::multimap<int, ConditionalLine> best_lines;
  std::unordered_map<std::string, std::shared_ptr<ConditionalLine>> saved_work;
  std::unordered_map<std::string,
                     std::vector<std::shared_ptr<ConditionalLine>>>
      saved_work_alts;
  std::unordered_map<std::string, int> saved_covered;
  std::map<Trick1CardPair, size_t> best_t1_min_conds;
  // Below the top level only one best line is needed, so groups that can at
  // best tie the current coverage are skipped. At the top level ties are
  // evaluated so that equivalent alternative lines are all returned.
  bool skip_ties = (lvl > 0 && top_k <= 1);
  int most_boards_covered = 0;
  int known_best_cov = 0;
  size_t known_best_idx = groupings.size();

  auto top_k_prune_floor = [&]() -> int {
    int floor_cov = std::max(1, min_covered);
    if (static_cast<int>(top_candidates.size()) >= top_k && top_k > 0) {
      floor_cov = std::max(floor_cov, top_candidates.back().covered);
    }
    return floor_cov;
  };

  auto add_top_candidate = [&](const ConditionalLine &cand,
                               const Trick1CardPair &t1,
                               size_t group_conds) -> bool {
    int cov = cand.CoveredFinalBoards();
    if (cov < std::max(1, min_covered)) {
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
    std::map<int, std::string> sig = cand.DeclarerPlaySignature();
    size_t conds = cand.conditions().size();
    bool changed = false;
    bool found_sig = false;
    for (auto &tc : top_candidates) {
      if (tc.sig == sig) {
        found_sig = true;
        if (cov > tc.covered || (cov == tc.covered && conds < tc.num_conds)) {
          tc.covered = cov;
          tc.num_conds = conds;
          tc.line = cand;
          changed = true;
        }
        break;
      }
    }
    if (!found_sig) {
      if (static_cast<int>(top_candidates.size()) < top_k ||
          cov > top_candidates.back().covered ||
          (cov == top_candidates.back().covered &&
           conds < top_candidates.back().num_conds)) {
        top_candidates.push_back(
            TopCandidate{cov, conds, std::move(sig), cand});
        changed = true;
      }
    }
    if (changed) {
      std::stable_sort(top_candidates.begin(), top_candidates.end(),
                       [](const TopCandidate &a, const TopCandidate &b) {
                         if (a.covered != b.covered) {
                           return a.covered > b.covered;
                         }
                         return a.num_conds < b.num_conds;
                       });
      if (static_cast<int>(top_candidates.size()) > top_k) {
        top_candidates.resize(top_k);
      }
      most_boards_covered = top_candidates.front().covered;
    }
    return changed;
  };

  auto req_group_cov = [&](size_t j) -> int {
    if (top_k > 1) {
      int req = top_k_prune_floor();
      if (!group_uniform_t1[j].first.empty()) {
        auto it = best_t1_cov_conds.find(group_uniform_t1[j]);
        if (it != best_t1_cov_conds.end()) {
          int c_t1 = it->second.first;
          size_t min_conds = it->second.second;
          req = std::max(
              req, (groupings[j].size() >= min_conds) ? (c_t1 + 1) : c_t1);
        }
      }
      return req;
    }
    int req = std::max(1, min_covered);
    bool skip_tie_for_j = skip_ties;
    if (!skip_tie_for_j && most_boards_covered > 0 &&
        !group_uniform_t1[j].first.empty()) {
      auto it = best_t1_min_conds.find(group_uniform_t1[j]);
      if (it != best_t1_min_conds.end() &&
          groupings[j].size() >= it->second) {
        skip_tie_for_j = true;
      }
    }
    if (most_boards_covered > 0) {
      req = std::max(req, skip_tie_for_j ? (most_boards_covered + 1)
                                         : most_boards_covered);
    }
    if (known_best_cov > 0) {
      req = std::max(req, (skip_ties && j > known_best_idx)
                              ? (known_best_cov + 1)
                              : known_best_cov);
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
      int covered = 0;
      if (lvl == 0 && top_k_lvl1_ > 1) {
        std::vector<ConditionalLine> lines = GetLinesFromTrick(
            *batch[b_idx].second, min_tricks, cache, board_trees,
            board_offsets, leader, lvl, trick_progress[b_idx], req_cov);
        covered = lines.empty() ? 0 : lines[0].CoveredFinalBoards();
        if (covered >= req_cov) {
          batch_results[b_idx] =
              std::make_shared<ConditionalLine>(std::move(lines[0]));
          batch_covered[b_idx] = covered;
          for (size_t m = 1; m < lines.size(); ++m) {
            if (lines[m].CoveredFinalBoards() >= req_cov) {
              batch_alt_results[b_idx].push_back(
                  std::make_shared<ConditionalLine>(std::move(lines[m])));
            }
          }
        } else {
          batch_results[b_idx] = nullptr;
          batch_covered[b_idx] = std::max(0, req_cov - 1);
        }
      } else {
        ConditionalLine line = GetLineFromTrick(
            *batch[b_idx].second, min_tricks, cache, board_trees,
            board_offsets, leader, lvl, trick_progress[b_idx], req_cov);
        covered = line.CoveredFinalBoards();
        if (covered >= req_cov) {
          batch_results[b_idx] =
              std::make_shared<ConditionalLine>(std::move(line));
          batch_covered[b_idx] = covered;
        } else {
          batch_results[b_idx] = nullptr;
          batch_covered[b_idx] = std::max(0, req_cov - 1);
        }
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
      if (all_known && updated_possible > known_best_cov) {
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
        omp_in_parallel() && (lvl == 0 && i == 0 && board_offsets.size() >= 30);

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
              (group_uniform_t1[j].first.empty() ||
               batch_t1_seen.count(group_uniform_t1[j]) == 0)) {
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
        if (lvl <= 2 && i >= 1) {
          size_t max_batch = (lvl == 0) ? 16 : ((lvl == 1) ? 12 : 8);
          for (size_t j = i + 1;
               j < groupings.size() && batch.size() < max_batch; ++j) {
            if (!is_pruned(j) &&
                (group_uniform_t1[j].first.empty() ||
                 batch_t1_seen.count(group_uniform_t1[j]) == 0)) {
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

    // Make a line for this group
    ConditionalLine a_line;
    for (size_t k = 0; k < group.size(); ++k) {
      const std::string &trick_str = keys[i][k];
      a_line.AddCondition(new TrickPlayCondition(trick_str),
                          saved_work.at(trick_str));
    }
    int covered_boards = a_line.CoveredFinalBoards();
    if (top_k > 1) {
      bool changed =
          add_top_candidate(a_line, group_uniform_t1[i], group.size());
      if (lvl == 0 && top_k_lvl1_ > 1) {
        for (size_t k = 0; k < group.size(); ++k) {
          auto alt_it = saved_work_alts.find(keys[i][k]);
          if (alt_it == saved_work_alts.end()) continue;
          for (const auto &alt_ptr : alt_it->second) {
            ConditionalLine alt_line;
            for (size_t m = 0; m < group.size(); ++m) {
              const std::string &t_str = keys[i][m];
              alt_line.AddCondition(
                  new TrickPlayCondition(t_str),
                  (m == k) ? alt_ptr : saved_work.at(t_str));
            }
            if (add_top_candidate(alt_line, Trick1CardPair{"", ""},
                                  group.size())) {
              changed = true;
            }
          }
        }
      }
      if (lvl == 0) {
        g_best_covered = std::max(most_boards_covered, known_best_cov);
      }
      if (changed && progress != nullptr) {
        SearchProgress::SetRemaining(progress, remaining_batches(i, nullptr));
      }
    } else if (covered_boards >= most_boards_covered) {
      bool improved = covered_boards > most_boards_covered;
      most_boards_covered = covered_boards;
      if (lvl == 0) {
        g_best_covered = std::max(most_boards_covered, known_best_cov);
      }
      if (improved) {
        best_lines.clear();
        best_t1_min_conds.clear();
      }
      if (!group_uniform_t1[i].first.empty()) {
        auto it = best_t1_min_conds.find(group_uniform_t1[i]);
        if (it == best_t1_min_conds.end() || group.size() < it->second) {
          best_t1_min_conds[group_uniform_t1[i]] = group.size();
        }
      }
      best_lines.insert(std::make_pair(covered_boards, a_line));
      VLOG(2) << "Found new best line at lvl "
              << lvl << " with " << covered_boards
              << " to get " << best_lines.size()
              << " of " << a_line.Describe();
      if (improved && progress != nullptr) {
        // A higher bar prunes more of the groups still to come.
        SearchProgress::SetRemaining(progress, remaining_batches(i, nullptr));
      }
    } else {
      VLOG(2) << "Only covered_boards " << covered_boards
              << " vs " << most_boards_covered;
    }
  }
  if (progress != nullptr) {
    SearchProgress::SetRemaining(progress, {});
  }
  std::vector<ConditionalLine> ans;
  if (top_k > 1) {
    ans.reserve(top_candidates.size());
    for (auto &tc : top_candidates) {
      ans.push_back(std::move(tc.line));
    }
  } else {
    for (const auto &[num_boards, a_line] : best_lines) {
      if (num_boards == most_boards_covered) {
        ans.push_back(a_line);
      }
    }
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
    int min_covered) {
  int best_covered = ans.empty() ? 0 : ans[0].CoveredFinalBoards();
  int target_cov = std::max(best_covered + 1, min_covered);
  if (IsStopRequested() || lvl <= 0 || leader % 2 != 0 ||
      static_cast<int>(solvable_boards.size()) < target_cov ||
      solvable_boards.size() < 2 ||
      get_line_from_trick_calls.load(std::memory_order_relaxed) >= 5000) {
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

  const BoardState *first_bs =
      board_trees.at(*solvable_boards.begin())->MyBoard();
  uint64_t mask = ~first_bs->played_cards();
  uint64_t decl_cards =
      (first_bs->GetHand(0) | first_bs->GetHand(2)) & mask;
  for (int suit = 0; suit < 4; ++suit) {
    if (IsStopRequested()) {
      break;
    }
    uint16_t suit_bits = (decl_cards >> (suit * 16)) & 0xffff;
    if (suit_bits == 0) continue;
    // Check highest remaining card held by N/S in this suit.
    int top_val = 31 - __builtin_clz(suit_bits);
    CardType top_card =
        Card::GetSuitedCard(static_cast<Suit>(suit), top_val);
    IsCardGoodCondition cond_true(top_card, true);
    std::set<int> good_boards;
    std::set<int> not_good_boards;
    for (int b : solvable_boards) {
      if (cond_true.Evaluate(*board_trees.at(b)->MyBoard())) {
        good_boards.insert(b);
      } else {
        not_good_boards.insert(b);
      }
    }
    if (good_boards.empty() || not_good_boards.empty()) {
      continue;
    }
    int cur_target = std::max(best_covered + 1, min_covered);
    int req_good =
        std::max(1, cur_target - static_cast<int>(not_good_boards.size()));
    std::vector<SearchProgress::Node *> half_progress =
        split_progress != nullptr
            ? SearchProgress::AddChildren(
                  split_progress,
                  {static_cast<double>(good_boards.size()),
                   static_cast<double>(not_good_boards.size())})
            : std::vector<SearchProgress::Node *>{nullptr, nullptr};
    std::vector<ConditionalLine> good_lines =
        SolveHelper(min_tricks, cache_in, board_trees, good_boards,
                    leader, lvl + 1, half_progress[0], req_good);
    int good_cov = good_lines.empty() ? 0 : good_lines[0].CoveredFinalBoards();
    if (good_lines.empty() || good_cov < req_good) {
      // SolveHelper completes the nodes it runs; this one never runs.
      SearchProgress::Complete(half_progress[1]);
      continue;
    }
    int req_not_good = std::max(1, cur_target - good_cov);
    std::vector<ConditionalLine> not_good_lines =
        SolveHelper(min_tricks, cache_in, board_trees, not_good_boards,
                    leader, lvl + 1, half_progress[1], req_not_good);
    int not_good_cov =
        not_good_lines.empty() ? 0 : not_good_lines[0].CoveredFinalBoards();
    if (not_good_lines.empty() || not_good_cov < req_not_good) continue;
    int total_cov = good_cov + not_good_cov;
    if (total_cov >= cur_target) {
      ConditionalLine split_line;
      split_line.AddCondition(
          new IsCardGoodCondition(top_card, true),
          std::make_shared<ConditionalLine>(std::move(good_lines[0])));
      split_line.AddCondition(
          new IsCardGoodCondition(top_card, false),
          std::make_shared<ConditionalLine>(std::move(not_good_lines[0])));
      best_covered = total_cov;
      if (lvl == 1 && top_k_lvl1_ > 1) {
        ans.insert(ans.begin(), std::move(split_line));
        if (static_cast<int>(ans.size()) > top_k_lvl1_) {
          ans.resize(top_k_lvl1_);
        }
      } else {
        ans = {std::move(split_line)};
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
    const std::vector<ConditionalLine> &res, int top_k) {
  int achieved = res.empty() ? 0 : res[0].CoveredFinalBoards();
  bool is_exact =
      (min_covered <= 1) ||
      (top_k <= 1
           ? (achieved >= min_covered)
           : (static_cast<int>(res.size()) >= top_k &&
              res.back().CoveredFinalBoards() >= min_covered));
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
      entry.exact = true;
    } else {
      entry.fail_low_bound = min_covered;
      entry.exact = false;
    }
    shards[shard].emplace(std::move(key), std::move(entry));
  } else if (!it->second.exact) {
    if (is_exact) {
      it->second.lines = res;
      it->second.exact = true;
    } else if (min_covered < it->second.fail_low_bound) {
      it->second.fail_low_bound = min_covered;
    }
  }
}

std::vector<ConditionalLine> LineFinder::SolveHelper(
    int min_tricks,
    SolverCache &cache_in,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress,
    int min_covered) {
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
  int cur_top_k = (lvl == 1) ? top_k_lvl1_ : 1;
  bool use_memo = (lvl > 0 && solve_memo_ != nullptr);
  std::string key;
  if (use_memo) {
    key.reserve(3 + board_offsets.size() * 12);
    key.push_back(static_cast<char>(leader));
    key.push_back(static_cast<char>(std::max(min_tricks, 0)));
    if (cur_top_k > 1) {
      key.push_back(static_cast<char>(std::min(cur_top_k, 127)));
    }
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
        uint32_t bid = static_cast<uint32_t>(b);
        uint64_t played = play_tree->MyBoard()->played_cards();
        key.append(reinterpret_cast<const char *>(&bid), sizeof(bid));
        key.append(reinterpret_cast<const char *>(&played), sizeof(played));
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
                               min_covered);
  }
  std::vector<ConditionalLine> cached_lines;
  if (solve_memo_->Lookup(key, min_covered, &cached_lines)) {
    g_memo_hits.fetch_add(1, std::memory_order_relaxed);
    SearchProgress::Complete(progress);
    return cached_lines;
  }
  g_memo_misses.fetch_add(1, std::memory_order_relaxed);
  std::vector<ConditionalLine> res = SolveHelperUncached(
      min_tricks, cache_in, board_trees, solvable_boards, leader, lvl,
      progress, min_covered);
  if (!IsStopRequested()) {
    solve_memo_->Insert(std::move(key), min_covered, res, cur_top_k);
  }
  return res;
}

std::vector<ConditionalLine> LineFinder::SolveHelperUncached(
    int min_tricks,
    SolverCache &cache_in,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress,
    int min_covered) {
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
  if (gen_tricks.size() == kMaxTricks) {
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

  // Among groups that share the same uniform declarer play (my1, my2), order
  // by total boards descending, then unsplit 1-trick groups first, followed
  // by the finest partition (most branches) before intermediate partial
  // splits so subset/partition bounds can prune the partial splits.
  auto group_total_boards = [](const std::vector<GeneralizedTrick> &g) {
    size_t sum = 0;
    for (const auto &t : g) sum += t.NumBoards();
    return sum;
  };
  std::map<Trick1CardPair, std::vector<size_t>> uniform_buckets;
  for (size_t idx = 0; idx < groupings.size(); ++idx) {
    Trick1CardPair t1 = GetUniformDeclarerPlay(groupings[idx]);
    if (!t1.first.empty()) {
      uniform_buckets[t1].push_back(idx);
    }
  }
  for (const auto &[_, idxs] : uniform_buckets) {
    if (idxs.size() <= 1) continue;
    std::vector<std::vector<GeneralizedTrick>> bucket_groups;
    bucket_groups.reserve(idxs.size());
    for (size_t idx : idxs) {
      bucket_groups.push_back(std::move(groupings[idx]));
    }
    std::stable_sort(
        bucket_groups.begin(), bucket_groups.end(),
        [&](const std::vector<GeneralizedTrick> &a,
            const std::vector<GeneralizedTrick> &b) {
          size_t tot_a = group_total_boards(a);
          size_t tot_b = group_total_boards(b);
          if (tot_a != tot_b) return tot_a > tot_b;
          bool a_single = (a.size() == 1);
          bool b_single = (b.size() == 1);
          if (a_single != b_single) return a_single;
          return a.size() > b.size();
        });
    for (size_t k = 0; k < idxs.size(); ++k) {
      groupings[idxs[k]] = std::move(bucket_groups[k]);
    }
  }

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

  std::vector<ConditionalLine> ans =
      GetLinesFromGroups(groupings, min_tricks, cache_in, board_trees,
                         board_offsets, leader, lvl, progress, min_covered);
  TryBranchOnGoodCards(min_tricks, cache_in, board_trees, board_offsets,
                       cards_left, leader, lvl, ans, progress, min_covered);
  return ans;
}
