#include "conditional_line.h"
#include "glog/logging.h"
#include "interface_dds.h"

#include <algorithm>
#include <functional>
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

std::string PlayerDiscardedCondition::Describe(bool only_choice) const {
  std::string prefix = only_choice ? "when " : "if ";
  std::string dir = PlayerDirectionDebugString(player_);
  std::string seat = dir.empty() ? std::to_string(player_) : dir.substr(0, 1);
  return prefix + seat +
         (discarded_ ? " discarded on " : " did not discard on ") +
         SuitDebugString(suit_);
}

bool PlayerDiscardedCondition::Evaluate(const BoardState &bs) const {
  return bs.HasPlayerDiscardedOnSuit(player_, suit_);
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

static void FillCoveredBoardsBitmaskHelper(
    const ConditionalLine *line_ptr, std::vector<uint64_t> *mask,
    std::vector<const ConditionalLine *> *visited) {
  if (line_ptr == nullptr) {
    return;
  }
  for (const ConditionalLine *v : *visited) {
    if (v == line_ptr) {
      return;
    }
  }
  visited->push_back(line_ptr);
  if (!line_ptr->final_covered_boards().empty()) {
    for (int b : line_ptr->final_covered_boards()) {
      if (b < 0) continue;
      size_t word = static_cast<size_t>(b) >> 6;
      if (word >= mask->size()) {
        mask->resize(word + 1, 0ULL);
      }
      (*mask)[word] |= (1ULL << (static_cast<unsigned>(b) & 63u));
    }
    return;
  }
  for (const auto &[_condition, child_line] : line_ptr->conditions()) {
    if (child_line) {
      FillCoveredBoardsBitmaskHelper(child_line.get(), mask, visited);
    }
  }
}

void ConditionalLine::FillCoveredBoardsBitmask(
    std::vector<uint64_t> *mask) const {
  mask->clear();
  std::vector<const ConditionalLine *> visited;
  visited.reserve(32);
  FillCoveredBoardsBitmaskHelper(this, mask, &visited);
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
std::string SeatLetter(int player) {
  switch ((player % 4 + 4) % 4) {
    case NORTH: return "N";
    case EAST: return "E";
    case SOUTH: return "S";
    case WEST: return "W";
    default: return std::to_string(player);
  }
}

std::string FormatTrickLine(size_t trick_num, int leader,
                            const std::string &trick_str, int winner,
                            Suit trump) {
  std::vector<std::string> cards = SplitWords(trick_str);
  if (cards.size() == 4) {
    int win_idx = -1;
    if (winner >= 0 && winner < 4) {
      win_idx = (winner - leader + 4) % 4;
    } else {
      Suit cur_win_suit = NOTRUMP;
      int cur_win_val = -1;
      win_idx = 0;
      for (int c = 0; c < 4; ++c) {
        if (cards[c].size() != 2) continue;
        CardType ct = Card::ParseStringCard(cards[c]);
        auto [s, v] = Card::DecodeCard(ct);
        if (c == 0) {
          cur_win_suit = s;
          cur_win_val = v;
          win_idx = 0;
        } else if (s == cur_win_suit) {
          if (v > cur_win_val) {
            cur_win_val = v;
            win_idx = c;
          }
        } else if (s == trump) {
          cur_win_suit = s;
          cur_win_val = v;
          win_idx = c;
        }
      }
    }
    cards[win_idx] += "*";
    return "  #" + std::to_string(trick_num) + " (" + SeatLetter(leader) +
           ") " + cards[0] + " " + cards[1] + " " + cards[2] + " " +
           cards[3] + "\n";
  }
  return "  #" + std::to_string(trick_num) + " (" + SeatLetter(leader) + ") " +
         trick_str + "\n";
}
}  // namespace

std::map<int, std::string> ConditionalLine::DeclarerPlaySignature() const {
  std::map<int, std::string> ans;
  DeclarerPlaySignatureHelper(*this, ans);
  return ans;
}

std::string ConditionalLine::ExamplePlay(bool success) const {
  Suit trump = has_sample_board_ ? sample_board_.GetTrump() : NOTRUMP;
  if (!success) {
    std::set<int> covered = CoveredBoardsSet();
    std::vector<const BoardPlayTrace *> current;
    for (const auto &[b, tr] : board_traces_) {
      if (covered.count(b) == 0 && !tr.steps.empty()) {
        current.push_back(&tr);
      }
    }
    if (current.size() <= 1) {
      return "";
    }
    for (size_t step_idx = 0; !current.empty(); ++step_idx) {
      struct Stage1Group {
        const ConditionalLine *node = nullptr;
        int unmatched_seat = -1;
        std::string card_key;
        int winner = -1;
        bool terminates = false;
        std::vector<const BoardPlayTrace *> traces;
      };
      std::vector<Stage1Group> stage1_groups;
      for (const BoardPlayTrace *tr : current) {
        if (step_idx >= tr->steps.size()) continue;
        const PlayedTrickStep &step = tr->steps[step_idx];
        bool term = false;
        std::string card_key;
        int win_key = -1;
        if (step.unmatched_seat >= 0) {
          term = true;
          win_key = step.winner;
          Suit suit_led = step.trick_str.empty()
                              ? NOTRUMP
                              : Card::ParseSuit(step.trick_str[0]);
          Suit card_suit = step.unmatched_card.empty()
                               ? NOTRUMP
                               : Card::ParseSuit(step.unmatched_card[0]);
          bool is_discard = (step.unmatched_seat != step.leader &&
                             suit_led != NOTRUMP && card_suit != suit_led &&
                             card_suit != trump);
          card_key = is_discard ? "R_" : step.unmatched_card;
        }
        auto it = std::find_if(
            stage1_groups.begin(), stage1_groups.end(),
            [&](const Stage1Group &g) {
              return g.node == step.node &&
                     g.unmatched_seat == step.unmatched_seat &&
                     g.card_key == card_key && g.winner == win_key;
            });
        if (it == stage1_groups.end()) {
          stage1_groups.push_back(Stage1Group{
              step.node, step.unmatched_seat, card_key, win_key, term, {tr}});
        } else {
          it->traces.push_back(tr);
        }
      }
      if (stage1_groups.empty()) break;
      const Stage1Group *best_s1 = &stage1_groups[0];
      for (size_t i = 1; i < stage1_groups.size(); ++i) {
        if (stage1_groups[i].traces.size() > best_s1->traces.size()) {
          best_s1 = &stage1_groups[i];
        }
      }
      current = best_s1->traces;
      if (best_s1->terminates) {
        break;
      }

      struct Stage2Group {
        bool terminates = false;
        int winner = -1;
        std::vector<const BoardPlayTrace *> traces;
      };
      std::vector<Stage2Group> stage2_groups;
      for (const BoardPlayTrace *tr : current) {
        const PlayedTrickStep &step = tr->steps[step_idx];
        bool term = (step_idx + 1 >= tr->steps.size());
        auto it = std::find_if(
            stage2_groups.begin(), stage2_groups.end(),
            [&](const Stage2Group &g) {
              return g.terminates == term && g.winner == step.winner;
            });
        if (it == stage2_groups.end()) {
          stage2_groups.push_back(Stage2Group{term, step.winner, {tr}});
        } else {
          it->traces.push_back(tr);
        }
      }
      if (stage2_groups.empty()) break;
      const Stage2Group *best_s2 = &stage2_groups[0];
      for (size_t i = 1; i < stage2_groups.size(); ++i) {
        if (stage2_groups[i].traces.size() > best_s2->traces.size()) {
          best_s2 = &stage2_groups[i];
        }
      }
      current = best_s2->traces;
      if (best_s2->terminates) {
        std::vector<std::pair<std::string, std::vector<const BoardPlayTrace *>>>
            card_groups;
        for (const BoardPlayTrace *tr : current) {
          const std::string &wc = tr->steps[step_idx].winning_card;
          auto it = std::find_if(card_groups.begin(), card_groups.end(),
                                 [&](const auto &p) { return p.first == wc; });
          if (it == card_groups.end()) {
            card_groups.push_back({wc, {tr}});
          } else {
            it->second.push_back(tr);
          }
        }
        if (!card_groups.empty()) {
          const auto *best_cg = &card_groups[0].second;
          for (size_t i = 1; i < card_groups.size(); ++i) {
            if (card_groups[i].second.size() > best_cg->size()) {
              best_cg = &card_groups[i].second;
            }
          }
          current = *best_cg;
        }
        break;
      }
    }
    if (current.empty()) {
      return "";
    }
    const BoardPlayTrace *chosen_trace = current[0];
    std::string out = "Example Failed Play (Board " +
                      std::to_string(chosen_trace->board_id) + "):\n";
    for (size_t i = 0; i < chosen_trace->steps.size(); ++i) {
      const PlayedTrickStep &step = chosen_trace->steps[i];
      out += FormatTrickLine(i + 1, step.leader, step.trick_str, step.winner,
                             trump);
    }
    return out;
  }

  struct ActionStep {
    const ConditionalLine *node = nullptr;
    int branch_winner = -1;
  };
  std::vector<ActionStep> action_path;
  const ConditionalLine *curr = this;
  while (curr != nullptr) {
    if (!curr->do_first_.Empty()) {
      action_path.push_back(ActionStep{curr, -1});
    }
    if (curr->conditions_.empty()) {
      break;
    }
    const ConditionalLine *best_child = nullptr;
    const PlayCondition *best_cond = nullptr;
    int best_cov = -1;
    std::string best_desc;
    bool only_choice = (curr->conditions_.size() == 1);
    for (const auto &[cond, child] : curr->conditions_) {
      if (!child) continue;
      int cov = child->CoveredFinalBoards();
      if (cov == 0 && !child->do_first_.Empty()) {
        cov = static_cast<int>(child->do_first_.NumBoards());
      }
      std::string desc = cond ? cond->Describe(only_choice) : "";
      if (best_child == nullptr || cov > best_cov ||
          (cov == best_cov && desc < best_desc)) {
        best_child = child.get();
        best_cond = cond.get();
        best_cov = cov;
        best_desc = std::move(desc);
      }
    }
    if (!action_path.empty() && action_path.back().node == curr) {
      if (auto *wc = dynamic_cast<const WinnerPlayCondition *>(best_cond)) {
        action_path.back().branch_winner = wc->who();
      }
    }
    curr = best_child;
  }

  if (action_path.empty()) {
    return "";
  }

  auto get_trick_for_board =
      [&](const ActionStep &step_info, int b) -> std::pair<std::string, int> {
    const ConditionalLine *node = step_info.node;
    auto tr_it = board_traces_.find(b);
    if (tr_it != board_traces_.end()) {
      for (const auto &step : tr_it->second.steps) {
        if (step.node == node && !step.trick_str.empty()) {
          return {step.trick_str, step.winner};
        }
      }
    }
    const auto &gt = node->do_first_;
    for (const auto &entry : gt.board_entries) {
      if (entry.board == b && !entry.trick_str.empty()) {
        return {entry.trick_str, static_cast<int>(entry.winner)};
      }
    }
    auto it = gt.demi2.board_to_trick.find(b);
    if (it != gt.demi2.board_to_trick.end() && !it->second.empty()) {
      return {it->second, step_info.branch_winner};
    }
    return {"", -1};
  };

  std::vector<int> candidates;
  if (curr != nullptr && !curr->final_covered_boards_.empty()) {
    candidates.assign(curr->final_covered_boards_.begin(),
                      curr->final_covered_boards_.end());
  }
  std::set<int> fallback_set = action_path.back().node->CoveredBoardsSet();
  for (int b : fallback_set) {
    if (std::find(candidates.begin(), candidates.end(), b) ==
        candidates.end()) {
      candidates.push_back(b);
    }
  }
  for (int b : action_path.back().node->do_first_.boards) {
    if (std::find(candidates.begin(), candidates.end(), b) ==
        candidates.end()) {
      candidates.push_back(b);
    }
  }

  int chosen_board = -1;
  std::vector<std::pair<std::string, int>> chosen_tricks;
  for (int b : candidates) {
    std::vector<std::pair<std::string, int>> tricks;
    tricks.reserve(action_path.size());
    bool all_found = true;
    for (const ActionStep &step_info : action_path) {
      auto t = get_trick_for_board(step_info, b);
      if (t.first.empty()) {
        all_found = false;
        break;
      }
      tricks.push_back(std::move(t));
    }
    if (all_found) {
      chosen_board = b;
      chosen_tricks = std::move(tricks);
      break;
    }
  }

  if (chosen_board < 0 || chosen_tricks.empty()) {
    return "";
  }

  std::string out =
      "Example Play (Board " + std::to_string(chosen_board) + "):\n";
  for (size_t i = 0; i < chosen_tricks.size(); ++i) {
    out += FormatTrickLine(i + 1, action_path[i].node->do_first_.leader,
                           chosen_tricks[i].first, chosen_tricks[i].second,
                           trump);
  }
  return out;
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
    std::string example = ExamplePlay(true);
    if (!example.empty()) {
      ans += example;
    }
    std::string failed_example = ExamplePlay(false);
    if (!failed_example.empty()) {
      ans += failed_example;
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

int PatternRankVal(char r) {
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

uint16_t InSuitPatternRankMask(const std::string &c) {
  if (c.size() == 2) {
    char r = c[1];
    if (r == '_') return 0x1fff;
    if (r == 'x') return 0x00ff;
    if (r == 'X') return 0x1f00;
    int rv = PatternRankVal(r);
    return (rv >= 0) ? static_cast<uint16_t>(1u << rv) : 0;
  }
  if (c.size() == 3 && c[1] == '<') {
    int rv = PatternRankVal(c[2]);
    if (rv > 0) {
      return static_cast<uint16_t>((1u << rv) - 1u);
    }
  }
  return 0;
}

int PatternSpecificity(const std::string &pattern) {
  if (pattern == "__") return 1;
  if (pattern == "R_") return 2;
  if (pattern.size() == 2) {
    if (pattern[1] == '_') return 3;
    if (pattern[1] == 'x' || pattern[1] == 'X') return 8;
    return 9;
  }
  if (pattern.size() == 3 && pattern[1] == '<') {
    int rv = PatternRankVal(pattern[2]);
    // 'A' (12) -> 4, 'K' (11) -> 5, 'Q' (10) -> 6, 'J' (9) -> 7.
    if (rv >= 9 && rv <= 12) return 16 - rv;
  }
  return 0;
}

// Returns true if the card patterns can cover the same card.
bool CardsAreRedundant(const std::string &c1, const std::string &c2) {
  if (c1.size() < 2 || c2.size() < 2) {
    return c1 == c2;
  }
  // 'R' is only redundant with another discard.
  if (c1[0] == 'R' || c2[0] == 'R') {
    return c1[0] == c2[0];
  }
  if (c1 == "__" || c2 == "__") {
    return true;
  }
  if (c1[0] != c2[0] && c1[0] != '_' && c2[0] != '_') {
    return false;
  }
  return (InSuitPatternRankMask(c1) & InSuitPatternRankMask(c2)) != 0;
}

Suit GetSuitOfCard(const std::string &c) {
  CHECK(c.size() == 2 || (c.size() == 3 && c[1] == '<')) << c;
  if (c[0] == '_') {
    return NOTRUMP;
  }
  return Card::ParseSuit(c[0]);
}

std::vector<std::string> GeneralizeOppCard(const std::string &card,
                                           Suit suit_led, Suit trump,
                                           uint8_t upper_honor_mask = 0x0f) {
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

    int rv = PatternRankVal(card[1]);
    static constexpr std::pair<char, int> kUpperHonors[] = {
        {'A', 12}, {'K', 11}, {'Q', 10}, {'J', 9}};
    for (const auto &[h_ch, h_rv] : kUpperHonors) {
      if ((upper_honor_mask & (1u << (h_rv - 9))) != 0 &&
          rv >= 0 && rv < h_rv) {
        std::string lt_pat(1, card[0]);
        lt_pat += '<';
        lt_pat += h_ch;
        cards.push_back(std::move(lt_pat));
      }
    }

    std::string suited = card;
    suited[1] = '_';
    cards.push_back(suited);
  } else {
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
  if ((gen_card.size() != 2 &&
       !(gen_card.size() == 3 && gen_card[1] == '<')) ||
      actual_card.size() < 2) {
    return false;
  }
  char actual_suit = actual_card[0];
  char gen_suit = gen_card[0];
  if (gen_card == "__") {
    return true;
  }
  Suit card_suit = GetSuitOfCard(actual_card);
  bool is_discard =
      (suit_led != NOTRUMP && card_suit != suit_led && card_suit != trump);
  if (!is_discard && gen_suit == actual_suit) {
    return (InSuitPatternRankMask(actual_card) &
            InSuitPatternRankMask(gen_card)) != 0;
  }
  if (gen_card == "R_") {
    return is_discard;
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
    uint16_t rank_mask = 0;
  };
  uint8_t honors_by_suit[4] = {0, 0, 0, 0};
  for (const RawPlayBucket &b : buckets_) {
    if (b.opp_card.size() < 2) continue;
    Suit suit_led =
        has_by_suit_
            ? b.suit_led
            : (known_suit_led != NOTRUMP
                   ? known_suit_led
                   : (leader % 2 == 0 ? GetSuitOfCard(b.my_card)
                                      : GetSuitOfCard(b.opp_card)));
    Suit cs = GetSuitOfCard(b.opp_card);
    bool is_discard =
        (suit_led != NOTRUMP && cs != suit_led && cs != trump_suit_);
    if (!is_discard && cs >= 0 && cs < 4) {
      int rv = PatternRankVal(b.opp_card[1]);
      if (rv >= 9 && rv <= 12) {
        honors_by_suit[cs] |= static_cast<uint8_t>(1u << (rv - 9));
      }
    }
  }
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
    Suit cs = b.opp_card.size() >= 2 ? GetSuitOfCard(b.opp_card) : NOTRUMP;
    bool is_discard =
        (suit_led != NOTRUMP && cs != suit_led && cs != trump_suit_);
    uint16_t b_rank_mask =
        (b.opp_card.size() >= 2 && !is_discard)
            ? InSuitPatternRankMask(b.opp_card)
            : 0;
    uint8_t h_mask = (cs >= 0 && cs < 4) ? honors_by_suit[cs] : 0;
    auto opp_card_choices =
        GeneralizeOppCard(b.opp_card, suit_led, trump_suit_, h_mask);
    for (const auto &choice : opp_card_choices) {
      GenBucket *dest = nullptr;
      for (GenBucket &gb : generalized) {
        if (gb.opp_card == choice && gb.my_card == b.my_card) {
          dest = &gb;
          break;
        }
      }
      if (dest == nullptr) {
        generalized.push_back(
            GenBucket{choice, b.my_card, b.boards, b_rank_mask});
      } else {
        dest->boards.insert(dest->boards.end(), b.boards.begin(),
                            b.boards.end());
        dest->rank_mask |= b_rank_mask;
      }
    }
  }
  generalized.erase(
      std::remove_if(generalized.begin(), generalized.end(),
                     [](const GenBucket &gb) {
                       return gb.opp_card.size() == 3 &&
                              ((gb.rank_mask & 0x00ff) == 0 ||
                               (gb.rank_mask & 0x0f00) == 0);
                     }),
      generalized.end());
  std::sort(generalized.begin(), generalized.end(),
            [](const GenBucket &a, const GenBucket &b) {
              if (a.opp_card.size() != b.opp_card.size()) {
                return a.opp_card.size() < b.opp_card.size();
              }
              if (a.opp_card.size() == 3 && a.opp_card[0] == b.opp_card[0]) {
                int ra = PatternRankVal(a.opp_card[2]);
                int rb = PatternRankVal(b.opp_card[2]);
                if (ra != rb) return ra > rb;
              }
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
          (CardsAreRedundant(ans[k].opp_card, gb.opp_card) ||
           (ans[k].opp_card.size() == 2 && gb.opp_card.size() == 2 &&
            ans[k].opp_card[0] == gb.opp_card[0] &&
            ans[k].opp_card[1] == 'X' && gb.opp_card[1] == 'x')) &&
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
    if (c > 0 && s != rt.suit_led && s != trump_suit_) {
      int seat = (leader + c) % 4;
      rt.discard_mask |= static_cast<uint16_t>(
          1u << (seat * 4 + static_cast<int>(rt.suit_led)));
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
  uint8_t d1_honors_by_suit[4] = {0, 0, 0, 0};
  for (const auto &rb : raw_d1_to_tricks) {
    if (rb.opp1.size() < 2) continue;
    Suit suit_led =
        (leader_ % 2 == 0) ? GetSuitOfCard(rb.my1) : GetSuitOfCard(rb.opp1);
    Suit cs = GetSuitOfCard(rb.opp1);
    bool is_discard =
        (suit_led != NOTRUMP && cs != suit_led && cs != trump_suit_);
    if (!is_discard && cs >= 0 && cs < 4) {
      int rv = PatternRankVal(rb.opp1[1]);
      if (rv >= 9 && rv <= 12) {
        d1_honors_by_suit[cs] |= static_cast<uint8_t>(1u << (rv - 9));
      }
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
    Suit cs = rb.opp1.size() >= 2 ? GetSuitOfCard(rb.opp1) : NOTRUMP;
    uint8_t h_mask = (cs >= 0 && cs < 4) ? d1_honors_by_suit[cs] : 0;
    auto opp1_choices =
        GeneralizeOppCard(rb.opp1, suit_led, trump_suit_, h_mask);
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
      const auto &d2 = demi2_plays[d2_idx];
      if (leader_ % 2 == 0 && d2.opp_card.size() == 3) {
        continue;
      }
      int inter_count = static_cast<int>(d2.boards_vec.size());
      if (inter_count > 0 && d1.opp_card.size() == 3 && tricks_vec != nullptr) {
        uint16_t opp1_mask = 0;
        for (unsigned int i : *tricks_vec) {
          const RecordedTrick &rt = tricks_[i];
          const std::string &my2 =
              (leader_ % 2 == 0) ? rt.cards[2] : rt.cards[3];
          if (my2 != d2.my_card) continue;
          const std::string &opp2 =
              (leader_ % 2 == 0) ? rt.cards[3] : rt.cards[2];
          if (!GeneralizesTo(opp2, d2.opp_card, rt.suit_led, trump_suit_)) {
            continue;
          }
          const std::string &opp1 =
              (leader_ % 2 == 0) ? rt.cards[1] : rt.cards[0];
          opp1_mask |= InSuitPatternRankMask(opp1);
        }
        if ((opp1_mask & 0x00ff) == 0 || (opp1_mask & 0x0f00) == 0) {
          continue;
        }
      }
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
        entry.discard_mask = rt.discard_mask;
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

struct TrickOppPatternMask {
  bool computed = false;
  bool always_true = false;
  bool has_any = false;
  bool has_discard = false;
  uint16_t suit_mask[4] = {0, 0, 0, 0};

  bool Matches(const std::string &specific_pattern) const {
    if (always_true) return true;
    if (specific_pattern == "__") return has_any;
    if (specific_pattern == "R_") return has_discard;
    if (specific_pattern.size() < 2) return false;
    Suit s = Card::ParseSuit(specific_pattern[0]);
    if (s < 0 || s >= 4) return false;
    uint16_t pat_mask = InSuitPatternRankMask(specific_pattern);
    return (suit_mask[s] & pat_mask) != 0;
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
    int rv = PatternRankVal(opp_rank);
    if (rv >= 0) {
      mask->suit_mask[card_suit] |= static_cast<uint16_t>(1u << rv);
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

bool OppSlotDistinguishable(const GeneralizedTrick &trick1,
                            const GeneralizedTrick &trick2,
                            const std::string &opp1,
                            const std::string &opp2,
                            int opp_pos,
                            TrickOppPatternMask *mask1,
                            TrickOppPatternMask *mask2) {
  bool is_any_vs_discard =
      (opp1 == "__" && opp2 == "R_") || (opp1 == "R_" && opp2 == "__");
  if (!CardsAreRedundant(opp1, opp2) && !is_any_vs_discard) {
    return true;
  }
  if (trick1.leader % 2 == 0) {
    return false;
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
    return !BuildTrickOppPatternMask(trick1, opp_pos).Matches(opp2);
  }
  if (spec2 < spec1) {
    if (mask2 != nullptr) {
      if (!mask2->computed) {
        *mask2 = BuildTrickOppPatternMask(trick2, opp_pos);
      }
      return !mask2->Matches(opp1);
    }
    return !BuildTrickOppPatternMask(trick2, opp_pos).Matches(opp1);
  }
  return false;
}

bool AreCardsDistinguishable(
    const GeneralizedTrick &trick1,
    const GeneralizedTrick &trick2,
    std::array<TrickOppPatternMask, 2> *masks1,
    std::array<TrickOppPatternMask, 2> *masks2,
    int my1_id1, int my1_id2,
    int my2_id1, int my2_id2) {
  bool same_my1 = (my1_id1 == my1_id2);
  bool same_my2 = (my2_id1 == my2_id2);
  if (trick1.leader % 2 == 0) {
    if (!same_my1 || same_my2) {
      return false;
    }
    return OppSlotDistinguishable(trick1, trick2, trick1.demi1.opp_card,
                                  trick2.demi1.opp_card, 1,
                                  masks1 ? &(*masks1)[0] : nullptr,
                                  masks2 ? &(*masks2)[0] : nullptr);
  } else {
    if (OppSlotDistinguishable(trick1, trick2, trick1.demi1.opp_card,
                               trick2.demi1.opp_card, 0,
                               masks1 ? &(*masks1)[0] : nullptr,
                               masks2 ? &(*masks2)[0] : nullptr)) {
      return !same_my1 || !same_my2 ||
             trick1.demi1.opp_card[0] != trick2.demi1.opp_card[0];
    }
    if (!same_my1 || same_my2) {
      return false;
    }
    return OppSlotDistinguishable(trick1, trick2, trick1.demi2.opp_card,
                                  trick2.demi2.opp_card, 2,
                                  masks1 ? &(*masks1)[1] : nullptr,
                                  masks2 ? &(*masks2)[1] : nullptr);
  }
}

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
      auto pattern_subsumes = [](const std::string &sup,
                                 const std::string &sub) {
        if (sup == sub || sup == "__") return true;
        if (sup.size() >= 2 && sub.size() >= 2 && sup[0] == sub[0] &&
            sup[0] != 'R' && sup[0] != '_') {
          uint16_t sup_m = InSuitPatternRankMask(sup);
          uint16_t sub_m = InSuitPatternRankMask(sub);
          return sub_m != 0 && (sub_m & ~sup_m) == 0;
        }
        return false;
      };
      auto group_subsumes = [&](const std::vector<int> &sup_grp,
                                const std::vector<int> &sub_grp) {
        for (int b_idx : sub_grp) {
          const uint64_t *mb = &masks[b_idx * num_words];
          bool found = false;
          for (int a_idx : sup_grp) {
            if (my1_id[a_idx] != my1_id[b_idx] ||
                my2_id[a_idx] != my2_id[b_idx]) {
              continue;
            }
            if (!pattern_subsumes(input[a_idx].demi1.opp_card,
                                  input[b_idx].demi1.opp_card) ||
                !pattern_subsumes(input[a_idx].demi2.opp_card,
                                  input[b_idx].demi2.opp_card)) {
              continue;
            }
            const uint64_t *ma = &masks[a_idx * num_words];
            bool mask_sub = true;
            for (size_t w = 0; w < num_words; ++w) {
              if ((mb[w] & ~ma[w]) != 0) {
                mask_sub = false;
                break;
              }
            }
            if (mask_sub) {
              found = true;
              break;
            }
          }
          if (!found) return false;
        }
        return true;
      };
      bool dominated = false;
      for (const auto &existing_grp : group_idx_list) {
        if (group_subsumes(existing_grp, group_indices)) {
          dominated = true;
          break;
        }
      }
      for (int idx : group_indices) {
        int kid = key_id[idx];
        if (!key_grouped[kid]) {
          key_grouped[kid] = 1;
          --ungrouped_keys;
          if (!def_leads) {
            --ungrouped_by_my1[my1_id[idx]];
          }
        }
      }
      if (!dominated) {
        for (int idx : group_indices) {
          ref_count[idx]++;
        }
        group_idx_list.push_back(group_indices);
      }
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

bool AreCandidateDeclarerCardsLegalForTrick(const BoardState &bs,
                                            const ConditionalLine *cand,
                                            int from_pos, Suit suit_led) {
  if (suit_led == NOTRUMP || suit_led < 0 || suit_led >= 4) {
    return true;
  }
  const GeneralizedTrick &gt = cand->do_first();
  int trick_leader = bs.cur_trick_leader();
  uint64_t played_mask = bs.played_cards();
  for (int i = 0; i < bs.CardsPlayedThisTrick(); ++i) {
    played_mask |= bs.trick_cards_played()[i];
  }
  uint64_t suit_bits = 0xffffULL << (static_cast<int>(suit_led) * 16);
  for (int d_pos = std::max(1, from_pos); d_pos < 4; ++d_pos) {
    int seat = (trick_leader + d_pos) % 4;
    if (seat % 2 != 0) continue;
    std::string decl_str = GetPatternAtPos(gt, d_pos);
    if (decl_str.size() < 2) continue;
    CardType decl_card = Card::ParseStringCard(decl_str);
    Suit decl_suit = Card::DecodeCard(decl_card).first;
    uint64_t hand_rem = bs.GetHand(seat) & (~played_mask);
    if (decl_suit != suit_led && (hand_rem & suit_bits) != 0) {
      return false;
    }
    played_mask |= decl_card;
  }
  return true;
}

std::vector<const ConditionalLine *> FilterCandidatesByDefenderCard(
    const std::vector<const ConditionalLine *> &candidates,
    const std::string &def_card_str, int pos, Suit suit_led, Suit trump,
    const BoardState *bs = nullptr) {
  std::vector<const ConditionalLine *> matching;
  for (const ConditionalLine *cand : candidates) {
    std::string pattern = GetPatternAtPos(cand->do_first(), pos);
    if (!GeneralizesTo(def_card_str, pattern, suit_led, trump)) {
      continue;
    }
    if (bs != nullptr &&
        !AreCandidateDeclarerCardsLegalForTrick(*bs, cand, pos + 1, suit_led)) {
      continue;
    }
    matching.push_back(cand);
  }
  return matching;
}

std::vector<const ConditionalLine *> FilterCandidatesByDeclarerCard(
    const std::vector<const ConditionalLine *> &candidates,
    const std::string &decl_card_str, int pos) {
  std::vector<const ConditionalLine *> matching;
  for (const ConditionalLine *cand : candidates) {
    if (GetPatternAtPos(cand->do_first(), pos) == decl_card_str) {
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

std::vector<const ConditionalLine *> KeepMostSpecificCandidates(
    const std::vector<const ConditionalLine *> &matching, int up_to_pos) {
  if (matching.empty()) return {};
  auto best_spec = CandidateSpecificityScore(matching[0], up_to_pos);
  for (size_t i = 1; i < matching.size(); ++i) {
    auto s = CandidateSpecificityScore(matching[i], up_to_pos);
    if (s > best_spec) {
      best_spec = s;
    }
  }
  std::vector<const ConditionalLine *> top_matching;
  top_matching.reserve(matching.size());
  for (const ConditionalLine *cand : matching) {
    if (CandidateSpecificityScore(cand, up_to_pos) == best_spec) {
      top_matching.push_back(cand);
    }
  }
  return top_matching;
}

const ConditionalLine *SelectMostSpecificCandidate(
    const std::vector<const ConditionalLine *> &candidates, int up_to_pos,
    const BoardState *bs = nullptr, Suit suit_led = NOTRUMP) {
  CHECK(!candidates.empty());
  const ConditionalLine *best = nullptr;
  auto best_score = CandidateSpecificityScore(candidates[0], up_to_pos);
  if (bs != nullptr) {
    for (const ConditionalLine *cand : candidates) {
      if (!AreCandidateDeclarerCardsLegalForTrick(*bs, cand, up_to_pos,
                                                  suit_led)) {
        continue;
      }
      auto s = CandidateSpecificityScore(cand, up_to_pos);
      if (best == nullptr || s > best_score) {
        best = cand;
        best_score = s;
      }
    }
    if (best != nullptr) {
      return best;
    }
  }
  best = candidates[0];
  best_score = CandidateSpecificityScore(best, up_to_pos);
  for (size_t i = 1; i < candidates.size(); ++i) {
    auto s = CandidateSpecificityScore(candidates[i], up_to_pos);
    if (s > best_score) {
      best_score = s;
      best = candidates[i];
    }
  }
  return best;
}

const ConditionalLine *AdvanceThroughStateConditions(
    const ConditionalLine *cur_line, const BoardState &bs) {
  while (cur_line && cur_line->do_first().Empty()) {
    const ConditionalLine *good_branch = nullptr;
    for (const auto &[cond, child_line] : cur_line->conditions()) {
      if (!child_line || !cond) continue;
      if (cond->IsStateCondition() && cond->EvaluateState(bs)) {
        good_branch = child_line.get();
        break;
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
  return cur_line;
}

const ConditionalLine *MatchWinnerContinuation(
    const ConditionalLine *chosen_node,
    const std::vector<const ConditionalLine *> &candidates, int winner,
    const ConditionalLine **out_leaf, const ConditionalLine **out_next_line) {
  auto try_match = [&](const ConditionalLine *node) {
    for (const auto &[cond, child_line] : node->conditions()) {
      if (!child_line) continue;
      auto *winner_cond = dynamic_cast<const WinnerPlayCondition *>(cond.get());
      if (winner_cond && winner_cond->who() == winner) {
        if (child_line->do_first().Empty() &&
            child_line->conditions().empty()) {
          *out_leaf = child_line.get();
        } else {
          *out_next_line = child_line.get();
        }
        return true;
      }
    }
    return false;
  };
  if (try_match(chosen_node)) {
    return chosen_node;
  }
  for (const ConditionalLine *cand : candidates) {
    if (cand != chosen_node && try_match(cand)) {
      return cand;
    }
  }
  return chosen_node;
}

ConditionalLine::DefenderStateKey MakeDefenderStateKey(const BoardState &bs) {
  uint64_t played = bs.played_cards();
  uint64_t cur_bits =
      (static_cast<uint64_t>(bs.cur_trick_leader() & 3) << 13) |
      (static_cast<uint64_t>(bs.CardsPlayedThisTrick() & 3) << 29);
  for (int i = 0; i < bs.CardsPlayedThisTrick(); ++i) {
    cur_bits |= bs.trick_cards_played()[i];
  }
  return {played, cur_bits, bs.GetHand(1) & ~played, bs.GetHand(3) & ~played};
}

bool IsSingleCandidateCompatibleAtLead(const BoardState &bs,
                                       const ConditionalLine *cand,
                                       const std::string &def_card_str,
                                       Suit suit_led) {
  std::string pat0 = GetPatternAtPos(cand->do_first(), 0);
  if (pat0 != "__" &&
      (pat0.empty() || def_card_str.empty() || pat0[0] != def_card_str[0])) {
    return false;
  }
  return AreCandidateDeclarerCardsLegalForTrick(bs, cand, 1, suit_led);
}

int ComputeUnclampedFinalNsTricks(const BoardState &bs) {
  if (bs.Finished()) {
    return bs.tricks_won_even();
  }
  int rem_tricks =
      __builtin_popcountll(bs.GetHand(bs.CurPlayer()) & ~bs.played_cards());
  if (rem_tricks <= 0) {
    return bs.tricks_won_even();
  }
  int opt = FindOptimalTricks(bs);
  int rem_ns = (bs.CurPlayer() % 2 == 0) ? opt : (rem_tricks - opt);
  return bs.tricks_won_even() + rem_ns;
}

using SubtreeTierKey = std::tuple<const ConditionalLine *, uint64_t, uint32_t>;
using SubtreeTierCache = std::map<SubtreeTierKey, int>;

ConditionalLine::BoardPlayTrace SimulateBoardImpl(
    const ConditionalLine *root_line, const BoardState &bs, int board_id,
    bool adversarial, int root_min_tricks, bool clamp_missing_leaf,
    ConditionalLine::DefenderCardsCache *def_cache,
    SubtreeTierCache *tier_cache);

double BranchSuccessProbability(
    const BoardState &copy, int pos, const std::string &card_str,
    const std::vector<const ConditionalLine *> &candidates) {
  CardType card_val = Card::ParseStringCard(card_str);
  Suit suit_led =
      (pos == 0) ? Card::DecodeCard(card_val).first : copy.SuitLed();
  std::vector<const ConditionalLine *> matching =
      FilterCandidatesByDefenderCard(
          candidates, card_str, pos, suit_led, copy.GetTrump(), &copy);
  if (matching.empty() && candidates.size() == 1 && pos == 0 &&
      IsSingleCandidateCompatibleAtLead(copy, candidates[0], card_str,
                                        suit_led)) {
    matching.push_back(candidates[0]);
  }
  if (matching.empty()) {
    return -1.0;
  }
  std::vector<const ConditionalLine *> top_matching =
      KeepMostSpecificCandidates(matching, pos + 1);
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
    int root_min_tricks, ConditionalLine::DefenderCardsCache *def_cache,
    SubtreeTierCache *tier_cache,
    ConditionalLine::DefenderEvalCache *eval_cache = nullptr,
    bool *out_guaranteed_defeat = nullptr);

int EvaluateSubtreeTierFast(const ConditionalLine *root_line,
                            const BoardState &bs, int root_min_tricks,
                            ConditionalLine::DefenderEvalCache *eval_cache,
                            SubtreeTierCache *tier_cache);

struct TrickContinuationOutcome {
  bool reached_leaf = false;
  const ConditionalLine *target_node = nullptr;
  BoardState next_bs;
};

TrickContinuationOutcome EvaluateTrickContinuation(
    const BoardState &copy, int pos, const std::string &card_str,
    const std::vector<const ConditionalLine *> &candidates,
    int root_min_tricks, ConditionalLine::DefenderCardsCache *def_cache,
    SubtreeTierCache *tier_cache,
    ConditionalLine::DefenderEvalCache *eval_cache = nullptr) {
  BoardState sim_bs = copy;
  std::vector<const ConditionalLine *> sim_cands = candidates;
  CardType def_card = Card::ParseStringCard(card_str);
  Suit suit_led =
      (pos == 0) ? Card::DecodeCard(def_card).first : sim_bs.SuitLed();
  sim_bs.PlayCard(sim_bs.CurPlayer(), def_card);
  if (sim_cands.size() > 1) {
    std::vector<const ConditionalLine *> matching =
        FilterCandidatesByDefenderCard(sim_cands, card_str, pos, suit_led,
                                       sim_bs.GetTrump(), &sim_bs);
    if (matching.empty()) {
      return {false, nullptr, sim_bs};
    }
    sim_cands = std::move(matching);
  } else if (pos == 0 && !sim_cands.empty() &&
             !IsSingleCandidateCompatibleAtLead(sim_bs, sim_cands[0], card_str,
                                                suit_led)) {
    return {false, nullptr, sim_bs};
  }

  for (int rem_pos = pos + 1; rem_pos < 4; ++rem_pos) {
    if (sim_bs.Finished()) break;
    int rem_p = sim_bs.CurPlayer();
    if (rem_p % 2 == 0) {
      const ConditionalLine *best_cand =
          SelectMostSpecificCandidate(sim_cands, rem_pos);
      std::string decl_card_str =
          GetPatternAtPos(best_cand->do_first(), rem_pos);
      CardType chosen_card = ChooseValidCard(sim_bs, decl_card_str);
      sim_bs.PlayCard(rem_p, chosen_card);
      if (sim_cands.size() > 1) {
        auto matching =
            FilterCandidatesByDeclarerCard(sim_cands, decl_card_str, rem_pos);
        if (!matching.empty()) {
          sim_cands = std::move(matching);
        }
      }
    } else {
      std::string next_def_str = ChooseAdversarialDefenderCard(
          sim_bs, rem_pos, sim_cands, root_min_tricks, def_cache, tier_cache,
          eval_cache);
      CardType next_def_card =
          !next_def_str.empty() ? Card::ParseStringCard(next_def_str)
                                : sim_bs.ValidCardsToPlay()[0];
      if (next_def_str.empty()) {
        next_def_str = Card::PrintCard(next_def_card);
      }
      Suit rem_suit_led = sim_bs.SuitLed();
      sim_bs.PlayCard(rem_p, next_def_card);
      if (sim_cands.size() > 1) {
        std::vector<const ConditionalLine *> matching =
            FilterCandidatesByDefenderCard(sim_cands, next_def_str, rem_pos,
                                           rem_suit_led, sim_bs.GetTrump(),
                                           &sim_bs);
        if (matching.empty()) {
          return {false, nullptr, sim_bs};
        }
        sim_cands = std::move(matching);
      }
    }
  }

  int winner = sim_bs.CurPlayer();
  const ConditionalLine *chosen_node =
      SelectMostSpecificCandidate(sim_cands, 4);
  if (sim_bs.Finished() && chosen_node->conditions().empty()) {
    return {true, chosen_node, sim_bs};
  }
  const ConditionalLine *next_line = nullptr;
  const ConditionalLine *leaf = nullptr;
  MatchWinnerContinuation(chosen_node, sim_cands, winner, &leaf, &next_line);
  if (leaf != nullptr) {
    return {true, leaf, sim_bs};
  }
  if (next_line == nullptr || sim_bs.Finished()) {
    return {false, nullptr, sim_bs};
  }
  next_line = AdvanceThroughStateConditions(next_line, sim_bs);
  if (next_line && next_line->do_first().Empty() &&
      next_line->conditions().empty()) {
    return {true, next_line, sim_bs};
  }
  return {false, next_line, sim_bs};
}

// Ranks a candidate defender continuation into three tiers (lower = preferred
// by the defender):
//   0: Genuine defeat (`leaf == nullptr` and unclamped N/S tricks <
//      `root_min_tricks`, or `root_min_tricks <= 0`).
//   1: Declarer makes and reaches an existing tree leaf (`leaf != nullptr`).
//   2: Missing Step-1 branch (`leaf == nullptr`), while declarer still has
//      `>= root_min_tricks` double-dummy. Ranked worse than Tier 1 so missing
//      edges do not trick the defender into pointless deviations.
int ClassifyContinuationTier(
    const TrickContinuationOutcome &out, int root_min_tricks,
    ConditionalLine::DefenderCardsCache *def_cache,
    SubtreeTierCache *tier_cache,
    ConditionalLine::DefenderEvalCache *eval_cache = nullptr) {
  if (out.reached_leaf) {
    return 1;
  }
  if (out.target_node == nullptr) {
    if (root_min_tricks <= 0) {
      return 0;
    }
    bool called_dds = false;
    bool reuse = (eval_cache != nullptr && eval_cache->tt_initialized);
    bool makeable =
        IsDealMakeable(out.next_bs, root_min_tricks, &called_dds, reuse);
    if (called_dds && eval_cache != nullptr) {
      eval_cache->tt_initialized = true;
    }
    return makeable ? 2 : 0;
  }
  uint32_t meta =
      static_cast<uint32_t>(out.next_bs.CurPlayer() & 3) |
      (static_cast<uint32_t>(out.next_bs.tricks_won_even() & 0xff) << 2) |
      (static_cast<uint32_t>(std::max(0, root_min_tricks) & 0xff) << 10);
  SubtreeTierKey cache_key{out.target_node, out.next_bs.played_cards(), meta};
  if (tier_cache != nullptr) {
    auto it = tier_cache->find(cache_key);
    if (it != tier_cache->end()) {
      return it->second;
    }
  }
  int tier = 1;
  if (eval_cache != nullptr) {
    tier = EvaluateSubtreeTierFast(out.target_node, out.next_bs,
                                   root_min_tricks, eval_cache, tier_cache);
  } else {
    ConditionalLine::BoardPlayTrace sub_trace =
        SimulateBoardImpl(out.target_node, out.next_bs, /*board_id=*/-1,
                          /*adversarial=*/true, root_min_tricks,
                          /*clamp_missing_leaf=*/false, def_cache, tier_cache);
    if (sub_trace.leaf == nullptr) {
      tier =
          (root_min_tricks <= 0 || sub_trace.final_ns_tricks < root_min_tricks)
              ? 0
              : 2;
    }
  }
  if (tier_cache != nullptr) {
    (*tier_cache)[cache_key] = tier;
  }
  return tier;
}

std::string ChooseAdversarialDefenderCard(
    const BoardState &copy, int pos,
    const std::vector<const ConditionalLine *> &candidates,
    int root_min_tricks, ConditionalLine::DefenderCardsCache *def_cache,
    SubtreeTierCache *tier_cache,
    ConditionalLine::DefenderEvalCache *eval_cache,
    bool *out_guaranteed_defeat) {
  if (out_guaranteed_defeat != nullptr) {
    *out_guaranteed_defeat = false;
  }
  const std::vector<std::string> *opt_ptr = nullptr;
  std::vector<std::string> local_opt;
  int max_def_tricks = -1;
  if (eval_cache != nullptr) {
    ConditionalLine::DefenderStateKey state_key = MakeDefenderStateKey(copy);
    auto it = eval_cache->entries.find(state_key);
    if (it == eval_cache->entries.end()) {
      int computed_max = -1;
      auto cards = FindOptimalDefenderCards(copy, &computed_max,
                                            eval_cache->tt_initialized);
      if (computed_max >= 0) {
        eval_cache->tt_initialized = true;
      }
      it = eval_cache->entries
               .emplace(state_key, ConditionalLine::DefenderEvalEntry{
                                       std::move(cards), computed_max})
               .first;
    }
    opt_ptr = &it->second.cards;
    max_def_tricks = it->second.max_def_tricks;
  } else if (def_cache != nullptr) {
    ConditionalLine::DefenderStateKey state_key = MakeDefenderStateKey(copy);
    auto it = def_cache->find(state_key);
    if (it == def_cache->end()) {
      it = def_cache->emplace(state_key, FindOptimalDefenderCards(copy)).first;
    }
    opt_ptr = &it->second;
  } else {
    local_opt = FindOptimalDefenderCards(copy, &max_def_tricks);
    opt_ptr = &local_opt;
  }
  const std::vector<std::string> &opt_cards = *opt_ptr;
  if (out_guaranteed_defeat != nullptr && max_def_tricks >= 0 &&
      root_min_tricks > 0) {
    int rem_tricks = __builtin_popcountll(
        copy.GetHand(copy.CurPlayer()) & ~copy.played_cards());
    if (copy.tricks_won_even() + (rem_tricks - max_def_tricks) <
        root_min_tricks) {
      *out_guaranteed_defeat = true;
      return opt_cards.empty() ? "" : opt_cards[0];
    }
  }
  if (opt_cards.empty()) {
    return "";
  }
  if (opt_cards.size() == 1 || candidates.empty()) {
    return opt_cards[0];
  }
  std::vector<size_t> valid_idxs;
  std::vector<double> probs(opt_cards.size(), -1.0);
  for (size_t i = 0; i < opt_cards.size(); ++i) {
    probs[i] = BranchSuccessProbability(copy, pos, opt_cards[i], candidates);
    if (probs[i] >= 0.0) {
      valid_idxs.push_back(i);
    }
  }
  if (valid_idxs.empty()) {
    return opt_cards[0];
  }
  if (valid_idxs.size() == 1) {
    return opt_cards[valid_idxs[0]];
  }

  bool has_distinct_branches = false;
  if (pos < 3 && candidates.size() > 1) {
    std::vector<const ConditionalLine *> first_top;
    for (size_t k = 0; k < valid_idxs.size(); ++k) {
      size_t idx = valid_idxs[k];
      CardType card_val = Card::ParseStringCard(opt_cards[idx]);
      Suit suit_led =
          (pos == 0) ? Card::DecodeCard(card_val).first : copy.SuitLed();
      std::vector<const ConditionalLine *> matching =
          FilterCandidatesByDefenderCard(candidates, opt_cards[idx], pos,
                                         suit_led, copy.GetTrump(), &copy);
      std::vector<const ConditionalLine *> top =
          KeepMostSpecificCandidates(matching, pos + 1);
      if (k == 0) {
        first_top = std::move(top);
      } else if (top != first_top) {
        has_distinct_branches = true;
        break;
      }
    }
  }

  bool has_distinct_probs = false;
  for (size_t k = 1; k < valid_idxs.size(); ++k) {
    if (std::abs(probs[valid_idxs[k]] - probs[valid_idxs[0]]) > 1e-12) {
      has_distinct_probs = true;
      break;
    }
  }

  // Only reward Tier-0 falsecard defeats when responding mid-trick (`pos == 1`
  // or `pos == 2`) across explicit branch splits in `candidates`. Unsplit
  // `(S_) SQ` branches generalize 2nd-hand-low Step-1 play without emitting an
  // `(SK) SA` cover, so popping `SK` there would be a generalization artifact.
  // Whenever branches or empirical probabilities differ across `pos = 0..3`,
  // still classify Tier-2 missing Step-1 branches (`leaf == nullptr` with
  // unclamped tricks >= `root_min_tricks`) worse than Tier 1.
  const bool allow_falsecard_defeat =
      (pos > 0 && pos < 3 && has_distinct_branches);
  const bool need_tier_check =
      has_distinct_branches || has_distinct_probs;
  if (!need_tier_check) {
    return opt_cards[valid_idxs[0]];
  }

  auto eval_tier = [&](size_t idx) -> int {
    TrickContinuationOutcome out = EvaluateTrickContinuation(
        copy, pos, opt_cards[idx], candidates, root_min_tricks, def_cache,
        tier_cache, eval_cache);
    int tier = ClassifyContinuationTier(out, root_min_tricks, def_cache,
                                        tier_cache, eval_cache);
    return allow_falsecard_defeat ? tier : std::max(1, tier);
  };

  std::string best_card = opt_cards[valid_idxs[0]];
  int best_tier = eval_tier(valid_idxs[0]);
  double best_prob = probs[valid_idxs[0]];
  for (size_t k = 1; k < valid_idxs.size(); ++k) {
    size_t idx = valid_idxs[k];
    int tier = eval_tier(idx);
    double prob = probs[idx];
    if (tier < best_tier || (tier == best_tier && prob < best_prob - 1e-12)) {
      best_tier = tier;
      best_prob = prob;
      best_card = opt_cards[idx];
    }
  }
  return best_card;
}

std::string ChooseStep1DefenderCard(
    const BoardState &copy,
    ConditionalLine::Step1DefenderCardCache *def_cache) {
  int noise_pct = GetDefenderNoisePct();
  if (def_cache == nullptr) {
    return FindBestDefenderCard(copy, noise_pct);
  }
  ConditionalLine::DefenderStateKey state_key = MakeDefenderStateKey(copy);
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
      if (!child_line || !cond) continue;
      if (cond->IsStateCondition()) {
        if (cond->EvaluateState(copy)) {
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

  for (int pos = 0; pos < 4; ++pos) {
    bool is_pre = (pos < start_pos);
    if (!is_pre && copy.Finished()) break;
    int p = is_pre ? ((copy.cur_trick_leader() + pos) % 4) : copy.CurPlayer();
    if (p % 2 == 0) {
      if (is_pre) {
        std::string pre_str = Card::PrintCard(copy.trick_cards_played()[pos]);
        auto matching =
            FilterCandidatesByDeclarerCard(candidates, pre_str, pos);
        if (matching.empty()) return;
        candidates = std::move(matching);
      } else {
        const ConditionalLine *best_cand =
            SelectMostSpecificCandidate(candidates, pos);
        std::string decl_card_str = GetPatternAtPos(best_cand->do_first(), pos);
        CardType chosen_card = ChooseValidCard(copy, decl_card_str);
        copy.PlayCard(p, chosen_card);
        if (candidates.size() > 1) {
          auto matching =
              FilterCandidatesByDeclarerCard(candidates, decl_card_str, pos);
          if (!matching.empty()) {
            candidates = std::move(matching);
          }
        }
      }
    } else {
      CardType def_card = 0;
      std::string def_card_str;
      Suit suit_led = NOTRUMP;
      if (is_pre) {
        def_card = copy.trick_cards_played()[pos];
        def_card_str = Card::PrintCard(def_card);
        suit_led = Card::DecodeCard(copy.trick_cards_played()[0]).first;
      } else {
        def_card_str = ChooseStep1DefenderCard(copy, def_cache);
        CHECK(!def_card_str.empty());
        def_card = Card::ParseStringCard(def_card_str);
        suit_led =
            (pos == 0) ? Card::DecodeCard(def_card).first : copy.SuitLed();
      }
      std::vector<const ConditionalLine *> matching =
          FilterCandidatesByDefenderCard(
              candidates, def_card_str, pos, suit_led, copy.GetTrump(), &copy);
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
      if (!is_pre) {
        copy.PlayCard(p, def_card);
      }
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
ConditionalLine::BoardPlayTrace SimulateBoardImpl(
    const ConditionalLine *root_line, const BoardState &bs, int board_id,
    bool adversarial, int root_min_tricks, bool clamp_missing_leaf,
    ConditionalLine::DefenderCardsCache *def_cache,
    SubtreeTierCache *tier_cache) {
  ConditionalLine::BoardPlayTrace trace;
  trace.board_id = board_id;
  BoardState copy = bs;
  const ConditionalLine *cur_line = root_line;

  if (cur_line && cur_line->do_first().Empty() &&
      cur_line->conditions().empty()) {
    trace.leaf = cur_line;
    trace.final_ns_tricks = copy.tricks_won_even();
    return trace;
  }

  while (!copy.Finished() && cur_line != nullptr) {
    cur_line = AdvanceThroughStateConditions(cur_line, copy);
    if (!cur_line) {
      break;
    }
    if (cur_line->do_first().Empty() && cur_line->conditions().empty()) {
      trace.leaf = cur_line;
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
    const bool had_multiple_candidates = (candidates.size() > 1);
    std::vector<std::string> played_cards;
    played_cards.reserve(4);
    std::string my1_str;
    std::string my2_str;
    bool diverged_during_trick = false;
    int unmatched_seat = -1;
    std::string unmatched_card;

    for (int pos = 0; pos < 4; ++pos) {
      bool is_pre = (pos < start_pos);
      if (!is_pre && copy.Finished()) break;
      int p = is_pre ? ((trick_leader + pos) % 4) : copy.CurPlayer();
      if (p % 2 == 0) {
        std::string decl_card_str;
        std::string chosen_str;
        if (is_pre) {
          decl_card_str = Card::PrintCard(copy.trick_cards_played()[pos]);
          chosen_str = decl_card_str;
        } else {
          const ConditionalLine *best_cand =
              SelectMostSpecificCandidate(candidates, pos);
          decl_card_str = GetPatternAtPos(best_cand->do_first(), pos);
          CardType chosen_card = ChooseValidCard(copy, decl_card_str);
          chosen_str = Card::PrintCard(chosen_card);
          copy.PlayCard(p, chosen_card);
        }
        played_cards.push_back(chosen_str);
        if (pos < 2) {
          my1_str = chosen_str;
        } else {
          my2_str = chosen_str;
        }
        if (candidates.size() > 1) {
          auto matching =
              FilterCandidatesByDeclarerCard(candidates, decl_card_str, pos);
          if (!matching.empty()) {
            candidates = std::move(matching);
          }
        }
      } else {
        std::string def_card_str;
        Suit suit_led = NOTRUMP;
        if (is_pre) {
          def_card_str = Card::PrintCard(copy.trick_cards_played()[pos]);
          suit_led = Card::DecodeCard(copy.trick_cards_played()[0]).first;
        } else {
          def_card_str =
              adversarial ? ChooseAdversarialDefenderCard(
                                copy, pos, candidates, root_min_tricks,
                                def_cache, tier_cache)
                          : FindBestDefenderCard(copy);
          CardType def_card = 0;
          if (!def_card_str.empty()) {
            def_card = Card::ParseStringCard(def_card_str);
          } else {
            def_card = copy.ValidCardsToPlay()[0];
            def_card_str = Card::PrintCard(def_card);
          }
          suit_led =
              (pos == 0) ? Card::DecodeCard(def_card).first : copy.SuitLed();
          copy.PlayCard(p, def_card);
        }
        played_cards.push_back(def_card_str);

        if (!adversarial || candidates.size() > 1) {
          std::vector<const ConditionalLine *> matching =
              FilterCandidatesByDefenderCard(candidates, def_card_str, pos,
                                             suit_led, copy.GetTrump(), &copy);
          if (!matching.empty()) {
            candidates = std::move(matching);
          } else {
            if (!diverged_during_trick) {
              unmatched_seat = p;
              unmatched_card = def_card_str;
            }
            diverged_during_trick = true;
            candidates = {SelectMostSpecificCandidate(candidates, pos + 1,
                                                      &copy, suit_led)};
          }
        } else if (pos == 0 && !is_pre && !candidates.empty() &&
                   !IsSingleCandidateCompatibleAtLead(copy, candidates[0],
                                                      def_card_str, suit_led)) {
          if (!diverged_during_trick) {
            unmatched_seat = p;
            unmatched_card = def_card_str;
          }
          diverged_during_trick = true;
        }
      }
    }

    if (played_cards.size() < 4) {
      break;
    }

    int winner = copy.CurPlayer();
    int win_idx = (winner - trick_leader + 4) % 4;
    const ConditionalLine *best_node =
        SelectMostSpecificCandidate(candidates, 4);
    const ConditionalLine *chosen_node =
        diverged_during_trick ? nullptr : best_node;
    const ConditionalLine *next_line = nullptr;
    if (chosen_node) {
      if (copy.Finished() && chosen_node->conditions().empty()) {
        trace.leaf = chosen_node;
      } else {
        chosen_node = MatchWinnerContinuation(chosen_node, candidates, winner,
                                              &trace.leaf, &next_line);
        best_node = chosen_node;
      }
    }

    ConditionalLine::PlayedTrickStep step;
    step.leader = trick_leader;
    step.trick_str = played_cards[0] + " " + played_cards[1] + " " +
                     played_cards[2] + " " + played_cards[3];
    step.my1_card = my1_str;
    step.my2_card = my2_str;
    step.winner = winner;
    step.winning_card = played_cards[win_idx];
    step.ns_tricks_so_far = copy.tricks_won_even();
    step.diverged_after = (next_line == nullptr && trace.leaf == nullptr);
    const bool single_empty_parent =
        cur_line->do_first().Empty() && !had_multiple_candidates;
    step.node = diverged_during_trick
                    ? (single_empty_parent ? best_node : cur_line)
                    : chosen_node;
    step.unmatched_seat = unmatched_seat;
    step.unmatched_card = unmatched_card;
    trace.steps.push_back(std::move(step));

    cur_line = next_line;
  }

  if (copy.Finished() || trace.leaf != nullptr) {
    trace.final_ns_tricks = copy.tricks_won_even();
  } else {
    trace.final_ns_tricks = ComputeUnclampedFinalNsTricks(copy);
  }
  if (clamp_missing_leaf && trace.leaf == nullptr && root_min_tricks > 0 &&
      trace.final_ns_tricks >= root_min_tricks) {
    trace.final_ns_tricks = root_min_tricks - 1;
  }
  return trace;
}

// Fast trace-free equivalent of `SimulateBoardImpl(..., clamp_missing_leaf =
// false)` for classifying subtree continuations into Tier 0/1/2.
int EvaluateSubtreeTierFast(const ConditionalLine *root_line,
                            const BoardState &bs, int root_min_tricks,
                            ConditionalLine::DefenderEvalCache *eval_cache,
                            SubtreeTierCache *tier_cache) {
  BoardState copy = bs;
  const ConditionalLine *cur_line = root_line;
  if (cur_line && cur_line->do_first().Empty() &&
      cur_line->conditions().empty()) {
    return 1;
  }

  std::vector<const ConditionalLine *> candidates;
  candidates.reserve(8);
  const ConditionalLine *reached_leaf = nullptr;

  while (!copy.Finished() && cur_line != nullptr) {
    cur_line = AdvanceThroughStateConditions(cur_line, copy);
    if (!cur_line) {
      break;
    }
    if (cur_line->do_first().Empty() && cur_line->conditions().empty()) {
      reached_leaf = cur_line;
      break;
    }

    candidates.clear();
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
    int cards_in_trick = 0;
    bool diverged_during_trick = false;

    for (int pos = 0; pos < 4; ++pos) {
      bool is_pre = (pos < start_pos);
      if (!is_pre && copy.Finished()) break;
      int p = is_pre ? ((trick_leader + pos) % 4) : copy.CurPlayer();
      if (p % 2 == 0) {
        std::string decl_card_str;
        if (is_pre) {
          decl_card_str = Card::PrintCard(copy.trick_cards_played()[pos]);
        } else {
          const ConditionalLine *best_cand =
              SelectMostSpecificCandidate(candidates, pos);
          decl_card_str = GetPatternAtPos(best_cand->do_first(), pos);
          CardType chosen_card = ChooseValidCard(copy, decl_card_str);
          copy.PlayCard(p, chosen_card);
        }
        ++cards_in_trick;
        if (candidates.size() > 1) {
          auto matching =
              FilterCandidatesByDeclarerCard(candidates, decl_card_str, pos);
          if (!matching.empty()) {
            candidates = std::move(matching);
          }
        }
      } else {
        std::string def_card_str;
        Suit suit_led = NOTRUMP;
        if (is_pre) {
          def_card_str = Card::PrintCard(copy.trick_cards_played()[pos]);
          suit_led = Card::DecodeCard(copy.trick_cards_played()[0]).first;
        } else {
          def_card_str = ChooseAdversarialDefenderCard(
              copy, pos, candidates, root_min_tricks, nullptr, tier_cache,
              eval_cache, nullptr);
          CardType def_card = 0;
          if (!def_card_str.empty()) {
            def_card = Card::ParseStringCard(def_card_str);
          } else {
            def_card = copy.ValidCardsToPlay()[0];
            def_card_str = Card::PrintCard(def_card);
          }
          suit_led =
              (pos == 0) ? Card::DecodeCard(def_card).first : copy.SuitLed();
          copy.PlayCard(p, def_card);
        }
        ++cards_in_trick;
        if (candidates.size() > 1) {
          std::vector<const ConditionalLine *> matching =
              FilterCandidatesByDefenderCard(candidates, def_card_str, pos,
                                             suit_led, copy.GetTrump(), &copy);
          if (!matching.empty()) {
            candidates = std::move(matching);
          } else {
            diverged_during_trick = true;
            candidates = {SelectMostSpecificCandidate(candidates, pos + 1,
                                                      &copy, suit_led)};
          }
        } else if (pos == 0 && !is_pre && !candidates.empty() &&
                   !IsSingleCandidateCompatibleAtLead(copy, candidates[0],
                                                      def_card_str, suit_led)) {
          diverged_during_trick = true;
        }
      }
    }

    if (cards_in_trick < 4) {
      break;
    }

    int winner = copy.CurPlayer();
    const ConditionalLine *chosen_node =
        diverged_during_trick ? nullptr
                              : SelectMostSpecificCandidate(candidates, 4);
    const ConditionalLine *next_line = nullptr;
    if (chosen_node) {
      if (copy.Finished() && chosen_node->conditions().empty()) {
        reached_leaf = chosen_node;
      } else {
        MatchWinnerContinuation(chosen_node, candidates, winner, &reached_leaf,
                                &next_line);
      }
    }
    cur_line = next_line;
  }

  if (reached_leaf != nullptr) {
    return 1;
  }
  if (root_min_tricks <= 0) {
    return 0;
  }
  if (copy.Finished()) {
    return (copy.tricks_won_even() < root_min_tricks) ? 0 : 2;
  }
  bool called_dds = false;
  bool reuse = (eval_cache != nullptr && eval_cache->tt_initialized);
  bool makeable = IsDealMakeable(copy, root_min_tricks, &called_dds, reuse);
  if (called_dds && eval_cache != nullptr) {
    eval_cache->tt_initialized = true;
  }
  return makeable ? 2 : 0;
}

}  // namespace

int ConditionalLine::ResultsOnHand(const BoardState &bs) const {
  return SimulateBoard(bs, /*board_id=*/0, /*adversarial=*/false, nullptr)
      .final_ns_tricks;
}

int ConditionalLine::ResultsOnHandAdversarial(
    const BoardState &bs, DefenderCardsCache *def_cache,
    RescoreBoardTrace *out_trace) const {
  BoardPlayTrace trace =
      SimulateBoard(bs, /*board_id=*/0, /*adversarial=*/true, def_cache);
  if (out_trace != nullptr) {
    out_trace->leaf = trace.leaf;
    out_trace->node_tricks.clear();
    out_trace->node_tricks.reserve(trace.steps.size());
    for (const auto &step : trace.steps) {
      if (step.node != nullptr && step.unmatched_seat < 0) {
        out_trace->node_tricks.emplace_back(step.node, step.trick_str);
      }
    }
  }
  return trace.final_ns_tricks;
}

bool ConditionalLine::EvaluateBoardAdversarialFast(
    const BoardState &bs, int min_tricks, DefenderEvalCache *eval_cache) const {
  DefenderEvalCache local_eval_cache;
  if (eval_cache == nullptr) {
    eval_cache = &local_eval_cache;
  }
  const int root_min_tricks = (min_tricks > 0) ? min_tricks : min_tricks_;
  BoardState copy = bs;
  const ConditionalLine *cur_line = this;

  if (cur_line->do_first().Empty() && cur_line->conditions().empty()) {
    return copy.tricks_won_even() >= min_tricks;
  }

  SubtreeTierCache tier_cache;
  std::vector<const ConditionalLine *> candidates;
  candidates.reserve(8);

  while (!copy.Finished() && cur_line != nullptr) {
    if (copy.CardsPlayedThisTrick() == 0 && min_tricks > 0) {
      int rem_tricks = __builtin_popcountll(
          copy.GetHand(copy.CurPlayer()) & ~copy.played_cards());
      if (copy.tricks_won_even() + rem_tricks < min_tricks) {
        return false;
      }
    }
    cur_line = AdvanceThroughStateConditions(cur_line, copy);
    if (!cur_line) {
      return false;
    }
    if (cur_line->do_first().Empty() && cur_line->conditions().empty()) {
      return copy.tricks_won_even() >= min_tricks;
    }

    candidates.clear();
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
      return false;
    }

    int trick_leader = copy.cur_trick_leader();
    int start_pos = copy.CardsPlayedThisTrick();
    int cards_in_trick = 0;

    for (int pos = 0; pos < 4; ++pos) {
      bool is_pre = (pos < start_pos);
      if (!is_pre && copy.Finished()) break;
      int p = is_pre ? ((trick_leader + pos) % 4) : copy.CurPlayer();
      if (p % 2 == 0) {
        std::string decl_card_str;
        if (is_pre) {
          decl_card_str = Card::PrintCard(copy.trick_cards_played()[pos]);
        } else {
          const ConditionalLine *best_cand =
              SelectMostSpecificCandidate(candidates, pos);
          decl_card_str = GetPatternAtPos(best_cand->do_first(), pos);
          CardType chosen_card = ChooseValidCard(copy, decl_card_str);
          copy.PlayCard(p, chosen_card);
        }
        ++cards_in_trick;
        if (candidates.size() > 1) {
          auto matching =
              FilterCandidatesByDeclarerCard(candidates, decl_card_str, pos);
          if (!matching.empty()) {
            candidates = std::move(matching);
          }
        }
      } else {
        std::string def_card_str;
        Suit suit_led = NOTRUMP;
        if (is_pre) {
          def_card_str = Card::PrintCard(copy.trick_cards_played()[pos]);
          suit_led = Card::DecodeCard(copy.trick_cards_played()[0]).first;
        } else {
          bool guaranteed_defeat = false;
          def_card_str = ChooseAdversarialDefenderCard(
              copy, pos, candidates, root_min_tricks, nullptr, &tier_cache,
              eval_cache,
              (min_tricks > 0 && root_min_tricks <= min_tricks)
                  ? &guaranteed_defeat
                  : nullptr);
          if (guaranteed_defeat) {
            return false;
          }
          CardType def_card = 0;
          if (!def_card_str.empty()) {
            def_card = Card::ParseStringCard(def_card_str);
          } else {
            def_card = copy.ValidCardsToPlay()[0];
            def_card_str = Card::PrintCard(def_card);
          }
          suit_led =
              (pos == 0) ? Card::DecodeCard(def_card).first : copy.SuitLed();
          copy.PlayCard(p, def_card);
        }
        ++cards_in_trick;
        if (candidates.size() > 1) {
          std::vector<const ConditionalLine *> matching =
              FilterCandidatesByDefenderCard(candidates, def_card_str, pos,
                                             suit_led, copy.GetTrump(), &copy);
          if (matching.empty()) {
            return false;
          }
          candidates = std::move(matching);
        } else if (pos == 0 && !is_pre && !candidates.empty() &&
                   !IsSingleCandidateCompatibleAtLead(copy, candidates[0],
                                                      def_card_str, suit_led)) {
          return false;
        }
      }
    }

    if (cards_in_trick < 4) {
      return false;
    }

    int winner = copy.CurPlayer();
    const ConditionalLine *chosen_node =
        SelectMostSpecificCandidate(candidates, 4);
    if (copy.Finished() && chosen_node->conditions().empty()) {
      return copy.tricks_won_even() >= min_tricks;
    }
    const ConditionalLine *leaf = nullptr;
    const ConditionalLine *next_line = nullptr;
    MatchWinnerContinuation(chosen_node, candidates, winner, &leaf, &next_line);
    if (leaf != nullptr) {
      return copy.tricks_won_even() >= min_tricks;
    }
    cur_line = next_line;
  }

  return false;
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
    if (it1 != opp1_counts.end()) {
      do_first_.opp1_unmakeable_boards += it1->second;
    }
    auto it2 = opp2_counts.find(this);
    if (it2 != opp2_counts.end()) {
      do_first_.opp2_unmakeable_boards += it2->second;
    }
    auto itw = winner_counts.find(this);
    if (itw != winner_counts.end()) {
      for (int w = 0; w < 4; ++w) {
        do_first_.winner_unmakeable_boards[w] += itw->second[w];
      }
    }
  }
  for (auto &[cond, child] : conditions_) {
    if (child) {
      child->ApplyUnmakeableBoardCounts(opp1_counts, opp2_counts,
                                        winner_counts);
    }
  }
}

void ConditionalLine::DeepCloneChildren(bool is_root) {
  std::vector<ConditionalLine *> action_children;
  for (auto &[cond, child] : conditions_) {
    if (child) {
      child = std::make_shared<ConditionalLine>(*child);
      if (!child->do_first_.Empty()) {
        action_children.push_back(child.get());
      }
      child->DeepCloneChildren(/*is_root=*/false);
    }
  }
  if (action_children.size() > 1 &&
      action_children[0]->do_first_.leader % 2 != 0) {
    bool has_wildcard_lead = false;
    for (const ConditionalLine *c : action_children) {
      if (c->do_first_.demi1.opp_card == "__") {
        has_wildcard_lead = true;
        break;
      }
    }
    bool can_relax_specific_tj =
        !is_root && action_children.size() == conditions_.size();
    bool updated_cond_str = false;
    for (size_t k = 0;
         (has_wildcard_lead || can_relax_specific_tj) &&
         k < action_children.size();
         ++k) {
      ConditionalLine *ck = action_children[k];
      GeneralizedTrick &gt_k = ck->do_first_;
      const std::string &opp1_k = gt_k.demi1.opp_card;
      if (opp1_k.size() < 2 || opp1_k[0] == '_' || opp1_k[0] == 'R' ||
          opp1_k[1] == '_') {
        continue;
      }
      if (!has_wildcard_lead) {
        const std::string &my1_k = gt_k.demi1.my_card;
        const std::string &my2_k = gt_k.demi2.my_card;
        if (opp1_k.size() != 2 || (opp1_k[1] != 'T' && opp1_k[1] != 'J')) {
          continue;
        }
        bool sound_cover = false;
        if (my1_k.size() == 2 && my1_k[0] == opp1_k[0]) {
          int opp_rv = Card::DecodeCard(Card::ParseStringCard(opp1_k)).second;
          int my1_rv = Card::DecodeCard(Card::ParseStringCard(my1_k)).second;
          sound_cover = (my1_rv > opp_rv);
          if (sound_cover && my2_k.size() == 2 && my2_k[0] == opp1_k[0]) {
            int my2_rv = Card::DecodeCard(Card::ParseStringCard(my2_k)).second;
            if (my2_rv >= 8 && my2_rv < opp_rv) {
              sound_cover = false;
            }
          }
        }
        bool sound_ruff = !gt_k.board_entries.empty();
        for (const auto &e : gt_k.board_entries) {
          if (e.winner % 2 != 0 || e.win_card[0] == '\0' ||
              e.win_card[0] == opp1_k[0] ||
              (my1_k != e.win_card && my2_k != e.win_card)) {
            sound_ruff = false;
            break;
          }
        }
        if (!sound_cover && !sound_ruff) {
          continue;
        }
      }
      bool another_same_suit = false;
      for (size_t m = 0; m < action_children.size(); ++m) {
        if (m == k) continue;
        const std::string &opp1_m =
            action_children[m]->do_first_.demi1.opp_card;
        if (opp1_m.size() >= 2 && opp1_m[0] == opp1_k[0]) {
          another_same_suit = true;
          break;
        }
      }
      if (!another_same_suit) {
        gt_k.demi1.opp_card = std::string(1, opp1_k[0]) + "_";
        updated_cond_str = true;
      }
    }
    if (updated_cond_str) {
      for (auto &[cond, child] : conditions_) {
        if (!child || child->do_first_.Empty()) continue;
        if (dynamic_cast<const TrickPlayCondition *>(cond.get()) != nullptr) {
          cond = std::make_shared<TrickPlayCondition>(
              child->do_first_.ToString(true));
        }
      }
    }
  }
}

void ConditionalLine::CollapseUniformInSuitSplits() {
  for (auto &[_c, child] : conditions_) {
    if (child) {
      child->CollapseUniformInSuitSplits();
    }
  }
  size_t act_idx = 0;
  size_t act_count = 0;
  for (size_t i = 0; i < conditions_.size(); ++i) {
    const auto &child = conditions_[i].second;
    if (child && !child->do_first_.Empty()) {
      act_idx = i;
      ++act_count;
    }
  }
  if (act_count != 1) return;
  GeneralizedTrick &gt = conditions_[act_idx].second->do_first_;
  if (gt.leader % 2 != 0 || gt.demi1.my_card.size() < 2) return;
  std::string suit_wild = std::string(1, gt.demi1.my_card[0]) + "_";
  bool changed = false;
  if (gt.demi1.opp_card.size() == 3) {
    gt.demi1.opp_card = suit_wild;
    changed = true;
  }
  if (gt.demi2.opp_card.size() == 3) {
    gt.demi2.opp_card = suit_wild;
    changed = true;
  }
  if (changed && dynamic_cast<const TrickPlayCondition *>(
                     conditions_[act_idx].first.get()) != nullptr) {
    conditions_[act_idx].first =
        std::make_shared<TrickPlayCondition>(gt.ToString(true));
  }
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

ConditionalLine::BoardPlayTrace ConditionalLine::SimulateBoard(
    const BoardState &bs, int board_id, bool adversarial,
    DefenderCardsCache *def_cache) const {
  SubtreeTierCache local_tier_cache;
  return SimulateBoardImpl(this, bs, board_id, adversarial, min_tricks_,
                           /*clamp_missing_leaf=*/true, def_cache,
                           &local_tier_cache);
}

ConditionalLine::BoardPlayTrace ConditionalLine::TraceOnBoard(
    const BoardState &bs, int board_id) const {
  if (board_id >= 0) {
    auto it = board_traces_.find(board_id);
    if (it != board_traces_.end()) {
      return it->second;
    }
  }
  return SimulateBoard(bs, board_id, /*adversarial=*/has_rescored_boards_,
                       nullptr);
}

