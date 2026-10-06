#include "conditional_line.h"
#include "glog/logging.h"
#include "interface_dds.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

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

std::string ConditionalLine::Describe(int lvl) const {
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
  for (const auto &[play_condition, cond_line] : conditions_) {
    ans += "\n";
    for (int i = 0; i < (lvl + 1) * 2; i++) {
      ans += " ";
    }
    ans += play_condition->Describe(conditions_.size() == 1);
    ans += ": " + cond_line->Describe(lvl + 1);
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

  Suit card_suit = GetSuitOfCard(card);
  if (CONSIDER_DISCARDS && card_suit != suit_led && card_suit != trump) {
    suited[0] = 'R';  // discaRd
    cards.push_back(suited);
  }

  std::string unsuited = suited;
  unsuited[0] = '_';
  cards.push_back(unsuited);

  return cards;
}

bool GeneralizesTo(const std::string& actual_card,
                   const std::string &gen_card,
                   Suit suit_led, Suit trump) {
  std::vector<std::string> possible_cards =
      GeneralizeOppCard(actual_card, suit_led, trump);
  for (const auto &c : possible_cards) {
    if (c == gen_card) {
      return true;
    }
  }
  return false;
}

// If 1, it's faster, but doesn't seem to work as well.
#define TRY_GENERAL_CARDS_FIRST 0

using std::pair;
using std::set;
using std::make_pair;
GeneralizedPlaysVector GeneralizedDemiPlay::GetGeneralizedPlays(
    int leader, Suit known_suit_led, unsigned int top) {
  VLOG(2) << "GetGeneralizedPlays";
  std::map<pair<std::string, std::string>, std::set<int>> generalized;
  for (const auto &[opp_card, card_to_boards] : opp_to_card_to_boards_) {
    for (const auto &[my_card, boards] : card_to_boards) {
      Suit suit_led = known_suit_led != NOTRUMP ? known_suit_led
                      : (leader % 2 == 0
                         ? GetSuitOfCard(my_card)
                         : GetSuitOfCard(opp_card));
      auto opp_card_choices =
          GeneralizeOppCard(opp_card, suit_led, trump_suit_);
      for (int b : boards) {
        for (const auto &choice : opp_card_choices) {
          generalized[make_pair(choice, my_card)].insert(b);
        }
      }
    }
  }
  std::multimap<int, GeneralizedDemiStruct> sorted;
#if TRY_GENERAL_CARDS_FIRST
  for (const auto &[opp_me, boards] : generalized) {
#else
  for (auto it = generalized.rbegin(); it != generalized.rend(); ++it) {
    const auto &[opp_me, boards] = *it;
#endif
    GeneralizedDemiStruct gds(opp_me.first, opp_me.second, boards);
    for (int b : boards) {
      gds.board_to_trick[b] = saved_tricks_[opp_me.first][opp_me.second][b];
    }
    sorted.insert(make_pair(boards.size(), gds));
  }

  GeneralizedPlaysVector ans;

  std::map<std::set<int>, std::set<std::pair<std::string, std::string>>>
      seen_plays;
  for (auto it = sorted.rbegin(); it != sorted.rend(); it++) {
    const auto &[_count, value] = *it;
    if (ans.size() >= top) {
      break;
    }
    const std::set<int> &boards = value.boards;
    bool redundant = false;
    for (const auto &[opp, me] : seen_plays[boards]) {
      if (me == value.my_card && CardsAreRedundant(opp, value.opp_card)) {
        redundant = true;
        break;
      }
    }
    if (redundant) {
      continue;
    }
    auto opp_me = make_pair(value.opp_card, value.my_card);
    seen_plays[boards].insert(opp_me);
    // GeneralizedDemiStruct gds(value.opp_card, value.my_card, boards);
    ans.push_back(value);
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
  for (uint i = 0; i < tricks_.size(); i++) {
    const auto &[cards, board] = tricks_[i];
    if (leader_ % 2 == 0) {
      demi1.RecordOptimalPlay(cards[1], cards[0], board, &raw_tricks_[i]);
    } else {
      demi1.RecordOptimalPlay(cards[0], cards[1], board, &raw_tricks_[i]);
    }
  }

  std::multimap<int, GeneralizedTrick> sorted;

  GeneralizedPlaysVector demi1_plays =
      demi1.GetGeneralizedPlays(leader_, NOTRUMP, top * 2);
  for (const GeneralizedDemiStruct &demi1 : demi1_plays) {
    GeneralizedDemiPlay demi2(trump_suit_);
    for (uint i = 0; i < tricks_.size(); i++) {
      const auto &[cards, board] = tricks_[i];
      if (leader_ % 2 == 0) {
        Suit suit_led = GetSuitOfCard(demi1.my_card);
        if (demi1.my_card != cards[0] ||
            !GeneralizesTo(cards[1], demi1.opp_card, suit_led, trump_suit_)) {
          continue;
        }
        demi2.RecordOptimalPlay(cards[3], cards[2], board, &raw_tricks_[i]);
      } else {
        Suit suit_led = GetSuitOfCard(demi1.opp_card);
        if (!GeneralizesTo(cards[0], demi1.opp_card, suit_led, trump_suit_) ||
            demi1.my_card != cards[1]) {
          continue;
        }
        demi2.RecordOptimalPlay(cards[2], cards[3], board, &raw_tricks_[i]);
      }
    }

    const auto &boards1 = demi1.boards;
    auto opp_me1 = make_pair(demi1.opp_card, demi1.my_card);
    Suit suit_led = (leader_ % 2 == 0 ? GetSuitOfCard(demi1.my_card)
                     : GetSuitOfCard(demi1.opp_card));
    GeneralizedPlaysVector demi2_plays =
        demi2.GetGeneralizedPlays(leader_, suit_led, top * 2);
    for (const GeneralizedDemiStruct &demi2 : demi2_plays) {
      const auto &boards2 = demi2.boards;
      auto opp_me2 = make_pair(demi2.opp_card, demi2.my_card);
      std::set<int> intersection;
      // LOG_EVERY_N(INFO, 1000) << "Generalized set_intersection";
      std::set_intersection(boards1.begin(), boards1.end(),
                            boards2.begin(), boards2.end(),
                            std::inserter(intersection, intersection.begin()));
      if (!intersection.empty()) {
        sorted.insert(make_pair(intersection.size(),
                                GeneralizedTrick(demi1, demi2, intersection,
                                                 leader_)));
      }
    }
  }
  GeneralizedTricksVector ans;
  for (auto it = sorted.rbegin(); it != sorted.rend(); it++) {
    const auto &[_count, value] = *it;
    if (ans.size() >= top) {
      break;
    }
    ans.push_back(value);
  }
  return ans;
}

std::string GeneralizedTrick::ToString(bool compact) const {
  std::string play_str;
  auto board_to_trick = demi2.board_to_trick;

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
      play_str += " / " + board_to_trick[b];
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
}  // namespace

// Returns true if declarer can decide to play between them based
// on the opponents cards.
bool IsDistinguishable(const GeneralizedTrick& trick1,
                       const GeneralizedTrick& trick2) {
  if (AnyIntersection(trick1.boards, trick2.boards)) {
    // The same hand plays under both patterns, so can't be distinguishable.
    return false;
  }
  std::string trick1_str = trick1.ToString(true);
  std::string trick2_str = trick2.ToString(true);
  if (trick2_str == trick1_str) {
    // There's no difference in line of play.
    return false;
  }
  std::vector<std::string> cards1 = SplitWords(trick1_str);
  std::vector<std::string> cards2 = SplitWords(trick2_str);

  CHECK_EQ(cards1.size(), 4) << trick1_str;
  CHECK_EQ(cards2.size(), 4) << trick2_str;
  bool any_defensive_diff = false;
  for (int i = 0; i < 4; i++) {
    if (cards1[i][0] == '(') {
      // This is an opp turn
      if (!CardsAreRedundant(cards1[i].substr(1, 2),
                             cards2[i].substr(1, 2))) {
        any_defensive_diff = true;
      }
    } else {
      // declarer play
      if (cards1[i] != cards2[i] && !any_defensive_diff) {
        // Declarer is supposed to play a different card, but defense
        // hasn't done anything interesting.
        return false;
      }
    }
  }
  return any_defensive_diff;
}

std::vector<GeneralizedTrick> MakeGroupStartingWith(
    const std::vector<GeneralizedTrick> &input, int start) {
  std::vector<GeneralizedTrick> ans;
  for (uint i = start; i < input.size(); i++) {
    const GeneralizedTrick &trick = input[i];
    bool any_indistinguishable = false;
    for (const GeneralizedTrick &existing : ans) {
      if (!IsDistinguishable(existing, trick)) {
        any_indistinguishable = true;
        break;
      }
    }
    if (!any_indistinguishable) {
      ans.push_back(trick);
    }
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
  std::set<std::set<int>> boards_grouped;
  for (uint i = 0; i < input.size(); i++) {
    std::vector<GeneralizedTrick> group = MakeGroupStartingWith(input, i);
    bool any_new_groupings = false;
    for (const GeneralizedTrick &trick : group) {
      if (boards_grouped.count(trick.boards) == 0) {
        any_new_groupings = true;
        break;
      }
    }

    if (any_new_groupings) {
      for (const GeneralizedTrick &trick : group) {
        boards_grouped.insert(trick.boards);
      }
      ans.push_back(group);
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
  for (int pos = 0; pos < 4; ++pos) {
    if (copy.Finished()) break;
    int p = copy.CurPlayer();
    if (p % 2 == 0) {
      // Declarer side: play the card required by the ConditionalLine.
      std::string decl_card_str =
          GetPatternAtPos(candidates[0]->do_first(), pos);
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
    } else {
      // Defender side: query DDS OptimalPlayTree when needed.
      if (cur_opt_lines.empty()) {
        auto tree = OptimalPlayTree::FindOptimalPlayTree(copy, 0);
        cur_opt_lines = ExtractTrickLines(tree->GetLines());
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
        std::vector<const ConditionalLine *> matching;
        for (const ConditionalLine *cand : candidates) {
          std::string pattern = GetPatternAtPos(cand->do_first(), pos);
          if (GeneralizesTo(def_card_str, pattern, suit_led, copy.GetTrump())) {
            matching.push_back(cand);
          }
        }
        if (!matching.empty()) {
          candidates = std::move(matching);
        }
      }
    }
  }

  const ConditionalLine *chosen_node = candidates[0];
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
  return ResultsOnHand(bs);
}
