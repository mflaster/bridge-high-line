#include "line_finder.h"

#include <ctype.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <random>
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

FuzzyBoardState::FastDealSpec FuzzyBoardState::PrepareFastDealSpec() const {
  FastDealSpec spec;
  for (size_t i = 0; i < 4 && i < known_hands_.size(); ++i) {
    spec.known_hands[i] = known_hands_[i];
  }
  spec.unknown_cards = unknown_cards_;
  int max_cards = 0;
  for (uint64_t h : spec.known_hands) {
    max_cards = std::max(max_cards, CountCardsInHand(h));
  }
  int unk_cnt = CountCardsInHand(spec.unknown_cards);
  int hand1_known = CountCardsInHand(spec.known_hands[1]);
  int hand3_known = CountCardsInHand(spec.known_hands[3]);
  int target_cards = max_cards;
  if (target_cards == 0 ||
      (hand1_known + hand3_known + unk_cnt) != 2 * target_cards) {
    target_cards = (hand1_known + hand3_known + unk_cnt) / 2;
  }
  spec.hand1_needed = std::max(0, target_cards - hand1_known);
  for (const auto &r : hand_reqs1_) {
    if (r && (target_cards == 13 || r->AppliesToRemainingHand())) {
      spec.active_reqs1.push_back(r);
    }
  }
  for (const auto &r : hand_reqs3_) {
    if (r && (target_cards == 13 || r->AppliesToRemainingHand())) {
      spec.active_reqs3.push_back(r);
    }
  }
  return spec;
}

