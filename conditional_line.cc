#include "conditional_line.h"
#include "glog/logging.h"
#include "interface_dds.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <tuple>

std::string WinnerPlayCondition::Describe(bool only_choice) const {
  std::string ans = only_choice ? "when " : "if ";
  ans += std::to_string(who_) + " wins";
  if (!winning_cards_.empty()) {
    ans += " the " + StrJoin(winning_cards_, ",");
  }
  return ans;
}

std::string TrickPlayCondition::Describe(bool only_choice) const {
  std::string ans = "after " + trick_;
  return ans;
}

std::string IsCardGoodCondition::Describe(bool only_choice) const {
  std::string prefix = only_choice ? "when " : "if ";
  return prefix + Card::PrintCard(card_) +
         (is_good_ ? " is good" : " is not good");
}

bool IsCardGoodCondition::Evaluate(const BoardState &bs) const {
  auto [suit, val] = Card::DecodeCard(card_);
  uint64_t mask = ~bs.played_cards();
  uint64_t opp_cards = (bs.GetHand(1) | bs.GetHand(3)) & mask;
  uint16_t opp_suit = (opp_cards >> (suit * 16)) & 0xffff;
  uint16_t higher_mask = ~((1U << (val + 1)) - 1) & 0x1fff;
  return (opp_suit & higher_mask) == 0;
}

int ConditionalLine::CoveredFinalBoards() const {
  return CoveredFinalBoardsSet().size();
}

static std::set<int> CoveredFinalBoardsSetHelper(const ConditionalLine *line_ptr, std::set<const ConditionalLine*> &visited) {
  if (!line_ptr || visited.count(line_ptr)) {
    return {};
  }
  visited.insert(line_ptr);

  if (!line_ptr->final_covered_boards().empty()) {
    return line_ptr->final_covered_boards();
  }

  std::set<int> conditional;
  for (const auto &[_condition, child_line] : line_ptr->conditions()) {
    if (child_line) {
      const auto &cb = CoveredFinalBoardsSetHelper(child_line.get(), visited);
      conditional.insert(cb.begin(), cb.end());
    }
  }
  return conditional;
}

std::set<int> ConditionalLine::CoveredFinalBoardsSet(std::set<const ConditionalLine*> visited) const {
  return CoveredFinalBoardsSetHelper(this, visited);
}

namespace {
// Extracts the cards played by declarer's side (even seats) from a concrete
// four-card trick string led by `leader`.
std::string DeclarerCardsOfTrick(const std::string &trick, int leader) {
  std::vector<std::string> cards = SplitWords(trick);
  std::string ans;
  for (size_t i = 0; i < cards.size(); ++i) {
    if ((leader + static_cast<int>(i)) % 2 == 0) {
      ans += cards[i] + " ";
    }
  }
  return ans;
}

void DeclarerPlaySignatureHelper(const ConditionalLine &line,
                                 std::map<int, std::string> &out) {
  if (!line.final_covered_boards().empty()) {
    for (int b : line.final_covered_boards()) {
      out.emplace(b, "");
    }
    return;
  }
  const GeneralizedTrick &action = line.do_first();
  for (const auto &[_cond, child] : line.conditions()) {
    if (!child) {
      continue;
    }
    std::map<int, std::string> sub;
    DeclarerPlaySignatureHelper(*child, sub);
    for (auto &[b, rest] : sub) {
      std::string prefix;
      if (!action.Empty()) {
        auto it = action.demi2.board_to_trick.find(b);
        std::string trick =
            (it != action.demi2.board_to_trick.end()) ? it->second
                                                       : action.ToString(true);
        prefix = DeclarerCardsOfTrick(trick, action.leader) + "/ ";
      }
      out[b] = prefix + rest;
    }
  }
}
}  // namespace

std::map<int, std::string> ConditionalLine::DeclarerPlaySignature() const {
  std::map<int, std::string> ans;
  DeclarerPlaySignatureHelper(*this, ans);
  return ans;
}

std::string ConditionalLine::Describe(int lvl, bool sort_branches) const {
  if (!final_covered_boards_.empty()) {
    return "HAND SUCCEEDED FOR: " + StrJoin(final_covered_boards_);
  }
  std::string ans;
  if (lvl == 0) {
    if (total_boards_ > 0) {
      double pct = (100.0 * CoveredBoards()) / total_boards_;
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(1) << pct;
      ans = "Covers " + std::to_string(CoveredBoards()) + "/" +
            std::to_string(total_boards_) + " (" + oss.str() + "%) [" +
            StrJoin(CoveredBoardsSet()) + "]\n";
    } else {
      ans = "Covers " + std::to_string(CoveredBoards()) +
            " [" + StrJoin(CoveredBoardsSet()) + "]\n";
    }
    if (has_summary()) {
      ans += summary_.ToString();
    }
  }
  if (!do_first_.Empty()) {
    ans += do_first_.ToString();
  }
  if (!conditions_.empty()) {
    struct BranchInfo {
      std::string desc;
      std::string sub_tree;
      std::string full_formatted;
    };
    std::vector<BranchInfo> formatted_branches;
    formatted_branches.reserve(conditions_.size());

    bool only_choice = (conditions_.size() == 1);
    for (const auto &[play_condition, cond_line] : conditions_) {
      BranchInfo bi;
      bi.desc = play_condition->Describe(only_choice);
      bi.sub_tree = cond_line->Describe(lvl + 1, sort_branches);

      std::string branch_str = "\n";
      for (int i = 0; i < (lvl + 1) * 2; i++) {
        branch_str += " ";
      }
      branch_str += bi.desc + ": " + bi.sub_tree;
      bi.full_formatted = branch_str;

      formatted_branches.push_back(bi);
    }

    if (sort_branches) {
      std::sort(formatted_branches.begin(), formatted_branches.end(),
                [](const BranchInfo &a, const BranchInfo &b) {
                  if (a.desc != b.desc) return a.desc < b.desc;
                  return a.sub_tree < b.sub_tree;
                });
    }

    for (const auto &fb : formatted_branches) {
      ans += fb.full_formatted;
    }
  }
  return ans;
}

