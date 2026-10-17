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
    std::string honor_or_not = card;
    if (isdigit(honor_or_not[1])) {
      honor_or_not[1] = 'x';
    } else {
      honor_or_not[1] = 'X';
      // Only record raw card if it's an honor
      cards.push_back(card);
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

using std::pair;
using std::set;
using std::make_pair;
GeneralizedPlaysVector GeneralizedDemiPlay::GetGeneralizedPlays(
    int leader, Suit known_suit_led, unsigned int top,
    bool populate_boards_set) {
  VLOG(2) << "GetGeneralizedPlays";
  struct GenBucket {
    std::string opp_card;
    std::string my_card;
    std::vector<int> boards;
  };
  std::vector<GenBucket> generalized;
  generalized.reserve(buckets_.size() * 3);
  for (const RawPlayBucket &b : buckets_) {
    Suit suit_led =
        has_by_suit_
            ? b.suit_led
            : (known_suit_led != NOTRUMP
                   ? known_suit_led
                   : (leader % 2 == 0 ? GetSuitOfCard(b.my_card)
                                      : GetSuitOfCard(b.opp_card)));
    auto opp_card_choices =
        GeneralizeOppCard(b.opp_card, suit_led, trump_suit_);
    for (const auto &choice : opp_card_choices) {
      std::vector<int> *dest = nullptr;
      for (GenBucket &gb : generalized) {
        if (gb.opp_card == choice && gb.my_card == b.my_card) {
          dest = &gb.boards;
          break;
        }
      }
      if (dest == nullptr) {
        generalized.push_back(GenBucket{choice, b.my_card, b.boards});
      } else {
        dest->insert(dest->end(), b.boards.begin(), b.boards.end());
      }
    }
  }
  std::sort(generalized.begin(), generalized.end(),
            [](const GenBucket &a, const GenBucket &b) {
              if (a.opp_card != b.opp_card) return a.opp_card < b.opp_card;
              return a.my_card < b.my_card;
            });
  for (GenBucket &gb : generalized) {
    auto &vec = gb.boards;
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
  }
  std::stable_sort(generalized.begin(), generalized.end(),
                   [](const GenBucket &a, const GenBucket &b) {
                     return a.boards.size() > b.boards.size();
                   });

  GeneralizedPlaysVector ans;
  ans.reserve(std::min<size_t>(top, generalized.size()));

  size_t count_group_start = 0;
  int prev_count = -1;
  for (GenBucket &gb : generalized) {
    int count = static_cast<int>(gb.boards.size());
    if (ans.size() >= top) {
      break;
    }
    if (count != prev_count) {
      count_group_start = ans.size();
      prev_count = count;
    }
    std::vector<int> &boards_vec = gb.boards;
    bool redundant = false;
    for (size_t k = count_group_start; k < ans.size(); ++k) {
      if (ans[k].my_card == gb.my_card &&
          CardsAreRedundant(ans[k].opp_card, gb.opp_card) &&
          ans[k].boards_vec == boards_vec) {
        redundant = true;
        break;
      }
    }
    if (redundant) {
      continue;
    }
    GeneralizedDemiStruct gds;
    gds.opp_card = std::move(gb.opp_card);
    gds.my_card = std::move(gb.my_card);
    gds.boards_vec = std::move(boards_vec);
    if (populate_boards_set) {
      for (int b : gds.boards_vec) {
        gds.boards.insert(gds.boards.end(), b);
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
  Suit suit_led = NOTRUMP;
  if (trick) {
    CHECK_NE((*trick)[0], '(') << *trick;
    Suit parsed = Card::ParseSuit((*trick)[0]);
    if (parsed >= 0 && parsed < 4) {
      has_by_suit_ = true;
      suit_led = parsed;
    }
  }
  for (RawPlayBucket &b : buckets_) {
    if (b.suit_led == suit_led && b.opp_card == opp_card &&
        b.my_card == my_card) {
      if (b.boards.empty() || b.boards.back() != board_num) {
        b.boards.push_back(board_num);
      }
      return;
    }
  }
  buckets_.push_back(RawPlayBucket{suit_led, opp_card, my_card, {board_num}});
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

void GeneralizedTrick::PopulateLegacyMaps() {
  if (boards.empty() && !demi2.boards_vec.empty()) {
    for (int b : demi2.boards_vec) {
      boards.insert(boards.end(), b);
    }
  }
  if (demi2.board_to_trick.empty() && !board_entries.empty()) {
    for (const auto &entry : board_entries) {
      demi2.board_to_trick.emplace_hint(demi2.board_to_trick.end(),
                                        entry.board, entry.trick_str);
    }
  }
}

void GeneralizedTrick::StripBoardsAtOrAbove(int max_board_id) {
  size_t orig_entries = board_entries.size();
  boards.erase(boards.lower_bound(max_board_id), boards.end());
  demi1.boards.erase(demi1.boards.lower_bound(max_board_id),
                     demi1.boards.end());
  demi2.boards.erase(demi2.boards.lower_bound(max_board_id),
                     demi2.boards.end());
  demi1.boards_vec.erase(
      std::remove_if(demi1.boards_vec.begin(), demi1.boards_vec.end(),
                     [max_board_id](int b) { return b >= max_board_id; }),
      demi1.boards_vec.end());
  demi2.boards_vec.erase(
      std::remove_if(demi2.boards_vec.begin(), demi2.boards_vec.end(),
                     [max_board_id](int b) { return b >= max_board_id; }),
      demi2.boards_vec.end());
  demi1.board_to_trick.erase(demi1.board_to_trick.lower_bound(max_board_id),
                             demi1.board_to_trick.end());
  demi2.board_to_trick.erase(demi2.board_to_trick.lower_bound(max_board_id),
                             demi2.board_to_trick.end());
  board_entries.erase(
      std::remove_if(board_entries.begin(), board_entries.end(),
                     [max_board_id](const BoardTrickEntry &e) {
                       return e.board >= max_board_id;
                     }),
      board_entries.end());
  size_t new_entries = board_entries.size();
  if (leader % 2 == 0) {
    if (!demi1.boards_vec.empty() || orig_entries > 0) {
      opp1_step1_boards = demi1.boards_vec.size();
    }
    opp2_step1_boards = demi2.boards_vec.size();
  } else if (orig_entries > 0) {
    opp1_step1_boards =
        (opp1_step1_boards * new_entries + orig_entries / 2) / orig_entries;
    opp2_step1_boards =
        (opp2_step1_boards * new_entries + orig_entries / 2) / orig_entries;
  }
}

void GeneralizedTrick::TightenToEntries(Suit trump_suit) {
  if (board_entries.empty()) return;
  auto can_tighten = [](const std::string &p) {
    return p == "__" ||
           (p.size() == 2 && p[0] != 'R' && (p[1] == '_' || p[1] == 'X'));
  };
  if (!can_tighten(demi1.opp_card) && !can_tighten(demi2.opp_card)) return;
  auto split_trick = [](const std::string &ts) {
    std::array<std::string, 4> out;
    size_t pos = 0;
    int idx = 0;
    while (pos < ts.size() && idx < 4) {
      while (pos < ts.size() && ts[pos] == ' ') ++pos;
      if (pos >= ts.size()) break;
      size_t end = pos;
      while (end < ts.size() && ts[end] != ' ') ++end;
      out[idx++] = ts.substr(pos, end - pos);
      pos = end;
    }
    return out;
  };
  std::vector<std::array<std::string, 4>> parsed;
  parsed.reserve(board_entries.size());
  for (const auto &e : board_entries) {
    parsed.push_back(split_trick(e.trick_str));
  }
  int opp1_idx = (leader % 2 == 0) ? 1 : 0;
  int opp2_idx = (leader % 2 == 0) ? 3 : 2;
  if (can_tighten(demi1.opp_card)) {
    Suit suit_led1 =
        (leader % 2 == 0) ? GetSuitOfCard(parsed[0][0])
                          : GetSuitOfCard(parsed[0][opp1_idx]);
    auto cands1 = GeneralizeOppCard(parsed[0][opp1_idx], suit_led1, trump_suit);
    std::sort(cands1.begin(), cands1.end());
    for (const auto &pat : cands1) {
      if (pat == demi1.opp_card) break;
      if (pat == "R_" || pat == "__") continue;
      bool all_ok = true;
      for (const auto &cards : parsed) {
        Suit sl = (leader % 2 == 0) ? GetSuitOfCard(cards[0])
                                    : GetSuitOfCard(cards[opp1_idx]);
        if (!GeneralizesTo(cards[opp1_idx], pat, sl, trump_suit)) {
          all_ok = false;
          break;
        }
      }
      if (all_ok) {
        demi1.opp_card = pat;
        break;
      }
    }
  }
  if (can_tighten(demi2.opp_card)) {
    Suit suit_led2 = GetSuitOfCard(parsed[0][0]);
    auto cands2 = GeneralizeOppCard(parsed[0][opp2_idx], suit_led2, trump_suit);
    std::sort(cands2.begin(), cands2.end());
    for (const auto &pat : cands2) {
      if (pat == demi2.opp_card) break;
      if (pat == "R_" || pat == "__") continue;
      bool all_ok = true;
      for (const auto &cards : parsed) {
        Suit sl = GetSuitOfCard(cards[0]);
        if (!GeneralizesTo(cards[opp2_idx], pat, sl, trump_suit)) {
          all_ok = false;
          break;
        }
      }
      if (all_ok) {
        demi2.opp_card = pat;
        break;
      }
    }
  }
}

std::string GeneralizedTrick::GetTrickForBoard(int b) const {
  if (!board_entries.empty()) {
    auto it = std::lower_bound(
        board_entries.begin(), board_entries.end(), b,
        [](const BoardTrickEntry &e, int val) { return e.board < val; });
    assert(it != board_entries.end() && it->board == b);
    return it->trick_str;
  }
  auto it = demi2.board_to_trick.find(b);
  assert(it != demi2.board_to_trick.end());
  return it->second;
}

void GeneralizedTrickFinder::RecordOptimalTrick(
    int leader, const std::string &trick, int board) {
  if (leader_ < 0) {
    leader_ = leader;
  }
  CHECK_EQ(leader, leader_);
  if (!tricks_.empty() && board < tricks_.back().board) {
    monotonic_boards_ = false;
  }
  RecordedTrick rt;
  rt.board = board;
  size_t pos = 0;
  int idx = 0;
  while (pos < trick.size() && idx < 4) {
    while (pos < trick.size() && trick[pos] == ' ') ++pos;
    if (pos >= trick.size()) break;
    size_t end = pos;
    while (end < trick.size() && trick[end] != ' ') ++end;
    rt.cards[idx++] = trick.substr(pos, end - pos);
    pos = end;
  }
  CHECK_EQ(4, idx) << trick;
  Suit cur_win_suit = NOTRUMP;
  int cur_win_val = -1;
  int win_idx = 0;
  uint64_t mask = 0;
  for (int c = 0; c < 4; ++c) {
    CardType ct = Card::ParseStringCard(rt.cards[c]);
    mask |= ct;
    auto [s, v] = Card::DecodeCard(ct);
    if (c == 0) {
      rt.suit_led = s;
      cur_win_suit = s;
      cur_win_val = v;
      win_idx = 0;
    } else if (s == cur_win_suit) {
      if (v > cur_win_val) {
        cur_win_val = v;
        win_idx = c;
      }
    } else if (s == trump_suit_) {
      cur_win_suit = s;
      cur_win_val = v;
      win_idx = c;
    }
  }
  rt.trick_mask = mask;
  rt.winner = static_cast<uint8_t>((leader + win_idx) % 4);
  rt.win_card_idx = static_cast<uint8_t>(win_idx);
  tricks_.push_back(std::move(rt));
  raw_tricks_.push_back(trick);
}

GeneralizedTricksVector GeneralizedTrickFinder::GetGeneralizedTricks(
    unsigned int top, bool populate_boards_set) {
  GeneralizedDemiPlay demi1(trump_suit_);
  struct RawD1Bucket {
    std::string opp1;
    std::string my1;
    std::vector<unsigned int> indices;
  };
  std::vector<RawD1Bucket> raw_d1_to_tricks;
  raw_d1_to_tricks.reserve(16);
  for (unsigned int i = 0; i < tricks_.size(); i++) {
    const RecordedTrick &rt = tricks_[i];
    const std::string &opp1 = (leader_ % 2 == 0) ? rt.cards[1] : rt.cards[0];
    const std::string &my1 = (leader_ % 2 == 0) ? rt.cards[0] : rt.cards[1];
    demi1.RecordOptimalPlay(opp1, my1, rt.board, nullptr);
    bool found = false;
    for (RawD1Bucket &b : raw_d1_to_tricks) {
      if (b.opp1 == opp1 && b.my1 == my1) {
        b.indices.push_back(i);
        found = true;
        break;
      }
    }
    if (!found) {
      raw_d1_to_tricks.push_back(RawD1Bucket{opp1, my1, {i}});
    }
  }
  struct GenD1Bucket {
    std::string opp1;
    std::string my1;
    std::vector<unsigned int> indices;
  };
  std::vector<GenD1Bucket> d1_to_tricks;
  d1_to_tricks.reserve(raw_d1_to_tricks.size() * 3);
  for (const auto &rb : raw_d1_to_tricks) {
    Suit suit_led =
        (leader_ % 2 == 0) ? GetSuitOfCard(rb.my1) : GetSuitOfCard(rb.opp1);
    auto opp1_choices = GeneralizeOppCard(rb.opp1, suit_led, trump_suit_);
    for (const auto &choice : opp1_choices) {
      std::vector<unsigned int> *dest = nullptr;
      for (GenD1Bucket &gb : d1_to_tricks) {
        if (gb.opp1 == choice && gb.my1 == rb.my1) {
          dest = &gb.indices;
          break;
        }
      }
      if (dest == nullptr) {
        d1_to_tricks.push_back(GenD1Bucket{choice, rb.my1, rb.indices});
      } else {
        dest->insert(dest->end(), rb.indices.begin(), rb.indices.end());
      }
    }
  }

  GeneralizedPlaysVector demi1_plays =
      demi1.GetGeneralizedPlays(leader_, NOTRUMP, top * 2,
                                /*populate_boards_set=*/false);
  struct BoardLeadCard {
    int board;
    std::string opp1;
    Suit suit_led;
  };
  std::vector<BoardLeadCard> def_lead_boards;
  if (leader_ % 2 != 0) {
    def_lead_boards.reserve(tricks_.size());
    for (const RecordedTrick &rt : tricks_) {
      if (monotonic_boards_) {
        if (!def_lead_boards.empty() &&
            def_lead_boards.back().board == rt.board) {
          continue;
        }
      } else {
        bool seen = false;
        for (const auto &bl : def_lead_boards) {
          if (bl.board == rt.board) {
            seen = true;
            break;
          }
        }
        if (seen) continue;
      }
      def_lead_boards.push_back(
          BoardLeadCard{rt.board, rt.cards[0], rt.suit_led});
    }
  }
  std::vector<size_t> d1_opp1_boards(demi1_plays.size(), 0);
  for (size_t i = 0; i < demi1_plays.size(); ++i) {
    if (leader_ % 2 == 0) {
      d1_opp1_boards[i] = demi1_plays[i].boards_vec.size();
    } else {
      size_t cnt = 0;
      const std::string &pat = demi1_plays[i].opp_card;
      for (const auto &bl : def_lead_boards) {
        if (GeneralizesTo(bl.opp1, pat, bl.suit_led, trump_suit_)) {
          ++cnt;
        }
      }
      d1_opp1_boards[i] = cnt;
    }
  }
  std::vector<GeneralizedPlaysVector> all_demi2_plays(demi1_plays.size());
  std::vector<const std::vector<unsigned int> *> d1_trick_vecs(
      demi1_plays.size(), nullptr);
  struct TrickPairCand {
    int count;
    size_t d1_idx;
    size_t d2_idx;
  };
  std::vector<TrickPairCand> sorted;

  int max_d1_size =
      demi1_plays.empty()
          ? 0
          : static_cast<int>(demi1_plays[0].boards_vec.size());
  std::vector<int> size_hist(max_d1_size + 1, 0);
  int prev_max_possible = max_d1_size;
  int pairs_strictly_above = 0;

  for (size_t d1_idx = 0; d1_idx < demi1_plays.size(); ++d1_idx) {
    const GeneralizedDemiStruct &d1 = demi1_plays[d1_idx];
    int max_possible = static_cast<int>(d1.boards_vec.size());
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
    std::vector<unsigned int> *tricks_vec = nullptr;
    for (GenD1Bucket &gb : d1_to_tricks) {
      if (gb.opp1 == d1.opp_card && gb.my1 == d1.my_card) {
        tricks_vec = &gb.indices;
        break;
      }
    }
    if (tricks_vec != nullptr) {
      std::sort(tricks_vec->begin(), tricks_vec->end());
      d1_trick_vecs[d1_idx] = tricks_vec;
      for (unsigned int i : *tricks_vec) {
        const RecordedTrick &rt = tricks_[i];
        const std::string *trick_ptr =
            (d1_suit_led == NOTRUMP) ? &raw_tricks_[i] : nullptr;
        if (leader_ % 2 == 0) {
          demi2.RecordOptimalPlay(rt.cards[3], rt.cards[2], rt.board,
                                  trick_ptr);
        } else {
          demi2.RecordOptimalPlay(rt.cards[2], rt.cards[3], rt.board,
                                  trick_ptr);
        }
      }
    }

    all_demi2_plays[d1_idx] =
        demi2.GetGeneralizedPlays(leader_, d1_suit_led, top * 2,
                                  /*populate_boards_set=*/false);
    const auto &demi2_plays = all_demi2_plays[d1_idx];
    for (size_t d2_idx = 0; d2_idx < demi2_plays.size(); ++d2_idx) {
      int inter_count = static_cast<int>(demi2_plays[d2_idx].boards_vec.size());
      if (inter_count > 0) {
        sorted.push_back(TrickPairCand{inter_count, d1_idx, d2_idx});
        if (inter_count <= max_d1_size) {
          size_hist[inter_count]++;
        }
      }
    }
  }
  std::sort(sorted.begin(), sorted.end(),
            [](const TrickPairCand &a, const TrickPairCand &b) {
              if (a.count != b.count) return a.count > b.count;
              if (a.d1_idx != b.d1_idx) return a.d1_idx > b.d1_idx;
              return a.d2_idx > b.d2_idx;
            });

  GeneralizedTricksVector ans;
  ans.reserve(std::min<size_t>(top, sorted.size()));
  std::vector<size_t> ans_d1_board_count;
  ans_d1_board_count.reserve(std::min<size_t>(top, sorted.size()));
  size_t count_group_start = 0;
  int prev_inter_count = -1;

  for (const TrickPairCand &cand : sorted) {
    if (ans.size() >= top) {
      break;
    }
    if (cand.count != prev_inter_count) {
      count_group_start = ans.size();
      prev_inter_count = cand.count;
    }
    size_t d1_idx = cand.d1_idx;
    size_t d2_idx = cand.d2_idx;
    const auto &d1 = demi1_plays[d1_idx];
    const GeneralizedDemiStruct &d2 = all_demi2_plays[d1_idx][d2_idx];
    const std::vector<unsigned int> *idx_vec = d1_trick_vecs[d1_idx];
    size_t opp1_boards = d1_opp1_boards[d1_idx];
    if (leader_ % 2 != 0) {
      bool replaced_or_redundant = false;
      for (size_t ans_idx = count_group_start; ans_idx < ans.size();
           ++ans_idx) {
        const GeneralizedTrick &prev = ans[ans_idx];
        const std::string &prev_opp1 = prev.demi1.opp_card;
        if (prev_opp1[0] == d1.opp_card[0] &&
            prev.demi1.my_card == d1.my_card &&
            prev.demi2.my_card == d2.my_card &&
            prev.demi2.opp_card == d2.opp_card &&
            CardsAreRedundant(prev_opp1, d1.opp_card) &&
            prev.demi2.boards_vec == d2.boards_vec) {
          if (d1.boards_vec.size() > ans_d1_board_count[ans_idx]) {
            ans[ans_idx] = BuildGeneralizedTrick(
                d1, d2, idx_vec, opp1_boards, populate_boards_set);
            ans_d1_board_count[ans_idx] = d1.boards_vec.size();
          }
          replaced_or_redundant = true;
          break;
        }
      }
      if (replaced_or_redundant) {
        continue;
      }
    }
    ans.push_back(BuildGeneralizedTrick(
        d1, d2, idx_vec, opp1_boards, populate_boards_set));
    ans_d1_board_count.push_back(d1.boards_vec.size());
  }
  return ans;
}

GeneralizedTrick GeneralizedTrickFinder::BuildGeneralizedTrick(
    const GeneralizedDemiStruct &d1,
    const GeneralizedDemiStruct &d2,
    const std::vector<unsigned int> *d1_trick_indices,
    size_t opp1_step1_boards,
    bool populate_boards_set) const {
  GeneralizedTrick gt;
  gt.leader = leader_;
  gt.opp1_step1_boards = opp1_step1_boards;
  gt.demi1.opp_card = d1.opp_card;
  gt.demi1.my_card = d1.my_card;
  gt.demi1.boards_vec = d1.boards_vec;
  gt.demi2.opp_card = d2.opp_card;
  gt.demi2.my_card = d2.my_card;
  gt.demi2.boards_vec = d2.boards_vec;
  if (leader_ % 2 != 0 && d1_trick_indices != nullptr) {
    int last_b = -1;
    std::vector<int> seen_b;
    for (unsigned int idx : *d1_trick_indices) {
      const RecordedTrick &rt = tricks_[idx];
      if (!GeneralizesTo(rt.cards[2], d2.opp_card, rt.suit_led, trump_suit_)) {
        continue;
      }
      if (monotonic_boards_) {
        if (rt.board == last_b) continue;
        last_b = rt.board;
        ++gt.opp2_step1_boards;
      } else {
        if (std::find(seen_b.begin(), seen_b.end(), rt.board) ==
            seen_b.end()) {
          seen_b.push_back(rt.board);
          ++gt.opp2_step1_boards;
        }
      }
    }
  } else {
    gt.opp2_step1_boards = d2.boards_vec.size();
  }
  if (d1_trick_indices != nullptr) {
    size_t target_boards = d2.boards_vec.size();
    gt.board_entries.reserve(target_boards);
    for (auto rit = d1_trick_indices->rbegin();
         rit != d1_trick_indices->rend(); ++rit) {
      unsigned int i = *rit;
      const RecordedTrick &rt = tricks_[i];
      const std::string &my2 = (leader_ % 2 == 0) ? rt.cards[2] : rt.cards[3];
      if (my2 != d2.my_card) {
        continue;
      }
      const std::string &opp2 = (leader_ % 2 == 0) ? rt.cards[3] : rt.cards[2];
      if (GeneralizesTo(opp2, d2.opp_card, rt.suit_led, trump_suit_)) {
        if (monotonic_boards_) {
          if (!gt.board_entries.empty() &&
              gt.board_entries.back().board == rt.board) {
            continue;
          }
        } else {
          bool exists = false;
          for (const auto &e : gt.board_entries) {
            if (e.board == rt.board) {
              exists = true;
              break;
            }
          }
          if (exists) continue;
        }
        GeneralizedTrick::BoardTrickEntry entry;
        entry.board = rt.board;
        entry.winner = rt.winner;
        const std::string &wc = rt.cards[rt.win_card_idx];
        entry.win_card[0] = wc.size() > 0 ? wc[0] : '\0';
        entry.win_card[1] = wc.size() > 1 ? wc[1] : '\0';
        entry.win_card[2] = '\0';
        entry.trick_mask = rt.trick_mask;
        entry.trick_str = raw_tricks_[i];
        gt.board_entries.push_back(std::move(entry));
        if (gt.board_entries.size() == target_boards) {
          break;
        }
      }
    }
    if (monotonic_boards_) {
      std::reverse(gt.board_entries.begin(), gt.board_entries.end());
    } else {
      std::sort(gt.board_entries.begin(), gt.board_entries.end(),
                [](const auto &a, const auto &b) { return a.board < b.board; });
    }
  }
  if (populate_boards_set) {
    gt.PopulateLegacyMaps();
  }
  return gt;
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
    if (!boards.empty()) {
      play_str += " [" + StrJoin(boards, ",") + "]";
      int num_tricks = 0;
      for (int b : boards) {
        auto it = demi2.board_to_trick.find(b);
        play_str +=
            " / " + (it != demi2.board_to_trick.end() ? it->second : "");
        if (++num_tricks >= 5) {
          break;
        }
      }
    } else {
      std::set<int> bset(demi2.boards_vec.begin(), demi2.boards_vec.end());
      play_str += " [" + StrJoin(bset, ",") + "]";
      int num_tricks = 0;
      for (const auto &e : board_entries) {
        play_str += " / " + e.trick_str;
        if (++num_tricks >= 5) {
          break;
        }
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

int PatternCharBit(char c) {
  switch (c) {
    case '_': return 0;
    case 'x': return 1;
    case 'X': return 2;
    case 'T': return 3;
    case 'J': return 4;
    case 'Q': return 5;
    case 'K': return 6;
    case 'A': return 7;
    default:
      if (c >= '2' && c <= '9') return 8 + (c - '2');
      return -1;
  }
}

struct TrickOppPatternMask {
  bool computed = false;
  bool always_true = false;
  bool has_any = false;
  bool has_discard = false;
  uint32_t suit_mask[4] = {0, 0, 0, 0};

  bool Matches(const std::string &specific_pattern) const {
    if (always_true) return true;
    if (specific_pattern.size() != 2) return false;
    char gen_suit = specific_pattern[0];
    char gen_rank = specific_pattern[1];
    if (gen_suit == '_' && gen_rank == '_') return has_any;
    if (gen_suit == 'R' && gen_rank == '_') {
      return CONSIDER_DISCARDS && has_discard;
    }
    Suit s = Card::ParseSuit(gen_suit);
    if (s < 0 || s >= 4) return false;
    int bit = PatternCharBit(gen_rank);
    if (bit < 0) return false;
    return (suit_mask[s] & (1u << bit)) != 0;
  }
};

bool AccumulateTrickStringIntoMask(const std::string &t_str, int opp_pos,
                                   TrickOppPatternMask *mask) {
  size_t c0_pos = 0;
  size_t c0_len = 0;
  size_t opp_c_pos = 0;
  size_t opp_c_len = 0;
  size_t pos = 0;
  int idx = 0;
  while (pos < t_str.size() && idx <= opp_pos) {
    while (pos < t_str.size() && t_str[pos] == ' ') ++pos;
    if (pos >= t_str.size()) break;
    size_t end = pos;
    while (end < t_str.size() && t_str[end] != ' ') ++end;
    if (idx == 0) {
      c0_pos = pos;
      c0_len = end - pos;
    }
    if (idx == opp_pos) {
      opp_c_pos = pos;
      opp_c_len = end - pos;
    }
    ++idx;
    pos = end;
  }
  if (idx <= opp_pos || c0_len < 2 || opp_c_len < 2) {
    mask->always_true = true;
    return false;
  }
  mask->has_any = true;
  Suit suit_led = Card::ParseSuit(t_str[c0_pos]);
  Suit card_suit = Card::ParseSuit(t_str[opp_c_pos]);
  char opp_rank = t_str[opp_c_pos + 1];
  bool is_discard = (suit_led != NOTRUMP && card_suit != suit_led);
  if (is_discard) {
    mask->has_discard = true;
  } else if (card_suit >= 0 && card_suit < 4) {
    uint32_t &sm = mask->suit_mask[card_suit];
    sm |= (1u << 0);  // '_'
    if (isdigit(opp_rank)) {
      sm |= (1u << 1);  // 'x'
    } else {
      sm |= (1u << 2);  // 'X'
      int bit = PatternCharBit(opp_rank);
      if (bit >= 0) {
        sm |= (1u << bit);
      }
    }
  }
  return true;
}

TrickOppPatternMask BuildTrickOppPatternMask(
    const GeneralizedTrick &general_trick, int opp_pos) {
  TrickOppPatternMask mask;
  mask.computed = true;
  if (!general_trick.board_entries.empty()) {
    for (const auto &entry : general_trick.board_entries) {
      if (!AccumulateTrickStringIntoMask(entry.trick_str, opp_pos, &mask)) {
        return mask;
      }
    }
    return mask;
  }
  if (general_trick.demi2.board_to_trick.empty()) {
    mask.always_true = true;
    return mask;
  }
  for (int b : general_trick.boards) {
    auto it = general_trick.demi2.board_to_trick.find(b);
    if (it == general_trick.demi2.board_to_trick.end()) {
      mask.always_true = true;
      return mask;
    }
    if (!AccumulateTrickStringIntoMask(it->second, opp_pos, &mask)) {
      return mask;
    }
  }
  return mask;
}

bool HasActualCardMatchingPattern(const GeneralizedTrick &general_trick,
                                  const std::string &specific_pattern,
                                  int opp_pos) {
  return BuildTrickOppPatternMask(general_trick, opp_pos)
      .Matches(specific_pattern);
}

bool OppSlotDistinguishable(const GeneralizedTrick &trick1,
                            const GeneralizedTrick &trick2,
                            const std::string &opp1,
                            const std::string &opp2,
                            int opp_pos,
                            TrickOppPatternMask *mask1 = nullptr,
                            TrickOppPatternMask *mask2 = nullptr) {
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
      !trick1.HasBoardTricks() ||
      !trick2.HasBoardTricks()) {
    return false;
  }
  int spec1 = PatternSpecificity(opp1);
  int spec2 = PatternSpecificity(opp2);
  if (spec1 < spec2) {
    if (mask1 != nullptr) {
      if (!mask1->computed) {
        *mask1 = BuildTrickOppPatternMask(trick1, opp_pos);
      }
      return !mask1->Matches(opp2);
    }
    return !HasActualCardMatchingPattern(trick1, opp2, opp_pos);
  }
  if (spec2 < spec1) {
    if (mask2 != nullptr) {
      if (!mask2->computed) {
        *mask2 = BuildTrickOppPatternMask(trick2, opp_pos);
      }
      return !mask2->Matches(opp1);
    }
    return !HasActualCardMatchingPattern(trick2, opp1, opp_pos);
  }
  return false;
}

bool AreCardsDistinguishable(
    const GeneralizedTrick& trick1,
    const GeneralizedTrick& trick2,
    std::array<TrickOppPatternMask, 2> *masks1 = nullptr,
    std::array<TrickOppPatternMask, 2> *masks2 = nullptr,
    int my1_id1 = -1, int my1_id2 = -2,
    int my2_id1 = -1, int my2_id2 = -2) {
  bool same_my1 = (my1_id1 >= 0 && my1_id2 >= 0)
                      ? (my1_id1 == my1_id2)
                      : (trick1.demi1.my_card == trick2.demi1.my_card);
  bool same_my2 = (my2_id1 >= 0 && my2_id2 >= 0)
                      ? (my2_id1 == my2_id2)
                      : (trick1.demi2.my_card == trick2.demi2.my_card);
  bool any_defensive_diff = false;
  if (trick1.leader % 2 == 0) {
    if (!same_my1) {
      return false;
    }
    if (OppSlotDistinguishable(trick1, trick2, trick1.demi1.opp_card,
                               trick2.demi1.opp_card, 1)) {
      any_defensive_diff = true;
    }
    if (!any_defensive_diff && !same_my2) {
      return false;
    }
    if (!any_defensive_diff &&
        OppSlotDistinguishable(trick1, trick2, trick1.demi2.opp_card,
                               trick2.demi2.opp_card, 3)) {
      any_defensive_diff = true;
    }
  } else {
    if (OppSlotDistinguishable(trick1, trick2, trick1.demi1.opp_card,
                               trick2.demi1.opp_card, 0,
                               masks1 ? &(*masks1)[0] : nullptr,
                               masks2 ? &(*masks2)[0] : nullptr)) {
      any_defensive_diff = true;
    }
    if (!any_defensive_diff && !same_my1) {
      return false;
    }
    if (!any_defensive_diff &&
        OppSlotDistinguishable(trick1, trick2, trick1.demi2.opp_card,
                               trick2.demi2.opp_card, 2,
                               masks1 ? &(*masks1)[1] : nullptr,
                               masks2 ? &(*masks2)[1] : nullptr)) {
      any_defensive_diff = true;
    }
    if (!any_defensive_diff && !same_my2) {
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
  for (unsigned int i = start; i < input.size(); i++) {
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

namespace {

std::vector<uint64_t> BuildTrickBoardBitmasks(
    const std::vector<GeneralizedTrick> &input, size_t *num_words_out) {
  int max_board = 0;
  for (const GeneralizedTrick &trick : input) {
    if (!trick.demi2.boards_vec.empty()) {
      max_board = std::max(max_board, trick.demi2.boards_vec.back());
    } else if (!trick.boards.empty()) {
      max_board = std::max(max_board, *trick.boards.rbegin());
    }
  }
  size_t num_words = (max_board / 64) + 1;
  *num_words_out = num_words;
  std::vector<uint64_t> masks(input.size() * num_words, 0);
  for (size_t i = 0; i < input.size(); ++i) {
    uint64_t *m = &masks[i * num_words];
    if (!input[i].demi2.boards_vec.empty()) {
      for (int b : input[i].demi2.boards_vec) {
        if (b >= 0) {
          m[b / 64] |= (1ULL << (b % 64));
        }
      }
    } else {
      for (int b : input[i].boards) {
        if (b >= 0) {
          m[b / 64] |= (1ULL << (b % 64));
        }
      }
    }
  }
  return masks;
}

int EncodeCardId(const std::string &c) {
  if (c.size() >= 2) {
    return (static_cast<unsigned char>(c[0]) << 8) |
           static_cast<unsigned char>(c[1]);
  }
  return c.empty() ? 0 : static_cast<unsigned char>(c[0]);
}

// Assigns an equivalence key ID to each trick based on (my1, my2, board_mask).
// Two tricks with the same declarer cards and identical board sets differ only
// in opponent-pattern spelling; once one is grouped, the others are redundant.
int AssignRedundancyKeys(
    const std::vector<GeneralizedTrick> &input,
    const std::vector<uint64_t> &masks, size_t num_words,
    const std::vector<int> &my1_id, const std::vector<int> &my2_id,
    bool def_leads, std::vector<int> *key_id,
    std::unordered_map<int, int> *ungrouped_by_my1) {
  key_id->assign(input.size(), -1);
  int next_key_id = 0;
  for (size_t i = 0; i < input.size(); ++i) {
    if ((*key_id)[i] >= 0) continue;
    int kid = next_key_id++;
    (*key_id)[i] = kid;
    if (!def_leads) {
      (*ungrouped_by_my1)[my1_id[i]]++;
    }
    const uint64_t *mi = &masks[i * num_words];
    size_t size_i = input[i].NumBoards();
    for (size_t j = i + 1; j < input.size(); ++j) {
      if ((*key_id)[j] >= 0) continue;
      if (size_i != input[j].NumBoards()) {
        // Since input is sorted by NumBoards() descending, no later j can
        // have the same board set size.
        break;
      }
      if (my1_id[i] == my1_id[j] && my2_id[i] == my2_id[j]) {
        const uint64_t *mj = &masks[j * num_words];
        bool same_mask = true;
        for (size_t w = 0; w < num_words; ++w) {
          if (mi[w] != mj[w]) {
            same_mask = false;
            break;
          }
        }
        if (same_mask) {
          (*key_id)[j] = kid;
        }
      }
    }
  }
  return next_key_id;
}

std::vector<std::vector<GeneralizedTrick>> GroupTricksImpl(
    std::vector<GeneralizedTrick> *mutable_input,
    const std::vector<GeneralizedTrick> &input) {
  VLOG(1) << "GroupTricks on " << input.size();
  for (const GeneralizedTrick &trick : input) {
    VLOG(1) << "  " << trick.ToString(false);
  }
  std::vector<std::vector<GeneralizedTrick>> ans;
  if (input.empty()) {
    return ans;
  }

  size_t num_words = 0;
  std::vector<uint64_t> masks = BuildTrickBoardBitmasks(input, &num_words);

  std::vector<int> my1_id(input.size());
  std::vector<int> my2_id(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    my1_id[i] = EncodeCardId(input[i].demi1.my_card);
    my2_id[i] = EncodeCardId(input[i].demi2.my_card);
  }

  bool def_leads = (input[0].leader % 2 != 0);
  std::vector<std::array<TrickOppPatternMask, 2>> opp_masks;
  if (def_leads) {
    opp_masks.resize(input.size());
  }

  std::vector<int> key_id;
  std::unordered_map<int, int> ungrouped_by_my1;
  int next_key_id = AssignRedundancyKeys(
      input, masks, num_words, my1_id, my2_id, def_leads, &key_id,
      &ungrouped_by_my1);
  int ungrouped_keys = next_key_id;
  std::vector<uint8_t> key_grouped(next_key_id, 0);
  std::vector<int> group_indices;
  std::vector<uint64_t> group_mask(num_words, 0);
  std::vector<std::vector<int>> group_idx_list;
  std::vector<int> ref_count(input.size(), 0);

  auto try_add = [&](unsigned int j) {
    if (!def_leads && my1_id[j] != my1_id[group_indices[0]]) {
      return;
    }
    const uint64_t *mj = &masks[j * num_words];
    for (size_t w = 0; w < num_words; ++w) {
      if ((group_mask[w] & mj[w]) != 0) {
        return;
      }
    }
    const GeneralizedTrick &trick = input[j];
    auto *m_j = def_leads ? &opp_masks[j] : nullptr;
    for (int idx : group_indices) {
      auto *m_idx = def_leads ? &opp_masks[idx] : nullptr;
      if (!AreCardsDistinguishable(input[idx], trick, m_idx, m_j,
                                   my1_id[idx], my1_id[j],
                                   my2_id[idx], my2_id[j])) {
        return;
      }
    }
    group_indices.push_back(j);
    for (size_t w = 0; w < num_words; ++w) {
      group_mask[w] |= mj[w];
    }
  };

  for (unsigned int i = 0; i < input.size(); i++) {
    if (ungrouped_keys == 0) {
      break;
    }
    if (!def_leads && ungrouped_by_my1[my1_id[i]] == 0) {
      continue;
    }
    group_indices.clear();
    group_indices.push_back(i);
    const uint64_t *mi = &masks[i * num_words];
    for (size_t w = 0; w < num_words; ++w) {
      group_mask[w] = mi[w];
    }

    if (def_leads && input[i].HasBoardTricks()) {
      for (unsigned int j = 0; j < i; j++) {
        try_add(j);
      }
    }
    for (unsigned int j = i + 1; j < input.size(); j++) {
      try_add(j);
    }
    bool any_new_groupings = false;
    for (int idx : group_indices) {
      if (!key_grouped[key_id[idx]]) {
        any_new_groupings = true;
        break;
      }
    }

    if (any_new_groupings) {
      for (int idx : group_indices) {
        int kid = key_id[idx];
        if (!key_grouped[kid]) {
          key_grouped[kid] = 1;
          --ungrouped_keys;
          if (!def_leads) {
            --ungrouped_by_my1[my1_id[idx]];
          }
        }
        ref_count[idx]++;
      }
      group_idx_list.push_back(group_indices);
    }
  }

  ans.reserve(group_idx_list.size());
  for (const auto &g_indices : group_idx_list) {
    std::vector<GeneralizedTrick> group;
    group.reserve(g_indices.size());
    for (int idx : g_indices) {
      if (mutable_input != nullptr && --ref_count[idx] == 0) {
        group.push_back(std::move((*mutable_input)[idx]));
      } else {
        group.push_back(input[idx]);
      }
    }
    ans.push_back(std::move(group));
  }
  VLOG(1) << "Got groups " << ans.size();
  return ans;
}

}  // namespace

std::vector<std::vector<GeneralizedTrick>> GroupTricks(
    const std::vector<GeneralizedTrick> &input) {
  return GroupTricksImpl(nullptr, input);
}

std::vector<std::vector<GeneralizedTrick>> GroupTricks(
    std::vector<GeneralizedTrick> &&input) {
  return GroupTricksImpl(&input, input);
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

auto CandidateSpecificityScore(const ConditionalLine *cand, int up_to_pos) {
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
}

const ConditionalLine *SelectMostSpecificCandidate(
    const std::vector<const ConditionalLine *> &candidates, int up_to_pos) {
  CHECK(!candidates.empty());
  const ConditionalLine *best = candidates[0];
  auto best_score = CandidateSpecificityScore(best, up_to_pos);
  for (size_t i = 1; i < candidates.size(); ++i) {
    auto s = CandidateSpecificityScore(candidates[i], up_to_pos);
    if (s > best_score) {
      best_score = s;
      best = candidates[i];
    }
  }
  return best;
}

double BranchSuccessProbability(
    const BoardState &copy, int pos, const std::string &card_str,
    const std::vector<const ConditionalLine *> &candidates) {
  CardType card_val = Card::ParseStringCard(card_str);
  Suit suit_led =
      (pos == 0) ? Card::DecodeCard(card_val).first : copy.SuitLed();
  std::vector<const ConditionalLine *> matching =
      FilterCandidatesByDefenderCard(
          candidates, card_str, pos, suit_led, copy.GetTrump());
  if (matching.empty()) {
    return -1.0;
  }
  auto best_spec = CandidateSpecificityScore(matching[0], pos + 1);
  for (size_t i = 1; i < matching.size(); ++i) {
    auto s = CandidateSpecificityScore(matching[i], pos + 1);
    if (s > best_spec) {
      best_spec = s;
    }
  }
  std::vector<const ConditionalLine *> top_matching;
  top_matching.reserve(matching.size());
  for (const ConditionalLine *cand : matching) {
    if (CandidateSpecificityScore(cand, pos + 1) == best_spec) {
      top_matching.push_back(cand);
    }
  }
  if (pos == 3) {
    const ConditionalLine *chosen = top_matching[0];
    BoardState next_bs = copy;
    next_bs.PlayCard(copy.CurPlayer(), card_val);
    int winner = next_bs.CurPlayer();
    size_t total_for_winner = 0;
    const auto &entries = chosen->do_first().board_entries;
    if (!entries.empty()) {
      for (const auto &entry : entries) {
        if (entry.winner == winner) {
          ++total_for_winner;
        }
      }
    } else {
      total_for_winner = chosen->do_first().NumBoards();
    }
    if (winner >= 0 && winner < 4) {
      total_for_winner += chosen->do_first().winner_unmakeable_boards[winner];
    }
    if (total_for_winner == 0) {
      return -1.0;
    }
    for (const auto &[cond, child_line] : chosen->conditions()) {
      if (!child_line) continue;
      auto *winner_cond = dynamic_cast<const WinnerPlayCondition *>(cond.get());
      if (winner_cond && winner_cond->who() == winner) {
        return static_cast<double>(child_line->step1_covered_boards()) /
               static_cast<double>(total_for_winner);
      }
    }
    return 0.0;
  }
  const GeneralizedTrick &gt0 = top_matching[0]->do_first();
  size_t step1_denom =
      (pos < 2) ? gt0.opp1_step1_boards : gt0.opp2_step1_boards;
  size_t unmakeable_denom =
      (pos < 2) ? gt0.opp1_unmakeable_boards : gt0.opp2_unmakeable_boards;
  size_t fallback_boards = 0;
  int covered_boards = 0;
  for (const ConditionalLine *cand : top_matching) {
    fallback_boards += cand->do_first().NumBoards();
    covered_boards += cand->step1_covered_boards();
  }
  size_t total_boards =
      (step1_denom > 0 ? step1_denom : fallback_boards) + unmakeable_denom;
  if (total_boards == 0) {
    return -1.0;
  }
  return static_cast<double>(covered_boards) /
         static_cast<double>(total_boards);
}

std::string ChooseAdversarialDefenderCard(
    const BoardState &copy, int pos,
    const std::vector<const ConditionalLine *> &candidates,
    ConditionalLine::DefenderCardsCache *def_cache) {
  const std::vector<std::string> *opt_ptr = nullptr;
  std::vector<std::string> local_opt;
  if (def_cache != nullptr) {
    uint64_t cur_bits =
        (static_cast<uint64_t>(copy.cur_trick_leader() & 3) << 13) |
        (static_cast<uint64_t>(copy.CardsPlayedThisTrick() & 3) << 29);
    for (int i = 0; i < copy.CardsPlayedThisTrick(); ++i) {
      cur_bits |= copy.trick_cards_played()[i];
    }
    ConditionalLine::DefenderStateKey state_key{copy.played_cards(), cur_bits};
    auto it = def_cache->find(state_key);
    if (it == def_cache->end()) {
      it = def_cache->emplace(state_key, FindOptimalDefenderCards(copy)).first;
    }
    opt_ptr = &it->second;
  } else {
    local_opt = FindOptimalDefenderCards(copy);
    opt_ptr = &local_opt;
  }
  const std::vector<std::string> &opt_cards = *opt_ptr;
  if (opt_cards.empty()) {
    return "";
  }
  if (opt_cards.size() == 1 || candidates.empty()) {
    return opt_cards[0];
  }
  std::string best_card = opt_cards[0];
  double best_prob = -1.0;
  for (size_t i = 0; i < opt_cards.size(); ++i) {
    double prob = BranchSuccessProbability(copy, pos, opt_cards[i], candidates);
    if (prob < 0.0) {
      continue;
    }
    if (best_prob < 0.0 || prob < best_prob - 1e-12) {
      best_prob = prob;
      best_card = opt_cards[i];
    }
  }
  return best_card;
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

int ResultsOnHandHelper(
    const ConditionalLine *line, BoardState copy, bool adversarial = false,
    ConditionalLine::DefenderCardsCache *def_cache = nullptr,
    ConditionalLine::RescoreBoardTrace *out_trace = nullptr) {
  if (!line) {
    return copy.tricks_won_even();
  }

  // Terminal leaf node
  if (line->do_first().Empty() && line->conditions().empty()) {
    if (out_trace != nullptr) {
      out_trace->leaf = line;
    }
    return copy.tricks_won_even();
  }

  if (copy.Finished()) {
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
          return ResultsOnHandHelper(child_line.get(), copy, adversarial,
                                     def_cache, out_trace);
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
        return ResultsOnHandHelper(line->conditions()[0].second.get(), copy,
                                   adversarial, def_cache, out_trace);
      }
      return copy.tricks_won_even();
    }
  }

  int start_pos = copy.CardsPlayedThisTrick();
  bool diverged_during_trick = false;
  std::array<std::string, 4> played_cards;
  for (int pre_pos = 0; pre_pos < start_pos; ++pre_pos) {
    int pre_p = (copy.cur_trick_leader() + pre_pos) % 4;
    CardType pre_card = copy.trick_cards_played()[pre_pos];
    std::string pre_str = Card::PrintCard(pre_card);
    if (out_trace != nullptr) {
      played_cards[pre_pos] = pre_str;
    }
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
    } else if (candidates.size() > 1) {
      Suit suit_led = Card::DecodeCard(copy.trick_cards_played()[0]).first;
      std::vector<const ConditionalLine *> matching =
          FilterCandidatesByDefenderCard(
              candidates, pre_str, pre_pos, suit_led, copy.GetTrump());
      if (!matching.empty()) {
        candidates = std::move(matching);
      } else {
        diverged_during_trick = true;
        candidates = {SelectMostSpecificCandidate(candidates, pre_pos)};
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
      if (out_trace != nullptr) {
        played_cards[pos] = Card::PrintCard(chosen_card);
      }
      copy.PlayCard(p, chosen_card);

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
      // Defender side: query DDS directly (or adversarially) for this card.
      std::string def_card_str =
          adversarial
              ? ChooseAdversarialDefenderCard(copy, pos, candidates, def_cache)
              : FindBestDefenderCard(copy);
      CHECK(!def_card_str.empty());
      CardType def_card = Card::ParseStringCard(def_card_str);
      Suit suit_led =
          (pos == 0) ? Card::DecodeCard(def_card).first : copy.SuitLed();
      if (out_trace != nullptr) {
        played_cards[pos] = def_card_str;
      }
      copy.PlayCard(p, def_card);

      if (candidates.size() > 1) {
        std::vector<const ConditionalLine *> matching =
            FilterCandidatesByDefenderCard(
                candidates, def_card_str, pos, suit_led, copy.GetTrump());
        if (!matching.empty()) {
          candidates = std::move(matching);
        } else {
          diverged_during_trick = true;
          candidates = {SelectMostSpecificCandidate(candidates, pos)};
        }
      }
    }
  }

  if (diverged_during_trick) {
    return copy.tricks_won_even();
  }

  const ConditionalLine *chosen_node =
      SelectMostSpecificCandidate(candidates, 4);
  if (out_trace != nullptr && !played_cards[3].empty()) {
    out_trace->node_tricks.emplace_back(
        chosen_node, played_cards[0] + " " + played_cards[1] + " " +
                         played_cards[2] + " " + played_cards[3]);
  }
  if (copy.Finished() && chosen_node->conditions().empty()) {
    if (out_trace != nullptr) {
      out_trace->leaf = chosen_node;
    }
    return copy.tricks_won_even();
  }
  int winner = copy.CurPlayer();
  for (const auto &[cond, child_line] : chosen_node->conditions()) {
    if (!child_line) continue;
    auto *winner_cond = dynamic_cast<const WinnerPlayCondition *>(cond.get());
    if (winner_cond && winner_cond->who() == winner) {
      return ResultsOnHandHelper(child_line.get(), copy, adversarial,
                                 def_cache, out_trace);
    }
  }

  return copy.tricks_won_even();
}

std::string ChooseStep1DefenderCard(
    const BoardState &copy,
    ConditionalLine::Step1DefenderCardCache *def_cache) {
  int noise_pct = GetDefenderNoisePct();
  if (def_cache == nullptr) {
    return FindBestDefenderCard(copy, noise_pct);
  }
  uint64_t cur_bits =
      (static_cast<uint64_t>(copy.cur_trick_leader() & 3) << 13) |
      (static_cast<uint64_t>(copy.CardsPlayedThisTrick() & 3) << 29);
  for (int i = 0; i < copy.CardsPlayedThisTrick(); ++i) {
    cur_bits |= copy.trick_cards_played()[i];
  }
  ConditionalLine::DefenderStateKey state_key{copy.played_cards(), cur_bits};
  auto it = def_cache->find(state_key);
  if (it == def_cache->end()) {
    it = def_cache->emplace(state_key, FindBestDefenderCard(copy, noise_pct))
             .first;
  }
  return it->second;
}

const GeneralizedTrick::BoardTrickEntry *FindBoardEntry(
    const std::vector<GeneralizedTrick::BoardTrickEntry> &entries,
    int board_num) {
  auto it = std::lower_bound(
      entries.begin(), entries.end(), board_num,
      [](const GeneralizedTrick::BoardTrickEntry &e, int val) {
        return e.board < val;
      });
  if (it != entries.end() && it->board == board_num) {
    return &(*it);
  }
  return nullptr;
}

bool WasCountedInStep1Demi1(const ConditionalLine *cand, int board_num,
                            bool in_step1_set) {
  if (!in_step1_set) return false;
  const auto &bv = cand->do_first().demi1.boards_vec;
  return std::binary_search(bv.begin(), bv.end(), board_num);
}

void PlayTrickStringOnBoard(const std::string &trick_str, BoardState *bs) {
  size_t pos = 0;
  int played = 0;
  while (pos < trick_str.size() && played < 4) {
    while (pos < trick_str.size() && trick_str[pos] == ' ') ++pos;
    if (pos >= trick_str.size()) break;
    size_t end = pos;
    while (end < trick_str.size() && trick_str[end] != ' ') ++end;
    CardType ct = Card::ParseStringCard(trick_str.substr(pos, end - pos));
    bs->PlayCard(bs->CurPlayer(), ct);
    ++played;
    pos = end;
  }
}

void TraceUnmakeableBoardHelper(
    const ConditionalLine *line, BoardState copy, int board_num,
    bool in_step1_set, ConditionalLine::Step1DefenderCardCache *def_cache,
    ConditionalLine::UnmakeableBoardTrace *out_trace) {
  if (!line || out_trace == nullptr || copy.Finished()) {
    return;
  }
  if (line->do_first().Empty() && line->conditions().empty()) {
    return;
  }

  std::vector<const ConditionalLine *> candidates;
  if (!line->do_first().Empty()) {
    candidates.push_back(line);
  } else {
    for (const auto &[cond, child_line] : line->conditions()) {
      if (!child_line) continue;
      if (auto *good_cond =
              dynamic_cast<const IsCardGoodCondition *>(cond.get())) {
        if (good_cond->Evaluate(copy) == good_cond->is_good()) {
          TraceUnmakeableBoardHelper(child_line.get(), copy, board_num,
                                     in_step1_set, def_cache, out_trace);
          return;
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
        TraceUnmakeableBoardHelper(line->conditions()[0].second.get(), copy,
                                   board_num, in_step1_set, def_cache,
                                   out_trace);
      }
      return;
    }
  }

  int start_pos = copy.CardsPlayedThisTrick();
  if (in_step1_set && start_pos == 0) {
    const ConditionalLine *best_step1_cand = nullptr;
    const GeneralizedTrick::BoardTrickEntry *best_entry = nullptr;
    for (const ConditionalLine *cand : candidates) {
      const auto *entry =
          FindBoardEntry(cand->do_first().board_entries, board_num);
      if (entry == nullptr) continue;
      if (best_step1_cand == nullptr ||
          CandidateSpecificityScore(cand, 4) >
              CandidateSpecificityScore(best_step1_cand, 4)) {
        best_step1_cand = cand;
        best_entry = entry;
      }
    }
    if (best_step1_cand != nullptr && best_entry != nullptr) {
      if (best_step1_cand->conditions().empty()) {
        return;
      }
      PlayTrickStringOnBoard(best_entry->trick_str, &copy);
      if (copy.Finished()) {
        return;
      }
      int winner = best_entry->winner;
      for (const auto &[cond, child_line] : best_step1_cand->conditions()) {
        if (!child_line) continue;
        auto *winner_cond =
            dynamic_cast<const WinnerPlayCondition *>(cond.get());
        if (winner_cond && winner_cond->who() == winner) {
          TraceUnmakeableBoardHelper(child_line.get(), copy, board_num,
                                     /*in_step1_set=*/true, def_cache,
                                     out_trace);
          return;
        }
      }
      return;
    }
  }

  for (int pre_pos = 0; pre_pos < start_pos; ++pre_pos) {
    int pre_p = (copy.cur_trick_leader() + pre_pos) % 4;
    CardType pre_card = copy.trick_cards_played()[pre_pos];
    std::string pre_str = Card::PrintCard(pre_card);
    if (pre_p % 2 == 0) {
      std::vector<const ConditionalLine *> matching;
      for (const ConditionalLine *cand : candidates) {
        if (GetPatternAtPos(cand->do_first(), pre_pos) == pre_str) {
          matching.push_back(cand);
        }
      }
      if (matching.empty()) return;
      candidates = std::move(matching);
    } else {
      Suit suit_led = Card::DecodeCard(copy.trick_cards_played()[0]).first;
      std::vector<const ConditionalLine *> matching =
          FilterCandidatesByDefenderCard(
              candidates, pre_str, pre_pos, suit_led, copy.GetTrump());
      if (matching.empty()) return;
      if (pre_pos == 0) {
        if (!in_step1_set) {
          for (const ConditionalLine *cand : matching) {
            out_trace->opp1_nodes.push_back(cand);
          }
        }
      } else if (pre_pos == 1) {
        for (const ConditionalLine *cand : matching) {
          if (!WasCountedInStep1Demi1(cand, board_num, in_step1_set)) {
            out_trace->opp1_nodes.push_back(cand);
          }
        }
      } else if (pre_pos == 2) {
        for (const ConditionalLine *cand : matching) {
          if (!WasCountedInStep1Demi1(cand, board_num, in_step1_set)) {
            out_trace->opp2_nodes.push_back(cand);
          }
        }
      }
      candidates = std::move(matching);
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
      copy.PlayCard(p, chosen_card);
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
      std::string def_card_str = ChooseStep1DefenderCard(copy, def_cache);
      CHECK(!def_card_str.empty());
      CardType def_card = Card::ParseStringCard(def_card_str);
      Suit suit_led =
          (pos == 0) ? Card::DecodeCard(def_card).first : copy.SuitLed();
      std::vector<const ConditionalLine *> matching =
          FilterCandidatesByDefenderCard(
              candidates, def_card_str, pos, suit_led, copy.GetTrump());
      if (matching.empty()) {
        return;
      }
      if (pos == 0) {
        if (!in_step1_set) {
          for (const ConditionalLine *cand : matching) {
            out_trace->opp1_nodes.push_back(cand);
          }
        }
      } else if (pos == 1) {
        for (const ConditionalLine *cand : matching) {
          if (!WasCountedInStep1Demi1(cand, board_num, in_step1_set)) {
            out_trace->opp1_nodes.push_back(cand);
          }
        }
      } else if (pos == 2) {
        for (const ConditionalLine *cand : matching) {
          if (!WasCountedInStep1Demi1(cand, board_num, in_step1_set)) {
            out_trace->opp2_nodes.push_back(cand);
          }
        }
      }
      copy.PlayCard(p, def_card);
      candidates = std::move(matching);
    }
  }

  const ConditionalLine *chosen_node =
      SelectMostSpecificCandidate(candidates, 4);
  int winner = copy.CurPlayer();
  bool in_entries =
      in_step1_set &&
      FindBoardEntry(chosen_node->do_first().board_entries, board_num) !=
          nullptr;
  if (!in_entries) {
    out_trace->winner_nodes.emplace_back(chosen_node, winner);
  }
  if (copy.Finished() || chosen_node->conditions().empty()) {
    return;
  }
  for (const auto &[cond, child_line] : chosen_node->conditions()) {
    if (!child_line) continue;
    auto *winner_cond = dynamic_cast<const WinnerPlayCondition *>(cond.get());
    if (winner_cond && winner_cond->who() == winner) {
      TraceUnmakeableBoardHelper(child_line.get(), copy, board_num,
                                 /*in_step1_set=*/in_entries, def_cache,
                                 out_trace);
      return;
    }
  }
}

}  // namespace

int ConditionalLine::ResultsOnHand(const BoardState &bs) const {
  return ResultsOnHandHelper(this, bs, /*adversarial=*/false, nullptr, nullptr);
}

int ConditionalLine::ResultsOnHandAdversarial(
    const BoardState &bs, DefenderCardsCache *def_cache,
    RescoreBoardTrace *out_trace) const {
  return ResultsOnHandHelper(this, bs, /*adversarial=*/true, def_cache,
                             out_trace);
}

void ConditionalLine::TraceUnmakeableBoard(
    const BoardState &bs, int board_num, bool in_step1_set,
    Step1DefenderCardCache *def_cache,
    UnmakeableBoardTrace *out_trace) const {
  TraceUnmakeableBoardHelper(this, bs, board_num, in_step1_set, def_cache,
                             out_trace);
}

void ConditionalLine::ApplyUnmakeableBoardCounts(
    const std::unordered_map<const ConditionalLine *, size_t> &opp1_counts,
    const std::unordered_map<const ConditionalLine *, size_t> &opp2_counts,
    const std::unordered_map<const ConditionalLine *, std::array<size_t, 4>>
        &winner_counts) {
  if (!do_first_.Empty()) {
    auto it1 = opp1_counts.find(this);
    do_first_.opp1_unmakeable_boards =
        (it1 != opp1_counts.end()) ? it1->second : 0;
    auto it2 = opp2_counts.find(this);
    do_first_.opp2_unmakeable_boards =
        (it2 != opp2_counts.end()) ? it2->second : 0;
    auto itw = winner_counts.find(this);
    if (itw != winner_counts.end()) {
      do_first_.winner_unmakeable_boards = itw->second;
    } else {
      do_first_.winner_unmakeable_boards = {0, 0, 0, 0};
    }
  }
  for (auto &[cond, child] : conditions_) {
    if (child) {
      child->ApplyUnmakeableBoardCounts(opp1_counts, opp2_counts,
                                        winner_counts);
    }
  }
}

void ConditionalLine::DeepCloneChildren() {
  for (auto &[cond, child] : conditions_) {
    if (child) {
      child = std::make_shared<ConditionalLine>(*child);
      child->DeepCloneChildren();
    }
  }
}

void ConditionalLine::StripBoardsAtOrAbove(int max_board_id, Suit trump_suit) {
  if (!final_covered_boards_.empty()) {
    final_covered_boards_.erase(final_covered_boards_.lower_bound(max_board_id),
                                final_covered_boards_.end());
    covered_count_ = static_cast<int>(final_covered_boards_.size());
    return;
  }

  if (!do_first_.Empty()) {
    do_first_.StripBoardsAtOrAbove(max_board_id);
  }

  std::vector<std::pair<std::shared_ptr<PlayCondition>,
                        std::shared_ptr<ConditionalLine>>>
      kept;
  kept.reserve(conditions_.size());
  int sum = 0;
  for (auto &[cond, child] : conditions_) {
    if (!child) continue;
    child->StripBoardsAtOrAbove(max_board_id, trump_suit);
    if (child->CoveredFinalBoards() <= 0) continue;
    if (!do_first_.Empty()) {
      if (auto *win_cond = dynamic_cast<WinnerPlayCondition *>(cond.get())) {
        std::set<std::string> real_win_cards;
        for (const auto &entry : do_first_.board_entries) {
          if (entry.winner == win_cond->who()) {
            real_win_cards.emplace(entry.win_card);
          }
        }
        cond = std::make_shared<WinnerPlayCondition>(win_cond->who(),
                                                     real_win_cards);
      }
    }
    sum += child->CoveredFinalBoards();
    kept.emplace_back(std::move(cond), std::move(child));
  }
  for (size_t i = 0; i < kept.size(); ++i) {
    if (!kept[i].second || kept[i].second->do_first_.Empty() ||
        dynamic_cast<TrickPlayCondition *>(kept[i].first.get()) == nullptr) {
      continue;
    }
    GeneralizedTrick &gt = kept[i].second->do_first_;
    std::string old_opp1 = gt.demi1.opp_card;
    std::string old_opp2 = gt.demi2.opp_card;
    gt.TightenToEntries(trump_suit);
    if (gt.demi1.opp_card == old_opp1 && gt.demi2.opp_card == old_opp2) {
      continue;
    }
    std::string new_str = gt.ToString(true);
    bool collides = false;
    for (size_t j = 0; j < kept.size(); ++j) {
      if (j != i && kept[j].second && !kept[j].second->do_first_.Empty() &&
          kept[j].second->do_first_.ToString(true) == new_str) {
        collides = true;
        break;
      }
    }
    if (collides) {
      gt.demi1.opp_card = std::move(old_opp1);
      gt.demi2.opp_card = std::move(old_opp2);
    } else {
      kept[i].first = std::make_shared<TrickPlayCondition>(std::move(new_str));
    }
  }
  conditions_ = std::move(kept);
  covered_count_ = sum;
}

void ConditionalLine::RecomputeCoveredCount() {
  if (conditions_.empty()) {
    covered_count_ = static_cast<int>(final_covered_boards_.size());
    return;
  }
  int sum = 0;
  for (const auto &[_cond, child] : conditions_) {
    if (child) {
      sum += child->CoveredFinalBoards();
    }
  }
  covered_count_ = sum;
}

void ConditionalLine::ApplyRescoredBoards(
    const std::unordered_map<const ConditionalLine *, std::set<int>>
        &leaf_boards,
    const std::unordered_map<const ConditionalLine *,
                             std::map<int, std::string>> &node_board_tricks) {
  if (!has_rescored_boards_) {
    step1_covered_count_ = covered_count_;
  }
  has_rescored_boards_ = true;

  if (!do_first_.Empty()) {
    auto nt_it = node_board_tricks.find(this);
    if (nt_it != node_board_tricks.end()) {
      for (const auto &[b, t] : nt_it->second) {
        do_first_.demi2.board_to_trick[b] = t;
      }
    }
  }

  if (conditions_.empty()) {
    auto it = leaf_boards.find(this);
    if (it != leaf_boards.end()) {
      final_covered_boards_ = it->second;
    } else {
      final_covered_boards_.clear();
    }
    covered_count_ = static_cast<int>(final_covered_boards_.size());
    return;
  }

  final_covered_boards_.clear();
  int sum = 0;
  for (auto &[cond, child] : conditions_) {
    if (child) {
      child->ApplyRescoredBoards(leaf_boards, node_board_tricks);
      sum += child->CoveredFinalBoards();
    }
  }
  covered_count_ = sum;
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
  return has_rescored_boards_ ? ResultsOnHandAdversarial(bs)
                              : ResultsOnHand(bs);
}

ConditionalLine::BoardPlayTrace ConditionalLine::TraceOnBoard(
    const BoardState &bs, int board_id) const {
  BoardPlayTrace trace;
  trace.board_id = board_id;
  BoardState copy = bs;
  const ConditionalLine *cur_line = this;
  bool adversarial = has_rescored_boards_;

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
        std::string def_card_str =
            adversarial ? ChooseAdversarialDefenderCard(copy, pos, candidates,
                                                        nullptr)
                        : FindBestDefenderCard(copy);
        CardType def_card = 0;
        if (!def_card_str.empty()) {
          def_card = Card::ParseStringCard(def_card_str);
        } else {
          def_card = copy.ValidCardsToPlay()[0];
          def_card_str = Card::PrintCard(def_card);
        }
        Suit suit_led =
            (pos == 0) ? Card::DecodeCard(def_card).first : copy.SuitLed();
        copy.PlayCard(p, def_card);
        played_cards.push_back(def_card_str);

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