BoardState FuzzyBoardState::FastRandomize(const FastDealSpec &spec,
                                          std::mt19937_64 &rng) {
  std::vector<uint64_t> hands(spec.known_hands.begin(), spec.known_hands.end());
  if (spec.unknown_cards != 0) {
    auto [hand1, hand3] = FastDealTwoRequiredHands(
        spec.unknown_cards, spec.active_reqs1, spec.active_reqs3,
        spec.hand1_needed, hands[1], hands[3], rng);
    hands[1] |= hand1;
    hands[3] |= hand3;
  }
  BoardState bs;
  bs.SetHands(hands);
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
  bool is_honor_wildcard = (rch == 'X' || rch == 'H');
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

// Opponent patterns that carry no information: "__" (any card) and "R_" (any
// discard).
bool IsUninformativeOppPattern(const std::string &p) {
  return p == "__" || p == "R_";
}

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
  if (t.board_entries.empty()) {
    char tc = SuitDebugString(trump_suit)[0];
    for (const auto &[_b, trick_str] : t.demi2.board_to_trick) {
      if (trick_str.size() >= 11 &&
          (trick_str[3] == tc || trick_str[9] == tc)) {
        return true;
      }
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

int CardCharRankVal(char r) {
  if (r >= '2' && r <= '9') return r - '2';
  switch (r) {
    case 'T': return 8;
    case 'J': return 9;
    case 'Q': return 10;
    case 'K': return 11;
    case 'A': return 12;
    default: return -1;
  }
}

uint16_t InSuitRankMask(const std::string &pat) {
  if (pat.size() < 2 || pat[0] == '_' || pat[0] == 'R') return 0;
  if (pat.size() == 2) {
    if (pat[1] == '_') return 0x1fff;
    if (pat[1] == 'x') return 0x00ff;
    if (pat[1] == 'X') return 0x1f00;
    int rv = CardCharRankVal(pat[1]);
    return rv >= 0 ? static_cast<uint16_t>(1u << rv) : 0;
  }
  if (pat.size() == 3) {
    int rv = CardCharRankVal(pat[2]);
    if (rv < 0) return 0;
    if (pat[1] == '<') {
      return static_cast<uint16_t>((1u << rv) - 1u);
    }
    if (pat[1] == '>') {
      return static_cast<uint16_t>(0x1fffu & ~((1u << (rv + 1)) - 1u));
    }
  }
  return 0;
}

int MaxInSuitPatternRankVal(const std::string &pat) {
  uint16_t m = InSuitRankMask(pat);
  return m != 0 ? (31 - __builtin_clz(m)) : -1;
}

int MaxObservedOppRankVal(const GeneralizedTrick &gt, bool is_demi1,
                          const std::string &pat) {
  int pat_max = MaxInSuitPatternRankVal(pat);
  if (pat_max < 0 || gt.board_entries.empty()) return pat_max;
  size_t pos = (gt.leader % 2 == 0) ? (is_demi1 ? 3 : 9) : (is_demi1 ? 0 : 6);
  int obs_max = -1;
  for (const auto &e : gt.board_entries) {
    if (pos + 1 < e.trick_str.size() && e.trick_str[pos] == pat[0]) {
      obs_max = std::max(obs_max, CardCharRankVal(e.trick_str[pos + 1]));
    }
  }
  return obs_max >= 0 ? std::min(pat_max, obs_max) : pat_max;
}

// True when `gt1` and `gt2` split declarer's play or continuation on two
// in-suit defender card patterns that both lose to declarer's already-committed
// card in that suit (e.g. `CJ (Cx) D6` vs `CJ (CT) HJ`, or `(Cx) CJ C8` vs
// `(CT) CJ CK`). Optionally outputs the acting defender seat, suit, and rank
// bitmasks for the two patterns.
bool FalsecardSplitInfo(const GeneralizedTrick &gt1,
                        const GeneralizedTrick &gt2,
                        int *opp_seat_out = nullptr,
                        Suit *suit_out = nullptr,
                        uint16_t *mask1_out = nullptr,
                        uint16_t *mask2_out = nullptr) {
  if (gt1.leader != gt2.leader) return false;
  auto offsuit_wins = [](const GeneralizedTrick &gt,
                         const std::string &my_c, char led_s) {
    return my_c.size() >= 2 && my_c[0] != led_s && !gt.board_entries.empty() &&
           gt.board_entries[0].win_card[0] == my_c[0];
  };
  auto fill_out = [&](int seat, char s, const std::string &p1,
                      const std::string &p2) {
    if (opp_seat_out != nullptr) *opp_seat_out = seat;
    if (suit_out != nullptr) *suit_out = Card::ParseSuit(s);
    if (mask1_out != nullptr) *mask1_out = InSuitRankMask(p1);
    if (mask2_out != nullptr) *mask2_out = InSuitRankMask(p2);
    return true;
  };
  const std::string &o1a = gt1.demi1.opp_card;
  const std::string &o1b = gt2.demi1.opp_card;
  if (o1a != o1b && o1a.size() >= 2 && o1b.size() >= 2 &&
      o1a[0] == o1b[0] && o1a[0] != '_' && o1a[0] != 'R') {
    char s = o1a[0];
    int max_a = MaxObservedOppRankVal(gt1, /*is_demi1=*/true, o1a);
    int max_b = MaxObservedOppRankVal(gt2, /*is_demi1=*/true, o1b);
    if (max_a >= 0 && max_b >= 0) {
      bool ns_wins_both = true;
      for (const auto &e : gt1.board_entries) {
        if (e.winner % 2 != 0) ns_wins_both = false;
      }
      for (const auto &e : gt2.board_entries) {
        if (e.winner % 2 != 0) ns_wins_both = false;
      }
      const std::string &m1a = gt1.demi1.my_card;
      const std::string &m1b = gt2.demi1.my_card;
      const std::string &m2a = gt1.demi2.my_card;
      const std::string &m2b = gt2.demi2.my_card;
      if (gt1.leader % 2 == 0) {
        if (m1a == m1b && m1a.size() >= 2 && m1a[0] == s) {
          int ref_rv = CardCharRankVal(m1a[1]);
          if (ns_wins_both && m2a.size() >= 2 && m2b.size() >= 2 &&
              m2a[0] == s && m2b[0] == s) {
            ref_rv = std::max(
                ref_rv,
                std::min(CardCharRankVal(m2a[1]), CardCharRankVal(m2b[1])));
          } else if (offsuit_wins(gt1, m2a, s) && offsuit_wins(gt2, m2b, s)) {
            ref_rv = 13;
          }
          if (max_a < ref_rv && max_b < ref_rv) {
            return fill_out((gt1.leader + 1) % 4, s, o1a, o1b);
          }
        }
      } else {
        int ref_rv = -1;
        if (m1a == m1b) {
          if (m1a.size() >= 2 && m1a[0] == s) {
            ref_rv = CardCharRankVal(m1a[1]);
          } else if (offsuit_wins(gt1, m1a, s) && offsuit_wins(gt2, m1b, s)) {
            ref_rv = 13;
          }
        } else if (ns_wins_both) {
          if (m1a.size() >= 2 && m1b.size() >= 2 && m1a[0] == s &&
              m1b[0] == s) {
            ref_rv = std::min(CardCharRankVal(m1a[1]), CardCharRankVal(m1b[1]));
          } else if (offsuit_wins(gt1, m1a, s) && offsuit_wins(gt2, m1b, s)) {
            ref_rv = 13;
          }
        }
        if (ns_wins_both) {
          if (m1a == m1b && m2a.size() >= 2 && m2b.size() >= 2 && m2a[0] == s &&
              m2b[0] == s) {
            ref_rv = std::max(
                ref_rv,
                std::min(CardCharRankVal(m2a[1]), CardCharRankVal(m2b[1])));
          } else if (m1a == m1b && offsuit_wins(gt1, m2a, s) &&
                     offsuit_wins(gt2, m2b, s)) {
            ref_rv = 13;
          } else if (m2a == m2b && m2a.size() >= 2 && m2a[0] == s &&
                     m1a.size() >= 2 && m1b.size() >= 2 && m1a[0] == s &&
                     m1b[0] == s) {
            int hi_m1 =
                std::max(CardCharRankVal(m1a[1]), CardCharRankVal(m1b[1]));
            int max_o2a = MaxObservedOppRankVal(gt1, /*is_demi1=*/false,
                                                gt1.demi2.opp_card);
            int max_o2b = MaxObservedOppRankVal(gt2, /*is_demi1=*/false,
                                                gt2.demi2.opp_card);
            int rv2 = CardCharRankVal(m2a[1]);
            if (rv2 > hi_m1 && max_o2a <= hi_m1 && max_o2b <= hi_m1) {
              ref_rv = std::max(ref_rv, rv2);
            }
          } else if (!offsuit_wins(gt1, m1a, s) &&
                     !offsuit_wins(gt1, m2a, s) &&
                     !offsuit_wins(gt2, m1b, s) &&
                     !offsuit_wins(gt2, m2b, s)) {
            auto suit_rv = [s](const std::string &c) {
              return (c.size() >= 2 && c[0] == s) ? CardCharRankVal(c[1]) : -1;
            };
            int win_a = std::max(suit_rv(m1a), suit_rv(m2a));
            int win_b = std::max(suit_rv(m1b), suit_rv(m2b));
            int min_win = std::min(win_a, win_b);
            int max_o2a = MaxObservedOppRankVal(gt1, /*is_demi1=*/false,
                                                gt1.demi2.opp_card);
            int max_o2b = MaxObservedOppRankVal(gt2, /*is_demi1=*/false,
                                                gt2.demi2.opp_card);
            if (min_win > ref_rv && max_o2a < min_win && max_o2b < min_win &&
                (std::max(max_a, max_b) + 1 < min_win ||
                 std::abs(win_a - win_b) <= 1)) {
              ref_rv = min_win;
            }
          }
        } else {
          ref_rv = -1;
        }
        if (max_a < ref_rv && max_b < ref_rv) {
          return fill_out(gt1.leader, s, o1a, o1b);
        }
      }
    }
  }
  const std::string &o2a = gt1.demi2.opp_card;
  const std::string &o2b = gt2.demi2.opp_card;
  if (o1a == o1b && gt1.demi1.my_card == gt2.demi1.my_card &&
      o2a != o2b && o2a.size() >= 2 && o2b.size() >= 2 &&
      o2a[0] == o2b[0] && o2a[0] != '_' && o2a[0] != 'R') {
    char s = o2a[0];
    int max_a = MaxObservedOppRankVal(gt1, /*is_demi1=*/false, o2a);
    int max_b = MaxObservedOppRankVal(gt2, /*is_demi1=*/false, o2b);
    if (max_a >= 0 && max_b >= 0) {
      int ref_rv = -1;
      if (gt1.demi1.my_card.size() >= 2 && gt1.demi1.my_card[0] == s) {
        ref_rv = CardCharRankVal(gt1.demi1.my_card[1]);
      } else if (offsuit_wins(gt1, gt1.demi1.my_card, s)) {
        ref_rv = 13;
      }
      if (gt1.demi2.my_card == gt2.demi2.my_card) {
        if (gt1.demi2.my_card.size() >= 2 && gt1.demi2.my_card[0] == s) {
          ref_rv = std::max(ref_rv, CardCharRankVal(gt1.demi2.my_card[1]));
        } else if (offsuit_wins(gt1, gt1.demi2.my_card, s) &&
                   offsuit_wins(gt2, gt2.demi2.my_card, s)) {
          ref_rv = 13;
        }
      }
      if (max_a < ref_rv && max_b < ref_rv) {
        int seat = (gt1.leader % 2 == 0) ? (gt1.leader + 3) % 4
                                         : (gt1.leader + 2) % 4;
        return fill_out(seat, s, o2a, o2b);
      }
    }
  }
  return false;
}

bool TricksHaveFalsecardSplit(const GeneralizedTrick &gt1,
                              const GeneralizedTrick &gt2) {
  return FalsecardSplitInfo(gt1, gt2);
}

bool GroupHasFalsecardSplit(const std::vector<GeneralizedTrick> &group) {
  for (size_t i = 1; i < group.size(); ++i) {
    for (size_t j = 0; j < i; ++j) {
      if (TricksHaveFalsecardSplit(group[j], group[i])) return true;
    }
  }
  return false;
}

int CountGroupUnforcedFalsecards(
    const std::vector<GeneralizedTrick> &group,
    const std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees) {
  std::set<int> unforced_boards;
  for (size_t a = 0; a < group.size(); ++a) {
    for (size_t b = a + 1; b < group.size(); ++b) {
      int opp_seat = -1;
      Suit fc_suit = NOTRUMP;
      uint16_t mask_a = 0;
      uint16_t mask_b = 0;
      if (!FalsecardSplitInfo(group[a], group[b], &opp_seat, &fc_suit, &mask_a,
                              &mask_b)) {
        continue;
      }
      if (opp_seat < 0 || fc_suit == NOTRUMP) {
        continue;
      }
      bool is_d1 = (group[a].demi1.opp_card != group[b].demi1.opp_card);
      char suit_ch =
          is_d1 ? group[a].demi1.opp_card[0] : group[a].demi2.opp_card[0];
      uint16_t group_suit_mask = 0;
      for (const auto &gt : group) {
        const std::string &pat = is_d1 ? gt.demi1.opp_card : gt.demi2.opp_card;
        if (!pat.empty()) {
          if (pat[0] == suit_ch) {
            group_suit_mask |= InSuitRankMask(pat);
          } else if (pat[0] == '_') {
            group_suit_mask = 0x1fff;
          }
        }
      }
      uint16_t uncovered_mask =
          static_cast<uint16_t>(0x1fffu & ~group_suit_mask);
      auto check_gt = [&](const GeneralizedTrick &gt, uint16_t flag_mask) {
        if (flag_mask == 0) return;
        auto check_brd = [&](int brd) {
          auto it_bt = board_trees.find(brd);
          if (it_bt == board_trees.end() || !it_bt->second) return;
          const BoardState *bs = it_bt->second->MyBoard();
          uint64_t rem = bs->GetHand(opp_seat) & ~bs->played_cards();
          uint16_t rem_s = static_cast<uint16_t>(
              (rem >> (static_cast<int>(fc_suit) * 16)) & 0x1fff);
          if ((rem_s & flag_mask) != 0) {
            unforced_boards.insert(brd);
          }
        };
        if (!gt.demi2.boards_vec.empty()) {
          for (int brd : gt.demi2.boards_vec) check_brd(brd);
        } else {
          for (int brd : gt.boards) check_brd(brd);
        }
      };
      check_gt(group[a],
               static_cast<uint16_t>((mask_b & ~mask_a) | uncovered_mask));
      check_gt(group[b],
               static_cast<uint16_t>((mask_a & ~mask_b) | uncovered_mask));
    }
  }
  return static_cast<int>(unforced_boards.size());
}

size_t FalsecardableSplits(const ConditionalLine &cl) {
  const auto &conds = cl.conditions();
  bool is_state = !conds.empty() && conds[0].first &&
                  conds[0].first->IsStateCondition();
  size_t count = 0;
  if (!is_state && conds.size() > 1) {
    for (size_t i = 1; i < conds.size(); ++i) {
      if (!conds[i].second || conds[i].second->do_first().Empty()) continue;
      for (size_t j = 0; j < i; ++j) {
        if (!conds[j].second || conds[j].second->do_first().Empty()) continue;
        if (TricksHaveFalsecardSplit(conds[j].second->do_first(),
                                     conds[i].second->do_first())) {
          ++count;
          break;
        }
      }
    }
  }
  for (const auto &[_cond, child] : conds) {
    if (child) {
      count += FalsecardableSplits(*child);
    }
  }
  return count;
}

size_t InitialTrumpDrawScore(const ConditionalLine &cl, Suit trump_suit) {
  if (trump_suit == NOTRUMP) return 0;
  size_t child_score = 0;
  for (const auto &[_cond, child] : cl.conditions()) {
    if (child) {
      child_score += InitialTrumpDrawScore(*child, trump_suit);
    }
  }
  if (!cl.do_first().Empty()) {
    const GeneralizedTrick &gt = cl.do_first();
    if (gt.leader % 2 == 0) {
      if (!TrickDrawsTrump(gt, trump_suit)) return 0;
      size_t own = static_cast<size_t>(std::max(1, cl.CoveredFinalBoards()));
      return own + child_score;
    }
  }
  return child_score;
}

// Encodes declarer's initial trump-drawing mode in [0, 3]:
//   0: seat 0 unsplit trump lead, 1: seat 0 split trump finesse,
//   2: seat 2 unsplit trump lead, 3: seat 2 split trump finesse,
//   4: mixed across seats,       -1: does not draw trump initially.
int ImmediateTrumpLeadMode(const ConditionalLine &cl, Suit trump_suit) {
  if (trump_suit == NOTRUMP) return -1;
  const ConditionalLine *cur = &cl;
  while (cur != nullptr) {
    const GeneralizedTrick &gt = cur->do_first();
    if (!gt.Empty() && gt.leader % 2 == 0) {
      return TrickDrawsTrump(gt, trump_suit) ? gt.leader : -1;
    }
    const auto &conds = cur->conditions();
    if (conds.empty()) break;
    if (conds.size() == 1) {
      cur = conds[0].second.get();
      continue;
    }
    if (conds[0].first && conds[0].first->IsStateCondition()) {
      const ConditionalLine *best_child = nullptr;
      int best_cov = -1;
      for (const auto &[_c, ch] : conds) {
        if (ch && ch->CoveredFinalBoards() > best_cov) {
          best_cov = ch->CoveredFinalBoards();
          best_child = ch.get();
        }
      }
      cur = best_child;
      continue;
    }
    if (gt.Empty() && conds[0].second &&
        !conds[0].second->do_first().Empty() &&
        conds[0].second->do_first().leader % 2 == 0) {
      bool distinct_my_play = false;
      const GeneralizedTrick &first_gt = conds[0].second->do_first();
      for (size_t i = 1; i < conds.size(); ++i) {
        if (!conds[i].second || conds[i].second->do_first().Empty()) continue;
        const GeneralizedTrick &igt = conds[i].second->do_first();
        if (igt.demi1.my_card != first_gt.demi1.my_card ||
            igt.demi2.my_card != first_gt.demi2.my_card) {
          distinct_my_play = true;
        }
        for (size_t j = 0; j < i; ++j) {
          if (!conds[j].second || conds[j].second->do_first().Empty()) {
            continue;
          }
          if (TricksHaveFalsecardSplit(conds[j].second->do_first(), igt)) {
            return -1;
          }
        }
      }
      for (const auto &[_c, ch] : conds) {
        if (ch && !ch->do_first().Empty() &&
            TrickDrawsTrump(ch->do_first(), trump_suit)) {
          return ch->do_first().leader | (distinct_my_play ? 1 : 0);
        }
      }
      return -1;
    }
    break;
  }
  return -1;
}

int ImmediateTrumpLeadSeat(const ConditionalLine &cl, Suit trump_suit) {
  int m = ImmediateTrumpLeadMode(cl, trump_suit);
  return (m >= 0 && m < 4) ? (m & ~1) : -1;
}

bool LeadsTrumpInitially(const ConditionalLine &cl, Suit trump_suit) {
  return ImmediateTrumpLeadMode(cl, trump_suit) >= 0;
}

bool AllBranchesLeadTrumpInitially(const ConditionalLine &cl,
                                   Suit trump_suit) {
  if (trump_suit == NOTRUMP) return true;
  const auto &conds = cl.conditions();
  if (conds.empty()) return LeadsTrumpInitially(cl, trump_suit);
  bool any = false;
  for (const auto &[_c, ch] : conds) {
    if (!ch) continue;
    any = true;
    if (!LeadsTrumpInitially(*ch, trump_suit)) return false;
  }
  return any ? true : LeadsTrumpInitially(cl, trump_suit);
}

int MergeTrumpLeadModes(int a, int b, bool allow_unsplit_coalesce = true) {
  if (a < 0) return b;
  if (b < 0 || a == b) return a;
  if (allow_unsplit_coalesce && a < 4 && b < 4 && (a & ~1) == (b & ~1)) {
    return a | 1;
  }
  return 4;
}

int MergeBranchTrumpModes(
    const std::vector<std::pair<int, int>> &branch_modes) {
  if (branch_modes.empty()) return -1;
  int seat = -1;
  bool has_split = false;
  bool has_multi_unsplit = false;
  for (const auto &[m, cov] : branch_modes) {
    if (m < 0) return -1;
    if (m >= 4) return 4;
    int s = m & ~1;
    if (seat < 0) {
      seat = s;
    } else if (seat != s) {
      return 4;
    }
    if ((m & 1) != 0) {
      has_split = true;
    } else if (cov >= 2) {
      has_multi_unsplit = true;
    }
  }
  if (has_split && has_multi_unsplit) return 4;
  return seat | (has_split ? 1 : 0);
}

int InitialTrumpLeadMode(const ConditionalLine &cl, Suit trump_suit) {
  if (trump_suit == NOTRUMP) return -1;
  int imm = ImmediateTrumpLeadMode(cl, trump_suit);
  if (imm >= 0) return imm;
  const GeneralizedTrick &gt = cl.do_first();
  const auto &conds = cl.conditions();
  if (gt.Empty() && !conds.empty()) {
    if (conds[0].first && conds[0].first->IsStateCondition()) {
      int merged = -1;
      for (const auto &[_c, ch] : conds) {
        if (!ch || ch->CoveredFinalBoards() <= 0) continue;
        int m = InitialTrumpLeadMode(*ch, trump_suit);
        if (m >= 0) merged = MergeTrumpLeadModes(merged, m);
      }
      return merged;
    }
    if (conds.size() == 1 && conds[0].second) {
      return InitialTrumpLeadMode(*conds[0].second, trump_suit);
    }
    bool all_def = true;
    bool all_decl = true;
    int decl_merged = -1;
    std::vector<std::pair<int, int>> def_modes;
    for (const auto &[_c, ch] : conds) {
      if (!ch || ch->do_first().Empty()) {
        all_def = false;
        all_decl = false;
        break;
      }
      if (ch->do_first().leader % 2 == 0) {
        all_def = false;
      } else {
        all_decl = false;
      }
      int cov = ch->CoveredFinalBoards();
      if (cov > 0) {
        int m = InitialTrumpLeadMode(*ch, trump_suit);
        if (m < 0) return -1;
        def_modes.emplace_back(m, cov);
        decl_merged = MergeTrumpLeadModes(decl_merged, m);
      }
    }
    if (all_def && !def_modes.empty()) {
      return MergeBranchTrumpModes(def_modes);
    }
    if (all_decl && decl_merged >= 0) {
      for (size_t i = 1; i < conds.size(); ++i) {
        for (size_t j = 0; j < i; ++j) {
          if (TricksHaveFalsecardSplit(conds[j].second->do_first(),
                                       conds[i].second->do_first())) {
            return -1;
          }
        }
      }
      return decl_merged;
    }
  }
  if (!gt.Empty() && !conds.empty()) {
    if (gt.leader % 2 != 0) {
      std::vector<std::pair<int, int>> w_modes;
      for (const auto &[w_c, w_ch] : conds) {
        const auto *wpc = dynamic_cast<const WinnerPlayCondition *>(w_c.get());
        if (wpc == nullptr || !w_ch) return -1;
        int cov = w_ch->CoveredFinalBoards();
        if (cov <= 0 && conds.size() > 1) continue;
        if (wpc->who() % 2 != 0 && cov > 0) {
          int m = InitialTrumpLeadMode(*w_ch, trump_suit);
          if (m < 0) return -1;
          w_modes.emplace_back(m, cov);
        } else if (wpc->who() % 2 == 0) {
          int m = InitialTrumpLeadMode(*w_ch, trump_suit);
          if (m < 0) return -1;
          w_modes.emplace_back(m, std::max(1, cov));
        }
      }
      return MergeBranchTrumpModes(w_modes);
    }
    if (!TrickDrawsTrump(gt, trump_suit) && conds.size() == 1 &&
        conds[0].second) {
      const auto *wpc =
          dynamic_cast<const WinnerPlayCondition *>(conds[0].first.get());
      if (wpc != nullptr && wpc->who() == (gt.leader + 2) % 4) {
        return ImmediateTrumpLeadMode(*conds[0].second, trump_suit);
      }
    }
  }
  return -1;
}

int InitialTrumpLeadSeat(const ConditionalLine &cl, Suit trump_suit) {
  int m = InitialTrumpLeadMode(cl, trump_suit);
  return (m >= 0 && m < 4) ? (m & ~1) : m;
}

bool DrawsTrumpSoon(const ConditionalLine &cl, Suit trump_suit) {
  return InitialTrumpLeadMode(cl, trump_suit) >= 0;
}

bool GroupWinsInPartner(const std::vector<GeneralizedTrick> &group,
                        int leader) {
  if (group.empty() || leader % 2 != 0) return false;
  int partner = (leader + 2) % 4;
  bool any = false;
  for (const GeneralizedTrick &t : group) {
    for (const auto &ent : t.board_entries) {
      any = true;
      if (ent.winner != partner) return false;
    }
  }
  return any;
}

size_t GroupExtraSplits(const std::vector<GeneralizedTrick> &group) {
  if (group.size() <= 1) return 0;
  if (group[0].leader % 2 != 0) {
    uint8_t suit_mask = 0;
    size_t wildcard_cnt = 0;
    for (const GeneralizedTrick &t : group) {
      const std::string &opp1 = t.demi1.opp_card;
      if (!opp1.empty() && (opp1[0] == 'S' || opp1[0] == 'H' ||
                            opp1[0] == 'D' || opp1[0] == 'C')) {
        suit_mask |= static_cast<uint8_t>(
            1u << static_cast<int>(Card::ParseSuit(opp1[0])));
      } else {
        ++wildcard_cnt;
      }
    }
    size_t distinct_buckets =
        static_cast<size_t>(__builtin_popcount(suit_mask)) + wildcard_cnt;
    if (distinct_buckets > 0) {
      return (group.size() > distinct_buckets)
                 ? (group.size() - distinct_buckets)
                 : 0;
    }
  }
  size_t in_suit_cnt = 0;
  for (const GeneralizedTrick &t : group) {
    if (t.demi1.opp_card != "R_") ++in_suit_cnt;
  }
  return std::max<size_t>(1, in_suit_cnt > 0 ? in_suit_cnt - 1 : 0);
}

bool IsIllusoryBossTrumpCoverEntry(
    const GeneralizedTrick::BoardTrickEntry &entry, const GeneralizedTrick &gt,
    int min_tricks,
    const std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees) {
  if (gt.leader % 2 != 0 || entry.trick_str.size() < 11) return false;
  const std::string &m1 = gt.demi1.my_card;
  const std::string &m2 = gt.demi2.my_card;
  if (m1.size() != 2 || m2.size() != 2 || m1[0] != m2[0]) return false;
  auto bt_it = board_trees.find(entry.board);
  if (bt_it == board_trees.end() || !bt_it->second) return false;
  const OptimalPlayTree &tree = *bt_it->second;
  if (tree.CardsLeft() <= 1 || min_tricks < tree.CardsLeft()) return false;
  const BoardState &bs = *bt_it->second->MyBoard();
  Suit trump_suit = bs.GetTrump();
  if (trump_suit == NOTRUMP) return false;
  Suit s_led = Card::ParseSuit(m1[0]);
  if (s_led != trump_suit) return false;
  int rv1 = CardCharRankVal(m1[1]);
  int rv2 = CardCharRankVal(m2[1]);
  if (rv2 <= rv1) return false;
  uint16_t opp1_pat_mask = (gt.demi1.opp_card == "__")
                               ? 0x1fffu
                               : InSuitRankMask(gt.demi1.opp_card);
  uint16_t low_mask = static_cast<uint16_t>((1u << rv1) - 1u);
  if ((opp1_pat_mask & low_mask) == 0) return false;
  if (entry.trick_str[3] != m1[0]) return false;
  int c1_rv = CardCharRankVal(entry.trick_str[4]);
  if (c1_rv <= rv1) return false;
  uint64_t unplayed = ~bs.played_cards();
  int opp1_seat = (gt.leader + 1) % 4;
  uint16_t opp1_trumps =
      static_cast<uint16_t>((bs.GetHand(opp1_seat) & unplayed) >>
                            (static_cast<int>(trump_suit) * 16));
  if ((opp1_trumps & opp1_pat_mask & low_mask) == 0) return false;
  CardType c_m1 = Card::ParseStringCard(m1);
  CardType c_m2 = Card::ParseStringCard(m2);
  uint64_t ns_rem =
      (bs.GetHand(gt.leader) | bs.GetHand((gt.leader + 2) % 4)) & unplayed &
      ~c_m1 & ~c_m2;
  uint16_t ns_rem_trumps = static_cast<uint16_t>(
      (ns_rem >> (static_cast<int>(trump_suit) * 16)) & 0x1fffu);
  return (ns_rem_trumps >> c1_rv) == 0;
}

size_t CountGroupIllusoryBossTrumpCovers(
    const std::vector<GeneralizedTrick> &group, int min_tricks,
    const std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees) {
  size_t cnt = 0;
  for (const GeneralizedTrick &gt : group) {
    for (const auto &entry : gt.board_entries) {
      if (IsIllusoryBossTrumpCoverEntry(entry, gt, min_tricks, board_trees)) {
        ++cnt;
      }
    }
  }
  return cnt;
}

size_t DefenderTrickWasteCost(
    const std::vector<GeneralizedTrick::BoardTrickEntry> &entries,
    const std::string &m1, const std::string &m2, int leader, size_t nb,
    const std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    Suit trump_suit) {
  if (leader % 2 == 0 || entries.empty() || m1.size() < 2 || m2.size() < 2) {
    return 0;
  }
  const auto &e0 = entries[0];
  if (e0.trick_str.size() < 8) return 0;
  Suit led_s = Card::ParseSuit(e0.trick_str[0]);
  Suit s1 = Card::ParseSuit(m1[0]);
  Suit s2 = Card::ParseSuit(m2[0]);
  int seat1 = (leader + 1) % 4;
  int seat2 = (leader + 3) % 4;
  auto it0 = board_trees.find(e0.board);
  if (it0 == board_trees.end() || it0->second == nullptr) return 0;
  const BoardState &bs0 = *it0->second->MyBoard();
  uint64_t unplayed0 = ~bs0.played_cards();

  if (trump_suit != NOTRUMP && led_s != trump_suit) {
    uint64_t trump_bits = 0xffffULL << (static_cast<int>(trump_suit) * 16);
    if (s1 == trump_suit && s2 == trump_suit) {
      if (((bs0.GetHand(seat1) | bs0.GetHand(seat2)) & unplayed0 &
           ~trump_bits) != 0) {
        return nb;
      }
    } else if ((s1 == trump_suit) != (s2 == trump_suit)) {
      uint16_t led_h1 = static_cast<uint16_t>(
          (bs0.GetHand(seat1) & unplayed0) >> (static_cast<int>(led_s) * 16));
      uint16_t led_h2 = static_cast<uint16_t>(
          (bs0.GetHand(seat2) & unplayed0) >> (static_cast<int>(led_s) * 16));
      if (led_h1 == 0 && led_h2 == 0) {
        int ruff_seat = (s1 == trump_suit) ? seat1 : seat2;
        int disc_seat = (s1 == trump_suit) ? seat2 : seat1;
        uint16_t t_ruff = static_cast<uint16_t>(
            (bs0.GetHand(ruff_seat) & unplayed0) >>
            (static_cast<int>(trump_suit) * 16));
        uint16_t t_disc = static_cast<uint16_t>(
            (bs0.GetHand(disc_seat) & unplayed0) >>
            (static_cast<int>(trump_suit) * 16));
        uint64_t ruff_non_trump =
            bs0.GetHand(ruff_seat) & unplayed0 & ~trump_bits;
        if (__builtin_popcount(t_ruff) < __builtin_popcount(t_disc) &&
            (t_disc & 0x00ffu) != 0 && ruff_non_trump != 0) {
          return nb;
        }
      }
    }
  }

  if (s1 == led_s && s2 == led_s) {
    uint16_t s1_bits = static_cast<uint16_t>(
        (bs0.GetHand(seat1) & unplayed0) >> (static_cast<int>(led_s) * 16));
    uint16_t s2_bits = static_cast<uint16_t>(
        (bs0.GetHand(seat2) & unplayed0) >> (static_cast<int>(led_s) * 16));
    if (__builtin_popcount(s2_bits) >= __builtin_popcount(s1_bits)) {
      return 0;
    }
    int rv1 = CardCharRankVal(m1[1]);
    int rv2 = CardCharRankVal(m2[1]);
    if (rv2 <= rv1 || rv2 < 8) return 0;
    size_t waste_cnt = 0;
    for (const auto &e : entries) {
      if (e.winner != seat2 || e.trick_str.size() < 11) continue;
      int c0_rv = CardCharRankVal(e.trick_str[1]);
      Suit c2_s = Card::ParseSuit(e.trick_str[6]);
      int c2_rv = CardCharRankVal(e.trick_str[7]);
      bool c2_ruffs =
          (trump_suit != NOTRUMP && led_s != trump_suit && c2_s == trump_suit);
      if (c2_ruffs) continue;
      int req_rv = c0_rv;
      if (c2_s == led_s && c2_rv > req_rv) req_rv = c2_rv;
      if (rv1 > req_rv && (s2_bits & ((1u << rv1) - 1u)) != 0) {
        ++waste_cnt;
      } else if (rv1 < 8 && rv2 >= 11 && (s2_bits & 0x00ffu) != 0) {
        uint16_t win_h1_mask = static_cast<uint16_t>(
            s1_bits & 0x1f00u & ((1u << rv2) - 1u) &
            ~((1u << (req_rv + 1)) - 1u));
        if (win_h1_mask != 0) {
          ++waste_cnt;
        }
      }
    }
    return waste_cnt;
  }
  return 0;
}

size_t GroupDefenderPlayCost(
    const std::vector<GeneralizedTrick> &group,
    const std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    Suit trump_suit) {
  size_t cost = 0;
  for (const GeneralizedTrick &t : group) {
    cost += DefenderTrickWasteCost(
        t.board_entries, t.demi1.my_card, t.demi2.my_card, t.leader,
        std::max<size_t>(1, t.NumBoards()), board_trees, trump_suit);
  }
  return cost;
}

size_t NonStateExtraBranches(const ConditionalLine &cl) {
  const auto &conds = cl.conditions();
  bool is_state = !conds.empty() && conds[0].first &&
                  conds[0].first->IsStateCondition();
  size_t extra = 0;
  if (!is_state && conds.size() > 1) {
    bool is_def_lead =
        cl.do_first().Empty() && conds[0].second &&
        !conds[0].second->do_first().Empty() &&
        conds[0].second->do_first().leader % 2 != 0;
    if (is_def_lead) {
      uint8_t suit_mask = 0;
      size_t wildcard_cnt = 0;
      for (const auto &[_c, child] : conds) {
        if (!child || child->do_first().Empty()) continue;
        const std::string &opp1 = child->do_first().demi1.opp_card;
        if (!opp1.empty() && (opp1[0] == 'S' || opp1[0] == 'H' ||
                              opp1[0] == 'D' || opp1[0] == 'C')) {
          suit_mask |= static_cast<uint8_t>(
              1u << static_cast<int>(Card::ParseSuit(opp1[0])));
        } else {
          ++wildcard_cnt;
        }
      }
      size_t distinct_buckets =
          static_cast<size_t>(__builtin_popcount(suit_mask)) + wildcard_cnt;
      if (distinct_buckets > 0 && conds.size() > distinct_buckets) {
        extra = conds.size() - distinct_buckets;
      }
    } else if (cl.do_first().Empty() && conds[0].second &&
               !conds[0].second->do_first().Empty() &&
               conds[0].second->do_first().leader % 2 == 0) {
      size_t in_suit_cnt = 0;
      for (const auto &[_c, child] : conds) {
        if (child && !child->do_first().Empty() &&
            child->do_first().demi1.opp_card != "R_") {
          ++in_suit_cnt;
        }
      }
      extra = std::max<size_t>(1, in_suit_cnt > 0 ? in_suit_cnt - 1 : 0);
    } else {
      extra = conds.size() - 1;
    }
  }
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

std::string FirstDeclarerPlayKey(const ConditionalLine &cl) {
  auto best_child_of = [](const ConditionalLine &node) {
    const ConditionalLine *best = nullptr;
    int best_cov = -1;
    for (const auto &[_c, ch] : node.conditions()) {
      if (ch && ch->CoveredFinalBoards() > best_cov) {
        best_cov = ch->CoveredFinalBoards();
        best = ch.get();
      }
    }
    return best;
  };
  std::string key;
  const ConditionalLine *cur = &cl;
  int decl_steps = 0;
  for (int depth = 0; depth < 6 && cur != nullptr; ++depth) {
    const GeneralizedTrick &gt = cur->do_first();
    if (!gt.Empty()) {
      if (!key.empty()) key.push_back('|');
      key.append(gt.demi1.my_card);
      key.push_back(':');
      key.append(gt.demi2.my_card);
      if (gt.leader % 2 == 0 && ++decl_steps >= 2) return key;
      cur = best_child_of(*cur);
      continue;
    }
    const auto &conds = cur->conditions();
    if (conds.empty()) break;
    if (conds[0].first && conds[0].first->IsStateCondition()) {
      if (!key.empty()) key.push_back('|');
      key.append("STATE:");
      key.append(conds[0].first->Describe(false));
      return key;
    }
    if (conds[0].second && !conds[0].second->do_first().Empty()) {
      bool decl_lead = (conds[0].second->do_first().leader % 2 == 0);
      if (decl_lead || conds.size() > 1) {
        if (!key.empty()) key.push_back('|');
        for (size_t i = 0; i < conds.size(); ++i) {
          if (!conds[i].second || conds[i].second->do_first().Empty()) {
            continue;
          }
          if (i > 0) key.push_back(',');
          const GeneralizedTrick &cgt = conds[i].second->do_first();
          key.append(cgt.demi1.my_card);
          key.push_back(':');
          key.append(cgt.demi2.my_card);
        }
        if (decl_lead && ++decl_steps >= 2) return key;
      }
      const ConditionalLine *best_act = best_child_of(*cur);
      cur = (best_act != nullptr) ? best_child_of(*best_act) : nullptr;
      continue;
    }
    cur = best_child_of(*cur);
  }
  return key;
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
  std::array<uint16_t, 4> equiv_opp_honors = {0, 0, 0, 0};
  if (!all_boards.empty()) {
    uint64_t ns_all = 0;
    uint64_t ew_all = 0;
    for (int b : all_boards) {
      uint64_t rem = ~boards[b].played_cards();
      ns_all |= (boards[b].GetHand(0) | boards[b].GetHand(2)) & rem;
      ew_all |= (boards[b].GetHand(1) | boards[b].GetHand(3)) & rem;
    }
    for (int s = 0; s < 4; ++s) {
      uint16_t ew_h =
          static_cast<uint16_t>((ew_all >> (s * 16)) & 0x1f00ULL);
      if (__builtin_popcount(ew_h) >= 2) {
        int min_h = __builtin_ctz(ew_h);
        int max_h = 31 - __builtin_clz(ew_h);
        uint16_t between = static_cast<uint16_t>(
            ((1u << max_h) - 1u) & ~((1u << (min_h + 1)) - 1u));
        uint16_t ns_s =
            static_cast<uint16_t>((ns_all >> (s * 16)) & 0x1fffULL);
        if ((ns_s & between) == 0) {
          equiv_opp_honors[s] = ew_h;
        }
      }
    }
  }
  Suit rescore_trump =
      all_boards.empty() ? NOTRUMP : boards[all_boards[0]].GetTrump();
  for (size_t l_i = 0; l_i < num_lines; ++l_i) {
    if (LineFinder::IsStopRequested()) break;
    (*lines)[l_i].CollapseUniformInSuitSplits();
    (*lines)[l_i].RelaxEquivalentHonorSplits(equiv_opp_honors, rescore_trump);
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

  Suit trump_suit = boards.empty() ? NOTRUMP : boards[0].GetTrump();
  std::stable_sort(
      lines->begin(), lines->end(),
      [trump_suit](const ConditionalLine &a, const ConditionalLine &b) {
        if (a.CoveredFinalBoards() != b.CoveredFinalBoards()) {
          return a.CoveredFinalBoards() > b.CoveredFinalBoards();
        }
        if (a.conditions().size() != b.conditions().size()) {
          return a.conditions().size() < b.conditions().size();
        }
        size_t fc_a = FalsecardableSplits(a);
        size_t fc_b = FalsecardableSplits(b);
        if (fc_a != fc_b) {
          return fc_a < fc_b;
        }
        size_t td_a = InitialTrumpDrawScore(a, trump_suit);
        size_t td_b = InitialTrumpDrawScore(b, trump_suit);
        if (td_a != td_b) {
          return td_a > td_b;
        }
        size_t extra_a = a.ExtraBranches();
        size_t extra_b = b.ExtraBranches();
        if (extra_a != extra_b) {
          return extra_a < extra_b;
        }
        return a.step1_covered_boards() > b.step1_covered_boards();
      });
}

uint8_t TrickLeadSuitsMask(const GeneralizedTrick &t) {
  const std::string &o1 = t.demi1.opp_card;
  if (!o1.empty() &&
      (o1[0] == 'S' || o1[0] == 'H' || o1[0] == 'D' || o1[0] == 'C')) {
    return static_cast<uint8_t>(1u << static_cast<int>(Card::ParseSuit(o1[0])));
  }
  if (t.board_entries.empty()) {
    return 0x0fu;
  }
  uint8_t mask = 0;
  for (const auto &ent : t.board_entries) {
    if (!ent.trick_str.empty() &&
        (ent.trick_str[0] == 'S' || ent.trick_str[0] == 'H' ||
         ent.trick_str[0] == 'D' || ent.trick_str[0] == 'C')) {
      mask |= static_cast<uint8_t>(
          1u << static_cast<int>(Card::ParseSuit(ent.trick_str[0])));
    }
  }
  return (mask == 0) ? 0x0fu : mask;
}

int TrickEffectiveLeadSuit(const GeneralizedTrick &t) {
  const std::string &o1 = t.demi1.opp_card;
  if (!o1.empty() &&
      (o1[0] == 'S' || o1[0] == 'H' || o1[0] == 'D' || o1[0] == 'C')) {
    return static_cast<int>(Card::ParseSuit(o1[0]));
  }
  uint8_t mask = TrickLeadSuitsMask(t);
  if (__builtin_popcount(mask) != 1 || t.demi1.my_card.size() < 2 ||
      t.demi2.my_card.size() < 2) {
    return -1;
  }
  int s0 = __builtin_ctz(mask);
  if (static_cast<int>(Card::ParseSuit(t.demi1.my_card[0])) == s0 ||
      static_cast<int>(Card::ParseSuit(t.demi2.my_card[0])) == s0) {
    return -1;
  }
  return s0;
}

uint64_t MixEvalSeed64(uint64_t z) {
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}

size_t BitmaskSymDiffCount(const std::vector<uint64_t> &a,
                           const std::vector<uint64_t> &b) {
  size_t n = std::max(a.size(), b.size());
  size_t diff = 0;
  for (size_t i = 0; i < n; ++i) {
    uint64_t wa = (i < a.size()) ? a[i] : 0ULL;
    uint64_t wb = (i < b.size()) ? b[i] : 0ULL;
    diff += static_cast<size_t>(PopCount64(wa ^ wb));
  }
  return diff;
}

size_t BitmaskMinusCount(const std::vector<uint64_t> &a,
                         const std::vector<uint64_t> &b) {
  size_t count = 0;
  for (size_t i = 0; i < a.size(); ++i) {
    uint64_t wb = (i < b.size()) ? b[i] : 0ULL;
    count += static_cast<size_t>(PopCount64(a[i] & ~wb));
  }
  return count;
}

bool AreCoveredBitmasksDistinct(const std::vector<uint64_t> &a,
                                const std::vector<uint64_t> &b,
                                size_t max_equiv_diff) {
  if (BitmaskSymDiffCount(a, b) <= max_equiv_diff) {
    return false;
  }
  return BitmaskMinusCount(a, b) > 0 && BitmaskMinusCount(b, a) > 0;
}

void SelectDistinctFinalistLines(std::vector<ConditionalLine> *lines,
                                 int max_finalists, int active_total,
                                 double equiv_diff_pct,
                                 int margin_override = -1,
                                 bool allow_subsets = false) {
  if (lines->empty() || max_finalists <= 0) return;
  int best_cov = (*lines)[0].CoveredFinalBoards();
  int stat_margin = std::max(
      4, static_cast<int>(
             std::ceil(2.8 * std::sqrt(std::max(1, active_total) * 0.25))));
  int margin = (margin_override >= 0) ? std::max(stat_margin, margin_override)
                                      : stat_margin;
  int min_cov = (best_cov > 0) ? std::max(1, best_cov - margin) : 0;
  size_t max_equiv_diff = static_cast<size_t>(
      std::floor(std::max(0.0, equiv_diff_pct) * std::max(0, active_total) +
                 1e-9));

  std::vector<ConditionalLine> kept;
  std::vector<std::vector<uint64_t>> kept_masks;
  std::vector<std::string> kept_play_keys;
  std::vector<uint64_t> cand_mask;

  for (ConditionalLine &cand : *lines) {
    int cov = cand.CoveredFinalBoards();
    if (cov < min_cov) {
      break;
    }
    cand.FillCoveredBoardsBitmask(&cand_mask);
    std::string play_key =
        allow_subsets ? FirstDeclarerPlayKey(cand) : std::string{};
    bool redundant = false;
    for (size_t k = 0; k < kept.size(); ++k) {
      bool distinct =
          allow_subsets
              ? (BitmaskSymDiffCount(cand_mask, kept_masks[k]) >
                     max_equiv_diff ||
                 (!play_key.empty() && play_key != kept_play_keys[k]))
              : AreCoveredBitmasksDistinct(cand_mask, kept_masks[k],
                                           max_equiv_diff);
      if (!distinct) {
        redundant = true;
        break;
      }
    }
    if (redundant) {
      continue;
    }
    kept_masks.push_back(cand_mask);
    if (allow_subsets) {
      kept_play_keys.push_back(std::move(play_key));
    }
    kept.push_back(std::move(cand));
    if (static_cast<int>(kept.size()) >= max_finalists) {
      break;
    }
  }
  *lines = std::move(kept);
}

void EvaluateFinalistLinesOutOfSample(
    const FuzzyBoardState &fuzzy_state, Suit trump, int leader,
    const std::vector<CardType> &cur_trick, int min_tricks, int target_deals,
    double max_eval_ms, uint64_t eval_seed, int num_threads,
    std::vector<ConditionalLine> *lines,
    std::vector<std::vector<uint64_t>> *out_eval_masks = nullptr) {
  if (lines->empty() || target_deals <= 0) {
    return;
  }
  const size_t num_lines = lines->size();
  const size_t num_words = static_cast<size_t>((target_deals + 63) / 64);
  for (size_t l_i = 0; l_i < num_lines; ++l_i) {
    (*lines)[l_i].set_min_tricks(min_tricks);
  }
  FuzzyBoardState::FastDealSpec deal_spec = fuzzy_state.PrepareFastDealSpec();
  std::unique_ptr<std::atomic<int>[]> line_hits(
      new std::atomic<int>[num_lines]);
  std::unique_ptr<std::atomic<uint64_t>[]> atomic_masks(
      new std::atomic<uint64_t>[num_lines * num_words]);
  for (size_t l_i = 0; l_i < num_lines; ++l_i) {
    line_hits[l_i].store(0, std::memory_order_relaxed);
    for (size_t w = 0; w < num_words; ++w) {
      atomic_masks[l_i * num_words + w].store(0ULL, std::memory_order_relaxed);
    }
  }

  constexpr int kBatchSize = 250;
  constexpr int kMinDealsBeforeBudgetStop = 1000;
  auto start_tp = std::chrono::steady_clock::now();
  int deals_done = 0;
  std::string noun =
      (num_lines == 1) ? " finalist line (" : " finalist lines (";

  while (deals_done < target_deals && !LineFinder::IsStopRequested()) {
    LineFinder::SetSolveStatus("Evaluating " + std::to_string(num_lines) +
                               noun + std::to_string(deals_done) + "/" +
                               std::to_string(target_deals) + " deals)...");
    int batch_n = std::min(kBatchSize, target_deals - deals_done);
    int batch_base = deals_done;
    ParallelForDynamic(
        static_cast<size_t>(batch_n), num_threads, [&](size_t offset) {
          if (LineFinder::IsStopRequested()) return;
          int d = batch_base + static_cast<int>(offset);
          uint64_t seed_d = MixEvalSeed64(
              eval_seed + static_cast<uint64_t>(d) * 0x9e3779b97f4a7c15ULL);
          std::mt19937_64 rng(seed_d);
          BoardState bs = FuzzyBoardState::FastRandomize(deal_spec, rng);
          bs.set_board_id(static_cast<uint32_t>(1000000u + d));
          bs.set_check_card_play(false);
          bs.SetTrump(trump);
          bs.SetLead(leader);
          for (CardType c : cur_trick) {
            bs.PlayCard(bs.CurPlayer(), c);
          }

          bool called_dds = false;
          if (!IsDealMakeable(bs, min_tricks, &called_dds,
                              /*reuse_tt=*/false)) {
            return;
          }
          ConditionalLine::DefenderEvalCache eval_cache;
          eval_cache.tt_initialized = called_dds;
          size_t word_idx = static_cast<size_t>(d) / 64;
          uint64_t bit_mask = 1ULL << (static_cast<unsigned>(d) % 64);
          for (size_t l_i = 0; l_i < num_lines; ++l_i) {
            if ((*lines)[l_i].EvaluateBoardAdversarialFast(bs, min_tricks,
                                                           &eval_cache)) {
              line_hits[l_i].fetch_add(1, std::memory_order_relaxed);
              atomic_masks[l_i * num_words + word_idx].fetch_or(
                  bit_mask, std::memory_order_relaxed);
            }
          }
        });
    if (LineFinder::IsStopRequested()) {
      break;
    }
    deals_done += batch_n;
    if (max_eval_ms > 0.0 && deals_done >= kMinDealsBeforeBudgetStop &&
        deals_done < target_deals) {
      double elapsed_ms =
          std::chrono::duration<double, std::milli>(
              std::chrono::steady_clock::now() - start_tp)
              .count();
      if (elapsed_ms >= max_eval_ms) {
        LOG(INFO) << "Finalist evaluation reached time budget (" << elapsed_ms
                  << " ms >= " << max_eval_ms << " ms) after " << deals_done
                  << " deals.";
        break;
      }
    }
  }

  if (deals_done <= 0 || LineFinder::IsStopRequested()) {
    return;
  }
  size_t used_words = static_cast<size_t>((deals_done + 63) / 64);
  for (size_t l_i = 0; l_i < num_lines; ++l_i) {
    (*lines)[l_i].set_eval_covered_boards(
        line_hits[l_i].load(std::memory_order_relaxed));
    (*lines)[l_i].set_eval_total_boards(deals_done);
  }
  std::vector<size_t> order(num_lines);
  for (size_t i = 0; i < num_lines; ++i) order[i] = i;
  std::stable_sort(
      order.begin(), order.end(), [&](size_t ia, size_t ib) {
        const ConditionalLine &a = (*lines)[ia];
        const ConditionalLine &b = (*lines)[ib];
        if (a.eval_covered_boards() != b.eval_covered_boards()) {
          return a.eval_covered_boards() > b.eval_covered_boards();
        }
        if (a.CoveredFinalBoards() != b.CoveredFinalBoards()) {
          return a.CoveredFinalBoards() > b.CoveredFinalBoards();
        }
        size_t fc_a = FalsecardableSplits(a);
        size_t fc_b = FalsecardableSplits(b);
        if (fc_a != fc_b) {
          return fc_a < fc_b;
        }
        size_t nseb_a = NonStateExtraBranches(a);
        size_t nseb_b = NonStateExtraBranches(b);
        if (nseb_a != nseb_b) {
          return nseb_a < nseb_b;
        }
        size_t td_a = InitialTrumpDrawScore(a, trump);
        size_t td_b = InitialTrumpDrawScore(b, trump);
        if (td_a != td_b) {
          return td_a > td_b;
        }
        if (a.ExtraBranches() != b.ExtraBranches()) {
          return a.ExtraBranches() < b.ExtraBranches();
        }
        return a.conditions().size() < b.conditions().size();
      });

  std::vector<ConditionalLine> sorted_lines;
  sorted_lines.reserve(num_lines);
  if (out_eval_masks != nullptr) {
    out_eval_masks->clear();
    out_eval_masks->resize(num_lines);
  }
  for (size_t pos = 0; pos < num_lines; ++pos) {
    size_t orig = order[pos];
    sorted_lines.push_back(std::move((*lines)[orig]));
    if (out_eval_masks != nullptr) {
      (*out_eval_masks)[pos].resize(used_words, 0ULL);
      for (size_t w = 0; w < used_words; ++w) {
        (*out_eval_masks)[pos][w] =
            atomic_masks[orig * num_words + w].load(std::memory_order_relaxed);
      }
    }
  }
  *lines = std::move(sorted_lines);
}

void FilterEvaluatedFinalistLines(
    std::vector<ConditionalLine> *lines,
    const std::vector<std::vector<uint64_t>> &eval_masks,
    int max_finalists, int active_total, double equiv_diff_pct) {
  if (lines->empty() || max_finalists <= 0 ||
      eval_masks.size() != lines->size()) {
    return;
  }
  if (max_finalists == 1) {
    int best_eval = (*lines)[0].eval_covered_boards();
    int best_cov = (*lines)[0].CoveredFinalBoards();
    size_t keep_n = 1;
    while (keep_n < lines->size() &&
           (*lines)[keep_n].eval_covered_boards() == best_eval &&
           (*lines)[keep_n].CoveredFinalBoards() == best_cov) {
      ++keep_n;
    }
    lines->resize(keep_n);
    DeduplicateTopLevelLines(lines, /*collapse_same_t1=*/true);
    return;
  }

  int deals_done = (*lines)[0].eval_total_boards();
  int best_eval = (*lines)[0].eval_covered_boards();
  int stat_margin = std::max(
      4, static_cast<int>(
             std::ceil(2.8 * std::sqrt(std::max(1, deals_done) * 0.25))));
  int eval_margin = std::max(stat_margin, deals_done / 5);
  int min_eval = (best_eval > 0) ? std::max(1, best_eval - eval_margin) : 0;
  size_t max_eval_equiv = static_cast<size_t>(
      std::floor(std::max(0.0, equiv_diff_pct) * std::max(0, deals_done) +
                 1e-9));
  size_t max_solve_equiv = static_cast<size_t>(
      std::floor(std::max(0.0, equiv_diff_pct) * std::max(0, active_total) +
                 1e-9));

  std::vector<ConditionalLine> kept;
  std::vector<std::vector<uint64_t>> kept_eval_masks;
  std::vector<std::vector<uint64_t>> kept_solve_masks;
  std::vector<uint64_t> cand_solve_mask;

  for (size_t i = 0; i < lines->size(); ++i) {
    ConditionalLine &cand = (*lines)[i];
    if (cand.eval_covered_boards() < min_eval) {
      break;
    }
    cand.FillCoveredBoardsBitmask(&cand_solve_mask);
    bool redundant = false;
    for (size_t k = 0; k < kept.size(); ++k) {
      if (!AreCoveredBitmasksDistinct(eval_masks[i], kept_eval_masks[k],
                                      max_eval_equiv) ||
          BitmaskSymDiffCount(cand_solve_mask, kept_solve_masks[k]) <=
              max_solve_equiv) {
        redundant = true;
        break;
      }
    }
    if (redundant) {
      continue;
    }
    kept_eval_masks.push_back(eval_masks[i]);
    kept_solve_masks.push_back(cand_solve_mask);
    kept.push_back(std::move(cand));
    if (static_cast<int>(kept.size()) >= max_finalists) {
      break;
    }
  }
  *lines = std::move(kept);
}

std::vector<ConditionalLine> SelectStageGuideLines(
    const std::vector<ConditionalLine> &lines,
    const std::vector<std::vector<uint64_t>> &eval_masks,
    size_t max_guides, double equiv_diff_pct) {
  std::vector<ConditionalLine> guides;
  if (lines.empty() || max_guides == 0) return guides;
  bool has_eval = (eval_masks.size() == lines.size() &&
                   lines[0].eval_total_boards() > 0);
  int best_eval = has_eval ? lines[0].eval_covered_boards()
                           : lines[0].CoveredFinalBoards();
  int ref_total = has_eval ? lines[0].eval_total_boards() : 100;
  int min_eval = std::max(1, best_eval - std::max(4, ref_total / 4));
  size_t max_equiv = static_cast<size_t>(
      std::floor(std::max(0.0, equiv_diff_pct) * std::max(0, ref_total) +
                 1e-9));

  std::vector<std::vector<uint64_t>> solve_masks(lines.size());
  for (size_t i = 0; i < lines.size(); ++i) {
    lines[i].FillCoveredBoardsBitmask(&solve_masks[i]);
  }

  std::vector<uint8_t> picked(lines.size(), 0);
  std::vector<std::vector<uint64_t>> kept_masks;
  std::vector<std::vector<uint64_t>> kept_solve_masks;
  auto try_pick_distinct = [&](size_t cap) {
    for (size_t i = 0; i < lines.size() && guides.size() < cap; ++i) {
      if (picked[i]) continue;
      int score = has_eval ? lines[i].eval_covered_boards()
                           : lines[i].CoveredFinalBoards();
      if (score < min_eval) break;
      const std::vector<uint64_t> &m_ref =
          has_eval ? eval_masks[i] : solve_masks[i];
      bool distinct = true;
      for (size_t k = 0; k < kept_masks.size(); ++k) {
        if (!AreCoveredBitmasksDistinct(m_ref, kept_masks[k], max_equiv) ||
            (has_eval && !AreCoveredBitmasksDistinct(
                             solve_masks[i], kept_solve_masks[k], 0))) {
          distinct = false;
          break;
        }
      }
      if (distinct) {
        kept_masks.push_back(m_ref);
        kept_solve_masks.push_back(solve_masks[i]);
        guides.push_back(lines[i]);
        picked[i] = 1;
      }
    }
  };

  size_t primary_cap = (max_guides > 2 && lines.size() > max_guides)
                           ? (max_guides - 2)
                           : max_guides;
  try_pick_distinct(primary_cap);

  int best_solve_cov = 0;
  for (const auto &l : lines) {
    best_solve_cov = std::max(best_solve_cov, l.CoveredFinalBoards());
  }
  std::set<std::string> seen_play_keys;
  for (const auto &g : guides) {
    seen_play_keys.insert(FirstDeclarerPlayKey(g));
  }
  int key_solve_floor =
      std::max(1, best_solve_cov - std::max(3, best_solve_cov / 3));
  for (size_t i = 0; i < lines.size() && guides.size() < max_guides; ++i) {
    if (picked[i] || FalsecardableSplits(lines[i]) > 0 ||
        lines[i].CoveredFinalBoards() < key_solve_floor) {
      continue;
    }
    int score = has_eval ? lines[i].eval_covered_boards()
                         : lines[i].CoveredFinalBoards();
    if (score < min_eval) continue;
    std::string pk = FirstDeclarerPlayKey(lines[i]);
    if (!pk.empty() && seen_play_keys.insert(pk).second) {
      kept_masks.push_back(has_eval ? eval_masks[i] : solve_masks[i]);
      kept_solve_masks.push_back(solve_masks[i]);
      guides.push_back(lines[i]);
      picked[i] = 1;
    }
  }

  try_pick_distinct(max_guides);

  int backup_floor = std::max(1, best_eval - std::max(3, ref_total / 6));
  for (size_t i = 0; i < lines.size() && guides.size() < max_guides; ++i) {
    if (picked[i] || FalsecardableSplits(lines[i]) > 0) continue;
    int score = has_eval ? lines[i].eval_covered_boards()
                         : lines[i].CoveredFinalBoards();
    if (score >= backup_floor) {
      guides.push_back(lines[i]);
      picked[i] = 1;
    }
  }
  return guides;
}

void PopulateDealSummaries(const std::vector<BoardState> &boards,
                           const std::set<int> &board_offsets,
                           int num_iters, int min_tricks, int num_threads,
                           std::vector<ConditionalLine> *lines) {
  if (lines->empty()) return;
  int active_total = board_offsets.empty()
                         ? num_iters
                         : static_cast<int>(board_offsets.size());
  for (auto &line : *lines) {
    line.set_total_boards(active_total);
    line.set_min_tricks(min_tricks);
    if (!boards.empty()) {
      int sample_idx = board_offsets.empty() ? 0 : *board_offsets.begin();
      if (sample_idx >= 0 && sample_idx < static_cast<int>(boards.size())) {
        line.set_sample_board(boards[sample_idx]);
      } else {
        line.set_sample_board(boards[0]);
      }
    }
  }
  if (!boards.empty() && !LineFinder::IsStopRequested()) {
    std::vector<int> missing_boards;
    for (int b_idx = 0; b_idx < static_cast<int>(boards.size()); ++b_idx) {
      if (!board_offsets.empty() && board_offsets.count(b_idx) == 0) {
        continue;
      }
      for (const auto &line : *lines) {
        if (line.board_traces().count(b_idx) == 0) {
          missing_boards.push_back(b_idx);
          break;
        }
      }
    }
    if (!missing_boards.empty()) {
      size_t num_m = missing_boards.size();
      size_t num_l = lines->size();
      std::vector<std::vector<ConditionalLine::BoardPlayTrace>> missing_traces(
          num_m, std::vector<ConditionalLine::BoardPlayTrace>(num_l));
      std::vector<std::vector<uint8_t>> missing_needed(
          num_m, std::vector<uint8_t>(num_l, 0));
      ParallelForDynamic(num_m, num_threads, [&](size_t m_i) {
        if (LineFinder::IsStopRequested()) return;
        int b_idx = missing_boards[m_i];
        ConditionalLine::DefenderCardsCache local_cache;
        for (size_t l_i = 0; l_i < num_l; ++l_i) {
          const auto &line = (*lines)[l_i];
          if (line.board_traces().count(b_idx) == 0) {
            missing_needed[m_i][l_i] = 1;
            missing_traces[m_i][l_i] = line.SimulateBoard(
                boards[b_idx], b_idx, line.has_rescored_boards(), &local_cache);
          }
        }
      });
      for (size_t l_i = 0; l_i < num_l; ++l_i) {
        std::map<int, ConditionalLine::BoardPlayTrace> traces =
            (*lines)[l_i].board_traces();
        bool updated = false;
        for (size_t m_i = 0; m_i < num_m; ++m_i) {
          if (missing_needed[m_i][l_i]) {
            traces.emplace(missing_boards[m_i],
                           std::move(missing_traces[m_i][l_i]));
            updated = true;
          }
        }
        if (updated) {
          (*lines)[l_i].set_board_traces(std::move(traces));
        }
      }
    }
  }
  CrossHandDealSetSummarizer summarizer;
  for (size_t line_idx = 0; line_idx < lines->size(); ++line_idx) {
    if (LineFinder::IsStopRequested()) break;
    auto &line = (*lines)[line_idx];
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
    fuzzy_state_.SetHands(hands, unknown_cards);
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

  bool known_cur_trick = true;
  for (size_t k = 0; k < cur_trick.size(); ++k) {
    int seat = (leader + static_cast<int>(k)) % 4;
    if ((hands[seat] & cur_trick[k]) == 0) {
      known_cur_trick = false;
      break;
    }
  }
  bool can_progressive =
      progressive_search_ && adversarial_rescore_ && !keep_all_top_k_ &&
      (max_finalist_lines_ > 1 || finalist_eval_deals_ > 0) &&
      unknown_cards != 0 && predealt_boards_.empty() &&
      replay_tricks_.empty() && branch_board_ids_.empty() && known_cur_trick;

  std::vector<int> active_vec(board_offsets.begin(), board_offsets.end());
  if (active_vec.empty()) {
    for (int i = 0; i < static_cast<int>(boards.size()); ++i) {
      active_vec.push_back(i);
    }
  }

  constexpr int kInitialStageDeals = 20;
  constexpr int kStageEvalDeals = 1000;
  constexpr size_t kMaxStageGuides = 6;
  std::vector<int> stage_counts;
  if (can_progressive && active_total > kInitialStageDeals) {
    int cur = kInitialStageDeals;
    while (cur < active_total) {
      stage_counts.push_back(cur);
      int next = cur * 2;
      if (next >= active_total || active_total - next <= cur / 2) {
        break;
      }
      cur = next;
    }
  }
  stage_counts.push_back(active_total);

  uint64_t stage_seed_base = finalist_eval_seed_;
  if (stage_seed_base == 0) {
    stage_seed_base = MixEvalSeed64(
        unknown_cards ^ (static_cast<uint64_t>(trump) << 16) ^
        (static_cast<uint64_t>(leader) << 8) ^
        static_cast<uint64_t>(min_tricks));
    if (!boards.empty()) {
      stage_seed_base = MixEvalSeed64(stage_seed_base ^ boards[0].GetHand(1));
    }
    if (stage_seed_base == 0) stage_seed_base = 0x517cc1b727220a95ULL;
  }

  SolverCache &cache = *solver_cache_;
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

  double init_trees_ms = 0.0;
  double search_trees_ms = 0.0;
  rescore_ms_ = 0.0;
  eval_ms_ = 0.0;
  size_t trees_built = 0;
  std::atomic<int> init_done{0};
  std::vector<ConditionalLine> guides;
  std::vector<ConditionalLine> ans;

  for (size_t s_idx = 0; s_idx < stage_counts.size(); ++s_idx) {
    int stage_n = std::min(stage_counts[s_idx],
                           static_cast<int>(active_vec.size()));
    bool is_final_stage = (s_idx + 1 == stage_counts.size());

    if (static_cast<size_t>(stage_n) > trees_built) {
      auto t0 = std::chrono::high_resolution_clock::now();
      size_t batch_cnt = static_cast<size_t>(stage_n) - trees_built;
      size_t base_pos = trees_built;
      ParallelForDynamic(batch_cnt, effective_threads, [&](size_t off) {
        if (IsStopRequested()) return;
        int b_idx = active_vec[base_pos + off];
        if (can_derive_init_trees && base_init_trees_[b_idx] != nullptr) {
          init_trees[b_idx] = DerivePlayTreeForForcedLead(
              *base_init_trees_[b_idx], base_boards_[b_idx], boards[b_idx],
              forced_lead_prefix, lead_decoded, min_tricks, &cache);
        } else {
          init_trees[b_idx] = OptimalPlayTree::FindOptimalPlayTree(
              boards[b_idx], min_tricks, &cache);
        }
        int done = ++init_done;
        if (!IsStopRequested()) {
          SetSolveStatus("Solving initial double-dummy trees (" +
                         std::to_string(done) + "/" +
                         std::to_string(active_vec.size()) + " deals)...");
        }
        VLOG(1) << "Finished computing tree for board " << b_idx
                << " with mini_max " << init_trees[b_idx]->MiniMax();
      });
      auto t1 = std::chrono::high_resolution_clock::now();
      init_trees_ms +=
          std::chrono::duration<double, std::milli>(t1 - t0).count();
      if (IsStopRequested()) {
        wide_beam_ = false;
        SetSolveStatus("Solve stopped.");
        return {};
      }
      for (size_t pos = trees_built; pos < static_cast<size_t>(stage_n);
           ++pos) {
        int b_idx = active_vec[pos];
        if (init_trees[b_idx]) {
          board_trees[b_idx] = init_trees[b_idx];
        }
      }
      trees_built = static_cast<size_t>(stage_n);
    }

    if (is_final_stage && cur_trick.empty() && replay_tricks_.empty() &&
        branch_board_ids_.empty() && predealt_boards_.empty()) {
      base_boards_ = boards;
      base_init_trees_ = init_trees;
      base_hands_ = hands;
      base_unknown_cards_ = unknown_cards;
      base_trump_ = trump;
      base_leader_ = leader;
      base_min_tricks_ = min_tricks;
      base_eval_seed_ = 0;
    }

    std::set<int> stage_offsets(active_vec.begin(),
                                active_vec.begin() + stage_n);
    if (is_final_stage && board_offsets.empty()) {
      stage_offsets.clear();
    }
    const std::set<int> &search_offsets =
        stage_offsets.empty() ? board_offsets : stage_offsets;
    g_total_boards = stage_n;
    g_best_covered = 0;
    if (s_idx > 0) {
      solve_memo_ = std::make_unique<SolveMemo>();
    }

    SetSolveStatus("Searching play trees across " + std::to_string(stage_n) +
                   " deals...");
    auto s_start = std::chrono::high_resolution_clock::now();
    ans.clear();
    if (s_idx == 0) {
      wide_beam_ =
          can_progressive &&
          (stage_counts.size() > 1 || max_finalist_lines_ > 1 ||
           finalist_eval_deals_ > 0);
      auto progress = std::make_shared<SearchProgress>();
      SetActiveProgress(progress);
      ParallelRegion(effective_threads, [&]() {
        ans = SolveHelper(min_tricks, cache, board_trees, search_offsets,
                          leader, 0, progress->root());
      });
      SetActiveProgress(nullptr);
      wide_beam_ = false;
    } else {
      wide_beam_ = false;
      auto progress = std::make_shared<SearchProgress>();
      SetActiveProgress(progress);
      ParallelRegion(effective_threads, [&]() {
        if (!guides.empty()) {
          std::vector<double> g_weights(guides.size(), 1.0);
          std::vector<SearchProgress::Node *> g_prog =
              SearchProgress::AddChildren(progress->root(), g_weights);
          std::vector<std::vector<ConditionalLine>> guided_out(guides.size());
          ParallelTaskBatch(guides.size(), [&](size_t g_i) {
            if (!IsStopRequested()) {
              guided_out[g_i] = SolveGuidedLine(
                  guides[g_i], min_tricks, cache, board_trees, search_offsets,
                  leader, 0);
            }
            SearchProgress::Complete(g_prog[g_i]);
          });
          for (auto &gv : guided_out) {
            for (auto &cl : gv) {
              ans.push_back(std::move(cl));
            }
          }
        }
        if (ans.empty() && !IsStopRequested()) {
          ans = SolveHelper(min_tricks, cache, board_trees, search_offsets,
                            leader, 0,
                            guides.empty() ? progress->root() : nullptr);
        }
      });
      SetActiveProgress(nullptr);
    }
    auto s_end = std::chrono::high_resolution_clock::now();
    search_trees_ms +=
        std::chrono::duration<double, std::milli>(s_end - s_start).count();
    if (IsStopRequested()) {
      SetSolveStatus("Solve stopped.");
      return {};
    }

    if (!is_final_stage && !ans.empty()) {
      DeduplicateTopLevelLines(&ans, /*collapse_same_t1=*/false);
      auto r0 = std::chrono::high_resolution_clock::now();
      RescoreTopLinesAdversarial(boards, search_offsets, board_trees,
                                 min_tricks, effective_threads, &ans);
      auto r1 = std::chrono::high_resolution_clock::now();
      rescore_ms_ +=
          std::chrono::duration<double, std::milli>(r1 - r0).count();
      if (IsStopRequested()) {
        SetSolveStatus("Solve stopped.");
        return {};
      }
      if (ans.size() > 24) {
        ans.resize(24);
      }
      std::vector<std::vector<uint64_t>> stage_eval_masks;
      if (ans[0].CoveredFinalBoards() > 0) {
        uint64_t stage_seed =
            MixEvalSeed64(stage_seed_base + static_cast<uint64_t>(s_idx + 1));
        auto e0 = std::chrono::high_resolution_clock::now();
        EvaluateFinalistLinesOutOfSample(
            fuzzy_state_, trump, leader, cur_trick, min_tricks,
            kStageEvalDeals, /*max_eval_ms=*/1500.0, stage_seed,
            effective_threads, &ans, &stage_eval_masks);
        auto e1 = std::chrono::high_resolution_clock::now();
        eval_ms_ +=
            std::chrono::duration<double, std::milli>(e1 - e0).count();
      }
      if (IsStopRequested()) {
        SetSolveStatus("Solve stopped.");
        return {};
      }
      guides = SelectStageGuideLines(ans, stage_eval_masks, kMaxStageGuides,
                                     finalist_equiv_diff_pct_);
    }
  }

#ifdef __EMSCRIPTEN__
  solve_memo_.reset();
#endif
  auto end_solve = std::chrono::high_resolution_clock::now();
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

  if (adversarial_rescore_ && !ans.empty()) {
    SetSolveStatus("Re-scoring top " + std::to_string(ans.size()) +
                   " lines against adversarial defense...");
    auto start_rescore = std::chrono::high_resolution_clock::now();
    RescoreTopLinesAdversarial(boards, board_offsets, board_trees, min_tricks,
                               effective_threads, &ans);
    auto end_rescore = std::chrono::high_resolution_clock::now();
    rescore_ms_ += std::chrono::duration<double, std::milli>(
                       end_rescore - start_rescore)
                       .count();
    LOG(INFO) << "TIMING: rescore_ms=" << rescore_ms_
              << " top_cov=" << ans[0].CoveredFinalBoards()
              << " (step1=" << ans[0].step1_covered_boards() << ")";
  }

  bool can_eval_out_of_sample =
      finalist_eval_deals_ > 0 && !ans.empty() &&
      ans[0].CoveredFinalBoards() > 0 && unknown_cards != 0 &&
      predealt_boards_.empty() && replay_tricks_.empty() &&
      branch_board_ids_.empty() && known_cur_trick;

  if (!keep_all_top_k_ && !ans.empty() &&
      (adversarial_rescore_ || top_k_lines_ > 1)) {
    if (max_finalist_lines_ > 1) {
      if (can_eval_out_of_sample) {
        int pre_cap = std::max(max_finalist_lines_ * 4, 12);
        SelectDistinctFinalistLines(&ans, pre_cap, active_total,
                                    finalist_equiv_diff_pct_,
                                    active_total / 3,
                                    /*allow_subsets=*/true);
      } else {
        SelectDistinctFinalistLines(&ans, max_finalist_lines_, active_total,
                                    finalist_equiv_diff_pct_);
      }
    } else {
      int best_cov = ans[0].CoveredFinalBoards();
      size_t keep_n = 1;
      while (keep_n < ans.size() &&
             ans[keep_n].CoveredFinalBoards() == best_cov) {
        ++keep_n;
      }
      ans.resize(keep_n);
      DeduplicateTopLevelLines(&ans, /*collapse_same_t1=*/true);
    }
  }

  if (can_eval_out_of_sample && !ans.empty()) {
    uint64_t eval_seed = finalist_eval_seed_;
    if (eval_seed == 0 && can_reuse_deals && base_eval_seed_ != 0) {
      eval_seed = base_eval_seed_;
    }
    if (eval_seed == 0) {
      std::random_device rd;
      uint64_t now_ns = static_cast<uint64_t>(
          std::chrono::steady_clock::now().time_since_epoch().count());
      eval_seed = MixEvalSeed64((static_cast<uint64_t>(rd()) << 32) ^
                                static_cast<uint64_t>(rd()) ^ now_ns);
      if (eval_seed == 0) eval_seed = 1;
      if (cur_trick.empty() && replay_tricks_.empty() &&
          branch_board_ids_.empty() && predealt_boards_.empty()) {
        base_eval_seed_ = eval_seed;
      }
    }
    std::vector<std::vector<uint64_t>> final_eval_masks;
    auto start_eval = std::chrono::high_resolution_clock::now();
    EvaluateFinalistLinesOutOfSample(
        fuzzy_state_, trump, leader, cur_trick, min_tricks,
        finalist_eval_deals_, finalist_eval_max_ms_, eval_seed,
        effective_threads, &ans, &final_eval_masks);
    auto end_eval = std::chrono::high_resolution_clock::now();
    eval_ms_ += std::chrono::duration<double, std::milli>(
                    end_eval - start_eval)
                    .count();
    if (IsStopRequested()) {
      SetSolveStatus("Solve stopped.");
      return {};
    }
    if (!keep_all_top_k_ && max_finalist_lines_ > 1) {
      FilterEvaluatedFinalistLines(&ans, final_eval_masks, max_finalist_lines_,
                                   active_total, finalist_equiv_diff_pct_);
    }
    LOG(INFO) << "TIMING: eval_ms=" << eval_ms_
              << " eval_deals=" << ans[0].eval_total_boards()
              << " top_eval_cov=" << ans[0].eval_covered_boards()
              << " finalists=" << ans.size();
  }

  auto start_invariant = std::chrono::high_resolution_clock::now();
  PopulateDealSummaries(boards, board_offsets, num_iters_, min_tricks,
                        effective_threads, &ans);
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
  if (wide_beam_) {
    if (lvl <= 0) {
      return std::max(top_k_lines_, 20);
    }
    if (num_boards < 2) {
      return 1;
    }
    if (lvl == 1) {
      return std::max(top_k_lvl1_, 8);
    }
    if (lvl <= 2) {
      return 4;
    }
    if (lvl == 3 || lvl == 4) {
      return 3;
    }
    return 1;
  }
  if (lvl <= 0) {
    return top_k_lines_;
  }
  bool finalist_stage =
      (max_finalist_lines_ > 1 && g_total_boards.load() <= 45);
  size_t min_boards = finalist_stage ? 2 : 4;
  if (top_k_lines_ <= 1 || top_k_lvl1_ <= 1 || num_boards < min_boards) {
    return 1;
  }
  if (lvl == 1) {
    return top_k_lvl1_;
  }
  if (finalist_stage && lvl <= 2) {
    return std::min(4, top_k_lvl1_);
  }
  if (finalist_stage && (lvl == 3 || lvl == 4)) {
    return std::min(3, top_k_lvl1_);
  }
  return std::min(2, top_k_lvl1_);
}

namespace {

std::array<int, 4> OrderedTrumpModes(int base_mode) {
  int bs = (base_mode >= 0 && base_mode < 4) ? (base_mode & ~1) : 0;
  int bf = (base_mode >= 0 && base_mode < 4) ? (base_mode & 1) : 0;
  return {bs | (1 - bf), (bs ^ 2) | (1 - bf), (bs ^ 2) | bf, bs | bf};
}

// Sorts candidate lines by CoveredFinalBoards() descending, FalsecardableSplits
// ascending, InitialTrumpDrawScore descending, then ExtraBranches() ascending;
// deduplicates identical non-zero (FalsecardableSplits, NonStateExtraBranches)
// pairs to the front when truncating to `limit`, and retains the minimum pair.
void FinalizeTopKLines(std::vector<ConditionalLine> *lines, int limit,
                       Suit trump_suit = NOTRUMP,
                       bool allow_clean_finalist_alts = false) {
  auto cmp = [trump_suit](const ConditionalLine &a, const ConditionalLine &b) {
    if ((a.unforced_fc_count() > 0) != (b.unforced_fc_count() > 0)) {
      int eff_a = a.CoveredFinalBoards() - a.unforced_fc_count();
      int eff_b = b.CoveredFinalBoards() - b.unforced_fc_count();
      if (eff_a != eff_b) {
        return eff_a > eff_b;
      }
      return a.unforced_fc_count() == 0;
    }
    if (a.CoveredFinalBoards() != b.CoveredFinalBoards()) {
      return a.CoveredFinalBoards() > b.CoveredFinalBoards();
    }
    size_t fc_a = FalsecardableSplits(a);
    size_t fc_b = FalsecardableSplits(b);
    if (fc_a != fc_b) {
      return fc_a < fc_b;
    }
    size_t td_a = InitialTrumpDrawScore(a, trump_suit);
    size_t td_b = InitialTrumpDrawScore(b, trump_suit);
    if (td_a != td_b) {
      return td_a > td_b;
    }
    return a.ExtraBranches() < b.ExtraBranches();
  };
  std::stable_sort(lines->begin(), lines->end(), cmp);
  if (static_cast<int>(lines->size()) > limit && limit > 0) {
    std::set<std::pair<size_t, size_t>> seen_fc_nseb;
    std::stable_partition(
        lines->begin(), lines->end(),
        [&](const ConditionalLine &l) {
          size_t fc = FalsecardableSplits(l);
          size_t nseb = NonStateExtraBranches(l);
          return (fc == 0 && (nseb == 0 || allow_clean_finalist_alts)) ||
                 seen_fc_nseb.insert({fc, nseb}).second;
        });
    if (limit > 1) {
      bool swapped = false;
      size_t min_idx = 0;
      auto min_key = std::make_pair(FalsecardableSplits((*lines)[0]),
                                    NonStateExtraBranches((*lines)[0]));
      for (size_t k = 1; k < lines->size(); ++k) {
        auto key = std::make_pair(FalsecardableSplits((*lines)[k]),
                                  NonStateExtraBranches((*lines)[k]));
        if (key < min_key) {
          min_key = key;
          min_idx = k;
        }
      }
      if (min_idx >= static_cast<size_t>(limit)) {
        std::swap((*lines)[limit - 1], (*lines)[min_idx]);
        swapped = true;
      }
      if (trump_suit != NOTRUMP) {
        if (allow_clean_finalist_alts && limit >= 3) {
          auto clean_mode = [&](const ConditionalLine &l) -> int {
            if (l.unforced_fc_count() > 0 || FalsecardableSplits(l) > 0) {
              return -1;
            }
            return InitialTrumpLeadMode(l, trump_suit);
          };
          std::vector<uint8_t> pinned(limit, 0);
          pinned[0] = 1;
          for (int target_mode : OrderedTrumpModes(clean_mode((*lines)[0]))) {
            bool present = false;
            for (int k = 0; k < limit; ++k) {
              if (clean_mode((*lines)[k]) == target_mode) {
                pinned[k] = 1;
                present = true;
                break;
              }
            }
            if (!present) {
              for (size_t k = static_cast<size_t>(limit); k < lines->size();
                   ++k) {
                if (clean_mode((*lines)[k]) == target_mode) {
                  for (int p = limit - 1; p >= 1; --p) {
                    if (!pinned[p]) {
                      std::swap((*lines)[p], (*lines)[k]);
                      pinned[p] = 1;
                      swapped = true;
                      break;
                    }
                  }
                  break;
                }
              }
            }
          }
        } else {
          auto is_trump_cand = [&](const ConditionalLine &l) {
            return LeadsTrumpInitially(l, trump_suit) ||
                   (allow_clean_finalist_alts && DrawsTrumpSoon(l, trump_suit));
          };
          bool has_trump_in_top = false;
          for (int k = 0; k < limit; ++k) {
            if (is_trump_cand((*lines)[k])) {
              has_trump_in_top = true;
              break;
            }
          }
          if (!has_trump_in_top) {
            for (size_t k = static_cast<size_t>(limit); k < lines->size();
                 ++k) {
              if (is_trump_cand((*lines)[k])) {
                size_t pos =
                    (min_idx >= static_cast<size_t>(limit) && limit >= 3)
                        ? static_cast<size_t>(limit - 2)
                        : static_cast<size_t>(limit - 1);
                std::swap((*lines)[pos], (*lines)[k]);
                swapped = true;
                break;
              }
            }
          }
        }
      }
      if (swapped) {
        std::stable_sort(lines->begin(), lines->begin() + limit, cmp);
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
//    in branch `idx` and 0 in all others) that improve (FalsecardableSplits,
//    NonStateExtraBranches) at `lvl > 0`, plus a composite `min_picks` line
//    choosing the minimum pair in every branch when at least
//    `min_diff_branches` branches strictly improve.
template <typename BuildLineFn>
std::vector<ConditionalLine> AssembleBranchLines(
    const std::vector<std::vector<std::shared_ptr<ConditionalLine>>> &branches,
    bool include_alts, int lvl, size_t min_diff_branches, Suit trump_suit,
    BuildLineFn build_line, bool allow_clean_finalist_alts = false) {
  std::vector<ConditionalLine> out;
  std::vector<size_t> picks(branches.size(), 0);
  out.push_back(build_line(picks));
  if (!include_alts) {
    return out;
  }
  auto line_trump_mode = [&](const ConditionalLine &l) -> int {
    return (allow_clean_finalist_alts && lvl <= 4)
               ? InitialTrumpLeadMode(l, trump_suit)
               : ImmediateTrumpLeadMode(l, trump_suit);
  };
  std::vector<size_t> min_picks(branches.size(), 0);
  std::vector<size_t> trump_picks(branches.size(), 0);
  size_t diff_branches = 0;
  size_t trump_diff_branches = 0;
  std::vector<uint64_t> base_mask;
  std::vector<uint64_t> alt_mask;
  for (size_t idx = 0; idx < branches.size(); ++idx) {
    if (branches[idx].empty()) continue;
    size_t base_fc = FalsecardableSplits(*branches[idx][0]);
    size_t base_nseb = NonStateExtraBranches(*branches[idx][0]);
    int base_t_mode = line_trump_mode(*branches[idx][0]);
    bool base_trump = (base_t_mode >= 0);
    if (allow_clean_finalist_alts && lvl == 1 && base_fc == 0 &&
        branches[idx].size() > 1) {
      branches[idx][0]->FillCoveredBoardsBitmask(&base_mask);
    }
    auto min_key = std::make_pair(base_fc, base_nseb);
    for (size_t alt = 1; alt < branches[idx].size(); ++alt) {
      size_t fc = FalsecardableSplits(*branches[idx][alt]);
      size_t nseb = NonStateExtraBranches(*branches[idx][alt]);
      int alt_t_mode = line_trump_mode(*branches[idx][alt]);
      bool alt_trump = (alt_t_mode >= 0);
      bool drops_clean_trump = (base_fc == 0 && base_trump && !alt_trump);
      auto key = std::make_pair(fc, nseb);
      if (!drops_clean_trump && key < min_key) {
        min_key = key;
        min_picks[idx] = alt;
      }
      if (allow_clean_finalist_alts && lvl <= 4 && !base_trump && alt_trump &&
          fc == 0 && branches[idx][alt]->unforced_fc_count() == 0 &&
          trump_picks[idx] == 0) {
        trump_picks[idx] = alt;
        ++trump_diff_branches;
      }
      bool novel_trump_alt =
          (lvl <= (allow_clean_finalist_alts ? 4 : 1) && alt_trump &&
           alt_t_mode != base_t_mode);
      bool clean_finalist_alt =
          (allow_clean_finalist_alts && base_fc == 0 && fc == 0 &&
           out.size() < 10 && lvl <= 4 && nseb <= base_nseb + 5);
      if (!clean_finalist_alt && allow_clean_finalist_alts && lvl == 1 &&
          base_fc == 0 && fc == 0 && out.size() < 10) {
        branches[idx][alt]->FillCoveredBoardsBitmask(&alt_mask);
        if (BitmaskMinusCount(alt_mask, base_mask) > 0) {
          clean_finalist_alt = true;
        }
      }
      if (lvl > 0 && !novel_trump_alt && !clean_finalist_alt &&
          (drops_clean_trump ||
           (base_fc > 0 && fc >= base_fc) ||
           (base_fc == 0 && (fc > 0 || nseb >= base_nseb)))) {
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
  std::vector<std::vector<size_t>> added_composites;
  auto push_composite = [&](const std::vector<size_t> &comp) {
    bool non_zero = false;
    for (size_t v : comp) {
      if (v > 0) non_zero = true;
    }
    if (!non_zero) return;
    for (const auto &prev : added_composites) {
      if (prev == comp) return;
    }
    added_composites.push_back(comp);
    out.push_back(build_line(comp));
  };
  if (branches.size() > 1 && diff_branches >= min_diff_branches) {
    push_composite(min_picks);
  }
  if (branches.size() > 1 && allow_clean_finalist_alts && lvl <= 4 &&
      trump_suit != NOTRUMP) {
    for (int target_mode : {0, 1, 2, 3}) {
      int target_seat = target_mode & ~1;
      std::vector<size_t> mode_picks(branches.size(), 0);
      bool all_match = true;
      bool any_exact_mode = false;
      for (size_t idx = 0; idx < branches.size(); ++idx) {
        if (branches[idx].empty()) continue;
        bool found = false;
        size_t seat_fallback = branches[idx].size();
        size_t any_fallback = branches[idx].size();
        for (size_t opt = 0; opt < branches[idx].size(); ++opt) {
          const auto &cl = *branches[idx][opt];
          if (cl.unforced_fc_count() > 0 || FalsecardableSplits(cl) > 0) {
            continue;
          }
          int t_mode = InitialTrumpLeadMode(cl, trump_suit);
          if (t_mode == target_mode) {
            mode_picks[idx] = opt;
            found = true;
            any_exact_mode = true;
            break;
          }
          if (t_mode >= 0 && t_mode < 4 && (t_mode & ~1) == target_seat &&
              seat_fallback == branches[idx].size()) {
            seat_fallback = opt;
          }
          if (t_mode >= 0 && any_fallback == branches[idx].size()) {
            any_fallback = opt;
          }
        }
        if (!found && seat_fallback < branches[idx].size()) {
          mode_picks[idx] = seat_fallback;
          found = true;
        } else if (!found && any_fallback < branches[idx].size()) {
          mode_picks[idx] = any_fallback;
          found = true;
        }
        if (!found) {
          all_match = false;
          break;
        }
      }
      if (all_match && any_exact_mode) {
        push_composite(mode_picks);
      }
    }
  }
  if (branches.size() > 1 && trump_diff_branches >= 1) {
    push_composite(trump_picks);
  }
  return out;
}

struct AugmentedTrickBoard {
  GeneralizedTrick::BoardTrickEntry entry;
  std::shared_ptr<OptimalPlayTree> after_tree;
};

bool MatchesAugOpp2Pattern(CardType c, const std::string &pat, Suit led_s,
                           Suit trump_suit) {
  if (pat == "__") return true;
  auto [cs, cr] = Card::DecodeCard(c);
  if (pat == "R_") {
    return cs != led_s && (trump_suit == NOTRUMP || cs != trump_suit);
  }
  std::string c_str = Card::PrintCard(c);
  if (c_str.size() < 2) return false;
  char s_ch = c_str[0];
  if (pat.size() == 2 && pat[0] == s_ch) {
    if (pat[1] == '_') return true;
    if (pat[1] == 'x') return cr < 8;
    if (pat[1] == 'X') return cr >= 8;
    return c_str == pat;
  }
  if (pat.size() == 3 && pat[0] == s_ch && pat[1] == '<') {
    int lim_rv = CardCharRankVal(pat[2]);
    return lim_rv >= 0 && cr < lim_rv;
  }
  return false;
}

std::vector<AugmentedTrickBoard> BuildAugmentedDefenderSuitEntries(
    const GeneralizedTrick &gen_trick, int min_tricks, SolverCache &cache,
    const std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets, const std::set<int> &orig_boards,
    int leader, int lvl, bool enabled) {
  if (!enabled || leader % 2 == 0 || lvl < 1 || lvl > 4 ||
      board_offsets.size() < 4 ||
      gen_trick.NumBoards() >= board_offsets.size() ||
      gen_trick.board_entries.empty()) {
    return {};
  }
  const std::string &opp1 = gen_trick.demi1.opp_card;
  if (opp1.size() != 2 || opp1[1] != '_' ||
      (opp1[0] != 'S' && opp1[0] != 'H' && opp1[0] != 'D' && opp1[0] != 'C')) {
    return {};
  }
  if (gen_trick.demi1.my_card.size() != 2 ||
      gen_trick.demi2.my_card.size() != 2) {
    return {};
  }
  const auto &first_tree = board_trees.at(gen_trick.board_entries[0].board);
  if (first_tree->CardsLeft() <= 6 ||
      first_tree->MyBoard()->CardsPlayedThisTrick() != 0) {
    return {};
  }
  Suit led_s = Card::ParseSuit(opp1[0]);
  CardType c1 = Card::ParseStringCard(gen_trick.demi1.my_card);
  CardType c3 = Card::ParseStringCard(gen_trick.demi2.my_card);
  Suit s1 = Card::DecodeCard(c1).first;
  Suit s3 = Card::DecodeCard(c3).first;
  int seat1 = (leader + 1) % 4;
  int opp2 = (leader + 2) % 4;
  int seat3 = (leader + 3) % 4;
  uint8_t active_w_mask = 0;
  for (const auto &e : gen_trick.board_entries) {
    active_w_mask |= static_cast<uint8_t>(1u << e.winner);
  }

  struct CandSpec {
    int board = 0;
    uint8_t winner = 0;
    std::string win_card_str;
    uint16_t discard_mask = 0;
    uint64_t trick_mask = 0;
    std::string trick_str;
    BoardState after_bs;
    int next_min_tricks = 0;
  };
  std::vector<CandSpec> specs;
  specs.reserve(board_offsets.size() - orig_boards.size());
  for (int b : board_offsets) {
    if (orig_boards.count(b) > 0) continue;
    auto bt_it = board_trees.find(b);
    if (bt_it == board_trees.end() || !bt_it->second) continue;
    const BoardState &bs = *bt_it->second->MyBoard();
    if (bs.CardsPlayedThisTrick() != 0) continue;
    Suit trump_suit = bs.GetTrump();
    uint64_t unplayed = ~bs.played_cards();
    uint16_t ldr_s = static_cast<uint16_t>(
        (bs.GetHand(leader) & unplayed) >> (static_cast<int>(led_s) * 16));
    if (ldr_s == 0) continue;
    uint64_t h1 = bs.GetHand(seat1) & unplayed;
    uint64_t h3 = bs.GetHand(seat3) & unplayed;
    if ((h1 & c1) == 0 || (h3 & c3) == 0) continue;
    if (s1 != led_s &&
        ((h1 >> (static_cast<int>(led_s) * 16)) & 0xffffULL) != 0) {
      continue;
    }
    if (s3 != led_s &&
        ((h3 >> (static_cast<int>(led_s) * 16)) & 0xffffULL) != 0) {
      continue;
    }
    CardType c0 = Card::GetSuitedCard(led_s, __builtin_ctz(ldr_s));
    BoardState sim_bs = bs;
    sim_bs.PlayCard(leader, c0);
    sim_bs.PlayCard(seat1, c1);
    std::vector<CardType> opp2_valid = sim_bs.ValidCardsToPlay();
    bool found_c2 = false;
    CardType best_c2 = 0;
    BoardState best_after;
    int best_w = -1;
    std::pair<int, int> best_key{100, 100};
    for (CardType cand_c2 : opp2_valid) {
      if (!MatchesAugOpp2Pattern(cand_c2, gen_trick.demi2.opp_card, led_s,
                                 trump_suit)) {
        continue;
      }
      BoardState t_bs = sim_bs;
      t_bs.PlayCard(opp2, cand_c2);
      t_bs.PlayCard(seat3, c3);
      int w = t_bs.CurPlayer();
      if ((active_w_mask & (1u << w)) == 0) continue;
      auto [cs2, cr2] = Card::DecodeCard(cand_c2);
      int cat = (w % 2 != 0)
                    ? 0
                    : ((trump_suit != NOTRUMP && cs2 != led_s &&
                        cs2 == trump_suit)
                           ? 2
                           : 1);
      std::pair<int, int> key{cat, cr2};
      if (!found_c2 || key < best_key) {
        found_c2 = true;
        best_c2 = cand_c2;
        best_after = t_bs;
        best_w = w;
        best_key = key;
      }
    }
    if (!found_c2) continue;
    std::array<CardType, 4> played = {c0, c1, best_c2, c3};
    int win_pos = (best_w - leader + 4) % 4;
    std::string wc_str = Card::PrintCard(played[win_pos]);
    std::string t_str = Card::PrintCard(c0) + " " + Card::PrintCard(c1) +
                        " " + Card::PrintCard(best_c2) + " " +
                        Card::PrintCard(c3);
    int trick_won = (best_w % 2 == 0) ? 1 : 0;
    specs.push_back(CandSpec{
        b, static_cast<uint8_t>(best_w), std::move(wc_str),
        best_after.discarded_suits_mask(),
        best_after.played_cards() & ~bs.played_cards(), std::move(t_str),
        best_after, std::max(0, min_tricks - trick_won)});
  }
  if (specs.empty()) return {};

  std::vector<std::shared_ptr<OptimalPlayTree>> trees(specs.size());
  if (omp_in_parallel() && specs.size() > 1) {
    ParallelTaskBatch(specs.size(), [&](size_t i) {
      if (LineFinder::IsStopRequested()) return;
      trees[i] = OptimalPlayTree::FindOptimalPlayTree(
          specs[i].after_bs, specs[i].next_min_tricks, &cache, 0);
    });
  } else {
    for (size_t i = 0; i < specs.size(); ++i) {
      if (LineFinder::IsStopRequested()) break;
      trees[i] = OptimalPlayTree::FindOptimalPlayTree(
          specs[i].after_bs, specs[i].next_min_tricks, &cache, 0);
    }
  }
  if (LineFinder::IsStopRequested()) return {};

  std::vector<AugmentedTrickBoard> out;
  out.reserve(specs.size());
  for (size_t i = 0; i < specs.size(); ++i) {
    if (!trees[i] ||
        !WonEnoughTricks(trees[i]->MiniMax(), specs[i].next_min_tricks,
                         specs[i].winner, trees[i]->CardsLeft())) {
      continue;
    }
    GeneralizedTrick::BoardTrickEntry ent;
    ent.board = specs[i].board;
    ent.winner = specs[i].winner;
    ent.win_card[0] =
        specs[i].win_card_str.size() > 0 ? specs[i].win_card_str[0] : '\0';
    ent.win_card[1] =
        specs[i].win_card_str.size() > 1 ? specs[i].win_card_str[1] : '\0';
    ent.win_card[2] = '\0';
    ent.discard_mask = specs[i].discard_mask;
    ent.trick_mask = specs[i].trick_mask;
    ent.trick_str = std::move(specs[i].trick_str);
    out.push_back(AugmentedTrickBoard{std::move(ent), std::move(trees[i])});
  }
  return out;
}

void PruneAndRestrictAugmentedCandidates(
    std::vector<ConditionalLine> *candidates, const std::set<int> &orig_boards,
    Suit trump_suit) {
  if (candidates == nullptr || candidates->empty()) return;
  if (candidates->size() > 1 && FalsecardableSplits((*candidates)[0]) == 0 &&
      (*candidates)[0].unforced_fc_count() == 0) {
    int cov0 = (*candidates)[0].CoveredFinalBoards();
    int mode0 = InitialTrumpLeadMode((*candidates)[0], trump_suit);
    std::vector<uint64_t> mask0;
    std::vector<uint64_t> mask_m;
    (*candidates)[0].FillCoveredBoardsBitmask(&mask0);
    candidates->erase(
        std::remove_if(candidates->begin() + 1, candidates->end(),
                       [&](const ConditionalLine &cl) {
                         if (cl.CoveredFinalBoards() >= cov0) return false;
                         if (InitialTrumpLeadMode(cl, trump_suit) != mode0) {
                           return false;
                         }
                         cl.FillCoveredBoardsBitmask(&mask_m);
                         return BitmaskMinusCount(mask_m, mask0) == 0;
                       }),
        candidates->end());
  }
  ConditionalLine::RestrictedCloneMap clone_memo;
  for (auto &cl : *candidates) {
    cl.CloneAndRestrictCoveredBoards(orig_boards, &clone_memo);
  }
  candidates->erase(
      std::remove_if(candidates->begin(), candidates->end(),
                     [](const ConditionalLine &cl) {
                       return cl.CoveredFinalBoards() <= 0;
                     }),
      candidates->end());
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
    size_t orig_size = 0;
    std::set<int> boards;
    std::set<std::string> winning_cards;
    std::vector<const GeneralizedTrick::BoardTrickEntry *> entries;
  };
  std::set<int> orig_boards;
  std::array<WinnerBranch, 4> by_winner;
  for (int w = 0; w < 4; ++w) {
    by_winner[w].next_leader = w;
  }
  bool allow_aug = adversarial_rescore_ && !keep_all_top_k_;
  for (const auto &entry : gen_trick.board_entries) {
    if (allow_aug &&
        IsIllusoryBossTrumpCoverEntry(entry, gen_trick, min_tricks,
                                      board_trees)) {
      continue;
    }
    orig_boards.insert(orig_boards.end(), entry.board);
    WinnerBranch &wb = by_winner[entry.winner];
    wb.boards.insert(wb.boards.end(), entry.board);
    wb.winning_cards.emplace(entry.win_card);
    wb.entries.push_back(&entry);
    wb.orig_size = wb.boards.size();
  }
  if (orig_boards.empty() ||
      min_covered > static_cast<int>(orig_boards.size())) {
    return {};
  }
  std::vector<AugmentedTrickBoard> aug_boards =
      BuildAugmentedDefenderSuitEntries(gen_trick, min_tricks, cache,
                                        board_trees, board_offsets,
                                        orig_boards, leader, lvl, allow_aug);
  if (IsStopRequested()) {
    return {};
  }
  std::unordered_map<int, std::shared_ptr<OptimalPlayTree>> aug_tree_map;
  const bool has_aug = !aug_boards.empty();
  if (has_aug) {
    aug_tree_map.reserve(aug_boards.size());
    for (auto &ab : aug_boards) {
      WinnerBranch &wb = by_winner[ab.entry.winner];
      if (wb.orig_size == 0) continue;
      wb.boards.insert(ab.entry.board);
      wb.winning_cards.emplace(ab.entry.win_card);
      wb.entries.push_back(&ab.entry);
      aug_tree_map.emplace(ab.entry.board, std::move(ab.after_tree));
    }
    for (int w = 0; w < 4; ++w) {
      if (by_winner[w].entries.size() > by_winner[w].orig_size) {
        std::sort(by_winner[w].entries.begin(), by_winner[w].entries.end(),
                  [](const GeneralizedTrick::BoardTrickEntry *a,
                     const GeneralizedTrick::BoardTrickEntry *b) {
                    return a->board < b->board;
                  });
      }
    }
  }
  std::vector<WinnerBranch> entries;
  entries.reserve(4);
  for (int w = 0; w < 4; ++w) {
    if (by_winner[w].orig_size > 0) {
      entries.push_back(std::move(by_winner[w]));
    }
  }
  if (min_covered > 0 && entries.size() > 1) {
    std::stable_sort(entries.begin(), entries.end(),
                     [](const WinnerBranch &a, const WinnerBranch &b) {
                       return a.orig_size < b.orig_size;
                     });
  }

  std::vector<double> branch_weights;
  if (progress != nullptr && lvl < 2) {
    branch_weights.reserve(entries.size());
    for (const auto &wb : entries) {
      branch_weights.push_back(wb.orig_size);
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
    if (lvl == 0 && entries.size() == 1 && child_top_k >= 8 &&
        (wide_beam_ || max_finalist_lines_ > 1)) {
      next_lvl_top_k = std::max(next_lvl_top_k, child_top_k);
    }
    size_t min_branch_boards =
        (wide_beam_ || (max_finalist_lines_ > 1 && g_total_boards.load() <= 45))
            ? 2
            : 4;
    int branch_top_k =
        (child_top_k <= 1 || wb.boards.size() < min_branch_boards)
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
    size_t non_aug_count = 0;
    if (omp_in_parallel() && wb.entries.size() > 1) {
      std::vector<size_t> pending_entries;
      pending_entries.reserve(wb.entries.size());
      for (size_t e_idx = 0; e_idx < wb.entries.size(); ++e_idx) {
        if (IsStopRequested()) break;
        const auto *entry = wb.entries[e_idx];
        if (has_aug) {
          auto a_it = aug_tree_map.find(entry->board);
          if (a_it != aug_tree_map.end()) {
            played_trees[e_idx] = a_it->second;
            continue;
          }
        }
        ++non_aug_count;
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
        if (has_aug) {
          auto a_it = aug_tree_map.find(entry->board);
          if (a_it != aug_tree_map.end()) {
            played_trees[e_idx] = a_it->second;
            continue;
          }
        }
        ++non_aug_count;
        played_trees[e_idx] =
            board_trees.at(entry->board)->PlayTrick(entry->trick_str, &cache);
      }
    }
    if (IsStopRequested()) {
      SearchProgress::Complete(branch_progress[idx]);
      return;
    }
    g_play_trick_calls.fetch_add(non_aug_count, std::memory_order_relaxed);
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
      int branch_size = static_cast<int>(entries[idx].orig_size);
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
      int branch_size = static_cast<int>(entries[idx].orig_size);
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
      rem_losses -= (branch_size - std::min(branch_size, branch_cov));
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

  Suit trump_suit =
      gen_trick.board_entries.empty()
          ? NOTRUMP
          : board_trees.at(gen_trick.board_entries[0].board)
                ->MyBoard()
                ->GetTrump();
  bool explore_alts = wide_beam_ || (max_finalist_lines_ > 1);
  std::vector<ConditionalLine> candidates = AssembleBranchLines(
      shared_branches, child_top_k > 1, lvl, /*min_diff_branches=*/1,
      trump_suit,
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
      },
      explore_alts);

  int primary_cov = candidates[0].CoveredFinalBoards();
  if ((min_covered > 0 && primary_cov < min_covered) || primary_cov <= 0) {
    return {};
  }
  if (child_top_k > 1 && candidates.size() > 1) {
    int eff_cov = primary_cov - candidates[0].unforced_fc_count();
    int alt_slack = (explore_alts && lvl <= 4) ? std::max(4, eff_cov / 3)
                                               : std::max(2, eff_cov / 4);
    int alt_floor = std::max(1, eff_cov - alt_slack);
    candidates.erase(
        std::remove_if(candidates.begin() + 1, candidates.end(),
                       [alt_floor](const ConditionalLine &cl) {
                         return cl.CoveredFinalBoards() < alt_floor;
                       }),
        candidates.end());
    FinalizeTopKLines(&candidates, child_top_k, trump_suit,
                      explore_alts && lvl <= 4);
  }
  if (has_aug) {
    PruneAndRestrictAugmentedCandidates(&candidates, orig_boards, trump_suit);
    if (candidates.empty() ||
        (min_covered > 0 &&
         candidates[0].CoveredFinalBoards() < min_covered) ||
        candidates[0].CoveredFinalBoards() <= 0) {
      return {};
    }
  }
  return candidates;
}

namespace {

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
  std::vector<bool> group_wins_partner(groupings.size(), false);
  std::vector<bool> group_has_fc(groupings.size(), false);
  std::vector<size_t> group_extra_splits(groupings.size(), 0);
  std::vector<int> group_unforced_fc_count(groupings.size(), 0);
  std::vector<size_t> group_def_waste(groupings.size(), 0);
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
    group_wins_partner[i] = GroupWinsInPartner(groupings[i], leader);
    group_has_fc[i] = GroupHasFalsecardSplit(groupings[i]);
    group_extra_splits[i] = GroupExtraSplits(groupings[i]);
    if (!keep_all_top_k_ && leader % 2 != 0) {
      group_def_waste[i] =
          GroupDefenderPlayCost(groupings[i], board_trees, trump_suit);
    }
    keys[i].reserve(groupings[i].size());
    group_trick_ids[i].reserve(groupings[i].size());
    if (progress != nullptr) {
      weights[i].reserve(groupings[i].size());
    }
    for (const auto &trick : groupings[i]) {
      group_possible_boards[i] += static_cast<int>(trick.NumBoards());
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
    if (!keep_all_top_k_ && group_has_fc[i]) {
      group_unforced_fc_count[i] =
          CountGroupUnforcedFalsecards(groupings[i], board_trees);
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
  bool explore_alts = wide_beam_ || (max_finalist_lines_ > 1);
  bool wide_root_lvl = (lvl == 0) || (lvl == 1 && top_k >= 8);
  size_t min_child_boards =
      (wide_beam_ || (explore_alts && g_total_boards.load() <= 45)) ? 2 : 4;
  int child_top_k = 1;
  if (top_k > 1 && (wide_beam_ || top_k_lvl1_ > 1) &&
      board_offsets.size() >= min_child_boards) {
    if (lvl == 0) {
      if (groupings.size() == 1) {
        int lvl1_k = std::max(TopKForLevel(1, board_offsets.size()),
                              explore_alts ? 8 : 1);
        child_top_k = wide_beam_ ? top_k : std::min(top_k, lvl1_k);
      } else {
        child_top_k =
            wide_beam_ ? std::min(top_k, 10)
                       : std::min(top_k, TopKForLevel(0, board_offsets.size()));
      }
    } else {
      child_top_k = std::min(top_k, TopKForLevel(lvl, board_offsets.size()));
    }
  }
  struct TopCandidate {
    int covered = 0;
    int unforced_fc = 0;
    size_t def_waste = 0;
    size_t num_conds = 0;
    size_t extra_branches = 0;
    size_t non_state_eb = 0;
    size_t falsecard_splits = 0;
    size_t trump_draw_score = 0;
    bool draws_trump = false;
    bool own_fc = false;
    bool leads_trump = false;
    bool all_branches_trump = false;
    int trump_seat = -1;
    int trump_mode = -1;
    std::map<int, std::string> sig;
    std::vector<uint64_t> cov_mask;
    std::string play_key;
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
  int best_clean_cov = 0;
  int best_zero_waste_cov = 0;
  int best_eff_cov = 0;
  int known_best_cov = 0;
  size_t known_best_idx = groupings.size();

  auto all_cands_falsecardable = [&]() -> bool {
    if (top_candidates.empty()) return false;
    for (const auto &tc : top_candidates) {
      if (!tc.own_fc && tc.unforced_fc == 0) return false;
    }
    return true;
  };

  bool decl_leads_here = (leader % 2 == 0);
  auto has_clean_trump = [&]() -> bool {
    if (trump_suit == NOTRUMP) return true;
    for (const auto &tc : top_candidates) {
      if (!tc.own_fc && tc.unforced_fc == 0 && tc.leads_trump) return true;
    }
    return false;
  };
  auto has_clean_trump_seat = [&](int target_seat) -> bool {
    if (trump_suit == NOTRUMP || target_seat < 0) return true;
    for (const auto &tc : top_candidates) {
      if (!tc.own_fc && tc.unforced_fc == 0 && tc.trump_seat == target_seat) {
        return true;
      }
    }
    return false;
  };
  auto has_clean_trump_mode = [&](int target_mode) -> bool {
    if (trump_suit == NOTRUMP || target_mode < 0) return true;
    for (const auto &tc : top_candidates) {
      if (!tc.own_fc && tc.unforced_fc == 0 && tc.trump_mode == target_mode) {
        return true;
      }
    }
    return false;
  };

  auto competitive_window_floor = [&](bool cand_has_fc = false) -> int {
    int ref_cov = cand_has_fc ? most_boards_covered : best_eff_cov;
    if (top_k > 1 && ref_cov > 0 &&
        (lvl > 0 || !keep_all_top_k_) &&
        ref_cov >= min_covered) {
      int slack = (!cand_has_fc && all_cands_falsecardable())
                      ? std::max(2, ref_cov / 4)
                      : std::max(2, ref_cov / 6);
      if (!cand_has_fc && explore_alts && lvl <= 4) {
        slack = std::max(slack, std::max(3, ref_cov / 3));
      }
      if (!cand_has_fc && wide_root_lvl && explore_alts) {
        int finalist_slack = std::max(
            4, static_cast<int>(std::ceil(
                   2.8 * std::sqrt(
                             std::max<size_t>(1, board_offsets.size()) *
                             0.25))));
        slack = std::max(slack, finalist_slack);
      }
      return std::max(1, ref_cov - slack);
    }
    return std::max(1, min_covered);
  };

  auto top_k_prune_floor = [&](size_t group_size, size_t extra_splits,
                               bool draws_trump, bool group_fc,
                               bool has_waste,
                               bool wins_in_partner) -> int {
    int floor_cov = competitive_window_floor(group_fc);
    int ref_best_cov =
        group_fc ? most_boards_covered
                 : (has_waste ? best_eff_cov : best_zero_waste_cov);
    bool allow_wide_clean = wide_root_lvl && lvl == 1 && explore_alts &&
                            !group_fc && extra_splits <= 1;
    int leader_mode = leader | (extra_splits > 0 ? 1 : 0);
    bool need_leader_trump =
        decl_leads_here && lvl >= 1 && lvl <= (explore_alts ? 4 : 2) &&
        extra_splits <= 1 && draws_trump &&
        !(explore_alts ? has_clean_trump_mode(leader_mode)
                       : has_clean_trump());
    bool need_partner_trump =
        explore_alts && decl_leads_here && lvl >= 1 && lvl <= 3 &&
        group_size == 1 && !draws_trump && wins_in_partner &&
        !has_clean_trump_seat((leader + 2) % 4);
    bool need_clean_alt =
        (top_k > 1 && !group_fc &&
         (all_cands_falsecardable() || need_leader_trump ||
          need_partner_trump));
    bool can_def_td_tie =
        (!decl_leads_here && lvl <= 4 && !group_fc && extra_splits <= 1 &&
         trump_suit != NOTRUMP && !top_candidates.empty() &&
         group_size <= top_candidates.back().num_conds &&
         !top_candidates.back().all_branches_trump);
    bool has_split_penalty =
        decl_leads_here ? (group_size > 1) : (extra_splits > 0);
    if (!need_clean_alt && !allow_wide_clean &&
        (lvl >= 1 || (!keep_all_top_k_ && leader % 2 != 0)) &&
        has_split_penalty && ref_best_cov > 0) {
      int split_floor = ((draws_trump && !group_fc) || can_def_td_tie)
                            ? ref_best_cov
                            : (ref_best_cov + 1);
      floor_cov = std::max(floor_cov, split_floor);
    }
    if (lvl >= 1 && !top_candidates.empty() && !need_clean_alt &&
        !allow_wide_clean) {
      size_t min_nseb = top_candidates[0].non_state_eb;
      int min_nseb_cov = top_candidates[0].covered;
      size_t min_nseb_conds = top_candidates[0].num_conds;
      size_t min_nseb_eb = top_candidates[0].extra_branches;
      bool min_nseb_dt = top_candidates[0].draws_trump;
      size_t min_nseb_dw = top_candidates[0].def_waste;
      for (const auto &tc : top_candidates) {
        if (tc.falsecard_splits == 0 && tc.non_state_eb < min_nseb) {
          min_nseb = tc.non_state_eb;
          min_nseb_cov = tc.covered;
          min_nseb_conds = tc.num_conds;
          min_nseb_eb = tc.extra_branches;
          min_nseb_dt = tc.draws_trump;
          min_nseb_dw = tc.def_waste;
        }
      }
      if (min_nseb == 0 && min_nseb_eb > 0 &&
          top_candidates[0].falsecard_splits == 0) {
        bool can_win_nseb0_tie =
            (!has_waste && min_nseb_dw > 0) ||
            (group_size < min_nseb_conds &&
             !(min_nseb_dt && min_nseb_conds <= 2 && !draws_trump)) ||
            (draws_trump && !group_fc && group_size <= 2 && !min_nseb_dt) ||
            (group_size == min_nseb_conds && draws_trump && !min_nseb_dt) ||
            can_def_td_tie;
        int eb0_floor =
            can_win_nseb0_tie ? min_nseb_cov : (min_nseb_cov + 1);
        floor_cov = std::max(floor_cov, eb0_floor);
      }
    }
    if (static_cast<int>(top_candidates.size()) >= top_k && top_k > 0) {
      if (need_clean_alt) {
        return floor_cov;
      }
      const TopCandidate &back_tc = top_candidates.back();
      int full_cov = (!group_fc && back_tc.unforced_fc > 0)
                         ? std::max(1, back_tc.covered - back_tc.unforced_fc)
                         : back_tc.covered;
      bool can_win_tie =
          (lvl == 0) || (!group_fc && back_tc.unforced_fc > 0) ||
          (!has_waste && back_tc.def_waste > 0) ||
          (group_size < back_tc.num_conds &&
           !(back_tc.falsecard_splits == 0 && back_tc.draws_trump &&
             back_tc.num_conds <= 2 && !draws_trump)) ||
          (draws_trump && !group_fc && group_size <= 2 &&
           !back_tc.draws_trump) ||
          (group_size == back_tc.num_conds && draws_trump &&
           !back_tc.draws_trump) ||
          can_def_td_tie;
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
      if (top_k > 1 && !group_fc && top_candidates[0].falsecard_splits > 0) {
        for (const auto &tc : top_candidates) {
          if (tc.falsecard_splits == 0) {
            bool can_beat_fc0 =
                (!has_waste && tc.def_waste > 0) ||
                (group_size < tc.num_conds &&
                 !(tc.draws_trump && tc.num_conds <= 2 && !draws_trump)) ||
                (draws_trump && !group_fc && group_size <= 2 &&
                 !tc.draws_trump) ||
                (group_size == tc.num_conds && draws_trump && !tc.draws_trump);
            int fc0_floor = can_beat_fc0 ? tc.covered : (tc.covered + 1);
            regular_floor = std::min(regular_floor, fc0_floor);
            break;
          }
        }
      }
      floor_cov = std::max(floor_cov, regular_floor);
    }
    return floor_cov;
  };

  auto is_better_meta = [](int cov_a, size_t dw_a, size_t conds_a, size_t fc_a,
                           size_t td_a, size_t eb_a, bool dt_a, int cov_b,
                           size_t dw_b, size_t conds_b, size_t fc_b,
                           size_t td_b, size_t eb_b, bool dt_b) {
    if (cov_a != cov_b) return cov_a > cov_b;
    if ((dw_a > 0) != (dw_b > 0)) return dw_a == 0;
    if (fc_a == 0 && fc_b == 0 && dt_a != dt_b &&
        (dt_a ? conds_a <= 2 : conds_b <= 2)) {
      return dt_a > dt_b;
    }
    if (conds_a != conds_b) return conds_a < conds_b;
    if (fc_a != fc_b) return fc_a < fc_b;
    if (dt_a != dt_b) return dt_a > dt_b;
    if (td_a != td_b) return td_a > td_b;
    return eb_a < eb_b;
  };

  auto cmp_top_cand = [&](const TopCandidate &a, const TopCandidate &b) {
    if ((a.unforced_fc > 0) != (b.unforced_fc > 0)) {
      int eff_a = a.covered - a.unforced_fc;
      int eff_b = b.covered - b.unforced_fc;
      if (eff_a != eff_b) return eff_a > eff_b;
      return a.unforced_fc == 0;
    }
    return is_better_meta(a.covered, a.def_waste, a.num_conds,
                          a.falsecard_splits, a.trump_draw_score,
                          a.extra_branches, a.draws_trump, b.covered,
                          b.def_waste, b.num_conds, b.falsecard_splits,
                          b.trump_draw_score, b.extra_branches, b.draws_trump);
  };

  auto add_top_candidate = [&](ConditionalLine &cand,
                               const Trick1CardPair &t1,
                               size_t group_conds,
                               bool draws_trump,
                               bool group_fc,
                               int unforced_fc,
                               size_t def_waste) -> bool {
    int cov = cand.CoveredFinalBoards();
    if (unforced_fc > 0 && best_clean_cov >= std::max(1, min_covered) &&
        cov - unforced_fc <= best_clean_cov) {
      return false;
    }
    if (def_waste > 0 && best_zero_waste_cov >= std::max(1, min_covered) &&
        cov <= best_zero_waste_cov) {
      return false;
    }
    size_t fc = FalsecardableSplits(cand);
    int min_floor = competitive_window_floor(group_fc || unforced_fc > 0);
    if (cov < min_floor) {
      return false;
    }
    bool evicted_fc = false;
    if (!group_fc && unforced_fc == 0 && def_waste == 0 &&
        cov > best_zero_waste_cov) {
      best_zero_waste_cov = cov;
      if (best_zero_waste_cov >= std::max(1, min_covered)) {
        size_t old_sz = top_candidates.size();
        top_candidates.erase(
            std::remove_if(top_candidates.begin(), top_candidates.end(),
                           [&](const TopCandidate &tc) {
                             return tc.def_waste > 0 &&
                                    tc.covered <= best_zero_waste_cov;
                           }),
            top_candidates.end());
        if (top_candidates.size() != old_sz) {
          evicted_fc = true;
        }
      }
    }
    if (!group_fc && unforced_fc == 0 && cov > best_clean_cov) {
      best_clean_cov = cov;
      if (best_clean_cov >= std::max(1, min_covered)) {
        size_t old_sz = top_candidates.size();
        top_candidates.erase(
            std::remove_if(top_candidates.begin(), top_candidates.end(),
                           [&](const TopCandidate &tc) {
                             return tc.unforced_fc > 0 &&
                                    tc.covered - tc.unforced_fc <=
                                        best_clean_cov;
                           }),
            top_candidates.end());
        if (top_candidates.size() != old_sz) {
          evicted_fc = true;
        }
      }
    }
    if (!t1.first.empty() && !group_fc && unforced_fc == 0) {
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
    std::vector<uint64_t> cov_mask;
    std::string play_key;
    if (explore_alts && top_k > 1 &&
        (lvl == 0 || (lvl == 1 && top_k >= 8))) {
      cand.FillCoveredBoardsBitmask(&cov_mask);
      play_key = FirstDeclarerPlayKey(cand);
    }
    size_t conds = cand.conditions().size();
    size_t extra_br = (top_k > 1) ? cand.ExtraBranches() : 0;
    size_t nseb = (top_k > 1) ? NonStateExtraBranches(cand) : 0;
    size_t td = InitialTrumpDrawScore(cand, trump_suit);
    int cand_trump_mode =
        (top_k > 1 && trump_suit != NOTRUMP)
            ? ((explore_alts && lvl <= 4)
                   ? InitialTrumpLeadMode(cand, trump_suit)
                   : ImmediateTrumpLeadMode(cand, trump_suit))
            : -1;
    int cand_trump_seat =
        (cand_trump_mode >= 0 && cand_trump_mode < 4)
            ? (cand_trump_mode & ~1)
            : cand_trump_mode;
    bool cand_leads_trump = (cand_trump_mode >= 0);
    bool all_br_trump =
        !decl_leads_here && AllBranchesLeadTrumpInitially(cand, trump_suit);
    auto cand_bkt = std::make_pair(fc, nseb);
    bool changed = evicted_fc;
    bool found_sig = false;
    auto min_existing_bkt =
        top_candidates.empty()
            ? std::make_pair(size_t{0}, size_t{0})
            : std::make_pair(top_candidates[0].falsecard_splits,
                             top_candidates[0].non_state_eb);
    int min_bkt_cov = top_candidates.empty() ? 0 : top_candidates[0].covered;
    size_t min_bkt_dw =
        top_candidates.empty() ? 0 : top_candidates[0].def_waste;
    size_t min_bkt_conds =
        top_candidates.empty() ? 0 : top_candidates[0].num_conds;
    size_t min_bkt_fc =
        top_candidates.empty() ? 0 : top_candidates[0].falsecard_splits;
    size_t min_bkt_td =
        top_candidates.empty() ? 0 : top_candidates[0].trump_draw_score;
    size_t min_bkt_eb =
        top_candidates.empty() ? 0 : top_candidates[0].extra_branches;
    bool min_bkt_dt =
        top_candidates.empty() ? false : top_candidates[0].draws_trump;
    size_t same_cov_count = 0;
    size_t worst_same_cov_idx = top_candidates.size();
    size_t same_key_idx = top_candidates.size();
    for (size_t tc_i = 0; tc_i < top_candidates.size(); ++tc_i) {
      auto &tc = top_candidates[tc_i];
      auto tc_bkt = std::make_pair(tc.falsecard_splits, tc.non_state_eb);
      if (tc_bkt < min_existing_bkt) {
        min_existing_bkt = tc_bkt;
        min_bkt_cov = tc.covered;
        min_bkt_dw = tc.def_waste;
        min_bkt_conds = tc.num_conds;
        min_bkt_fc = tc.falsecard_splits;
        min_bkt_td = tc.trump_draw_score;
        min_bkt_eb = tc.extra_branches;
        min_bkt_dt = tc.draws_trump;
      }
      if (top_k > 1 && tc.sig == sig) {
        found_sig = true;
        bool beats_ufc = (unforced_fc == 0 && tc.unforced_fc > 0 &&
                          cov >= tc.covered - tc.unforced_fc);
        if (beats_ufc ||
            is_better_meta(cov, def_waste, conds, fc, td, extra_br,
                           draws_trump, tc.covered, tc.def_waste, tc.num_conds,
                           tc.falsecard_splits, tc.trump_draw_score,
                           tc.extra_branches, tc.draws_trump)) {
          tc.covered = cov;
          tc.unforced_fc = unforced_fc;
          tc.def_waste = def_waste;
          tc.num_conds = conds;
          tc.extra_branches = extra_br;
          tc.non_state_eb = nseb;
          tc.falsecard_splits = fc;
          tc.trump_draw_score = td;
          tc.draws_trump = draws_trump;
          tc.own_fc = group_fc;
          tc.leads_trump = cand_leads_trump;
          tc.all_branches_trump = all_br_trump;
          tc.trump_seat = cand_trump_seat;
          tc.trump_mode = cand_trump_mode;
          tc.sig = std::move(sig);
          tc.cov_mask = std::move(cov_mask);
          tc.play_key = std::move(play_key);
          tc.line = cand;
          changed = true;
        }
        break;
      }
      bool same_clean_cov_set =
          (!cov_mask.empty() && !tc.cov_mask.empty() && fc == 0 && !group_fc &&
           unforced_fc == 0 && tc.falsecard_splits == 0 && !tc.own_fc &&
           tc.unforced_fc == 0 && cand_leads_trump == tc.leads_trump &&
           cand_trump_mode == tc.trump_mode &&
           BitmaskSymDiffCount(cov_mask, tc.cov_mask) == 0);
      if (top_k > 1 && same_clean_cov_set) {
        ++same_cov_count;
        worst_same_cov_idx = tc_i;
        if (play_key == tc.play_key) {
          same_key_idx = tc_i;
        }
      }
    }
    size_t merge_cov_idx =
        (same_key_idx < top_candidates.size())
            ? same_key_idx
            : ((same_cov_count >= 3) ? worst_same_cov_idx
                                     : top_candidates.size());
    if (!found_sig && merge_cov_idx < top_candidates.size()) {
      found_sig = true;
      auto &tc = top_candidates[merge_cov_idx];
      bool beats_ufc = (unforced_fc == 0 && tc.unforced_fc > 0 &&
                        cov >= tc.covered - tc.unforced_fc);
      if (beats_ufc ||
          is_better_meta(cov, def_waste, conds, fc, td, extra_br, draws_trump,
                         tc.covered, tc.def_waste, tc.num_conds,
                         tc.falsecard_splits, tc.trump_draw_score,
                         tc.extra_branches, tc.draws_trump)) {
        tc.covered = cov;
        tc.unforced_fc = unforced_fc;
        tc.def_waste = def_waste;
        tc.num_conds = conds;
        tc.extra_branches = extra_br;
        tc.non_state_eb = nseb;
        tc.falsecard_splits = fc;
        tc.trump_draw_score = td;
        tc.draws_trump = draws_trump;
        tc.own_fc = group_fc;
        tc.leads_trump = cand_leads_trump;
        tc.all_branches_trump = all_br_trump;
        tc.trump_seat = cand_trump_seat;
        tc.trump_mode = cand_trump_mode;
        tc.sig = std::move(sig);
        tc.cov_mask = std::move(cov_mask);
        tc.play_key = std::move(play_key);
        tc.line = cand;
        changed = true;
      }
    }
    if (!found_sig) {
      auto top0_bkt = top_candidates.empty()
                          ? std::make_pair(size_t{0}, size_t{0})
                          : std::make_pair(top_candidates[0].falsecard_splits,
                                           top_candidates[0].non_state_eb);
      bool novel_clean_trump =
          (top_k > 1 && !group_fc && unforced_fc == 0 && cand_leads_trump &&
           !((explore_alts && cand_trump_mode < 4)
                 ? has_clean_trump_mode(cand_trump_mode)
                 : has_clean_trump()));
      bool beats_back_ufc =
          (unforced_fc == 0 && !top_candidates.empty() &&
           top_candidates.back().unforced_fc > 0 &&
           cov >= top_candidates.back().covered -
                      top_candidates.back().unforced_fc);
      if (static_cast<int>(top_candidates.size()) < top_k ||
          novel_clean_trump || beats_back_ufc ||
          is_better_meta(cov, def_waste, conds, fc, td, extra_br, draws_trump,
                         top_candidates.back().covered,
                         top_candidates.back().def_waste,
                         top_candidates.back().num_conds,
                         top_candidates.back().falsecard_splits,
                         top_candidates.back().trump_draw_score,
                         top_candidates.back().extra_branches,
                         top_candidates.back().draws_trump) ||
          (top_k > 1 &&
           (cand_bkt < min_existing_bkt ||
            (cand_bkt == min_existing_bkt && min_existing_bkt < top0_bkt &&
             is_better_meta(cov, def_waste, conds, fc, td, extra_br,
                            draws_trump, min_bkt_cov, min_bkt_dw, min_bkt_conds,
                            min_bkt_fc, min_bkt_td, min_bkt_eb,
                            min_bkt_dt))))) {
        top_candidates.push_back(TopCandidate{
            cov, unforced_fc, def_waste, conds, extra_br, nseb, fc, td,
            draws_trump, group_fc, cand_leads_trump, all_br_trump,
            cand_trump_seat, cand_trump_mode, std::move(sig),
            std::move(cov_mask), std::move(play_key), cand});
        changed = true;
      }
    }
    if (changed) {
      std::stable_sort(top_candidates.begin(), top_candidates.end(),
                       cmp_top_cand);
      if (static_cast<int>(top_candidates.size()) > top_k) {
        if (lvl > 0 && top_k > 1) {
          std::set<std::pair<size_t, size_t>> seen_fc_nseb;
          bool keep_clean_alts = (explore_alts && lvl <= 4);
          std::stable_partition(
              top_candidates.begin(), top_candidates.end(),
              [&](const TopCandidate &tc) {
                return (tc.falsecard_splits == 0 &&
                        (tc.non_state_eb == 0 || keep_clean_alts)) ||
                       seen_fc_nseb
                           .insert({tc.falsecard_splits, tc.non_state_eb})
                           .second;
              });
        }
        if (top_k > 1) {
          bool swapped = false;
          size_t min_bkt_idx = 0;
          auto min_cand_bkt = std::make_pair(top_candidates[0].falsecard_splits,
                                             top_candidates[0].non_state_eb);
          for (size_t k = 1; k < top_candidates.size(); ++k) {
            auto bkt = std::make_pair(top_candidates[k].falsecard_splits,
                                      top_candidates[k].non_state_eb);
            if (bkt < min_cand_bkt) {
              min_cand_bkt = bkt;
              min_bkt_idx = k;
            }
          }
          if (min_bkt_idx >= static_cast<size_t>(top_k)) {
            std::swap(top_candidates[top_k - 1], top_candidates[min_bkt_idx]);
            swapped = true;
          }
          if (trump_suit != NOTRUMP) {
            if (explore_alts && top_k >= 3 && lvl <= 4) {
              std::vector<uint8_t> pinned(top_k, 0);
              pinned[0] = 1;
              int base_m = (!top_candidates[0].own_fc &&
                            top_candidates[0].unforced_fc == 0 &&
                            top_candidates[0].falsecard_splits == 0)
                               ? top_candidates[0].trump_mode
                               : -1;
              for (int target_mode : OrderedTrumpModes(base_m)) {
                bool present = false;
                for (int k = 0; k < top_k; ++k) {
                  if (!top_candidates[k].own_fc &&
                      top_candidates[k].unforced_fc == 0 &&
                      top_candidates[k].falsecard_splits == 0 &&
                      top_candidates[k].trump_mode == target_mode) {
                    pinned[k] = 1;
                    present = true;
                    break;
                  }
                }
                if (!present) {
                  for (size_t k = static_cast<size_t>(top_k);
                       k < top_candidates.size(); ++k) {
                    if (!top_candidates[k].own_fc &&
                        top_candidates[k].unforced_fc == 0 &&
                        top_candidates[k].falsecard_splits == 0 &&
                        top_candidates[k].trump_mode == target_mode) {
                      for (int p = top_k - 1; p >= 1; --p) {
                        if (!pinned[p]) {
                          std::swap(top_candidates[p], top_candidates[k]);
                          pinned[p] = 1;
                          swapped = true;
                          break;
                        }
                      }
                      break;
                    }
                  }
                }
              }
            } else {
              bool has_trump_in_top = false;
              for (int k = 0; k < top_k; ++k) {
                if (!top_candidates[k].own_fc &&
                    top_candidates[k].unforced_fc == 0 &&
                    top_candidates[k].leads_trump) {
                  has_trump_in_top = true;
                  break;
                }
              }
              if (!has_trump_in_top) {
                for (size_t k = static_cast<size_t>(top_k);
                     k < top_candidates.size(); ++k) {
                  if (!top_candidates[k].own_fc &&
                      top_candidates[k].unforced_fc == 0 &&
                      top_candidates[k].leads_trump) {
                    size_t pos =
                        (min_bkt_idx >= static_cast<size_t>(top_k) &&
                         top_k >= 3)
                            ? static_cast<size_t>(top_k - 2)
                            : static_cast<size_t>(top_k - 1);
                    std::swap(top_candidates[pos], top_candidates[k]);
                    swapped = true;
                    break;
                  }
                }
              }
            }
          }
          if (swapped) {
            std::stable_sort(top_candidates.begin(),
                             top_candidates.begin() + top_k, cmp_top_cand);
          }
        }
        top_candidates.resize(top_k);
      }
      most_boards_covered = 0;
      best_eff_cov = best_clean_cov;
      for (const auto &tc : top_candidates) {
        most_boards_covered = std::max(most_boards_covered, tc.covered);
        best_eff_cov = std::max(best_eff_cov, tc.covered - tc.unforced_fc);
      }
    }
    return changed;
  };

  auto req_group_cov = [&](size_t j) -> int {
    size_t eff_conds =
        decl_leads_here ? (group_extra_splits[j] + 1) : groupings[j].size();
    int req = top_k_prune_floor(eff_conds, group_extra_splits[j],
                                group_draws_trump[j], group_has_fc[j],
                                group_def_waste[j] > 0, group_wins_partner[j]);
    int grp_mode = leader | (group_extra_splits[j] > 0 ? 1 : 0);
    bool need_leader_t =
        decl_leads_here && lvl >= 1 && lvl <= (explore_alts ? 4 : 2) &&
        group_extra_splits[j] <= 1 && group_draws_trump[j] &&
        !(explore_alts ? has_clean_trump_mode(grp_mode) : has_clean_trump());
    bool need_partner_t =
        explore_alts && decl_leads_here && lvl >= 1 && lvl <= 3 &&
        groupings[j].size() == 1 && !group_draws_trump[j] &&
        group_wins_partner[j] && !has_clean_trump_seat((leader + 2) % 4);
    bool need_trump_grp =
        (top_k > 1 && !group_has_fc[j] && (need_leader_t || need_partner_t));
    bool wide_clean_grp = wide_root_lvl && lvl == 1 && explore_alts &&
                          top_k > 1 && !group_has_fc[j] &&
                          group_extra_splits[j] <= 1;
    if (!group_uniform_t1[j].first.empty()) {
      auto it = best_t1_cov_conds.find(group_uniform_t1[j]);
      if (it != best_t1_cov_conds.end()) {
        int c_t1 = it->second.first;
        size_t min_conds = it->second.second;
        req = std::max(req, (eff_conds >= min_conds) ? (c_t1 + 1) : c_t1);
      }
    } else if (lvl > 0 && !need_trump_grp && !wide_clean_grp &&
               (group_has_fc[j] || !all_cands_falsecardable())) {
      int ref_cov = group_has_fc[j]
                        ? most_boards_covered
                        : ((group_def_waste[j] > 0) ? best_eff_cov
                                                    : best_zero_waste_cov);
      if (ref_cov > 0) {
        int slack = 0;
        if (explore_alts && top_k > 1 && lvl <= 4 && !group_has_fc[j] &&
            group_def_waste[j] == 0 &&
            (group_extra_splits[j] == 0 ||
             (decl_leads_here && group_extra_splits[j] == 1 &&
              group_draws_trump[j]))) {
          slack = std::max(2, ref_cov / 3);
        }
        req = std::max(req, std::max(1, ref_cov - slack));
      }
    }
    if (top_k <= 1 && known_best_cov > 0) {
      bool need_def_td =
          !decl_leads_here && lvl <= 4 && !group_has_fc[j] &&
          group_extra_splits[j] <= 1 && trump_suit != NOTRUMP &&
          (top_candidates.empty() ||
           (groupings[j].size() <= top_candidates[0].num_conds &&
            !top_candidates[0].all_branches_trump));
      bool can_tie_known =
          (group_draws_trump[j] && !group_has_fc[j] &&
           group_extra_splits[j] <= 1 &&
           (known_best_idx >= groupings.size() ||
            !group_draws_trump[known_best_idx])) ||
          need_def_td;
      bool skip_known_tie = lvl > 0 && j != known_best_idx && !can_tie_known;
      req = std::max(req,
                     skip_known_tie ? (known_best_cov + 1) : known_best_cov);
    }
    return req;
  };
  auto is_pruned = [&](size_t j) {
    if (group_unforced_fc_count[j] > 0 &&
        best_clean_cov >= std::max(1, min_covered) &&
        group_possible_boards[j] - group_unforced_fc_count[j] <=
            best_clean_cov) {
      return true;
    }
    if (group_def_waste[j] > 0 &&
        best_zero_waste_cov >= std::max(1, min_covered) &&
        group_possible_boards[j] <= best_zero_waste_cov) {
      return true;
    }
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
        int eff_cov = covered - lines[0].unforced_fc_count();
        batch_results[b_idx] =
            std::make_shared<ConditionalLine>(std::move(lines[0]));
        batch_covered[b_idx] = covered;
        int alt_slack = (explore_alts && lvl <= 4) ? std::max(4, eff_cov / 3)
                                                   : std::max(2, eff_cov / 4);
        int alt_floor = std::max(1, eff_cov - alt_slack);
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
      int known_ufc = group_unforced_fc_count[j];
      for (size_t k = 0; k < groupings[j].size(); ++k) {
        size_t uid = group_trick_ids[j][k];
        updated_possible += u_upper_bound[uid];
        if (!u_evaluated[uid] || !saved_work.at(keys[j][k])) {
          all_known = false;
        } else {
          known_ufc += saved_work.at(keys[j][k])->unforced_fc_count();
        }
      }
      group_possible_boards[j] = updated_possible;
      if (all_known && known_ufc == 0 && group_def_waste[j] == 0 &&
          updated_possible >= req_group_cov(j) &&
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
        int max_possible_so_far =
            group_possible_boards[i] - group_unforced_fc_count[i];
        size_t best_j = i;
        for (size_t j = i + 1; j < groupings.size(); ++j) {
          int eff_poss_j =
              group_possible_boards[j] - group_unforced_fc_count[j];
          if (!is_pruned(j) && eff_poss_j > max_possible_so_far) {
            max_possible_so_far = eff_poss_j;
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
    if (group_unforced_fc_count[i] > 0 &&
        best_clean_cov >= std::max(1, min_covered) &&
        sum_covered - group_unforced_fc_count[i] <= best_clean_cov) {
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
        trump_suit,
        [&](const std::vector<size_t> &picks) {
          ConditionalLine cl;
          for (size_t m = 0; m < group.size(); ++m) {
            cl.AddCondition(new TrickPlayCondition(keys[i][m]),
                            group_branches[m][picks[m]]);
          }
          cl.set_unforced_fc_count(
              std::min(cl.CoveredFinalBoards(),
                       cl.unforced_fc_count() + group_unforced_fc_count[i]));
          return cl;
        },
        wide_beam_ || (max_finalist_lines_ > 1));
    bool changed = false;
    size_t eff_conds =
        decl_leads_here ? (group_extra_splits[i] + 1) : group.size();
    for (size_t g_idx = 0; g_idx < group_lines.size(); ++g_idx) {
      const Trick1CardPair &t1 =
          (g_idx == 0) ? group_uniform_t1[i] : Trick1CardPair{"", ""};
      if (add_top_candidate(group_lines[g_idx], t1, eff_conds,
                            group_draws_trump[i], group_has_fc[i],
                            group_lines[g_idx].unforced_fc_count(),
                            group_def_waste[i])) {
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
  if (!decl_leads_here && !IsStopRequested() && groupings.size() > 1) {
    struct SuitSlice {
      std::vector<GeneralizedTrick> tricks;
      std::vector<std::string> slice_keys;
      std::vector<size_t> uids;
      bool has_fc = false;
      bool all_suited = true;
      size_t extra_splits = 0;
      int ufc = 0;
      size_t def_waste = 0;
    };
    std::array<std::vector<SuitSlice>, 4> suit_slices;
    std::array<std::set<std::vector<std::string>>, 4> seen_slices;
    for (size_t g_i = 0; g_i < groupings.size(); ++g_i) {
      if (groupings[g_i].empty()) continue;
      std::array<std::vector<size_t>, 4> by_s;
      uint8_t multi_taint = 0;
      for (size_t k = 0; k < groupings[g_i].size(); ++k) {
        int s = TrickEffectiveLeadSuit(groupings[g_i][k]);
        if (s >= 0) {
          by_s[s].push_back(k);
        } else {
          multi_taint |= TrickLeadSuitsMask(groupings[g_i][k]);
        }
      }
      for (int s = 0; s < 4; ++s) {
        if ((multi_taint & (1u << s)) != 0 || by_s[s].empty()) continue;
        std::vector<std::string> skeys;
        skeys.reserve(by_s[s].size());
        for (size_t k : by_s[s]) skeys.push_back(keys[g_i][k]);
        if (!seen_slices[s].insert(skeys).second) continue;
        SuitSlice sl;
        sl.slice_keys = std::move(skeys);
        sl.tricks.reserve(by_s[s].size());
        sl.uids.reserve(by_s[s].size());
        for (size_t k : by_s[s]) {
          const GeneralizedTrick &tk = groupings[g_i][k];
          if (tk.demi1.opp_card.empty() || tk.demi1.opp_card[0] == '_') {
            sl.all_suited = false;
          }
          sl.tricks.push_back(tk);
          sl.uids.push_back(group_trick_ids[g_i][k]);
        }
        sl.has_fc = GroupHasFalsecardSplit(sl.tricks);
        sl.extra_splits = GroupExtraSplits(sl.tricks);
        if (!keep_all_top_k_ && sl.has_fc) {
          sl.ufc = CountGroupUnforcedFalsecards(sl.tricks, board_trees);
        }
        if (!keep_all_top_k_) {
          sl.def_waste =
              GroupDefenderPlayCost(sl.tricks, board_trees, trump_suit);
        }
        suit_slices[s].push_back(std::move(sl));
      }
    }
    int active_suits = 0;
    for (int s = 0; s < 4; ++s) {
      if (!suit_slices[s].empty()) ++active_suits;
    }
    if (active_suits >= 2) {
      std::array<int, 4> max_suit_ub = {0, 0, 0, 0};
      std::array<bool, 4> has_best_eval = {false, false, false, false};
      std::array<int, 4> best_eval_cov = {0, 0, 0, 0};
      std::array<size_t, 4> best_eval_conds = {0, 0, 0, 0};
      std::array<bool, 4> best_eval_all_trump = {false, false, false, false};
      for (int s = 0; s < 4; ++s) {
        for (const SuitSlice &sl : suit_slices[s]) {
          bool failed = false;
          bool all_ev = true;
          int ub = 0;
          int cov = 0;
          int tot_ufc = sl.ufc;
          bool all_tr = (trump_suit != NOTRUMP);
          for (size_t k = 0; k < sl.uids.size(); ++k) {
            size_t uid = sl.uids[k];
            ub += u_upper_bound[uid];
            if (!u_evaluated[uid]) {
              all_ev = false;
            } else if (!saved_work.at(sl.slice_keys[k])) {
              all_ev = false;
              if (u_upper_bound[uid] <= 0) {
                failed = true;
              }
            } else {
              const auto &ch = saved_work.at(sl.slice_keys[k]);
              cov += saved_covered.at(sl.slice_keys[k]);
              tot_ufc += ch->unforced_fc_count();
              if (trump_suit != NOTRUMP &&
                  !LeadsTrumpInitially(*ch, trump_suit)) {
                all_tr = false;
              }
            }
          }
          if (failed) continue;
          if (!sl.has_fc && sl.def_waste == 0) {
            max_suit_ub[s] = std::max(max_suit_ub[s], ub);
          }
          if (all_ev && !sl.has_fc && sl.def_waste == 0 && tot_ufc == 0) {
            if (!has_best_eval[s] || cov > best_eval_cov[s] ||
                (cov == best_eval_cov[s] &&
                 (sl.tricks.size() < best_eval_conds[s] ||
                  (sl.tricks.size() == best_eval_conds[s] && all_tr &&
                   !best_eval_all_trump[s])))) {
              has_best_eval[s] = true;
              best_eval_cov[s] = cov;
              best_eval_conds[s] = sl.tricks.size();
              best_eval_all_trump[s] = all_tr;
            }
          }
        }
      }
      int sum_max_ub =
          max_suit_ub[0] + max_suit_ub[1] + max_suit_ub[2] + max_suit_ub[3];
      int win_floor = competitive_window_floor(false);
      std::vector<std::pair<std::string, const GeneralizedTrick *>> s_batch;
      std::unordered_map<std::string, int> s_trick_req;
      if (sum_max_ub >= win_floor) {
        for (int s = 0; s < 4; ++s) {
          int other_ub = sum_max_ub - max_suit_ub[s];
          for (const SuitSlice &sl : suit_slices[s]) {
            if (sl.has_fc || sl.def_waste > 0 || sl.extra_splits > 1) continue;
            bool failed = false;
            bool any_unev = false;
            int sl_ub = 0;
            for (size_t k = 0; k < sl.uids.size(); ++k) {
              size_t uid = sl.uids[k];
              sl_ub += u_upper_bound[uid];
              if (!u_evaluated[uid]) {
                any_unev = true;
              } else if (!saved_work.at(sl.slice_keys[k])) {
                if (u_upper_bound[uid] > 0) {
                  any_unev = true;
                } else {
                  failed = true;
                }
              }
            }
            if (failed || !any_unev || sl_ub + other_ub < win_floor) continue;
            bool can_tie_s =
                !has_best_eval[s] || sl.tricks.size() < best_eval_conds[s] ||
                (lvl <= 4 && trump_suit != NOTRUMP &&
                 sl.tricks.size() <= best_eval_conds[s] &&
                 !best_eval_all_trump[s]);
            int req_sl = win_floor - other_ub;
            if (top_k <= 1 && has_best_eval[s]) {
              req_sl = std::max(
                  req_sl,
                  can_tie_s ? best_eval_cov[s] : (best_eval_cov[s] + 1));
            }
            if (sl_ub < req_sl) continue;
            for (size_t k = 0; k < sl.uids.size(); ++k) {
              size_t uid = sl.uids[k];
              const std::string &k_str = sl.slice_keys[k];
              if (u_evaluated[uid] && saved_work.at(k_str) != nullptr) continue;
              int t_req = std::max(1, req_sl - (sl_ub - u_upper_bound[uid]));
              if (t_req > u_upper_bound[uid]) continue;
              auto it = s_trick_req.find(k_str);
              if (it == s_trick_req.end()) {
                s_trick_req.emplace(k_str, t_req);
                s_batch.emplace_back(k_str, &sl.tricks[k]);
              } else {
                it->second = std::min(it->second, t_req);
              }
            }
          }
        }
      }
      if (!s_batch.empty() && !IsStopRequested()) {
        std::vector<std::shared_ptr<ConditionalLine>> s_res(s_batch.size());
        std::vector<std::vector<std::shared_ptr<ConditionalLine>>> s_alts(
            s_batch.size());
        std::vector<int> s_cov(s_batch.size(), 0);
        ParallelTaskBatch(s_batch.size(), [&](size_t b_idx) {
          if (IsStopRequested()) return;
          int req_cov = s_trick_req.at(s_batch[b_idx].first);
          std::vector<ConditionalLine> lines = GetLinesFromTrick(
              *s_batch[b_idx].second, min_tricks, cache, board_trees,
              board_offsets, leader, lvl, /*progress=*/nullptr, req_cov,
              child_top_k);
          int covered = lines.empty() ? 0 : lines[0].CoveredFinalBoards();
          if (covered >= req_cov) {
            int eff_cov = covered - lines[0].unforced_fc_count();
            s_res[b_idx] =
                std::make_shared<ConditionalLine>(std::move(lines[0]));
            s_cov[b_idx] = covered;
            int alt_slack = (explore_alts && lvl <= 4)
                                ? std::max(4, eff_cov / 3)
                                : std::max(2, eff_cov / 4);
            int alt_floor = std::max(1, eff_cov - alt_slack);
            for (size_t m = 1; m < lines.size(); ++m) {
              if (lines[m].CoveredFinalBoards() >= alt_floor) {
                s_alts[b_idx].push_back(
                    std::make_shared<ConditionalLine>(std::move(lines[m])));
              }
            }
          } else {
            s_cov[b_idx] = std::max(0, req_cov - 1);
          }
        });
        for (size_t b_idx = 0; b_idx < s_batch.size(); ++b_idx) {
          const std::string &k_str = s_batch[b_idx].first;
          saved_covered[k_str] = s_cov[b_idx];
          saved_work[k_str] = std::move(s_res[b_idx]);
          if (!s_alts[b_idx].empty()) {
            saved_work_alts[k_str] = std::move(s_alts[b_idx]);
          }
          size_t uid = key_to_uid.at(k_str);
          u_evaluated[uid] = 1;
          u_upper_bound[uid] = s_cov[b_idx];
        }
      }
      struct EvalSlice {
        const SuitSlice *sl = nullptr;
        int cov = 0;
        int ufc = 0;
        size_t def_waste = 0;
        size_t conds = 0;
        size_t fc = 0;
        size_t td = 0;
        size_t eb = 0;
        bool draws_trump_soon = false;
        uint8_t trump_mode_mask = 0;
        uint8_t imm_trump_mode_mask = 0;
      };
      std::array<std::vector<EvalSlice>, 4> ranked_slices;
      bool all_suits_have_eval = true;
      for (int s = 0; s < 4; ++s) {
        if (suit_slices[s].empty()) continue;
        for (const SuitSlice &sl : suit_slices[s]) {
          bool ok = true;
          bool dts_possible = (trump_suit != NOTRUMP);
          uint8_t mode_all_mask = 0x0fu;
          uint8_t imm_all_mask = 0x0fu;
          ConditionalLine sl_cl;
          for (size_t k = 0; k < sl.slice_keys.size(); ++k) {
            const std::string &ks = sl.slice_keys[k];
            auto it = saved_work.find(ks);
            if (it == saved_work.end() || !it->second) {
              ok = false;
              break;
            }
            sl_cl.AddCondition(new TrickPlayCondition(ks), it->second);
            if (dts_possible) {
              bool any_dts = false;
              uint8_t k_mode_mask = 0;
              uint8_t k_imm_mask = 0;
              auto check_opt = [&](const ConditionalLine &opt_cl) {
                int tm = InitialTrumpLeadMode(opt_cl, trump_suit);
                if (tm >= 0) {
                  any_dts = true;
                  if (tm < 4 && opt_cl.unforced_fc_count() == 0 &&
                      FalsecardableSplits(opt_cl) == 0) {
                    k_mode_mask |= static_cast<uint8_t>(1u << tm);
                    int imm_tm = ImmediateTrumpLeadMode(opt_cl, trump_suit);
                    if (imm_tm == tm) {
                      k_imm_mask |= static_cast<uint8_t>(1u << imm_tm);
                    }
                  }
                }
              };
              check_opt(*it->second);
              auto alt_it = saved_work_alts.find(ks);
              if (alt_it != saved_work_alts.end()) {
                for (const auto &alt_cl : alt_it->second) {
                  if (alt_cl) check_opt(*alt_cl);
                }
              }
              if (sl.tricks[k].NumBoards() <= 1) {
                k_mode_mask |= static_cast<uint8_t>((k_mode_mask & 0x5u) << 1);
                k_imm_mask |= static_cast<uint8_t>((k_imm_mask & 0x5u) << 1);
              }
              mode_all_mask &= k_mode_mask;
              imm_all_mask &= k_imm_mask;
              if (!any_dts) dts_possible = false;
            }
          }
          if (!ok) continue;
          uint8_t trump_mode_mask = dts_possible ? mode_all_mask : 0;
          uint8_t imm_trump_mode_mask = dts_possible ? imm_all_mask : 0;
          int cov = sl_cl.CoveredFinalBoards();
          int tot_ufc =
              std::min(cov, sl_cl.unforced_fc_count() + sl.ufc);
          ranked_slices[s].push_back(EvalSlice{
              &sl, cov, tot_ufc, sl.def_waste, sl.tricks.size(),
              FalsecardableSplits(sl_cl),
              InitialTrumpDrawScore(sl_cl, trump_suit),
              sl_cl.ExtraBranches(), dts_possible, trump_mode_mask,
              imm_trump_mode_mask});
        }
        if (ranked_slices[s].empty()) {
          all_suits_have_eval = false;
          break;
        }
        std::stable_sort(
            ranked_slices[s].begin(), ranked_slices[s].end(),
            [&](const EvalSlice &a, const EvalSlice &b) {
              if ((a.ufc > 0) != (b.ufc > 0)) {
                int ea = a.cov - a.ufc;
                int eb_v = b.cov - b.ufc;
                if (ea != eb_v) return ea > eb_v;
                return a.ufc == 0;
              }
              if (is_better_meta(a.cov, a.def_waste, a.conds, a.fc, a.td,
                                 a.eb, false, b.cov, b.def_waste, b.conds,
                                 b.fc, b.td, b.eb, false)) {
                return true;
              }
              if (is_better_meta(b.cov, b.def_waste, b.conds, b.fc, b.td,
                                 b.eb, false, a.cov, a.def_waste, a.conds,
                                 a.fc, a.td, a.eb, false)) {
                return false;
              }
              return a.sl->all_suited && !b.sl->all_suited;
            });
      }
      auto try_recombined = [&](const std::array<size_t, 4> &pick_s) {
        std::vector<std::string> comb_keys;
        std::vector<std::string> wild_keys;
        std::vector<uint64_t> used_mask(num_words, 0);
        bool comb_fc = false;
        int comb_ufc = 0;
        size_t comb_waste = 0;
        for (int s = 0; s < 4; ++s) {
          if (ranked_slices[s].empty()) continue;
          const SuitSlice &sl = *ranked_slices[s][pick_s[s]].sl;
          if (sl.has_fc) comb_fc = true;
          comb_ufc += sl.ufc;
          comb_waste += sl.def_waste;
          for (size_t k = 0; k < sl.uids.size(); ++k) {
            const uint64_t *m = &u_masks[sl.uids[k] * num_words];
            for (size_t w = 0; w < num_words; ++w) {
              if ((used_mask[w] & m[w]) != 0) return;
              used_mask[w] |= m[w];
            }
            const std::string &ks = sl.slice_keys[k];
            if (ks.size() >= 2 && ks[1] == '_') {
              wild_keys.push_back(ks);
            } else {
              comb_keys.push_back(ks);
            }
          }
        }
        comb_keys.insert(comb_keys.end(), wild_keys.begin(), wild_keys.end());
        if (comb_keys.empty()) return;
        std::vector<std::vector<std::shared_ptr<ConditionalLine>>> branches(
            comb_keys.size());
        for (size_t k = 0; k < comb_keys.size(); ++k) {
          branches[k].push_back(saved_work.at(comb_keys[k]));
          auto alt_it = saved_work_alts.find(comb_keys[k]);
          if (alt_it != saved_work_alts.end()) {
            branches[k].insert(branches[k].end(), alt_it->second.begin(),
                               alt_it->second.end());
          }
        }
        std::vector<ConditionalLine> comb_lines = AssembleBranchLines(
            branches, child_top_k > 1, lvl, /*min_diff_branches=*/1,
            trump_suit,
            [&](const std::vector<size_t> &picks) {
              ConditionalLine cl;
              for (size_t m = 0; m < comb_keys.size(); ++m) {
                cl.AddCondition(new TrickPlayCondition(comb_keys[m]),
                                branches[m][picks[m]]);
              }
              cl.set_unforced_fc_count(
                  std::min(cl.CoveredFinalBoards(),
                           cl.unforced_fc_count() + comb_ufc));
              return cl;
            },
            wide_beam_ || (max_finalist_lines_ > 1));
        for (size_t g_idx = 0; g_idx < comb_lines.size(); ++g_idx) {
          add_top_candidate(comb_lines[g_idx], Trick1CardPair{"", ""},
                            comb_keys.size(), /*draws_trump=*/false, comb_fc,
                            comb_lines[g_idx].unforced_fc_count(), comb_waste);
        }
      };
      if (all_suits_have_eval && !IsStopRequested()) {
        std::array<size_t, 4> base_picks = {0, 0, 0, 0};
        try_recombined(base_picks);
        if (top_k > 1) {
          for (int s = 0; s < 4; ++s) {
            if (ranked_slices[s].size() >= 2) {
              std::array<size_t, 4> alt_picks = base_picks;
              alt_picks[s] = 1;
              try_recombined(alt_picks);
            }
          }
          if (explore_alts && trump_suit != NOTRUMP) {
            std::vector<std::array<size_t, 4>> tried_trumps = {base_picks};
            auto push_trump_picks = [&](const std::array<size_t, 4> &tp) {
              for (const auto &prev : tried_trumps) {
                if (prev == tp) return;
              }
              tried_trumps.push_back(tp);
              try_recombined(tp);
            };
            for (int target_mode : {0, 1, 2, 3}) {
              uint8_t mode_bit = static_cast<uint8_t>(1u << target_mode);
              uint8_t seat_bits =
                  static_cast<uint8_t>(3u << (target_mode & ~1));
              std::array<size_t, 4> mode_picks = base_picks;
              bool any_mode_match = false;
              for (int s = 0; s < 4; ++s) {
                if (ranked_slices[s].empty()) continue;
                size_t imm_chosen = ranked_slices[s].size();
                size_t chosen = ranked_slices[s].size();
                size_t seat_fallback = ranked_slices[s].size();
                size_t dts_fallback = ranked_slices[s].size();
                for (size_t r_i = 0; r_i < ranked_slices[s].size(); ++r_i) {
                  const EvalSlice &es = ranked_slices[s][r_i];
                  if (es.ufc > 0 || es.fc > 0 || es.def_waste > 0) continue;
                  if ((es.imm_trump_mode_mask & mode_bit) != 0) {
                    imm_chosen = r_i;
                    any_mode_match = true;
                    break;
                  }
                  if ((es.trump_mode_mask & mode_bit) != 0 &&
                      chosen == ranked_slices[s].size()) {
                    chosen = r_i;
                    any_mode_match = true;
                  }
                  if ((es.trump_mode_mask & seat_bits) != 0 &&
                      seat_fallback == ranked_slices[s].size()) {
                    seat_fallback = r_i;
                  }
                  if (es.draws_trump_soon &&
                      dts_fallback == ranked_slices[s].size()) {
                    dts_fallback = r_i;
                  }
                }
                if (imm_chosen < ranked_slices[s].size()) {
                  mode_picks[s] = imm_chosen;
                } else if (chosen < ranked_slices[s].size()) {
                  mode_picks[s] = chosen;
                } else if (seat_fallback < ranked_slices[s].size()) {
                  mode_picks[s] = seat_fallback;
                } else if (dts_fallback < ranked_slices[s].size()) {
                  mode_picks[s] = dts_fallback;
                }
              }
              if (any_mode_match) {
                push_trump_picks(mode_picks);
              }
            }
          }
        }
        if (lvl == 0) {
          g_best_covered = std::max(most_boards_covered, known_best_cov);
        }
      }
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
  if (IsStopRequested() || lvl <= 0 || leader % 2 != 0 || state_depth >= 4 ||
      cards_left <= 0 || solvable_n < 2) {
    return;
  }
  const BoardState *first_bs =
      board_trees.at(*solvable_boards.begin())->MyBoard();
  Suit trump_suit = first_bs->GetTrump();
  bool may_tie_trump_disc =
      lvl <= 4 && state_depth == 0 && trump_suit != NOTRUMP &&
      prev_suit_led == trump_suit && !ans.empty() &&
      InitialTrumpDrawScore(ans[0], trump_suit) == 0;
  if (!may_tie_trump_disc &&
      solvable_n < std::max(best_covered + 1, min_covered)) {
    return;
  }

  std::set<int> disc_boards[2];
  std::set<int> no_disc_boards[2];
  int any_disc_count = 0;
  int both_disc_count = 0;
  bool opp_has_trump = false;
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
      if (may_tie_trump_disc && !opp_has_trump) {
        uint64_t opp_rem = (bs.GetHand(1) | bs.GetHand(3)) & ~bs.played_cards();
        if (((opp_rem >> (trump_suit * 16)) & 0xffffULL) != 0) {
          opp_has_trump = true;
        }
      }
    }
  }
  bool has_mixed_disc =
      (!disc_boards[0].empty() && !no_disc_boards[0].empty()) ||
      (!disc_boards[1].empty() && !no_disc_boards[1].empty());
  bool allow_trump_disc_tie =
      may_tie_trump_disc && opp_has_trump && has_mixed_disc;
  int min_possible_target =
      std::max(best_covered + (allow_trump_disc_tie ? 0 : 1), min_covered);
  if (solvable_n < min_possible_target) {
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
                                 auto make_cond,
                                 bool for_trump_draw = false) {
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
      if (fl.CoveredFinalBoards() <= 0) continue;
      if (for_trump_draw && !split_branches[1].empty() &&
          InitialTrumpDrawScore(fl, trump_suit) == 0) {
        continue;
      }
      split_branches[1].push_back(
          std::make_shared<ConditionalLine>(std::move(fl)));
    }
    std::vector<ConditionalLine> new_splits = AssembleBranchLines(
        split_branches, top_k > 1, /*lvl=*/1, /*min_diff_branches=*/2,
        trump_suit,
        [&](const std::vector<size_t> &picks) {
          ConditionalLine sl;
          sl.AddCondition(make_cond(true), split_branches[0][picks[0]]);
          sl.AddCondition(make_cond(false), split_branches[1][picks[1]]);
          return sl;
        },
        wide_beam_ || (max_finalist_lines_ > 1));
    int alt_slack = std::max(2, total_cov / 4);
    int alt_floor = std::max(1, total_cov - alt_slack);
    if (new_splits.size() > 1) {
      new_splits.erase(
          std::remove_if(new_splits.begin() + 1, new_splits.end(),
                         [alt_floor](const ConditionalLine &sl) {
                           return sl.CoveredFinalBoards() < alt_floor;
                         }),
          new_splits.end());
    }
    bool replace_for_trump =
        for_trump_draw && !new_splits.empty() &&
        FalsecardableSplits(new_splits[0]) == 0;
    if (top_k > 1 && !ans.empty() && !new_splits.empty() &&
        !replace_for_trump) {
      auto min_new_key = std::make_pair(FalsecardableSplits(new_splits[0]),
                                        NonStateExtraBranches(new_splits[0]));
      for (size_t k = 1; k < new_splits.size(); ++k) {
        min_new_key =
            std::min(min_new_key,
                     std::make_pair(FalsecardableSplits(new_splits[k]),
                                    NonStateExtraBranches(new_splits[k])));
      }
      size_t min_old_idx = 0;
      auto min_old_key = std::make_pair(FalsecardableSplits(ans[0]),
                                        NonStateExtraBranches(ans[0]));
      for (size_t k = 1; k < ans.size(); ++k) {
        auto key = std::make_pair(FalsecardableSplits(ans[k]),
                                  NonStateExtraBranches(ans[k]));
        if (key < min_old_key) {
          min_old_key = key;
          min_old_idx = k;
        }
      }
      bool keep_min_old =
          (min_old_key < min_new_key) &&
          !(FalsecardableSplits(new_splits[0]) == 0 &&
            LeadsTrumpInitially(new_splits[0], trump_suit) &&
            !LeadsTrumpInitially(ans[min_old_idx], trump_suit));
      std::vector<ConditionalLine> kept;
      bool keep_clean_finalist_alts =
          (wide_beam_ || max_finalist_lines_ > 1) && lvl <= 2;
      for (size_t k = 0; k < ans.size(); ++k) {
        bool is_min_old = keep_min_old && k == min_old_idx;
        bool is_trump_alt =
            lvl <= 2 && ans[k].CoveredFinalBoards() >= alt_floor &&
            LeadsTrumpInitially(ans[k], trump_suit);
        bool is_clean_alt =
            keep_clean_finalist_alts &&
            ans[k].CoveredFinalBoards() >= alt_floor &&
            FalsecardableSplits(ans[k]) == 0;
        if (is_min_old || is_trump_alt || is_clean_alt) {
          kept.push_back(std::move(ans[k]));
        }
      }
      ans = std::move(kept);
    } else {
      ans.clear();
    }
    for (auto &sl : new_splits) {
      ans.push_back(std::move(sl));
    }
    FinalizeTopKLines(&ans, top_k, trump_suit,
                      (wide_beam_ || max_finalist_lines_ > 1) && lvl <= 4);
  };

  uint64_t mask = ~first_bs->played_cards();
  uint64_t decl_cards = (first_bs->GetHand(0) | first_bs->GetHand(2)) & mask;
  uint64_t leader_cards = first_bs->GetHand(leader) & mask;

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
    bool is_trump_disc =
        (trump_suit != NOTRUMP && prev_suit_led == trump_suit);
    int disc_min_branch_cov =
        (lvl <= 5 && (is_trump_disc || cards_left <= 5))
            ? 1
            : ((cards_left >= 9 && has_unsplit_play)
                   ? std::max(min_branch_cov, std::max(4, solvable_n / 6))
                   : min_branch_cov);
    int base_disc_gain =
        (lvl <= 5 && (is_trump_disc || cards_left <= 5)) ? 1 : split_min_gain;
    int order[2] = {0, 1};
    if (disc_boards[1].size() > disc_boards[0].size()) {
      std::swap(order[0], order[1]);
    }
    bool first_disc_searched = false;
    for (int ord = 0; ord < 2; ++ord) {
      if (IsStopRequested() || best_covered > solvable_n ||
          (!allow_trump_disc_tie && best_covered >= solvable_n)) {
        break;
      }
      int disc_split_min_gain = allow_trump_disc_tie ? 0 : base_disc_gain;
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
      bool draws_trump_on_false =
          !false_lines.empty() &&
          InitialTrumpDrawScore(false_lines[0], trump_suit) > 0;
      bool adopted_for_trump =
          allow_trump_disc_tie && draws_trump_on_false &&
          total_cov < best_covered + base_disc_gain;
      int min_adopt_cov =
          (allow_trump_disc_tie && draws_trump_on_false)
              ? std::max(best_covered, min_covered)
              : std::max(best_covered + base_disc_gain, min_covered);
      if (total_cov >= min_adopt_cov) {
        Suit s = prev_suit_led;
        adopt_split_results(
            true_lines, false_lines, total_cov,
            [player, s](bool discarded) -> PlayCondition * {
              return new PlayerDiscardedCondition(player, s, discarded);
            },
            adopted_for_trump);
        allow_trump_disc_tie = false;
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

  // Null-move leads are double-dummy optimal on nearly every board, so they
  // always carry the largest upper bound and would otherwise be searched
  // first and win every coverage tie. Searching them last keeps them as
  // candidates while preferring informative leads; sorting by effective
  // non-falsecard board count puts clean full-coverage groups right ahead of
  // unforced-falsecard splits without promoting small subset groups.
  std::set<std::string> null_leads = FindNullMoveLeads(groupings, trump_suit);
  bool def_leads_here = (leader % 2 != 0);
  struct GroupSortKey {
    size_t idx = 0;
    bool is_null = false;
    bool has_fc = false;
    bool draws_trump = false;
    size_t extra_splits = 0;
    size_t max_b = 0;
    size_t tot = 0;
    size_t eff_tot = 0;
    size_t sz = 0;
    size_t card_cost = 0;
  };
  std::vector<GroupSortKey> sort_keys(groupings.size());
  for (size_t i = 0; i < groupings.size(); ++i) {
    const auto &g = groupings[i];
    size_t mx = 0;
    size_t tot = 0;
    for (const auto &t : g) {
      mx = std::max(mx, t.NumBoards());
      tot += t.NumBoards();
    }
    bool has_fc = GroupHasFalsecardSplit(g);
    size_t unforced =
        has_fc
            ? static_cast<size_t>(CountGroupUnforcedFalsecards(g, board_trees))
            : 0;
    if (adversarial_rescore_ && !keep_all_top_k_ && !def_leads_here) {
      unforced += CountGroupIllusoryBossTrumpCovers(g, min_tricks, board_trees);
    }
    sort_keys[i] = GroupSortKey{
        i,
        !g.empty() && g[0].leader % 2 == 0 &&
            null_leads.count(g[0].demi1.my_card) > 0,
        has_fc,
        GroupDrawsTrump(g, trump_suit),
        GroupExtraSplits(g),
        mx,
        tot,
        (unforced < tot) ? (tot - unforced) : 0,
        g.size(),
        def_leads_here ? GroupDefenderPlayCost(g, board_trees, trump_suit) : 0,
    };
  }
  std::stable_sort(
      sort_keys.begin(), sort_keys.end(),
      [def_leads_here](const GroupSortKey &a, const GroupSortKey &b) {
        if (a.is_null != b.is_null) return !a.is_null;
        if (def_leads_here) {
          if (a.eff_tot != b.eff_tot) return a.eff_tot > b.eff_tot;
          if (a.has_fc != b.has_fc) return !a.has_fc;
          if ((a.card_cost > 0) != (b.card_cost > 0)) return a.card_cost == 0;
          if (a.tot != b.tot) return a.tot > b.tot;
          if (a.sz != b.sz) return a.sz < b.sz;
          if (a.extra_splits != b.extra_splits) {
            return a.extra_splits < b.extra_splits;
          }
          return a.card_cost < b.card_cost;
        }
        size_t eff_b_a = (!a.has_fc && a.draws_trump) ? a.eff_tot : a.max_b;
        size_t eff_b_b = (!b.has_fc && b.draws_trump) ? a.eff_tot : b.max_b;
        if (eff_b_a != eff_b_b) return eff_b_a > eff_b_b;
        if (a.draws_trump != b.draws_trump) {
          return a.draws_trump > b.draws_trump;
        }
        if (a.sz != b.sz) return a.sz < b.sz;
        if (a.eff_tot != b.eff_tot) return a.eff_tot > b.eff_tot;
        if (a.has_fc != b.has_fc) return !a.has_fc;
        return a.tot > b.tot;
      });
  std::vector<std::vector<GeneralizedTrick>> sorted_groupings(groupings.size());
  for (size_t i = 0; i < groupings.size(); ++i) {
    sorted_groupings[i] = std::move(groupings[sort_keys[i].idx]);
  }
  groupings = std::move(sorted_groupings);

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

std::vector<ConditionalLine> LineFinder::SolveGuidedLine(
    const ConditionalLine &guide,
    int min_tricks,
    SolverCache &cache_in,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl,
    Suit prev_suit_led, int state_depth,
    int guide_depth) {
  constexpr int kMaxGuideDepth = 6;
  if (IsStopRequested() || board_offsets.empty()) {
    return {};
  }
  const std::shared_ptr<OptimalPlayTree> &first_tree =
      board_trees.at(*board_offsets.begin());
  CHECK(first_tree);
  int cards_left = first_tree->CardsLeft();
  if (cards_left == 0) {
    if (min_tricks > 0) return {};
    return {ConditionalLine(board_offsets)};
  }
  std::set<int> solvable_boards;
  for (int b : board_offsets) {
    const std::shared_ptr<OptimalPlayTree> &pt = board_trees.at(b);
    CHECK(pt);
    if (WonEnoughTricks(pt->MiniMax(), min_tricks, pt->MyBoard()->CurPlayer(),
                        cards_left)) {
      solvable_boards.insert(solvable_boards.end(), b);
    }
  }
  if (solvable_boards.empty()) {
    return {};
  }
  bool stage_explore =
      !keep_all_top_k_ && adversarial_rescore_ &&
      (g_total_boards.load() <= 45 ||
       (max_finalist_lines_ > 1 && solvable_boards.size() <= 35));
  int stage_alt_k = (max_finalist_lines_ > 1) ? 4 : 2;
  int exact_top_k_early =
      (stage_explore && solvable_boards.size() >= 3 && lvl <= 4)
          ? ((lvl <= 2) ? stage_alt_k : std::min(3, stage_alt_k))
          : 1;
  bool handoff_to_exact =
      guide_depth >= kMaxGuideDepth ||
      (guide_depth >= 2 &&
       (cards_left <= 6 || solvable_boards.size() <= 8)) ||
      (max_finalist_lines_ > 1 && guide_depth >= 1 && cards_left <= 7 &&
       solvable_boards.size() <= 28) ||
      (stage_explore && g_total_boards.load() <= 45 && guide_depth >= 1 &&
       leader % 2 != 0 && solvable_boards.size() <= 38) ||
      (!keep_all_top_k_ && adversarial_rescore_ && guide_depth >= 2 &&
       leader % 2 != 0 && solvable_boards.size() <= 65) ||
      (!keep_all_top_k_ && adversarial_rescore_ && guide_depth == 1 &&
       lvl >= 1 && leader % 2 != 0 && cards_left <= 11 &&
       solvable_boards.size() <= 60);
  if (handoff_to_exact ||
      (guide.do_first().Empty() && guide.conditions().empty())) {
    int handoff_top_k =
        (!keep_all_top_k_ && adversarial_rescore_ && max_finalist_lines_ > 1 &&
         leader % 2 != 0 && lvl <= 3)
            ? std::max(exact_top_k_early, 2)
            : exact_top_k_early;
    return SolveHelper(min_tricks, cache_in, board_trees, solvable_boards,
                       leader, lvl, /*progress=*/nullptr, /*min_covered=*/0,
                       handoff_top_k, prev_suit_led, state_depth);
  }

  const auto &g_conds = guide.conditions();
  if (guide.do_first().Empty() && !g_conds.empty() && g_conds[0].first &&
      g_conds[0].first->IsStateCondition()) {
    auto clone_state_cond = [](const PlayCondition *c) -> PlayCondition * {
      if (const auto *icg = dynamic_cast<const IsCardGoodCondition *>(c)) {
        return new IsCardGoodCondition(icg->card(), icg->is_good());
      }
      if (const auto *pdc =
              dynamic_cast<const PlayerDiscardedCondition *>(c)) {
        return new PlayerDiscardedCondition(pdc->player(), pdc->suit(),
                                            pdc->discarded());
      }
      return nullptr;
    };
    std::vector<std::set<int>> sub_boards(g_conds.size());
    size_t non_empty_cnt = 0;
    size_t sole_idx = 0;
    for (size_t idx = 0; idx < g_conds.size(); ++idx) {
      if (!g_conds[idx].first || !g_conds[idx].second) continue;
      for (int b : solvable_boards) {
        if (g_conds[idx].first->EvaluateState(
                *board_trees.at(b)->MyBoard())) {
          sub_boards[idx].insert(sub_boards[idx].end(), b);
        }
      }
      if (!sub_boards[idx].empty()) {
        ++non_empty_cnt;
        sole_idx = idx;
      }
    }
    if (non_empty_cnt == 1) {
      return SolveGuidedLine(*g_conds[sole_idx].second, min_tricks, cache_in,
                             board_trees, sub_boards[sole_idx], leader, lvl,
                             prev_suit_led, state_depth, guide_depth);
    }
    if (non_empty_cnt > 1) {
      ConditionalLine sl;
      bool ok = true;
      for (size_t idx = 0; idx < g_conds.size(); ++idx) {
        if (sub_boards[idx].empty()) continue;
        PlayCondition *cloned = clone_state_cond(g_conds[idx].first.get());
        if (cloned == nullptr) {
          ok = false;
          break;
        }
        std::vector<ConditionalLine> sub_res = SolveGuidedLine(
            *g_conds[idx].second, min_tricks, cache_in, board_trees,
            sub_boards[idx], leader, lvl + 1, prev_suit_led, state_depth + 1,
            guide_depth);
        if (sub_res.empty()) {
          delete cloned;
          ok = false;
          break;
        }
        sl.AddCondition(
            cloned, std::make_shared<ConditionalLine>(std::move(sub_res[0])));
      }
      if (ok && sl.CoveredFinalBoards() > 0) {
        return {std::move(sl)};
      }
    }
  }

  std::vector<const ConditionalLine *> guide_actions;
  if (!guide.do_first().Empty()) {
    guide_actions.push_back(&guide);
  } else {
    for (const auto &[_c, ch] : g_conds) {
      if (ch && !ch->do_first().Empty()) {
        guide_actions.push_back(ch.get());
      }
    }
  }
  if (guide_actions.empty()) {
    return (lvl == 0)
               ? std::vector<ConditionalLine>{}
               : SolveHelper(min_tricks, cache_in, board_trees, solvable_boards,
                             leader, lvl, /*progress=*/nullptr,
                             /*min_covered=*/0, exact_top_k_early,
                             prev_suit_led, state_depth);
  }

  Suit trump_suit = first_tree->MyBoard()->GetTrump();
  GeneralizedTrickFinder gen_trick_finder(trump_suit);
  for (int b : solvable_boards) {
    const OptimalLines &lines = board_trees.at(b)->GetLines();
    for (const OptimalLine &trick : lines) {
      gen_trick_finder.RecordOptimalTrick(leader, trick, b);
    }
  }
  GeneralizedTricksVector gen_tricks =
      gen_trick_finder.GetGeneralizedTricks(500, /*populate_boards_set=*/false);
  if (gen_tricks.empty()) {
    return {};
  }
  std::vector<std::vector<GeneralizedTrick>> groupings =
      GroupTricks(std::move(gen_tricks));

  std::vector<GeneralizedTrick> guide_gts;
  guide_gts.reserve(guide_actions.size());
  for (const ConditionalLine *ga : guide_actions) {
    guide_gts.push_back(ga->do_first());
  }
  bool guide_has_fc = GroupHasFalsecardSplit(guide_gts);
  size_t guide_extra_splits = GroupExtraSplits(guide_gts);
  bool def_leads = (leader % 2 != 0);

  struct MatchedGroup {
    size_t idx = 0;
    bool exact = false;
    bool has_fc = false;
    size_t extra_splits = 0;
    size_t eff_tot = 0;
    size_t tot = 0;
    size_t sz = 0;
    size_t card_cost = 0;
  };
  std::vector<MatchedGroup> matched;

  auto is_offsuit_discard = [&](int s, const std::string &c) {
    if (s < 0 || c.size() < 2) return false;
    Suit cs = Card::ParseSuit(c[0]);
    return cs != static_cast<Suit>(s) &&
           (trump_suit == NOTRUMP || cs != trump_suit);
  };
  auto same_lead_offsuit_discard = [&](const Trick1CardPair &gp,
                                       const Trick1CardPair &p) {
    if (gp.first != p.first || gp.first.size() < 2) return false;
    int s_led = static_cast<int>(Card::ParseSuit(gp.first[0]));
    return is_offsuit_discard(s_led, gp.second) &&
           is_offsuit_discard(s_led, p.second);
  };
  auto same_winner_offsuit_discard = [&](int s, const Trick1CardPair &gp,
                                         const Trick1CardPair &p) {
    return gp.second == p.second && is_offsuit_discard(s, gp.first) &&
           is_offsuit_discard(s, p.first);
  };

  if (!def_leads) {
    std::string guide_my1 = guide_gts[0].demi1.my_card;
    bool uniform_my1 = true;
    std::set<Trick1CardPair> guide_pairs;
    for (const GeneralizedTrick &gt : guide_gts) {
      if (gt.demi1.my_card != guide_my1) uniform_my1 = false;
      guide_pairs.emplace(gt.demi1.my_card, gt.demi2.my_card);
    }
    bool root_unsplit_suited =
        lvl == 0 && guide_gts.size() == 1 &&
        guide_gts[0].demi1.opp_card.size() == 2 &&
        guide_gts[0].demi1.opp_card[1] == '_' &&
        guide_gts[0].demi1.opp_card[0] != '_' &&
        guide_gts[0].demi1.opp_card[0] != 'R';
    bool filter_illusory = adversarial_rescore_ && !keep_all_top_k_;
    for (size_t i = 0; i < groupings.size(); ++i) {
      const auto &g = groupings[i];
      if (g.empty()) continue;
      bool g_fc = GroupHasFalsecardSplit(g);
      if (g_fc && !guide_has_fc) continue;
      size_t g_extra = GroupExtraSplits(g);
      std::set<Trick1CardPair> g_pairs;
      bool same_my1 = true;
      for (const GeneralizedTrick &t : g) {
        if (uniform_my1 && t.demi1.my_card != guide_my1) same_my1 = false;
        g_pairs.emplace(t.demi1.my_card, t.demi2.my_card);
      }
      bool is_exact =
          (g_pairs == guide_pairs && g_extra <= guide_extra_splits);
      bool is_discard_sub = false;
      if (!is_exact && !g_fc && g_extra <= guide_extra_splits &&
          g_pairs.size() == guide_pairs.size()) {
        is_discard_sub = true;
        for (const auto &gp : guide_pairs) {
          bool ok_gp = (g_pairs.count(gp) > 0);
          for (const auto &p : g_pairs) {
            if (ok_gp) break;
            if (same_lead_offsuit_discard(gp, p)) ok_gp = true;
          }
          if (!ok_gp) {
            is_discard_sub = false;
            break;
          }
        }
        for (const auto &p : g_pairs) {
          if (!is_discard_sub) break;
          bool ok_p = (guide_pairs.count(p) > 0);
          for (const auto &gp : guide_pairs) {
            if (ok_p) break;
            if (same_lead_offsuit_discard(gp, p)) ok_p = true;
          }
          if (!ok_p) {
            is_discard_sub = false;
            break;
          }
        }
      }
      bool is_superset = false;
      if (!is_exact && !is_discard_sub && !root_unsplit_suited && same_my1 &&
          !g_fc && g_extra <= guide_extra_splits + 1) {
        is_superset = std::includes(g_pairs.begin(), g_pairs.end(),
                                    guide_pairs.begin(), guide_pairs.end());
        if (is_superset) {
          bool has_spot_x = false;
          bool has_honor_above_t = false;
          bool covers_ten = false;
          for (const GeneralizedTrick &t : g) {
            const std::string &opp1 = t.demi1.opp_card;
            if (opp1.size() == 2 && opp1[0] != '_' && opp1[0] != 'R') {
              if (opp1[1] == 'x') has_spot_x = true;
              if (opp1[1] == 'J' || opp1[1] == 'Q' || opp1[1] == 'K' ||
                  opp1[1] == 'A') {
                has_honor_above_t = true;
              }
              if (opp1[1] == 'X' || opp1[1] == 'T' || opp1[1] == '_') {
                covers_ten = true;
              }
            }
          }
          if (has_spot_x && has_honor_above_t && !covers_ten) {
            is_superset = false;
          }
        }
        if (is_superset && guide_gts.size() == 1) {
          const std::string &guide_my2 = guide_gts[0].demi2.my_card;
          bool has_low_in_suit = false;
          bool low_preserves_guide = true;
          for (const GeneralizedTrick &t : g) {
            const std::string &opp1 = t.demi1.opp_card;
            bool is_low = opp1.find('x') != std::string::npos ||
                          opp1.find('<') != std::string::npos ||
                          (opp1.size() == 2 && opp1[1] == '_' &&
                           opp1[0] != '_' && opp1[0] != 'R');
            if (is_low) {
              has_low_in_suit = true;
              if (t.demi2.my_card != guide_my2) low_preserves_guide = false;
            }
          }
          bool deep_same_dir_honor = false;
          if (guide_depth >= 2 && guide_my1.size() == 2 &&
              guide_my2.size() == 2 && guide_my1[0] == guide_my2[0]) {
            int rv1 = CardCharRankVal(guide_my1[1]);
            int rv_g2 = CardCharRankVal(guide_my2[1]);
            deep_same_dir_honor = true;
            for (const GeneralizedTrick &t : g) {
              if (t.demi2.my_card.size() != 2 ||
                  t.demi2.my_card[0] != guide_my1[0]) {
                deep_same_dir_honor = false;
                break;
              }
              int rv_t2 = CardCharRankVal(t.demi2.my_card[1]);
              if (rv_t2 < 8 || (rv_t2 > rv1) != (rv_g2 > rv1)) {
                deep_same_dir_honor = false;
                break;
              }
            }
          }
          if (!(has_low_in_suit && low_preserves_guide) &&
              !deep_same_dir_honor) {
            is_superset = false;
          }
        }
      }
      if (!is_exact && !is_discard_sub && !is_superset) continue;
      std::set<int> distinct_boards;
      for (const GeneralizedTrick &t : g) {
        for (const auto &ent : t.board_entries) {
          if (filter_illusory &&
              IsIllusoryBossTrumpCoverEntry(ent, t, min_tricks, board_trees)) {
            continue;
          }
          distinct_boards.insert(ent.board);
        }
      }
      size_t tot = distinct_boards.size();
      size_t ufc =
          g_fc ? static_cast<size_t>(
                     CountGroupUnforcedFalsecards(g, board_trees))
               : 0;
      matched.push_back(MatchedGroup{
          i, is_exact, g_fc, g_extra, (ufc < tot) ? (tot - ufc) : 0, tot,
          g.size(), 0});
    }
    std::stable_sort(
        matched.begin(), matched.end(),
        [](const MatchedGroup &a, const MatchedGroup &b) {
          size_t adj_a = a.eff_tot + (a.exact ? 1 : 0);
          size_t adj_b = b.eff_tot + (b.exact ? 1 : 0);
          if (adj_a != adj_b) return adj_a > adj_b;
          if (a.exact != b.exact) return a.exact > b.exact;
          if (a.eff_tot != b.eff_tot) return a.eff_tot > b.eff_tot;
          if (a.has_fc != b.has_fc) return !a.has_fc;
          if (a.extra_splits != b.extra_splits) {
            return a.extra_splits < b.extra_splits;
          }
          return a.sz < b.sz;
        });
  } else {
    auto is_double_ruff_pair = [&](int s, const Trick1CardPair &gp) {
      return trump_suit != NOTRUMP && s != static_cast<int>(trump_suit) &&
             gp.first.size() >= 2 && gp.second.size() >= 2 &&
             Card::ParseSuit(gp.first[0]) == trump_suit &&
             Card::ParseSuit(gp.second[0]) == trump_suit;
    };
    std::array<std::set<Trick1CardPair>, 4> guide_suit_pairs;
    std::array<size_t, 4> guide_suit_counts = {0, 0, 0, 0};
    std::array<bool, 4> guide_unsplit_d2_wild = {false, false, false, false};
    std::set<Trick1CardPair> guide_wild_pairs;
    for (const GeneralizedTrick &gt : guide_gts) {
      int s = TrickEffectiveLeadSuit(gt);
      if (s >= 0) {
        guide_suit_pairs[s].emplace(gt.demi1.my_card, gt.demi2.my_card);
        ++guide_suit_counts[s];
        if (guide_suit_counts[s] == 1 && gt.demi2.opp_card == "__") {
          guide_unsplit_d2_wild[s] = true;
        } else {
          guide_unsplit_d2_wild[s] = false;
        }
      } else {
        guide_wild_pairs.emplace(gt.demi1.my_card, gt.demi2.my_card);
      }
    }
    std::array<std::set<Trick1CardPair>, 4> avail_guide_pairs;
    for (const auto &g : groupings) {
      for (const GeneralizedTrick &t : g) {
        int s = TrickEffectiveLeadSuit(t);
        if (s < 0) continue;
        Trick1CardPair p{t.demi1.my_card, t.demi2.my_card};
        for (const auto &gp : guide_suit_pairs[s]) {
          if (gp == p || same_winner_offsuit_discard(s, gp, p)) {
            avail_guide_pairs[s].insert(gp);
          }
        }
      }
    }
    auto match_suit_pairs =
        [&](int s, const std::vector<const GeneralizedTrick *> &s_tricks,
            const std::set<Trick1CardPair> &s_pairs, bool sl_fc,
            bool *exact_out) -> bool {
      if (avail_guide_pairs[s].empty()) {
        if (!guide_suit_pairs[s].empty()) *exact_out = false;
        return true;
      }
      if (guide_unsplit_d2_wild[s] && s_pairs.size() > 1) {
        const std::string &guide_my2 = guide_suit_pairs[s].begin()->second;
        char suit_ch = "SHDC"[s];
        for (const GeneralizedTrick *tp : s_tricks) {
          const std::string &o2 = tp->demi2.opp_card;
          if (o2.empty() || o2[0] != suit_ch) return false;
          if ((o2.find('x') != std::string::npos ||
               o2.find('<') != std::string::npos) &&
              tp->demi2.my_card != guide_my2) {
            return false;
          }
        }
      }
      for (const auto &gp : avail_guide_pairs[s]) {
        if (s_pairs.count(gp) == 0 && !is_double_ruff_pair(s, gp)) {
          bool sub_disc = false;
          for (const auto &p : s_pairs) {
            if (same_winner_offsuit_discard(s, gp, p)) {
              sub_disc = true;
              *exact_out = false;
              break;
            }
          }
          if (!sub_disc) return false;
        }
      }
      for (const auto &p : s_pairs) {
        if (guide_suit_pairs[s].count(p) > 0) continue;
        *exact_out = false;
        if (sl_fc) return false;
        bool allow_sub = false;
        for (const auto &gp : guide_suit_pairs[s]) {
          if (gp.first == p.first || is_double_ruff_pair(s, gp) ||
              same_winner_offsuit_discard(s, gp, p)) {
            allow_sub = true;
            break;
          }
        }
        if (!allow_sub) return false;
      }
      return true;
    };
    if (guide_wild_pairs.empty() && groupings.size() > 1) {
      std::array<std::vector<GeneralizedTrick>, 4> best_s_slice;
      std::array<size_t, 4> best_s_boards = {0, 0, 0, 0};
      std::array<bool, 4> best_s_exact = {false, false, false, false};
      std::array<size_t, 4> best_s_cost = {0, 0, 0, 0};
      for (const auto &g : groupings) {
        if (g.empty()) continue;
        std::array<std::vector<GeneralizedTrick>, 4> by_s;
        uint8_t multi_taint = 0;
        for (const GeneralizedTrick &t : g) {
          int s = TrickEffectiveLeadSuit(t);
          if (s >= 0) {
            by_s[s].push_back(t);
          } else {
            multi_taint |= TrickLeadSuitsMask(t);
          }
        }
        for (int s = 0; s < 4; ++s) {
          if ((multi_taint & (1u << s)) != 0 || by_s[s].empty()) continue;
          bool sl_fc = GroupHasFalsecardSplit(by_s[s]);
          if (sl_fc && !guide_has_fc) continue;
          bool cur_exact = true;
          size_t s_tot = 0;
          bool cur_suited = true;
          std::set<Trick1CardPair> s_pairs;
          std::vector<const GeneralizedTrick *> s_ptrs;
          s_ptrs.reserve(by_s[s].size());
          for (const GeneralizedTrick &t : by_s[s]) {
            s_tot += t.NumBoards();
            const std::string &o1 = t.demi1.opp_card;
            if (o1.empty() || o1[0] == '_') cur_suited = false;
            s_pairs.emplace(t.demi1.my_card, t.demi2.my_card);
            s_ptrs.push_back(&t);
          }
          if (!match_suit_pairs(s, s_ptrs, s_pairs, sl_fc, &cur_exact)) {
            continue;
          }
          size_t cur_cost =
              GroupDefenderPlayCost(by_s[s], board_trees, trump_suit);
          bool prev_suited = !best_s_slice[s].empty();
          for (const GeneralizedTrick &pt : best_s_slice[s]) {
            if (pt.demi1.opp_card.empty() || pt.demi1.opp_card[0] == '_') {
              prev_suited = false;
            }
          }
          bool better_tie =
              (cur_cost != best_s_cost[s])
                  ? (cur_cost < best_s_cost[s])
                  : ((cur_exact != best_s_exact[s])
                         ? cur_exact
                         : (cur_suited && !prev_suited));
          if (s_tot > best_s_boards[s] ||
              (s_tot == best_s_boards[s] && better_tie)) {
            best_s_boards[s] = s_tot;
            best_s_exact[s] = cur_exact;
            best_s_cost[s] = cur_cost;
            best_s_slice[s] = std::move(by_s[s]);
          }
        }
      }
      std::vector<GeneralizedTrick> synth;
      std::set<int> used_b;
      bool disjoint = true;
      for (int pass = 0; pass < 2 && disjoint; ++pass) {
        for (int s = 0; s < 4 && disjoint; ++s) {
          for (GeneralizedTrick &t : best_s_slice[s]) {
            bool is_wild =
                t.demi1.opp_card.empty() || t.demi1.opp_card[0] == '_';
            if ((pass == 0) == is_wild) continue;
            for (const auto &ent : t.board_entries) {
              if (!used_b.insert(ent.board).second) {
                disjoint = false;
                break;
              }
            }
            if (!disjoint) break;
            synth.push_back(std::move(t));
          }
        }
      }
      if (disjoint && !synth.empty()) {
        groupings.push_back(std::move(synth));
      }
    }
    for (size_t i = 0; i < groupings.size(); ++i) {
      const auto &g = groupings[i];
      if (g.empty()) continue;
      bool g_fc = GroupHasFalsecardSplit(g);
      if (g_fc && !guide_has_fc) continue;
      size_t g_extra = GroupExtraSplits(g);
      if (g_extra > guide_extra_splits + (!g_fc ? 1 : 0)) {
        continue;
      }
      bool ok = true;
      bool is_exact = (g_extra <= guide_extra_splits);
      std::array<std::set<Trick1CardPair>, 4> g_suit_pairs;
      std::array<std::vector<const GeneralizedTrick *>, 4> g_suit_ptrs;
      for (const GeneralizedTrick &t : g) {
        Trick1CardPair p{t.demi1.my_card, t.demi2.my_card};
        int s = TrickEffectiveLeadSuit(t);
        if (s >= 0) {
          g_suit_pairs[s].insert(p);
          g_suit_ptrs[s].push_back(&t);
        } else if (!guide_wild_pairs.empty() &&
                   guide_wild_pairs.count(p) == 0) {
          ok = false;
          break;
        }
      }
      if (!ok) continue;
      for (int s = 0; s < 4; ++s) {
        if (g_suit_pairs[s].empty()) continue;
        if (!match_suit_pairs(s, g_suit_ptrs[s], g_suit_pairs[s], g_fc,
                              &is_exact)) {
          ok = false;
          break;
        }
      }
      if (!ok) continue;
      std::set<int> distinct_boards;
      for (const GeneralizedTrick &t : g) {
        for (const auto &ent : t.board_entries) {
          distinct_boards.insert(ent.board);
        }
      }
      size_t tot = distinct_boards.size();
      size_t ufc =
          g_fc ? static_cast<size_t>(
                     CountGroupUnforcedFalsecards(g, board_trees))
               : 0;
      matched.push_back(MatchedGroup{
          i, is_exact, g_fc, g_extra, (ufc < tot) ? (tot - ufc) : 0, tot,
          g.size(), GroupDefenderPlayCost(g, board_trees, trump_suit)});
    }
    std::stable_sort(matched.begin(), matched.end(),
                     [](const MatchedGroup &a, const MatchedGroup &b) {
                       if (a.eff_tot != b.eff_tot) return a.eff_tot > b.eff_tot;
                       if (a.has_fc != b.has_fc) return !a.has_fc;
                       if (a.card_cost != b.card_cost) {
                         return a.card_cost < b.card_cost;
                       }
                       if (a.exact != b.exact) return a.exact > b.exact;
                       if (a.tot != b.tot) return a.tot > b.tot;
                       if (a.sz != b.sz) return a.sz < b.sz;
                       return a.extra_splits < b.extra_splits;
                     });
  }

  int exact_top_k = exact_top_k_early;
  if (matched.empty()) {
    return (lvl == 0)
               ? std::vector<ConditionalLine>{}
               : SolveHelperUncached(min_tricks, cache_in, board_trees,
                                     solvable_boards, leader, lvl,
                                     /*progress=*/nullptr, /*min_covered=*/0,
                                     exact_top_k, prev_suit_led, state_depth);
  }

  const std::vector<GeneralizedTrick> &best_group = groupings[matched[0].idx];
  std::vector<std::vector<std::shared_ptr<ConditionalLine>>> trick_nodes(
      best_group.size());

  for (size_t m = 0; m < best_group.size(); ++m) {
    if (IsStopRequested()) return {};
    const GeneralizedTrick &gt = best_group[m];
    const ConditionalLine *ga = nullptr;
    for (int pass = 0; pass < 2 && ga == nullptr; ++pass) {
      for (const ConditionalLine *cand_ga : guide_actions) {
        const GeneralizedTrick &c_gt = cand_ga->do_first();
        Trick1CardPair gp{c_gt.demi1.my_card, c_gt.demi2.my_card};
        Trick1CardPair p{gt.demi1.my_card, gt.demi2.my_card};
        if (pass == 0) {
          if (gp != p) continue;
        } else if (def_leads) {
          int s_a = TrickEffectiveLeadSuit(c_gt);
          int s_b = TrickEffectiveLeadSuit(gt);
          if (s_a < 0 || s_a != s_b ||
              !same_winner_offsuit_discard(s_a, gp, p)) {
            continue;
          }
        } else {
          if (!same_lead_offsuit_discard(gp, p)) continue;
        }
        if (!def_leads &&
            (c_gt.demi1.opp_card == "R_") != (gt.demi1.opp_card == "R_")) {
          continue;
        }
        if (def_leads) {
          int s_a = TrickEffectiveLeadSuit(c_gt);
          int s_b = TrickEffectiveLeadSuit(gt);
          if (s_a >= 0 && s_b >= 0 && s_a != s_b) continue;
        }
        ga = cand_ga;
        break;
      }
    }

    if (ga == nullptr) {
      int ug_top_k = exact_top_k;
      int g_mode =
          (trump_suit != NOTRUMP && gt.NumBoards() <= 45)
              ? InitialTrumpLeadMode(guide, trump_suit)
              : -1;
      if (g_mode >= 0) {
        ug_top_k = std::max(ug_top_k, 4);
      }
      std::vector<ConditionalLine> unguided = GetLinesFromTrick(
          gt, min_tricks, cache_in, board_trees, solvable_boards, leader, lvl,
          /*progress=*/nullptr, /*min_covered=*/0, ug_top_k);
      if (unguided.empty() || unguided[0].CoveredFinalBoards() <= 0) {
        return {};
      }
      if (g_mode >= 0 && unguided.size() > 1) {
        int top_ug_cov = unguided[0].CoveredFinalBoards();
        std::stable_sort(
            unguided.begin(), unguided.end(),
            [&](const ConditionalLine &a, const ConditionalLine &b) {
              auto rank_ug = [&](const ConditionalLine &cl) {
                if (cl.CoveredFinalBoards() < top_ug_cov ||
                    cl.unforced_fc_count() > 0 ||
                    FalsecardableSplits(cl) > 0) {
                  return 0;
                }
                int tm = InitialTrumpLeadMode(cl, trump_suit);
                if (tm == g_mode) return 4;
                if (tm >= 0 && (tm & ~1) == (g_mode & ~1)) return 3;
                if (tm >= 0) return 2;
                return 1;
              };
              return rank_ug(a) > rank_ug(b);
            });
      }
      if (static_cast<int>(unguided.size()) > exact_top_k) {
        unguided.resize(exact_top_k);
      }
      for (auto &ug : unguided) {
        if (ug.CoveredFinalBoards() > 0) {
          trick_nodes[m].push_back(
              std::make_shared<ConditionalLine>(std::move(ug)));
        }
      }
      continue;
    }

    struct GuidedWinnerBranch {
      int next_leader = 0;
      size_t orig_size = 0;
      std::set<int> boards;
      std::set<std::string> winning_cards;
      std::vector<const GeneralizedTrick::BoardTrickEntry *> entries;
    };
    std::set<int> orig_boards;
    std::array<GuidedWinnerBranch, 4> by_winner;
    for (int w = 0; w < 4; ++w) by_winner[w].next_leader = w;
    bool allow_aug = adversarial_rescore_ && !keep_all_top_k_;
    for (const auto &entry : gt.board_entries) {
      if (allow_aug &&
          IsIllusoryBossTrumpCoverEntry(entry, gt, min_tricks, board_trees)) {
        continue;
      }
      orig_boards.insert(orig_boards.end(), entry.board);
      GuidedWinnerBranch &wb = by_winner[entry.winner];
      wb.boards.insert(wb.boards.end(), entry.board);
      wb.winning_cards.emplace(entry.win_card);
      wb.entries.push_back(&entry);
      wb.orig_size = wb.boards.size();
    }
    if (orig_boards.empty()) return {};
    std::vector<AugmentedTrickBoard> aug_boards =
        BuildAugmentedDefenderSuitEntries(
            gt, min_tricks, cache_in, board_trees, solvable_boards,
            orig_boards, leader, lvl, allow_aug);
    if (IsStopRequested()) return {};
    std::unordered_map<int, std::shared_ptr<OptimalPlayTree>> aug_tree_map;
    const bool has_aug = !aug_boards.empty();
    if (has_aug) {
      aug_tree_map.reserve(aug_boards.size());
      for (auto &ab : aug_boards) {
        GuidedWinnerBranch &wb = by_winner[ab.entry.winner];
        if (wb.orig_size == 0) continue;
        wb.boards.insert(ab.entry.board);
        wb.winning_cards.emplace(ab.entry.win_card);
        wb.entries.push_back(&ab.entry);
        aug_tree_map.emplace(ab.entry.board, std::move(ab.after_tree));
      }
      for (int w = 0; w < 4; ++w) {
        if (by_winner[w].entries.size() > by_winner[w].orig_size) {
          std::sort(by_winner[w].entries.begin(), by_winner[w].entries.end(),
                    [](const GeneralizedTrick::BoardTrickEntry *a,
                       const GeneralizedTrick::BoardTrickEntry *b) {
                      return a->board < b->board;
                    });
        }
      }
    }

    std::vector<int> active_ws;
    std::vector<std::vector<std::shared_ptr<ConditionalLine>>> wb_branches;
    for (int w = 0; w < 4; ++w) {
      const GuidedWinnerBranch &wb = by_winner[w];
      if (wb.orig_size == 0 || IsStopRequested()) continue;
      int trick_won = (w % 2 == 0) ? 1 : 0;
      int next_min_tricks = min_tricks - trick_won;
      int rem_cards = board_trees.at(wb.entries[0]->board)->CardsLeft();
      if (rem_cards <= 1) {
        if (next_min_tricks <= 0) {
          active_ws.push_back(w);
          wb_branches.push_back(
              {std::make_shared<ConditionalLine>(wb.boards)});
        }
        continue;
      }

      std::vector<std::shared_ptr<OptimalPlayTree>> played_trees(
          wb.entries.size());
      size_t non_aug_count = 0;
      if (omp_in_parallel() && wb.entries.size() > 1) {
        std::vector<size_t> pending;
        pending.reserve(wb.entries.size());
        for (size_t e_i = 0; e_i < wb.entries.size(); ++e_i) {
          const auto *ent = wb.entries[e_i];
          if (has_aug) {
            auto a_it = aug_tree_map.find(ent->board);
            if (a_it != aug_tree_map.end()) {
              played_trees[e_i] = a_it->second;
              continue;
            }
          }
          ++non_aug_count;
          auto deepened =
              board_trees.at(ent->board)->TryGetDeepened(ent->trick_str);
          if (deepened) {
            played_trees[e_i] = std::move(deepened);
          } else {
            pending.push_back(e_i);
          }
        }
        ParallelTaskBatch(pending.size(), [&](size_t p_i) {
          if (IsStopRequested()) return;
          size_t e_i = pending[p_i];
          const auto *ent = wb.entries[e_i];
          played_trees[e_i] =
              board_trees.at(ent->board)->PlayTrick(ent->trick_str, &cache_in);
        });
      } else {
        for (size_t e_i = 0; e_i < wb.entries.size(); ++e_i) {
          if (IsStopRequested()) break;
          const auto *ent = wb.entries[e_i];
          if (has_aug) {
            auto a_it = aug_tree_map.find(ent->board);
            if (a_it != aug_tree_map.end()) {
              played_trees[e_i] = a_it->second;
              continue;
            }
          }
          ++non_aug_count;
          played_trees[e_i] =
              board_trees.at(ent->board)->PlayTrick(ent->trick_str, &cache_in);
        }
      }
      if (IsStopRequested()) return {};
      g_play_trick_calls.fetch_add(non_aug_count, std::memory_order_relaxed);
      std::map<int, std::shared_ptr<OptimalPlayTree>> after_trees;
      for (size_t e_i = 0; e_i < wb.entries.size(); ++e_i) {
        CHECK(played_trees[e_i]);
        after_trees.emplace_hint(after_trees.end(), wb.entries[e_i]->board,
                                 std::move(played_trees[e_i]));
      }
      Suit next_prev_suit = wb.entries[0]->trick_str.empty()
                                ? NOTRUMP
                                : Card::ParseSuit(wb.entries[0]->trick_str[0]);
      const ConditionalLine *guided_child = nullptr;
      for (const auto &[w_cond, w_child] : ga->conditions()) {
        const auto *wpc =
            dynamic_cast<const WinnerPlayCondition *>(w_cond.get());
        if (wpc != nullptr && wpc->who() == w && w_child != nullptr) {
          guided_child = w_child.get();
          bool overlap = false;
          for (const std::string &wc : wb.winning_cards) {
            if (wpc->winning_cards().count(wc) > 0) {
              overlap = true;
              break;
            }
          }
          if (overlap || wpc->winning_cards().empty()) break;
        }
      }

      int wb_top_k =
          (stage_explore && wb.boards.size() >= 3 && lvl + 1 <= 4)
              ? ((lvl + 1 <= 2) ? stage_alt_k : std::min(3, stage_alt_k))
              : 1;
      std::vector<ConditionalLine> branch_res;
      if (guided_child != nullptr) {
        branch_res = SolveGuidedLine(*guided_child, next_min_tricks, cache_in,
                                     after_trees, wb.boards, w, lvl + 1,
                                     next_prev_suit, /*state_depth=*/0,
                                     guide_depth + 1);
      }
      int guided_wb_cov =
          branch_res.empty() ? 0 : branch_res[0].CoveredFinalBoards();
      if ((branch_res.empty() ||
           (wb.boards.size() <= 16 &&
            guided_wb_cov * 2 < static_cast<int>(wb.boards.size()))) &&
          !IsStopRequested()) {
        std::vector<ConditionalLine> exact_wb =
            SolveHelper(next_min_tricks, cache_in, after_trees, wb.boards, w,
                        lvl + 1, /*progress=*/nullptr, /*min_covered=*/0,
                        wb_top_k, next_prev_suit, /*state_depth=*/0);
        if (!exact_wb.empty() &&
            exact_wb[0].CoveredFinalBoards() > guided_wb_cov) {
          branch_res = std::move(exact_wb);
        }
      }
      std::vector<std::shared_ptr<ConditionalLine>> valid_br;
      for (auto &br : branch_res) {
        if (br.CoveredFinalBoards() > 0) {
          valid_br.push_back(
              std::make_shared<ConditionalLine>(std::move(br)));
        }
      }
      if (!valid_br.empty()) {
        active_ws.push_back(w);
        wb_branches.push_back(std::move(valid_br));
      }
    }
    if (wb_branches.empty()) {
      return {};
    }
    bool allow_finalist_alts = max_finalist_lines_ > 1;
    int assemble_top_k =
        (allow_finalist_alts && !keep_all_top_k_ && adversarial_rescore_ &&
         lvl <= 2)
            ? std::max(exact_top_k, stage_alt_k)
            : exact_top_k;
    std::vector<ConditionalLine> action_cands = AssembleBranchLines(
        wb_branches, assemble_top_k > 1, lvl, /*min_diff_branches=*/1,
        trump_suit,
        [&](const std::vector<size_t> &picks) {
          ConditionalLine action_line;
          action_line.SetAction(gt);
          for (size_t idx = 0; idx < active_ws.size(); ++idx) {
            if (picks[idx] < wb_branches[idx].size()) {
              int w = active_ws[idx];
              action_line.AddCondition(
                  new WinnerPlayCondition(w, by_winner[w].winning_cards),
                  wb_branches[idx][picks[idx]]);
            }
          }
          return action_line;
        },
        allow_finalist_alts);
    if (action_cands.empty() || action_cands[0].CoveredFinalBoards() <= 0) {
      return {};
    }
    if (assemble_top_k > 1 && action_cands.size() > 1) {
      FinalizeTopKLines(&action_cands, assemble_top_k, trump_suit,
                        allow_finalist_alts);
    } else if (action_cands.size() > 1) {
      action_cands.resize(1);
    }
    if (has_aug) {
      PruneAndRestrictAugmentedCandidates(&action_cands, orig_boards,
                                          trump_suit);
      if (action_cands.empty()) {
        return {};
      }
    }
    for (auto &ac : action_cands) {
      trick_nodes[m].push_back(
          std::make_shared<ConditionalLine>(std::move(ac)));
    }
  }

  int group_ufc = (!keep_all_top_k_ && matched[0].has_fc)
                      ? CountGroupUnforcedFalsecards(best_group, board_trees)
                      : 0;
  bool allow_finalist_alts = max_finalist_lines_ > 1;
  int assemble_top_k =
      (allow_finalist_alts && !keep_all_top_k_ && adversarial_rescore_ &&
       lvl <= 2)
          ? std::max(exact_top_k, stage_alt_k)
          : exact_top_k;
  std::vector<ConditionalLine> ans = AssembleBranchLines(
      trick_nodes, assemble_top_k > 1, lvl, /*min_diff_branches=*/1,
      trump_suit,
      [&](const std::vector<size_t> &picks) {
        ConditionalLine group_cl;
        for (size_t m = 0; m < best_group.size(); ++m) {
          group_cl.AddCondition(
              new TrickPlayCondition(best_group[m].ToString(true)),
              trick_nodes[m][picks[m]]);
        }
        if (group_ufc > 0) {
          group_cl.set_unforced_fc_count(
              std::min(group_cl.CoveredFinalBoards(),
                       group_cl.unforced_fc_count() + group_ufc));
        }
        return group_cl;
      },
      allow_finalist_alts);
  if (ans.empty() || ans[0].CoveredFinalBoards() <= 0) {
    return {};
  }
  if (assemble_top_k > 1 && ans.size() > 1) {
    FinalizeTopKLines(&ans, assemble_top_k, trump_suit, allow_finalist_alts);
  } else if (ans.size() > 1) {
    ans.resize(1);
  }
  int guided_cov = ans[0].CoveredFinalBoards();
  bool may_tie_trump_disc =
      lvl >= 1 && lvl <= 4 && state_depth == 0 && trump_suit != NOTRUMP &&
      prev_suit_led == trump_suit &&
      InitialTrumpDrawScore(ans[0], trump_suit) == 0;
  if (lvl >= 1 && !def_leads &&
      (guided_cov < static_cast<int>(solvable_boards.size()) ||
       may_tie_trump_disc)) {
    TryBranchOnGoodCards(min_tricks, cache_in, board_trees, solvable_boards,
                         cards_left, leader, lvl, ans, /*progress=*/nullptr,
                         /*min_covered=*/0, exact_top_k, prev_suit_led,
                         state_depth, guided_cov);
  }
  return ans;
}