// Returns true if the card patterns can cover the same card.
bool CardsAreRedundant(const std::string &c1, const std::string &c2) {
  // First check suit
  bool suit_redundant = (c1[0] == c2[0] || c1[0] == '_' || c2[0] == '_');
  // 'R' is only redundant with another discard.
  if (c1[0] == 'R' || c2[0] == 'R') {
    suit_redundant = (c1[0] == c2[0]);
  }
  if (!suit_redundant) {
    return false;
  }
  bool card_redundant = (c1[1] == c2[1] || c1[1] == '_' || c2[1] == '_');
  if (card_redundant) {
    return true;
  }
  // Have to check x / X too.
  if ((c1[1] == 'x' && isdigit(c2[1])) ||
      (c2[1] == 'x' && isdigit(c1[1]))) {
    return true;
  }
  if ((c1[1] == 'X' && !isdigit(c2[1])) ||
      (c2[1] == 'X' && !isdigit(c1[1]))) {
    return true;
  }
  return false;
}

Suit GetSuitOfCard(const std::string &c) {
  CHECK_EQ(2, c.size()) << c;
  if (c[0] == '_') {
    return NOTRUMP;
  }
  return Card::ParseSuit(c[0]);
}

#define CONSIDER_INDIVIDUAL_LOW_CARDS 0
#define CONSIDER_DISCARDS 1

std::vector<std::string> GeneralizeOppCard(const std::string &card,
                                           Suit suit_led, Suit trump) {
  VLOG(2) << "GeneralizeOppCard on " << card;
  if (card.empty()) {
    return std::vector<std::string>({card});
  }
  CHECK_GE(card.size(), 2) << card;
  std::vector<std::string> cards;

  Suit card_suit = GetSuitOfCard(card);
  bool is_discard =
      (suit_led != NOTRUMP && card_suit != suit_led && card_suit != trump);

  if (!is_discard) {
#if CONSIDER_INDIVIDUAL_LOW_CARDS
    cards.push_back(card);
#endif

    std::string honor_or_not = card;
    if (isdigit(honor_or_not[1])) {
      honor_or_not[1] = 'x';
    } else {
      honor_or_not[1] = 'X';
#if !CONSIDER_INDIVIDUAL_LOW_CARDS
      // Only record raw card if it's an honor
      cards.push_back(card);
#endif
    }
    cards.push_back(honor_or_not);

    std::string suited = card;
    suited[1] = '_';
    cards.push_back(suited);
  } else if (CONSIDER_DISCARDS) {
    cards.push_back("R_");  // discaRd
  }

  cards.push_back("__");
  return cards;
}

bool GeneralizesTo(const std::string& actual_card,
                   const std::string &gen_card,
                   Suit suit_led, Suit trump) {
  if (actual_card.empty()) {
    return gen_card.empty();
  }
  if (gen_card.size() != 2 || actual_card.size() < 2) {
    return false;
  }
  char actual_suit = actual_card[0];
  char actual_rank = actual_card[1];
  char gen_suit = gen_card[0];
  char gen_rank = gen_card[1];
  if (gen_suit == '_' && gen_rank == '_') {
    return true;
  }
  Suit card_suit = GetSuitOfCard(actual_card);
  bool is_discard =
      (suit_led != NOTRUMP && card_suit != suit_led && card_suit != trump);
  if (!is_discard && gen_suit == actual_suit) {
    if (gen_rank == '_') return true;
    if (isdigit(actual_rank)) {
      return gen_rank == 'x';
    } else {
      return gen_rank == 'X' || gen_rank == actual_rank;
    }
  }
  if (gen_suit == 'R' && gen_rank == '_') {
    return CONSIDER_DISCARDS && is_discard;
  }
  return false;
}

// If 1, it's faster, but doesn't seem to work as well.
#define TRY_GENERAL_CARDS_FIRST 0

using std::pair;
using std::set;
using std::make_pair;
namespace {
struct VecPtrLess {
  bool operator()(const std::vector<int> *a,
                  const std::vector<int> *b) const {
    return *a < *b;
  }
};
}  // namespace

GeneralizedPlaysVector GeneralizedDemiPlay::GetGeneralizedPlays(
    int leader, Suit known_suit_led, unsigned int top) {
  VLOG(2) << "GetGeneralizedPlays";
  std::map<pair<std::string, std::string>, std::vector<int>> generalized;
  if (!saved_tricks_.empty()) {
    for (const auto &[opp_choice, me_map] : saved_tricks_) {
      for (const auto &[my_card, board_map] : me_map) {
        auto &dest = generalized[make_pair(opp_choice, my_card)];
        for (const auto &[b, _trick] : board_map) {
          dest.push_back(b);
        }
      }
    }
  } else {
    for (const auto &[opp_card, card_to_boards] : opp_to_card_to_boards_) {
      for (const auto &[my_card, boards] : card_to_boards) {
        Suit suit_led = known_suit_led != NOTRUMP ? known_suit_led
                        : (leader % 2 == 0
                           ? GetSuitOfCard(my_card)
                           : GetSuitOfCard(opp_card));
        auto opp_card_choices =
            GeneralizeOppCard(opp_card, suit_led, trump_suit_);
        for (const auto &choice : opp_card_choices) {
          auto &dest = generalized[make_pair(choice, my_card)];
          dest.insert(dest.end(), boards.begin(), boards.end());
        }
      }
    }
  }
  for (auto &[key, vec] : generalized) {
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
  }
  using GenMapIter =
      std::map<pair<std::string, std::string>, std::vector<int>>::const_iterator;
  std::multimap<int, GenMapIter> sorted;
#if TRY_GENERAL_CARDS_FIRST
  for (auto it = generalized.begin(); it != generalized.end(); ++it) {
    sorted.insert(make_pair(static_cast<int>(it->second.size()), it));
  }
#else
  for (auto it = generalized.rbegin(); it != generalized.rend(); ++it) {
    auto fwd_it = std::prev(it.base());
    sorted.insert(make_pair(static_cast<int>(it->second.size()), fwd_it));
  }
#endif

  GeneralizedPlaysVector ans;

  std::map<const std::vector<int> *,
           std::vector<std::pair<std::string, std::string>>, VecPtrLess>
      seen_plays;
  int prev_count = -1;
  for (auto it = sorted.rbegin(); it != sorted.rend(); it++) {
    const auto &[_count, map_it] = *it;
    if (ans.size() >= top) {
      break;
    }
    if (_count != prev_count) {
      seen_plays.clear();
      prev_count = _count;
    }
    const auto &opp_me = map_it->first;
    const std::vector<int> &boards_vec = map_it->second;
    auto &seen_for_boards = seen_plays[&boards_vec];
    bool redundant = false;
    for (const auto &[opp, me] : seen_for_boards) {
      if (me == opp_me.second && CardsAreRedundant(opp, opp_me.first)) {
        redundant = true;
        break;
      }
    }
    if (redundant) {
      continue;
    }
    seen_for_boards.push_back(opp_me);
    std::set<int> boards(boards_vec.begin(), boards_vec.end());
    GeneralizedDemiStruct gds(opp_me.first, opp_me.second, std::move(boards));
    if (!saved_tricks_.empty()) {
      auto saved_opp_it = saved_tricks_.find(opp_me.first);
      if (saved_opp_it != saved_tricks_.end()) {
        auto saved_me_it = saved_opp_it->second.find(opp_me.second);
        if (saved_me_it != saved_opp_it->second.end()) {
          for (int b : boards_vec) {
            auto b_it = saved_me_it->second.find(b);
            if (b_it != saved_me_it->second.end()) {
              gds.board_to_trick[b] = b_it->second;
            }
          }
        }
      }
    }
    ans.push_back(std::move(gds));
  }
  return ans;
}

void GeneralizedDemiPlay::RecordOptimalPlay(const std::string &opp_card,
                                            const std::string &my_card,
                                            int board_num,
                                            const std::string *trick) {
  opp_to_card_to_boards_[opp_card][my_card].insert(board_num);
  if (trick) {
    CHECK_NE((*trick)[0], '(') << *trick;
    Suit suit_led = Card::ParseSuit((*trick)[0]);
    auto opp_card_choices = GeneralizeOppCard(opp_card, suit_led, trump_suit_);
    for (const auto &opp_choice : opp_card_choices) {
      saved_tricks_[opp_choice][my_card][board_num] = *trick;
    }
  }
}

std::string GeneralizedDemiPlay::PrintGeneralizedPlays(
    const GeneralizedPlaysVector &plays) {
  std::string ans;
  for (const auto &play : plays) {
    std::string play_str;
    const std::set<int> &boards = play.boards;
    const std::string &opp = play.opp_card;
    const std::string &me = play.my_card;
    if (!opp.empty()) {
      play_str += "(" + opp + ") ";
    }
    play_str += me + " [" + std::to_string(boards.size()) + "]\n";
    ans += play_str;
  }
  return ans;
}


void GeneralizedTrickFinder::RecordOptimalTrick(
    int leader, const std::string &trick, int board) {
  if (leader_ < 0) {
    leader_ = leader;
  }
  CHECK_EQ(leader, leader_);
  std::vector<std::string> cards = SplitWords(trick);
  CHECK_EQ(4, cards.size()) << trick;
  tricks_.push_back(make_pair(cards, board));
  raw_tricks_.push_back(trick);
}

GeneralizedTricksVector GeneralizedTrickFinder::GetGeneralizedTricks(
    unsigned int top) {
  GeneralizedDemiPlay demi1(trump_suit_);
  std::map<pair<std::string, std::string>, std::vector<uint>> d1_to_tricks;
  for (uint i = 0; i < tricks_.size(); i++) {
    const auto &[cards, board] = tricks_[i];
    const std::string &opp1 = (leader_ % 2 == 0) ? cards[1] : cards[0];
    const std::string &my1 = (leader_ % 2 == 0) ? cards[0] : cards[1];
    demi1.RecordOptimalPlay(opp1, my1, board, nullptr);
    Suit suit_led = GetSuitOfCard(cards[0]);
    auto opp1_choices = GeneralizeOppCard(opp1, suit_led, trump_suit_);
    for (const auto &choice : opp1_choices) {
      d1_to_tricks[make_pair(choice, my1)].push_back(i);
    }
  }

  GeneralizedPlaysVector demi1_plays =
      demi1.GetGeneralizedPlays(leader_, NOTRUMP, top * 2);
  std::vector<GeneralizedPlaysVector> all_demi2_plays(demi1_plays.size());
  std::multimap<int, std::pair<size_t, size_t>> sorted;

  int max_d1_size =
      demi1_plays.empty() ? 0 : static_cast<int>(demi1_plays[0].boards.size());
  std::vector<int> size_hist(max_d1_size + 1, 0);
  int prev_max_possible = max_d1_size;
  int pairs_strictly_above = 0;

  for (size_t d1_idx = 0; d1_idx < demi1_plays.size(); ++d1_idx) {
    const GeneralizedDemiStruct &d1 = demi1_plays[d1_idx];
    int max_possible = static_cast<int>(d1.boards.size());
    while (prev_max_possible > max_possible) {
      pairs_strictly_above += size_hist[prev_max_possible];
      --prev_max_possible;
    }
    if (pairs_strictly_above >= static_cast<int>(top)) {
      break;
    }

    GeneralizedDemiPlay demi2(trump_suit_);
    Suit d1_suit_led = (leader_ % 2 == 0) ? GetSuitOfCard(d1.my_card)
                                          : GetSuitOfCard(d1.opp_card);
    auto tricks_it = d1_to_tricks.find(make_pair(d1.opp_card, d1.my_card));
    if (tricks_it != d1_to_tricks.end()) {
      for (uint i : tricks_it->second) {
        const auto &[cards, board] = tricks_[i];
        const std::string *trick_ptr =
            (d1_suit_led == NOTRUMP) ? &raw_tricks_[i] : nullptr;
        if (leader_ % 2 == 0) {
          demi2.RecordOptimalPlay(cards[3], cards[2], board, trick_ptr);
        } else {
          demi2.RecordOptimalPlay(cards[2], cards[3], board, trick_ptr);
        }
      }
    }

    all_demi2_plays[d1_idx] =
        demi2.GetGeneralizedPlays(leader_, d1_suit_led, top * 2);
    const auto &demi2_plays = all_demi2_plays[d1_idx];
    for (size_t d2_idx = 0; d2_idx < demi2_plays.size(); ++d2_idx) {
      int inter_count = static_cast<int>(demi2_plays[d2_idx].boards.size());
      if (inter_count > 0) {
        sorted.insert(make_pair(inter_count, make_pair(d1_idx, d2_idx)));
        if (inter_count <= max_d1_size) {
          size_hist[inter_count]++;
        }
      }
    }
  }
  GeneralizedTricksVector ans;
  ans.reserve(std::min<size_t>(top, sorted.size()));
  std::map<std::tuple<std::set<int>, std::string, std::string, std::string>,
           std::vector<size_t>>
      seen_d1_for_trick;
  int prev_inter_count = -1;
  for (auto it = sorted.rbegin(); it != sorted.rend(); it++) {
    if (ans.size() >= top) {
      break;
    }
    if (it->first != prev_inter_count) {
      seen_d1_for_trick.clear();
      prev_inter_count = it->first;
    }
    const auto &[d1_idx, d2_idx] = it->second;
    const auto &d1 = demi1_plays[d1_idx];
    GeneralizedDemiStruct d2 = all_demi2_plays[d1_idx][d2_idx];
    if (d2.board_to_trick.empty()) {
      auto tricks_it = d1_to_tricks.find(make_pair(d1.opp_card, d1.my_card));
      if (tricks_it != d1_to_tricks.end()) {
        for (uint i : tricks_it->second) {
          const auto &[cards, board] = tricks_[i];
          const std::string &opp2 = (leader_ % 2 == 0) ? cards[3] : cards[2];
          const std::string &my2 = (leader_ % 2 == 0) ? cards[2] : cards[3];
          if (my2 != d2.my_card) {
            continue;
          }
          Suit suit_led = Card::ParseSuit(raw_tricks_[i][0]);
          if (GeneralizesTo(opp2, d2.opp_card, suit_led, trump_suit_)) {
            d2.board_to_trick[board] = raw_tricks_[i];
          }
        }
      }
    }
    if (leader_ % 2 != 0) {
      auto &seen_indices = seen_d1_for_trick[std::make_tuple(
          d2.boards, d1.my_card, d2.my_card, d2.opp_card)];
      bool replaced_or_redundant = false;
      for (size_t ans_idx : seen_indices) {
        const std::string &prev_opp1 = ans[ans_idx].demi1.opp_card;
        if (prev_opp1[0] == d1.opp_card[0] &&
            CardsAreRedundant(prev_opp1, d1.opp_card)) {
          if (d1.boards.size() > ans[ans_idx].demi1.boards.size()) {
            ans[ans_idx] = GeneralizedTrick(d1, d2, d2.boards, leader_);
          }
          replaced_or_redundant = true;
          break;
        }
      }
      if (replaced_or_redundant) {
        continue;
      }
      seen_indices.push_back(ans.size());
    }
    ans.emplace_back(d1, d2, d2.boards, leader_);
  }
  return ans;
}

std::string GeneralizedTrick::ToString(bool compact) const {
  std::string play_str;

  const std::string &opp1 = demi1.opp_card;
  const std::string &me1 = demi1.my_card;
  CHECK(!opp1.empty());
  if (leader % 2 == 0) {
    play_str += me1 + " (" + opp1 + ")";
  } else {
    play_str += "(" + opp1 + ") " + me1;
  }

  const std::string &opp2 = demi2.opp_card;
  const std::string &me2 = demi2.my_card;
  CHECK(!opp2.empty()) << play_str;
  if (leader % 2 == 0) {
    play_str += " " + me2 + " (" + opp2 + ")";
  } else {
    play_str += " (" + opp2 + ") " + me2;
  }

  if (!compact) {
    // play_str += " [" + std::to_string(boards.size()) + "]";
    play_str += " [" + StrJoin(boards, ",") + "]";
    int num_tricks = 0;
    for (int b : boards) {
      auto it = demi2.board_to_trick.find(b);
      play_str += " / " + (it != demi2.board_to_trick.end() ? it->second : "");
      if (++num_tricks >= 5) {
        break;
      }
    }
  }
  return play_str;
}

std::string GeneralizedTrickFinder::PrintGeneralizedTricks(
    const GeneralizedTricksVector &plays) {
  std::string ans;
  for (const GeneralizedTrick &play : plays) {
    std::string play_str = play.ToString();
    ans += play_str + "\n";
  }
  return ans;

}

namespace {
bool AnyIntersection(const set<int> &b1, const set<int> &b2) {
  if (b1.empty() || b2.empty()) {
    return false;
  }
  if (*b1.begin() > *b2.rbegin() ||
      *b2.begin() > *b1.rbegin()) {
    return false;
  }

  auto b1_it = b1.begin();
  auto b2_it = b2.begin();
  while (b1_it != b1.end() && b2_it != b2.end()) {
    if (*b1_it == *b2_it) {
      return true;
    }
    if (*b1_it < *b2_it) {
      ++b1_it;
    } else {
      ++b2_it;
    }
  }
  return false;
}

int PatternSpecificity(const std::string &pattern) {
  if (pattern.size() != 2) return 0;
  if (pattern == "__") return 1;
  if (pattern == "R_") return 2;
  if (pattern[1] == '_') return 3;
  if (pattern[1] == 'x' || pattern[1] == 'X') return 4;
  return 5;
}

bool HasActualCardMatchingPattern(const GeneralizedTrick &general_trick,
                                  const std::string &specific_pattern,
                                  int opp_pos) {
  if (general_trick.demi2.board_to_trick.empty()) {
    return true;
  }
  for (int b : general_trick.boards) {
    auto it = general_trick.demi2.board_to_trick.find(b);
    if (it == general_trick.demi2.board_to_trick.end()) {
      return true;
    }
    std::vector<std::string> cards = SplitWords(it->second);
    if (static_cast<int>(cards.size()) <= opp_pos) {
      return true;
    }
    Suit suit_led = GetSuitOfCard(cards[0]);
    if (GeneralizesTo(cards[opp_pos], specific_pattern, suit_led, NOTRUMP)) {
      return true;
    }
  }
  return false;
}

bool OppSlotDistinguishable(const GeneralizedTrick &trick1,
                            const GeneralizedTrick &trick2,
                            const std::string &opp1,
                            const std::string &opp2,
                            int opp_pos) {
  if (!CardsAreRedundant(opp1, opp2)) {
    return true;
  }
  if (trick1.leader % 2 == 0) {
    return false;
  }
  if (opp1[0] == opp2[0] &&
      ((opp1[1] == 'X' && opp2[1] == 'x') ||
       (opp1[1] == 'x' && opp2[1] == 'X'))) {
    return true;
  }
  if (opp1 == opp2 ||
      trick1.demi2.board_to_trick.empty() ||
      trick2.demi2.board_to_trick.empty()) {
    return false;
  }
  int spec1 = PatternSpecificity(opp1);
  int spec2 = PatternSpecificity(opp2);
  if (spec1 < spec2) {
    return !HasActualCardMatchingPattern(trick1, opp2, opp_pos);
  }
  if (spec2 < spec1) {
    return !HasActualCardMatchingPattern(trick2, opp1, opp_pos);
  }
  return false;
}

bool AreCardsDistinguishable(const GeneralizedTrick& trick1,
                             const GeneralizedTrick& trick2) {
  bool any_defensive_diff = false;
  if (trick1.leader % 2 == 0) {
    if (trick1.demi1.my_card != trick2.demi1.my_card) {
      return false;
    }
    if (OppSlotDistinguishable(trick1, trick2, trick1.demi1.opp_card,
                               trick2.demi1.opp_card, 1)) {
      any_defensive_diff = true;
    }
    if (!any_defensive_diff && trick1.demi2.my_card != trick2.demi2.my_card) {
      return false;
    }
    if (!any_defensive_diff &&
        OppSlotDistinguishable(trick1, trick2, trick1.demi2.opp_card,
                               trick2.demi2.opp_card, 3)) {
      any_defensive_diff = true;
    }
  } else {
    if (OppSlotDistinguishable(trick1, trick2, trick1.demi1.opp_card,
                               trick2.demi1.opp_card, 0)) {
      any_defensive_diff = true;
    }
    if (!any_defensive_diff && trick1.demi1.my_card != trick2.demi1.my_card) {
      return false;
    }
    if (!any_defensive_diff &&
        OppSlotDistinguishable(trick1, trick2, trick1.demi2.opp_card,
                               trick2.demi2.opp_card, 2)) {
      any_defensive_diff = true;
    }
    if (!any_defensive_diff && trick1.demi2.my_card != trick2.demi2.my_card) {
      return false;
    }
  }
  return any_defensive_diff;
}
}  // namespace

// Returns true if declarer can decide to play between them based
// on the opponents cards.
bool IsDistinguishable(const GeneralizedTrick& trick1,
                       const GeneralizedTrick& trick2) {
  if (!AreCardsDistinguishable(trick1, trick2)) {
    return false;
  }
  return !AnyIntersection(trick1.boards, trick2.boards);
}

std::vector<GeneralizedTrick> MakeGroupStartingWith(
    const std::vector<GeneralizedTrick> &input, int start) {
  std::vector<int> indices;
  for (uint i = start; i < input.size(); i++) {
    const GeneralizedTrick &trick = input[i];
    bool any_indistinguishable = false;
    for (int idx : indices) {
      if (!IsDistinguishable(input[idx], trick)) {
        any_indistinguishable = true;
        break;
      }
    }
    if (!any_indistinguishable) {
      indices.push_back(i);
    }
  }
  std::vector<GeneralizedTrick> ans;
  ans.reserve(indices.size());
  for (int idx : indices) {
    ans.push_back(input[idx]);
  }
  return ans;
}

std::vector<std::vector<GeneralizedTrick>> GroupTricks(
    const std::vector<GeneralizedTrick> &input) {
  VLOG(1) << "GroupTricks on " << input.size();
  for (const GeneralizedTrick &trick : input) {
    VLOG(1) << "  " << trick.ToString(false);
  }
  std::vector<std::vector<GeneralizedTrick>> ans;
  if (input.empty()) {
    return ans;
  }

  int max_board = 0;
  for (const GeneralizedTrick &trick : input) {
    if (!trick.boards.empty()) {
      max_board = std::max(max_board, *trick.boards.rbegin());
    }
  }
  size_t num_words = (max_board / 64) + 1;
  std::vector<std::vector<uint64_t>> masks(
      input.size(), std::vector<uint64_t>(num_words, 0));
  for (size_t i = 0; i < input.size(); ++i) {
    for (int b : input[i].boards) {
      if (b >= 0) {
        masks[i][b / 64] |= (1ULL << (b % 64));
      }
    }
  }

  // A trick is redundant only if a trick with the same declarer cards and
  // the same board set has already been grouped, i.e. it is merely another
  // opponent-pattern spelling of the same play. Two different plays that
  // happen to be feasible on the same boards (say cashing an idle trump
  // versus taking a finesse) can still lead to different final coverage and
  // must each get their own group.
  using GroupKey = std::tuple<std::vector<uint64_t>, std::string, std::string>;
  auto key_of = [&](int idx) {
    return GroupKey(masks[idx], input[idx].demi1.my_card,
                    input[idx].demi2.my_card);
  };
  std::set<GroupKey> boards_grouped;
  std::vector<int> group_indices;
  std::vector<uint64_t> group_mask(num_words, 0);

  auto try_add = [&](uint j) {
    bool board_overlap = false;
    for (size_t w = 0; w < num_words; ++w) {
      if ((group_mask[w] & masks[j][w]) != 0) {
        board_overlap = true;
        break;
      }
    }
    if (board_overlap) {
      return;
    }
    const GeneralizedTrick &trick = input[j];
    for (int idx : group_indices) {
      if (!AreCardsDistinguishable(input[idx], trick)) {
        return;
      }
    }
    group_indices.push_back(j);
    for (size_t w = 0; w < num_words; ++w) {
      group_mask[w] |= masks[j][w];
    }
  };

  for (uint i = 0; i < input.size(); i++) {
    group_indices.clear();
    group_indices.push_back(i);
    group_mask = masks[i];

    if (input[i].leader % 2 != 0 && !input[i].demi2.board_to_trick.empty()) {
      for (uint j = 0; j < i; j++) {
        try_add(j);
      }
    }
    for (uint j = i + 1; j < input.size(); j++) {
      try_add(j);
    }
    bool any_new_groupings = false;
    for (int idx : group_indices) {
      if (boards_grouped.count(key_of(idx)) == 0) {
        any_new_groupings = true;
        break;
      }
    }

    if (any_new_groupings) {
      std::vector<GeneralizedTrick> group;
      group.reserve(group_indices.size());
      for (int idx : group_indices) {
        boards_grouped.insert(key_of(idx));
        group.push_back(input[idx]);
      }
      ans.push_back(std::move(group));
    }
  }
  VLOG(1) << "Got groups " << ans.size();
  return ans;
}

namespace {

CardType ChooseValidCard(const BoardState &bs, const std::string &card_str) {
  CardType target = Card::ParseStringCard(card_str);
  std::vector<CardType> valid_cards = bs.ValidCardsToPlay();
  CHECK(!valid_cards.empty());
  for (CardType c : valid_cards) {
    if (c == target) {
      return c;
    }
  }
  CardType best_card = valid_cards[0];
  int64_t min_d =
      (target > best_card) ? (target - best_card) : (best_card - target);
  for (size_t i = 1; i < valid_cards.size(); ++i) {
    CardType c = valid_cards[i];
    int64_t d = (target > c) ? (target - c) : (c - target);
    if (d < min_d) {
      min_d = d;
      best_card = c;
    }
  }
  return best_card;
}

std::string GetPatternAtPos(const GeneralizedTrick &gt, int pos) {
  bool decl_leads = (gt.leader % 2 == 0);
  if (decl_leads) {
    switch (pos) {
      case 0: return gt.demi1.my_card;
      case 1: return gt.demi1.opp_card;
      case 2: return gt.demi2.my_card;
      case 3: return gt.demi2.opp_card;
    }
  } else {
    switch (pos) {
      case 0: return gt.demi1.opp_card;
      case 1: return gt.demi1.my_card;
      case 2: return gt.demi2.opp_card;
      case 3: return gt.demi2.my_card;
    }
  }
  return "";
}

std::vector<const ConditionalLine *> FilterCandidatesByDefenderCard(
    const std::vector<const ConditionalLine *> &candidates,
    const std::string &def_card_str, int pos, Suit suit_led, Suit trump) {
  std::vector<const ConditionalLine *> matching;
  for (const ConditionalLine *cand : candidates) {
    std::string pattern = GetPatternAtPos(cand->do_first(), pos);
    if (GeneralizesTo(def_card_str, pattern, suit_led, trump)) {
      matching.push_back(cand);
    }
  }
  return matching;
}

const ConditionalLine *SelectMostSpecificCandidate(
    const std::vector<const ConditionalLine *> &candidates, int up_to_pos) {
  CHECK(!candidates.empty());
  const ConditionalLine *best = candidates[0];
  auto score_of = [up_to_pos](const ConditionalLine *cand) {
    const GeneralizedTrick &gt = cand->do_first();
    int d1_pos = (gt.leader % 2 == 0) ? 1 : 0;
    int d2_pos = (gt.leader % 2 == 0) ? 3 : 2;
    int spec1 = (d1_pos < up_to_pos)
                    ? PatternSpecificity(GetPatternAtPos(gt, d1_pos))
                    : 0;
    int spec2 = (d2_pos < up_to_pos)
                    ? PatternSpecificity(GetPatternAtPos(gt, d2_pos))
                    : 0;
    return std::make_tuple(std::max(spec1, spec2), spec1 + spec2, spec2, spec1);
  };
  auto best_score = score_of(best);
  for (size_t i = 1; i < candidates.size(); ++i) {
    auto s = score_of(candidates[i]);
    if (s > best_score) {
      best_score = s;
      best = candidates[i];
    }
  }
  return best;
}

std::vector<std::vector<std::string>> ExtractTrickLines(
    const OptimalLines &opt_lines) {
  std::vector<std::vector<std::string>> res;
  res.reserve(opt_lines.size());
  for (const auto &line : opt_lines) {
    std::vector<std::string> words = SplitWords(line);
    if (!words.empty()) {
      res.push_back(std::move(words));
    }
  }
  return res;
}

int ResultsOnHandHelper(const ConditionalLine *line, BoardState copy) {
  if (!line || copy.Finished()) {
    return copy.tricks_won_even();
  }

  // Terminal leaf node
  if (line->do_first().Empty() && line->conditions().empty()) {
    return copy.tricks_won_even();
  }

  // Collect candidate action nodes for this trick.
  std::vector<const ConditionalLine *> candidates;
  if (!line->do_first().Empty()) {
    candidates.push_back(line);
  } else {
    for (const auto &[cond, child_line] : line->conditions()) {
      if (!child_line) continue;
      if (auto *good_cond =
              dynamic_cast<const IsCardGoodCondition *>(cond.get())) {
        if (good_cond->Evaluate(copy) == good_cond->is_good()) {
          return ResultsOnHandHelper(child_line.get(), copy);
        }
      }
    }
    for (const auto &[_cond, child_line] : line->conditions()) {
      if (child_line && !child_line->do_first().Empty()) {
        candidates.push_back(child_line.get());
      }
    }
    if (candidates.empty()) {
      if (!line->conditions().empty() && line->conditions()[0].second) {
        return ResultsOnHandHelper(line->conditions()[0].second.get(), copy);
      }
      return copy.tricks_won_even();
    }
  }

  std::vector<std::vector<std::string>> cur_opt_lines;
  int start_pos = copy.CardsPlayedThisTrick();
  for (int pre_pos = 0; pre_pos < start_pos; ++pre_pos) {
    int pre_p = (copy.cur_trick_leader() + pre_pos) % 4;
    CardType pre_card = copy.trick_cards_played()[pre_pos];
    std::string pre_str = Card::PrintCard(pre_card);
    if (pre_p % 2 == 0) {
      if (candidates.size() > 1) {
        std::vector<const ConditionalLine *> matching;
        for (const ConditionalLine *cand : candidates) {
          if (GetPatternAtPos(cand->do_first(), pre_pos) == pre_str) {
            matching.push_back(cand);
          }
        }
        if (!matching.empty()) {
          candidates = std::move(matching);
        }
      }
    } else {
      Suit suit_led = Card::DecodeCard(copy.trick_cards_played()[0]).first;
      if (candidates.size() > 1) {
        std::vector<const ConditionalLine *> matching =
            FilterCandidatesByDefenderCard(
                candidates, pre_str, pre_pos, suit_led, copy.GetTrump());
        if (!matching.empty()) {
          candidates = std::move(matching);
        } else {
          candidates = {SelectMostSpecificCandidate(candidates, pre_pos)};
        }
      }
    }
  }
  for (int pos = start_pos; pos < 4; ++pos) {
    if (copy.Finished()) break;
    int p = copy.CurPlayer();
    if (p % 2 == 0) {
      // Declarer side: play the card required by the most specific matching
      // candidate given the defender cards seen so far.
      const ConditionalLine *best_cand =
          SelectMostSpecificCandidate(candidates, pos);
      std::string decl_card_str = GetPatternAtPos(best_cand->do_first(), pos);
      CardType chosen_card = ChooseValidCard(copy, decl_card_str);
      std::string chosen_str = Card::PrintCard(chosen_card);
      copy.PlayCard(p, chosen_card);

      // Keep walking down the optimal play tree if chosen_str matches;
      // otherwise clear cur_opt_lines so FindOptimalPlayTree is called next.
      std::vector<std::vector<std::string>> next_opt_lines;
      for (const auto &seq : cur_opt_lines) {
        if (!seq.empty() && seq[0] == chosen_str) {
          next_opt_lines.emplace_back(seq.begin() + 1, seq.end());
        }
      }
      cur_opt_lines = std::move(next_opt_lines);

      if (candidates.size() > 1) {
        std::vector<const ConditionalLine *> matching;
        for (const ConditionalLine *cand : candidates) {
          if (GetPatternAtPos(cand->do_first(), pos) == decl_card_str) {
            matching.push_back(cand);
          }
        }
        if (!matching.empty()) {
          candidates = std::move(matching);
        }
      }
    } else {
      // Defender side: query DDS OptimalPlayTree when needed.
      if (cur_opt_lines.empty()) {
        auto tree = OptimalPlayTree::FindOptimalPlayTree(copy, 0);
        int pre = copy.CardsPlayedThisTrick();
        for (const auto &line : tree->GetLines()) {
          std::vector<std::string> words = SplitWords(line);
          if (static_cast<int>(words.size()) > pre) {
            cur_opt_lines.emplace_back(words.begin() + pre, words.end());
          }
        }
      }
      CHECK(!cur_opt_lines.empty());
      CHECK(!cur_opt_lines[0].empty());
      std::string def_card_str = cur_opt_lines[0][0];
      CardType def_card = Card::ParseStringCard(def_card_str);
      Suit suit_led =
          (pos == 0) ? Card::DecodeCard(def_card).first : copy.SuitLed();
      copy.PlayCard(p, def_card);

      std::vector<std::vector<std::string>> next_opt_lines;
      for (const auto &seq : cur_opt_lines) {
        if (!seq.empty() && seq[0] == def_card_str) {
          next_opt_lines.emplace_back(seq.begin() + 1, seq.end());
        }
      }
      cur_opt_lines = std::move(next_opt_lines);

      if (candidates.size() > 1) {
        std::vector<const ConditionalLine *> matching =
            FilterCandidatesByDefenderCard(
                candidates, def_card_str, pos, suit_led, copy.GetTrump());
        if (!matching.empty()) {
          candidates = std::move(matching);
        } else {
          candidates = {SelectMostSpecificCandidate(candidates, pos)};
        }
      }
    }
  }

  const ConditionalLine *chosen_node =
      SelectMostSpecificCandidate(candidates, 4);
  int winner = copy.CurPlayer();
  for (const auto &[cond, child_line] : chosen_node->conditions()) {
    if (!child_line) continue;
    auto *winner_cond = dynamic_cast<const WinnerPlayCondition *>(cond.get());
    if (winner_cond && winner_cond->who() == winner) {
      return ResultsOnHandHelper(child_line.get(), copy);
    }
  }

  return copy.tricks_won_even();
}

}  // namespace

int ConditionalLine::ResultsOnHand(const BoardState &bs) const {
  return ResultsOnHandHelper(this, bs);
}

int ConditionalLine::ResultsOnHand(const PairedDeal &deal) const {
  if (!has_sample_board_) {
    return 0;
  }
  BoardState bs = sample_board_;
  std::vector<uint64_t> hands(4);
  hands[0] = sample_board_.GetHand(0);
  hands[1] = deal.hand1;
  hands[2] = sample_board_.GetHand(2);
  hands[3] = deal.hand2;
  bs.SetHands(hands);
  for (int i = 0; i < sample_board_.CardsPlayedThisTrick(); ++i) {
    CardType c = sample_board_.trick_cards_played()[i];
    if ((bs.GetHand(bs.CurPlayer()) & c) == 0) {
      return 0;
    }
    bs.PlayCard(bs.CurPlayer(), c);
  }
  return ResultsOnHand(bs);
}

ConditionalLine::BoardPlayTrace ConditionalLine::TraceOnBoard(
    const BoardState &bs, int board_id) const {
  BoardPlayTrace trace;
  trace.board_id = board_id;
  BoardState copy = bs;
  const ConditionalLine *cur_line = this;

  while (!copy.Finished() && cur_line != nullptr) {
    while (cur_line && cur_line->do_first().Empty()) {
      const ConditionalLine *good_branch = nullptr;
      for (const auto &[cond, child_line] : cur_line->conditions()) {
        if (!child_line) continue;
        if (auto *good_cond =
                dynamic_cast<const IsCardGoodCondition *>(cond.get())) {
          if (good_cond->Evaluate(copy) == good_cond->is_good()) {
            good_branch = child_line.get();
            break;
          }
        }
      }
      if (good_branch) {
        cur_line = good_branch;
        continue;
      }
      if (cur_line->conditions().size() == 1 &&
          cur_line->conditions()[0].second) {
        cur_line = cur_line->conditions()[0].second.get();
        continue;
      }
      break;
    }
    if (!cur_line ||
        (cur_line->do_first().Empty() && cur_line->conditions().empty())) {
      break;
    }

    std::vector<const ConditionalLine *> candidates;
    if (!cur_line->do_first().Empty()) {
      candidates.push_back(cur_line);
    } else {
      for (const auto &[_cond, child_line] : cur_line->conditions()) {
        if (child_line && !child_line->do_first().Empty()) {
          candidates.push_back(child_line.get());
        }
      }
      while (candidates.empty() && cur_line &&
             !cur_line->conditions().empty() &&
             cur_line->conditions()[0].second) {
        cur_line = cur_line->conditions()[0].second.get();
        if (!cur_line->do_first().Empty()) {
          candidates.push_back(cur_line);
        } else {
          for (const auto &[_cond, child_line] : cur_line->conditions()) {
            if (child_line && !child_line->do_first().Empty()) {
              candidates.push_back(child_line.get());
            }
          }
        }
      }
    }

    if (candidates.empty()) {
      break;
    }

    int trick_leader = copy.cur_trick_leader();
    int start_pos = copy.CardsPlayedThisTrick();
    std::vector<std::string> played_cards;
    played_cards.reserve(4);
    std::string my1_str;
    std::string my2_str;
    bool diverged_during_trick = false;
    int unmatched_seat = -1;
    std::string unmatched_card;
    std::vector<std::vector<std::string>> cur_opt_lines;

    for (int pre_pos = 0; pre_pos < start_pos; ++pre_pos) {
      int pre_p = (trick_leader + pre_pos) % 4;
      CardType pre_card = copy.trick_cards_played()[pre_pos];
      std::string pre_str = Card::PrintCard(pre_card);
      played_cards.push_back(pre_str);
      if (pre_p % 2 == 0) {
        if (pre_pos < 2) {
          my1_str = pre_str;
        } else {
          my2_str = pre_str;
        }
        if (candidates.size() > 1) {
          std::vector<const ConditionalLine *> matching;
          for (const ConditionalLine *cand : candidates) {
            if (GetPatternAtPos(cand->do_first(), pre_pos) == pre_str) {
              matching.push_back(cand);
            }
          }
          if (!matching.empty()) {
            candidates = std::move(matching);
          }
        }
      } else {
        Suit suit_led = Card::DecodeCard(copy.trick_cards_played()[0]).first;
        std::vector<const ConditionalLine *> matching =
            FilterCandidatesByDefenderCard(
                candidates, pre_str, pre_pos, suit_led, copy.GetTrump());
        if (!matching.empty()) {
          candidates = std::move(matching);
        } else {
          if (!diverged_during_trick) {
            unmatched_seat = pre_p;
            unmatched_card = pre_str;
          }
          diverged_during_trick = true;
          candidates = {SelectMostSpecificCandidate(candidates, pre_pos)};
        }
      }
    }

    for (int pos = start_pos; pos < 4; ++pos) {
      if (copy.Finished()) break;
      int p = copy.CurPlayer();
      if (p % 2 == 0) {
        const ConditionalLine *best_cand =
            SelectMostSpecificCandidate(candidates, pos);
        std::string decl_card_str = GetPatternAtPos(best_cand->do_first(), pos);
        CardType chosen_card = ChooseValidCard(copy, decl_card_str);
        std::string chosen_str = Card::PrintCard(chosen_card);
        copy.PlayCard(p, chosen_card);
        played_cards.push_back(chosen_str);
        if (pos < 2) {
          my1_str = chosen_str;
        } else {
          my2_str = chosen_str;
        }

        std::vector<std::vector<std::string>> next_opt_lines;
        for (const auto &seq : cur_opt_lines) {
          if (!seq.empty() && seq[0] == chosen_str) {
            next_opt_lines.emplace_back(seq.begin() + 1, seq.end());
          }
        }
        cur_opt_lines = std::move(next_opt_lines);

        if (candidates.size() > 1) {
          std::vector<const ConditionalLine *> matching;
          for (const ConditionalLine *cand : candidates) {
            if (GetPatternAtPos(cand->do_first(), pos) == decl_card_str) {
              matching.push_back(cand);
            }
          }
          if (!matching.empty()) {
            candidates = std::move(matching);
          }
        }
      } else {
        if (cur_opt_lines.empty()) {
          auto tree = OptimalPlayTree::FindOptimalPlayTree(copy, 0);
          int pre = copy.CardsPlayedThisTrick();
          for (const auto &line : tree->GetLines()) {
            std::vector<std::string> words = SplitWords(line);
            if (static_cast<int>(words.size()) > pre) {
              cur_opt_lines.emplace_back(words.begin() + pre, words.end());
            }
          }
        }
        std::string def_card_str;
        CardType def_card = 0;
        if (!cur_opt_lines.empty() && !cur_opt_lines[0].empty()) {
          def_card_str = cur_opt_lines[0][0];
          def_card = Card::ParseStringCard(def_card_str);
        } else {
          def_card = copy.ValidCardsToPlay()[0];
          def_card_str = Card::PrintCard(def_card);
        }
        Suit suit_led =
            (pos == 0) ? Card::DecodeCard(def_card).first : copy.SuitLed();
        copy.PlayCard(p, def_card);
        played_cards.push_back(def_card_str);

        std::vector<std::vector<std::string>> next_opt_lines;
        for (const auto &seq : cur_opt_lines) {
          if (!seq.empty() && seq[0] == def_card_str) {
            next_opt_lines.emplace_back(seq.begin() + 1, seq.end());
          }
        }
        cur_opt_lines = std::move(next_opt_lines);

        std::vector<const ConditionalLine *> matching =
            FilterCandidatesByDefenderCard(
                candidates, def_card_str, pos, suit_led, copy.GetTrump());
        if (!matching.empty()) {
          candidates = std::move(matching);
        } else {
          if (!diverged_during_trick) {
            unmatched_seat = p;
            unmatched_card = def_card_str;
          }
          diverged_during_trick = true;
          candidates = {SelectMostSpecificCandidate(candidates, pos)};
        }
      }
    }

    if (played_cards.size() < 4) {
      break;
    }

    int winner = copy.CurPlayer();
    int win_idx = (winner - trick_leader + 4) % 4;
    const ConditionalLine *chosen_node =
        diverged_during_trick ? nullptr
                              : SelectMostSpecificCandidate(candidates, 4);
    const ConditionalLine *next_line = nullptr;
    if (chosen_node) {
      for (const auto &[cond, child_line] : chosen_node->conditions()) {
        if (!child_line) continue;
        auto *winner_cond =
            dynamic_cast<const WinnerPlayCondition *>(cond.get());
        if (winner_cond && winner_cond->who() == winner) {
          if (!child_line->do_first().Empty() ||
              !child_line->conditions().empty()) {
            next_line = child_line.get();
          }
          break;
        }
      }
    }

    PlayedTrickStep step;
    step.leader = trick_leader;
    step.trick_str = played_cards[0] + " " + played_cards[1] + " " +
                     played_cards[2] + " " + played_cards[3];
    step.my1_card = my1_str;
    step.my2_card = my2_str;
    step.winner = winner;
    step.winning_card = played_cards[win_idx];
    step.ns_tricks_so_far = copy.tricks_won_even();
    step.diverged_after = (next_line == nullptr);
    step.node = diverged_during_trick ? cur_line : chosen_node;
    step.unmatched_seat = unmatched_seat;
    step.unmatched_card = unmatched_card;
    trace.steps.push_back(std::move(step));

    cur_line = next_line;
  }

  if (copy.Finished()) {
    trace.final_ns_tricks = copy.tricks_won_even();
  } else {
    int cards_left = CardsInHand(copy.GetHand(0), copy.played_cards()).size();
    int opt = FindOptimalTricks(copy);
    int rem_ns = (copy.CurPlayer() % 2 == 0) ? opt : (cards_left - opt);
    trace.final_ns_tricks = copy.tricks_won_even() + rem_ns;
  }

  return trace;
}
