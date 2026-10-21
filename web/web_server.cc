#include "web_server.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "board_state.h"
#include "card.h"
#include "conditional_line.h"
#include "cross_hand_summarizer.h"
#include "deal.h"
#include "glog/logging.h"
#include "hand.h"
#include "hand_requirements.h"
#include "line_finder.h"
#include "omp_compat.h"
#include "solver.h"
#include "string_util.h"
#ifndef __EMSCRIPTEN__
#include "web_assets.h"
#endif

namespace {

}  // namespace

std::string FormatWinnerCondition(const PlayCondition *cond_ptr) {
  if (!cond_ptr) return "";
  std::string desc = cond_ptr->Describe(true);
  if (desc.rfind("after ", 0) == 0) {
    desc = desc.substr(6);
  }
  if (desc.rfind("when ", 0) == 0) {
    desc = desc.substr(5);
  } else if (desc.rfind("if ", 0) == 0) {
    desc = desc.substr(3);
  }
  static const std::regex kPlayerWinsRegex("(\\d) wins");
  std::smatch match;
  while (std::regex_search(desc, match, kPlayerWinsRegex)) {
    int player = std::stoi(match[1].str());
    desc.replace(match.position(0), match.length(0),
                 Card::PlayerDirectionDebugString(player) + " wins");
  }
  return desc;
}

std::string SeatLetter(int player) {
  switch ((player % 4 + 4) % 4) {
    case NORTH: return "N";
    case EAST:  return "E";
    case SOUTH: return "S";
    case WEST:  return "W";
    default:    return std::to_string(player);
  }
}

std::string FormatBranchCondition(const PlayCondition *cond_ptr, bool only_choice) {
  if (!cond_ptr) return "";
  std::string desc = cond_ptr->Describe(only_choice);
  if (desc.rfind("after ", 0) == 0) {
    desc = desc.substr(6);
  }
  static const std::regex kPlayerWinsRegex("(\\d) wins");
  std::smatch match;
  while (std::regex_search(desc, match, kPlayerWinsRegex)) {
    int player = std::stoi(match[1].str());
    desc.replace(match.position(0), match.length(0),
                 SeatLetter(player) + " wins");
  }
  return desc;
}

std::string CleanCardToken(const std::string &token) {
  std::string card;
  for (char c : token) {
    if (std::isalnum(static_cast<unsigned char>(c))) {
      card += c;
    }
  }
  return card;
}

std::string ExtractFirstCard(const std::string &seq) {
  std::stringstream ss(seq);
  std::string token;
  while (ss >> token) {
    if (token == "/" || token == "Trick" || token.back() == ':') continue;
    std::string card = CleanCardToken(token);
    if (card.size() >= 2) {
      return card;
    }
  }
  return "";
}

std::string GetFirstCardFromNode(const std::shared_ptr<DecisionTreeNode> &node) {
  if (!node) return "";
  if (!node->trick.empty()) {
    std::string card = ExtractFirstCard(node->trick);
    if (!card.empty()) return card;
  }
  if (!node->trick_full.empty()) {
    std::string card = ExtractFirstCard(node->trick_full);
    if (!card.empty()) return card;
  }
  if (node->label.rfind("Trick ", 0) == 0 || node->label.rfind("#", 0) == 0) {
    std::string card = ExtractFirstCard(node->label);
    if (!card.empty()) return card;
  }
  for (const auto &child : node->children) {
    std::string card = GetFirstCardFromNode(child);
    if (!card.empty()) return card;
  }
  return "";
}

int GetLeaderFromNode(const std::shared_ptr<DecisionTreeNode> &node) {
  if (!node) return -1;
  if (node->leader >= 0) return node->leader;
  for (const auto &child : node->children) {
    int l = GetLeaderFromNode(child);
    if (l >= 0) return l;
  }
  return -1;
}

namespace {

std::string NsTricksRange(const FailureIndex::Group &group) {
  if (group.min_ns_tricks == group.max_ns_tricks) {
    return std::to_string(group.min_ns_tricks);
  }
  return std::to_string(group.min_ns_tricks) + "-" +
         std::to_string(group.max_ns_tricks);
}

std::shared_ptr<DecisionTreeNode> MakeFailureLeaf(
    const FailureIndex::Group &group, const std::string &condition,
    int &node_id, int total) {
  auto leaf = std::make_shared<DecisionTreeNode>();
  leaf->id = "node_" + std::to_string(++node_id);
  leaf->type = "outcome";
  leaf->failure = true;
  leaf->condition = condition;
  if (!group.trick_str.empty()) {
    leaf->leader = group.leader;
    leaf->winner = group.winner;
    leaf->trick = group.trick_str;
  } else {
    leaf->branch_winner = group.winner;
  }
  leaf->board_to_trick = group.board_to_trick;
  leaf->ns_tricks = group.max_ns_tricks;
  int cnt = 0;
  bool has_actual = false;
  for (int b : group.boards) {
    if (b >= 0) {
      ++cnt;
    } else if (b == -1) {
      has_actual = true;
    }
  }
  if (cnt > 0 || !has_actual) {
    leaf->label = "Hand Failed (" + std::to_string(cnt) + "/" +
                  std::to_string(total) + " boards) (" + NsTricksRange(group) +
                  " tricks won by N/S)";
  } else {
    leaf->label = "Actual Hand (#0) Failed (" + NsTricksRange(group) +
                  " tricks won by N/S)";
  }
  leaf->success_boards = 0;
  leaf->failure_boards = cnt;
  leaf->total_boards = total;
  leaf->covered_boards = cnt;
  leaf->boards = group.boards;
  return leaf;
}

// Adds the failed boards that completed `line`'s trick pattern and left the
// line because the trick winner had no continuation as failure leaves under
// `node`, and records what failed boards actually played at this trick so the
// inspector can replay them.
void AttachFailures(const std::shared_ptr<DecisionTreeNode> &node,
                    const ConditionalLine &line, const FailureIndex *failures,
                    int &node_id, int total) {
  if (!failures) return;
  auto tricks_it = failures->tricks.find(&line);
  if (tricks_it != failures->tricks.end()) {
    for (const auto &[b, t] : tricks_it->second) {
      node->board_to_trick.emplace(b, t);
    }
  }
  auto leaves_it = failures->leaves.find(&line);
  if (leaves_it == failures->leaves.end()) return;
  for (const auto &[_key, group] : leaves_it->second) {
    node->children.push_back(
        MakeFailureLeaf(group, group.condition, node_id, total));
  }
}

bool HasFailuresAt(const ConditionalLine &line, const FailureIndex *failures) {
  return failures && failures->leaves.count(&line) > 0;
}

void AttachUnmatchedFailures(const std::shared_ptr<DecisionTreeNode> &cond_node,
                             const ConditionalLine &line,
                             const FailureIndex *failures, int &node_id,
                             int total) {
  if (!failures) return;
  auto it = failures->unmatched_leaves.find(&line);
  if (it == failures->unmatched_leaves.end()) return;
  for (const auto &[_key, group] : it->second) {
    cond_node->children.push_back(
        MakeFailureLeaf(group, group.condition, node_id, total));
  }
}

bool HasUnmatchedFailuresAt(const ConditionalLine &line,
                            const FailureIndex *failures) {
  return failures && failures->unmatched_leaves.count(&line) > 0;
}

// When a single-variant trick node has unmatched defender plays (e.g.
// "if E plays D6" when the trick pattern required "(C_)"), wraps the trick
// node and those failure leaves in a condition node so the unmatched play is a
// sibling of the trick node rather than a child contradicting its cards.
std::shared_ptr<DecisionTreeNode> WrapWithUnmatchedFailures(
    const std::shared_ptr<DecisionTreeNode> &node, const ConditionalLine &line,
    const FailureIndex *failures, int &node_id, int trick_num, int total,
    int ns_tricks) {
  if (!HasUnmatchedFailuresAt(line, failures)) {
    return node;
  }
  const auto &gt = line.do_first();
  std::string cards_played = gt.ToString(true);
  std::string first_card = ExtractFirstCard(cards_played);

  auto cond_node = std::make_shared<DecisionTreeNode>();
  cond_node->id = "node_" + std::to_string(++node_id);
  cond_node->type = "condition";
  cond_node->collapsed = false;
  cond_node->ns_tricks = ns_tricks;
  cond_node->total_boards = total;
  cond_node->success_boards = node->success_boards;
  cond_node->failure_boards = node->failure_boards;
  cond_node->covered_boards = node->covered_boards;
  cond_node->boards = node->boards;
  if (gt.leader % 2 == 0 && !first_card.empty()) {
    cond_node->label = first_card + " is led";
  } else {
    cond_node->label = "Trick #" + std::to_string(trick_num);
  }

  node->condition = cards_played;
  cond_node->children.push_back(node);
  AttachUnmatchedFailures(cond_node, line, failures, node_id, total);
  return cond_node;
}

}  // namespace

std::shared_ptr<DecisionTreeNode> BuildTreeFromConditionalLine(
    const ConditionalLine &line, int &node_id, int trick_num, int root_total,
    int ns_tricks, const FailureIndex *failures) {
  int total = line.total_boards() > 0 ? line.total_boards() : root_total;

  if (line.do_first().Empty()) {
    const auto &conds = line.conditions();
    if (!conds.empty()) {
      if (conds.size() == 1 && conds[0].second &&
          !HasUnmatchedFailuresAt(line, failures)) {
        return BuildTreeFromConditionalLine(*conds[0].second, node_id,
                                            trick_num, total, ns_tricks,
                                            failures);
      }
      std::set<int> boards_set = line.CoveredBoardsSet();
      int success_cnt = boards_set.size();
      int failure_cnt = std::max(0, total - success_cnt);
      auto root_node = std::make_shared<DecisionTreeNode>();
      root_node->id = "node_" + std::to_string(++node_id);
      root_node->type = "condition";
      root_node->collapsed = false;
      root_node->ns_tricks = ns_tricks;
      root_node->total_boards = total;
      root_node->success_boards = success_cnt;
      root_node->failure_boards = failure_cnt;
      root_node->covered_boards = success_cnt;
      root_node->boards = std::vector<int>(boards_set.begin(), boards_set.end());
      for (const auto &[cond_ptr, child_ptr] : conds) {
        if (child_ptr) {
          auto child = BuildTreeFromConditionalLine(
              *child_ptr, node_id, trick_num, total, ns_tricks, failures);
          if (child && cond_ptr) {
            child->condition = FormatBranchCondition(cond_ptr.get(), false);
          }
          root_node->children.push_back(child);
        }
      }

      int branch_leader = GetLeaderFromNode(root_node);
      if (branch_leader >= 0 && branch_leader % 2 != 0) {
        root_node->label = "Defender leads";
      } else {
        std::string common_card = "";
        bool all_same = !root_node->children.empty();
        for (const auto &child : root_node->children) {
          std::string card = GetFirstCardFromNode(child);
          if (card.empty()) {
            all_same = false;
            break;
          }
          if (common_card.empty()) {
            common_card = card;
          } else if (common_card != card) {
            all_same = false;
            break;
          }
        }

        if (all_same && !common_card.empty()) {
          root_node->label = common_card + " is led";
        } else {
          root_node->label = "Declarer leads";
        }
      }

      AttachUnmatchedFailures(root_node, line, failures, node_id, total);
      return root_node;
    }

    const auto &final_boards = line.final_covered_boards();
    int success_cnt = final_boards.size();
    int failure_cnt = std::max(0, total - success_cnt);
    auto leaf = std::make_shared<DecisionTreeNode>();
    leaf->id = "node_" + std::to_string(++node_id);
    leaf->type = "outcome";
    leaf->ns_tricks = ns_tricks;
    leaf->label = "Hand Succeeded (" + std::to_string(success_cnt) + "/" + std::to_string(total) + " boards) (" + std::to_string(ns_tricks) + " tricks won by N/S)";
    leaf->success_boards = success_cnt;
    leaf->failure_boards = failure_cnt;
    leaf->total_boards = total;
    leaf->covered_boards = success_cnt;
    leaf->boards = std::vector<int>(final_boards.begin(), final_boards.end());
    if (failures && failures->actual_succeeded &&
        failures->actual_success_leaf == &line) {
      leaf->boards.push_back(-1);
    }
    return leaf;
  }

  const auto &gt = line.do_first();
  std::string cards_played = gt.ToString(true);
  std::set<int> boards_set = line.CoveredBoardsSet();
  int success_cnt = boards_set.size();
  int failure_cnt = std::max(0, total - success_cnt);
  bool has_failures = HasFailuresAt(line, failures);

  const auto &conds = line.conditions();
  if (conds.size() == 1) {
    const auto &[cond_ptr, child_line_ptr] = conds[0];
    int add_ns = 0;
    int trick_winner = -1;
    if (auto w = dynamic_cast<const WinnerPlayCondition*>(cond_ptr.get())) {
      trick_winner = w->who();
      if (trick_winner == NORTH || trick_winner == SOUTH) {
        add_ns = 1;
      }
    }
    int current_ns = ns_tricks + add_ns;

    bool is_final_trick = false;
    if (child_line_ptr) {
      if (child_line_ptr->do_first().Empty() && child_line_ptr->conditions().empty()) {
        is_final_trick = true;
      }
    } else {
      is_final_trick = true;
    }

    auto node = std::make_shared<DecisionTreeNode>();
    node->id = "node_" + std::to_string(++node_id);
    node->collapsed = false;
    node->leader = gt.leader;
    node->winner = trick_winner;
    node->trick = cards_played;
    node->trick_full = gt.ToString(false);
    for (const auto &[b, t] : gt.demi2.board_to_trick) {
      node->board_to_trick[std::to_string(b)] = t;
    }

    std::string label_str = "#" + std::to_string(trick_num) + ": " + cards_played;

    if (is_final_trick && !has_failures) {
      node->type = "outcome";
      node->ns_tricks = current_ns;
      if (child_line_ptr && !child_line_ptr->final_covered_boards().empty()) {
        const auto &fb = child_line_ptr->final_covered_boards();
        node->boards = std::vector<int>(fb.begin(), fb.end());
        int f_success = fb.size();
        node->success_boards = f_success;
        node->failure_boards = std::max(0, total - f_success);
        node->covered_boards = f_success;
      } else {
        node->boards = std::vector<int>(boards_set.begin(), boards_set.end());
        node->success_boards = success_cnt;
        node->failure_boards = failure_cnt;
        node->covered_boards = success_cnt;
      }
      if (failures && failures->actual_succeeded &&
          failures->actual_last_node == &line) {
        node->boards.push_back(-1);
      }
      node->total_boards = total;
      node->label = label_str;
      AttachFailures(node, line, failures, node_id, total);
      return WrapWithUnmatchedFailures(node, line, failures, node_id,
                                       trick_num, total, ns_tricks);
    }

    node->type = "action";
    node->ns_tricks = current_ns;
    node->label = label_str;
    node->success_boards = success_cnt;
    node->failure_boards = failure_cnt;
    node->total_boards = total;
    node->covered_boards = success_cnt;
    node->boards = std::vector<int>(boards_set.begin(), boards_set.end());

    std::shared_ptr<DecisionTreeNode> child_node;
    if (is_final_trick) {
      // Failures also hang off this last trick, so the success outcome gets
      // its own explicit leaf alongside them when any board succeeded here.
      const std::set<int> &fb =
          (child_line_ptr && !child_line_ptr->final_covered_boards().empty())
              ? child_line_ptr->final_covered_boards()
              : boards_set;
      bool actual_here = failures && failures->actual_succeeded &&
                         failures->actual_last_node == &line;
      if (!fb.empty() || actual_here) {
        child_node = std::make_shared<DecisionTreeNode>();
        child_node->id = "node_" + std::to_string(++node_id);
        child_node->type = "outcome";
        child_node->ns_tricks = current_ns;
        child_node->label =
            "Hand Succeeded (" + std::to_string(fb.size()) + "/" +
            std::to_string(total) + " boards) (" +
            std::to_string(current_ns) + " tricks won by N/S)";
        child_node->success_boards = fb.size();
        child_node->failure_boards = 0;
        child_node->total_boards = total;
        child_node->covered_boards = fb.size();
        child_node->boards = std::vector<int>(fb.begin(), fb.end());
        if (actual_here) {
          child_node->boards.push_back(-1);
        }
      }
    } else {
      child_node = BuildTreeFromConditionalLine(*child_line_ptr, node_id,
                                                trick_num + 1, total,
                                                current_ns, failures);
    }
    if (child_node) {
      child_node->branch_winner = trick_winner;
      if (has_failures && cond_ptr) {
        // With failure branches alongside, the success branch needs its
        // condition spelled out too.
        child_node->condition = FormatBranchCondition(cond_ptr.get(), false);
      }
      node->children.push_back(child_node);
    }
    AttachFailures(node, line, failures, node_id, total);
    return WrapWithUnmatchedFailures(node, line, failures, node_id,
                                     trick_num, total, ns_tricks);
  } else {
    auto node = std::make_shared<DecisionTreeNode>();
    node->id = "node_" + std::to_string(++node_id);
    node->type = "action";
    node->collapsed = false;
    node->leader = gt.leader;
    node->ns_tricks = ns_tricks;
    node->trick = cards_played;
    node->trick_full = gt.ToString(false);
    for (const auto &[b, t] : gt.demi2.board_to_trick) {
      node->board_to_trick[std::to_string(b)] = t;
    }
    node->label = "#" + std::to_string(trick_num) + ": " + cards_played;
    node->success_boards = success_cnt;
    node->failure_boards = failure_cnt;
    node->total_boards = total;
    node->covered_boards = success_cnt;
    node->boards = std::vector<int>(boards_set.begin(), boards_set.end());

    for (const auto &[cond_ptr, child_line_ptr] : conds) {
      if (child_line_ptr) {
        int branch_add_ns = 0;
        int branch_winner = -1;
        if (auto w = dynamic_cast<const WinnerPlayCondition*>(cond_ptr.get())) {
          branch_winner = w->who();
          if (branch_winner == NORTH || branch_winner == SOUTH) {
            branch_add_ns = 1;
          }
          if (node->winner < 0) {
            node->winner = branch_winner;
          }
        }
        int branch_ns = ns_tricks + branch_add_ns;
        auto child = BuildTreeFromConditionalLine(*child_line_ptr, node_id,
                                                  trick_num + 1, total,
                                                  branch_ns, failures);
        if (child) {
          child->branch_winner = branch_winner;
          if (cond_ptr) {
            child->condition = FormatBranchCondition(cond_ptr.get(), false);
          }
          node->children.push_back(child);
        }
      }
    }
    AttachFailures(node, line, failures, node_id, total);
    return WrapWithUnmatchedFailures(node, line, failures, node_id,
                                     trick_num, total, ns_tricks);
  }
}

namespace {

void RecordFailureTraceOnIndex(const ConditionalLine::BoardPlayTrace &trace,
                               Suit trump, FailureIndex *index) {
  if (!index) return;
  auto add_to_group = [](FailureIndex::Group &group, int board_id,
                         int ns_tricks) {
    if (group.boards.empty()) {
      group.min_ns_tricks = ns_tricks;
      group.max_ns_tricks = ns_tricks;
    } else {
      group.min_ns_tricks = std::min(group.min_ns_tricks, ns_tricks);
      group.max_ns_tricks = std::max(group.max_ns_tricks, ns_tricks);
    }
    group.boards.push_back(board_id);
  };

  if (trace.steps.empty() || !trace.steps.back().node) {
    if (index->unplaced.condition.empty()) {
      index->unplaced.condition = "no line applies";
    }
    add_to_group(index->unplaced, trace.board_id, trace.final_ns_tricks);
    return;
  }

  std::string board_key = std::to_string(trace.board_id);
  for (const auto &step : trace.steps) {
    if (step.node && step.unmatched_seat < 0) {
      index->tricks[step.node][board_key] = step.trick_str;
    }
  }

  const auto &last = trace.steps.back();
  if (last.unmatched_seat >= 0) {
    Suit suit_led = last.trick_str.empty()
                        ? NOTRUMP
                        : Card::ParseSuit(last.trick_str[0]);
    Suit card_suit = last.unmatched_card.empty()
                         ? NOTRUMP
                         : Card::ParseSuit(last.unmatched_card[0]);
    bool is_discard = (last.unmatched_seat != last.leader &&
                       suit_led != NOTRUMP && card_suit != suit_led &&
                       card_suit != trump);
    std::string card_key = is_discard ? "R_" : last.unmatched_card;
    std::string key = "plays:" + std::to_string(last.unmatched_seat) + ":" +
                      card_key + ":w" + std::to_string(last.winner);
    FailureIndex::Group &group = index->unmatched_leaves[last.node][key];
    if (group.condition.empty()) {
      group.condition = is_discard
                            ? ("if " + SeatLetter(last.unmatched_seat) +
                               " discards")
                            : ("if " + SeatLetter(last.unmatched_seat) +
                               " plays " + last.unmatched_card);
    }
    group.leader = last.leader;
    group.winner = last.winner;
    if (group.trick_str.empty()) {
      group.trick_str = last.trick_str;
    } else if (group.trick_str != last.trick_str) {
      std::vector<std::string> t1 = SplitWords(group.trick_str);
      std::vector<std::string> t2 = SplitWords(last.trick_str);
      if (t1.size() == 4 && t2.size() == 4) {
        int unmatched_pos = (last.unmatched_seat - last.leader + 4) % 4;
        for (int i = 0; i < 4; ++i) {
          if (t1[i] == t2[i]) continue;
          if (i == unmatched_pos && is_discard) {
            t1[i] = "(R_)";
          } else {
            std::string c1 = CleanCardToken(t1[i]);
            std::string c2 = CleanCardToken(t2[i]);
            if (!c1.empty() && !c2.empty() && c1[0] == c2[0] &&
                c1[0] != '_' && c1[0] != 'R') {
              t1[i] = std::string("(") + c1[0] + "_)";
            } else {
              t1[i] = "(__)";
            }
          }
        }
        group.trick_str =
            t1[0] + " " + t1[1] + " " + t1[2] + " " + t1[3];
      }
    }
    group.board_to_trick[board_key] = last.trick_str;
    add_to_group(group, trace.board_id, trace.final_ns_tricks);
  } else {
    std::string key =
        "wins:" + std::to_string(last.winner) + ":" + last.winning_card;
    WinnerPlayCondition cond(last.winner,
                             std::set<std::string>{last.winning_card});
    FailureIndex::Group &group = index->leaves[last.node][key];
    if (group.condition.empty()) {
      group.condition = FormatBranchCondition(&cond, false);
    }
    group.winner = last.winner;
    group.board_to_trick[board_key] = last.trick_str;
    add_to_group(group, trace.board_id, trace.final_ns_tricks);
  }
}

}  // namespace

FailureIndex IndexFailures(const ConditionalLine &line,
                           const std::vector<BoardState> &boards,
                           const std::vector<int> &failed_board_ids,
                           int num_threads, int min_tricks) {
  FailureIndex index;
  if (failed_board_ids.empty()) {
    return index;
  }
  for (int b_idx : failed_board_ids) {
    ConditionalLine::BoardPlayTrace trace =
        line.TraceOnBoard(boards[b_idx], b_idx);
    if (min_tricks > 0 && trace.final_ns_tricks >= min_tricks) {
      trace.final_ns_tricks = min_tricks - 1;
    }
    RecordFailureTraceOnIndex(trace, boards[b_idx].GetTrump(), &index);
  }
  (void)num_threads;
  return index;
}

std::vector<std::string> SplitTricks(const std::string &line_str) {
  std::vector<std::string> raw_segments;
  size_t start = 0;
  size_t end = line_str.find('/');
  while (end != std::string::npos) {
    std::string seg = line_str.substr(start, end - start);
    std::string trimmed = Trim(seg);
    if (!trimmed.empty()) {
      raw_segments.push_back(trimmed);
    }
    start = end + 1;
    end = line_str.find('/', start);
  }
  std::string trimmed_last = Trim(line_str.substr(start));
  if (!trimmed_last.empty()) {
    raw_segments.push_back(trimmed_last);
  }

  if (raw_segments.size() > 1) {
    return raw_segments;
  }

  std::stringstream tok_ss(line_str);
  std::string card;
  std::vector<std::string> cards;
  while (tok_ss >> card) {
    if (card != "/" && card.size() >= 2) {
      cards.push_back(card);
    }
  }

  if (cards.empty()) {
    return {};
  }

  std::vector<std::string> tricks;
  std::string cur_trick = "";
  for (size_t i = 0; i < cards.size(); ++i) {
    if (!cur_trick.empty()) cur_trick += " ";
    cur_trick += cards[i];
    if ((i + 1) % 4 == 0 || i + 1 == cards.size()) {
      tricks.push_back(cur_trick);
      cur_trick = "";
    }
  }
  return tricks;
}

std::shared_ptr<DecisionTreeNode> BuildTreeFromOptimalLines(
    const OptimalLines &lines, int tricks_won, int &node_id,
    const BoardState &initial_bs, int start_trick_num = 1,
    int initial_ns_tricks = 0) {
  if (lines.empty()) {
    auto root_node = std::make_shared<DecisionTreeNode>();
    root_node->id = "node_" + std::to_string(++node_id);
    root_node->type = "outcome";
    root_node->label = "No solution";
    root_node->success_boards = 0;
    root_node->failure_boards = 1;
    root_node->total_boards = 1;
    root_node->covered_boards = 0;
    return root_node;
  }

  std::vector<std::shared_ptr<DecisionTreeNode>> line_roots;

  for (const auto &line_str : lines) {
    std::vector<std::string> tricks = SplitTricks(line_str);
    if (tricks.empty()) continue;

    BoardState sim_bs = initial_bs;
    std::shared_ptr<DecisionTreeNode> first_trick_node = nullptr;
    std::shared_ptr<DecisionTreeNode> current_node = nullptr;
    int prev_winner = -1;
    int running_ns = initial_ns_tricks;

    for (size_t i = 0; i < tricks.size(); ++i) {
      int leader = sim_bs.cur_trick_leader();
      sim_bs.PlayTrick(tricks[i]);
      int winner = sim_bs.CurPlayer();
      if (winner == NORTH || winner == SOUTH) {
        running_ns++;
      }

      auto node = std::make_shared<DecisionTreeNode>();
      node->id = "node_" + std::to_string(++node_id);
      node->collapsed = false;
      node->leader = leader;
      node->winner = winner;
      node->branch_winner = prev_winner;
      node->ns_tricks = running_ns;
      node->trick = tricks[i];
      node->trick_full = tricks[i];
      node->boards = {0};
      node->board_to_trick["0"] = tricks[i];
      node->success_boards = 1;
      node->failure_boards = 0;
      node->total_boards = 1;
      node->covered_boards = 1;

      std::string label_str =
          "#" + std::to_string(start_trick_num + static_cast<int>(i)) + ": " +
          tricks[i];

      if (i == tricks.size() - 1) {
        node->type = "outcome";
        node->label = label_str;
        node->description = std::to_string(running_ns) + " tricks won";
      } else {
        node->type = "action";
        node->label = label_str;
      }

      if (!first_trick_node) {
        first_trick_node = node;
      }
      if (current_node) {
        current_node->children.push_back(node);
      }
      current_node = node;
      prev_winner = winner;
    }

    if (first_trick_node) {
      line_roots.push_back(first_trick_node);
    }
  }

  if (line_roots.empty()) {
    auto root_node = std::make_shared<DecisionTreeNode>();
    root_node->id = "node_" + std::to_string(++node_id);
    root_node->type = "outcome";
    root_node->boards = {0};
    root_node->ns_tricks = initial_ns_tricks + tricks_won;
    root_node->label = "Contract Result: " +
                       std::to_string(initial_ns_tricks + tricks_won) +
                       " tricks won";
    return root_node;
  }

  if (line_roots.size() == 1) {
    return line_roots[0];
  }

  auto root_node = std::make_shared<DecisionTreeNode>();
  root_node->id = "node_" + std::to_string(++node_id);
  root_node->type = "action";
  root_node->collapsed = false;
  root_node->boards = {0};
  root_node->success_boards = 1;
  root_node->failure_boards = 0;
  root_node->total_boards = 1;
  root_node->covered_boards = 1;
  root_node->children = line_roots;

  std::string common_card = "";
  bool all_same = !line_roots.empty();
  for (const auto &child : line_roots) {
    std::string card = GetFirstCardFromNode(child);
    if (card.empty()) {
      all_same = false;
      break;
    }
    if (common_card.empty()) {
      common_card = card;
    } else if (common_card != card) {
      all_same = false;
      break;
    }
  }

  if (initial_bs.cur_trick_leader() % 2 != 0) {
    root_node->label = "Defender leads";
  } else if (all_same && !common_card.empty()) {
    root_node->label = common_card + " is led";
  } else {
    root_node->label = "Declarer leads";
  }

  return root_node;
}

bool IsGenericOutcomeLabel(const std::string &l) {
  if (l.empty()) return true;
  if (l.rfind("Hand Succeeded", 0) == 0) return true;
  if (l.rfind("Hand Failed", 0) == 0) return true;
  if (l.rfind("Success:", 0) == 0) return true;
  if (l.rfind("Success (", 0) == 0) return true;
  if (l.rfind("Contract Result:", 0) == 0) return true;
  if (l == "Success") return true;
  return false;
}

std::string StripAfterPrefix(const std::string &l) {
  if (l.rfind("after ", 0) == 0) {
    return l.substr(6);
  }
  return l;
}

bool IsRedundantLabel(const std::string &sub, const std::string &main) {
  if (sub == main) return true;
  std::string stripped_sub = StripAfterPrefix(sub);
  std::string stripped_main = StripAfterPrefix(main);
  if (stripped_sub == stripped_main) return true;
  if (!stripped_sub.empty() && main.find(stripped_sub) != std::string::npos) return true;
  if (!stripped_main.empty() && sub.find(stripped_main) != std::string::npos) return true;
  return false;
}

void CollapseRedundantNodes(std::shared_ptr<DecisionTreeNode> &node) {
  if (!node) return;

  for (auto &child : node->children) {
    CollapseRedundantNodes(child);
  }

  while (node->children.size() == 1) {
    auto child = node->children[0];

    auto is_trick = [](const std::string &l) {
      return l.rfind("Trick ", 0) == 0 || l.rfind("#", 0) == 0;
    };
    // Don't merge two distinct Trick steps into each other
    if (is_trick(node->label) && is_trick(child->label)) {
      break;
    }
    if (is_trick(node->label)) {
      break;
    }

    if (!node->condition.empty()) {
      if (child->condition.empty() ||
          IsRedundantLabel(child->condition, child->label)) {
        child->condition = node->condition;
      } else if (!IsRedundantLabel(node->condition, child->condition)) {
        break;
      }
    }
    if (child->branch_winner < 0 && node->branch_winner >= 0) {
      child->branch_winner = node->branch_winner;
    }

    node = child;
  }
}

void SetDefaultCollapsedState(std::shared_ptr<DecisionTreeNode> &node, bool is_root = true) {
  if (!node) return;

  // A container root node (e.g. "Defender leads", "Declarer leads", or
  // "<Card> is led") has no trick of its own and multiple child branches. It
  // remains expanded (collapsed = false) so its child branches (the first
  // trick in each branch) are visible.
  if (is_root && node->trick.empty() && node->children.size() > 1 &&
      (node->label.find("Defender leads") != std::string::npos ||
       node->label.find("Declarer leads") != std::string::npos ||
       node->label.find("Solve Lines") != std::string::npos ||
       node->label.find("Failure Lines") != std::string::npos ||
       node->label.find("Optimal Lines") != std::string::npos ||
       node->label.find("variants") != std::string::npos ||
       node->label.find("is led") != std::string::npos ||
       node->type == "condition")) {
    node->collapsed = false;
  } else {
    // All trick nodes (e.g. Trick 1) and sub-nodes are collapsed by default.
    node->collapsed = true;
  }

  for (auto &child : node->children) {
    SetDefaultCollapsedState(child, false);
  }
}

// Makes failure_boards mean "boards that reach this node and go on to fail",
// i.e. the failure leaves in its subtree, rather than "all boards the subtree
// does not cover".
int AccumulateFailureCounts(const std::shared_ptr<DecisionTreeNode> &node) {
  if (!node) return 0;
  if (node->children.empty()) {
    if (!node->failure) {
      node->failure_boards = 0;
    }
    return node->failure_boards;
  }
  int sum = 0;
  for (auto &child : node->children) {
    sum += AccumulateFailureCounts(child);
  }
  node->failure_boards = sum;
  return sum;
}

bool PropagateActualBoardId(const std::shared_ptr<DecisionTreeNode> &node) {
  if (!node) return false;
  bool has_actual =
      std::find(node->boards.begin(), node->boards.end(), -1) !=
      node->boards.end();
  for (auto &child : node->children) {
    if (PropagateActualBoardId(child)) {
      has_actual = true;
    }
  }
  if (has_actual &&
      std::find(node->boards.begin(), node->boards.end(), -1) ==
          node->boards.end()) {
    node->boards.push_back(-1);
  }
  return has_actual;
}

void PruneEmptyBranches(const std::shared_ptr<DecisionTreeNode> &node) {
  if (!node || node->children.empty()) return;
  for (auto &child : node->children) {
    PruneEmptyBranches(child);
  }
  auto is_empty_branch = [](const std::shared_ptr<DecisionTreeNode> &c) {
    if (!c) return true;
    if (c->success_boards > 0 || c->failure_boards > 0) return false;
    return std::find(c->boards.begin(), c->boards.end(), -1) == c->boards.end();
  };
  bool any_non_empty = false;
  for (const auto &c : node->children) {
    if (!is_empty_branch(c)) {
      any_non_empty = true;
      break;
    }
  }
  if (any_non_empty) {
    node->children.erase(
        std::remove_if(node->children.begin(), node->children.end(),
                       is_empty_branch),
        node->children.end());
  }
}

ConditionalLine::BoardPlayTrace AttachActualBoardToFailures(
    const ConditionalLine &line, const BoardState &actual_bs, int min_tricks,
    FailureIndex *index) {
  ConditionalLine::BoardPlayTrace trace = line.TraceOnBoard(actual_bs, -1);
  if (!index) return trace;

  if (trace.leaf != nullptr && trace.final_ns_tricks >= min_tricks) {
    for (const auto &step : trace.steps) {
      if (step.node && step.unmatched_seat < 0) {
        index->tricks[step.node]["-1"] = step.trick_str;
      }
    }
    index->actual_succeeded = true;
    index->actual_last_node =
        trace.steps.empty() ? nullptr : trace.steps.back().node;
    index->actual_success_leaf = trace.leaf;
    return trace;
  }

  index->actual_succeeded = false;
  if (min_tricks > 0 && trace.final_ns_tricks >= min_tricks) {
    trace.final_ns_tricks = min_tricks - 1;
  }
  RecordFailureTraceOnIndex(trace, actual_bs.GetTrump(), index);
  return trace;
}

void to_json(nlohmann::json &j, const DecisionTreeNode &node) {
  j = nlohmann::json{
      {"id", node.id},
      {"type", node.type},
      {"label", node.label},
      {"collapsed", node.collapsed},
      {"failure", node.failure},
      {"ns_tricks", node.ns_tricks},
      {"success_boards", node.success_boards},
      {"failure_boards", node.failure_boards},
      {"total_boards", node.total_boards},
      {"covered_boards", node.covered_boards},
  };
  if (node.leader >= 0) {
    j["leader"] = node.leader;
  }
  if (node.winner >= 0) {
    j["winner"] = node.winner;
  }
  if (node.branch_winner >= 0) {
    j["branch_winner"] = node.branch_winner;
  }
  if (!node.description.empty()) {
    j["description"] = node.description;
  }
  if (!node.trick.empty()) {
    j["trick"] = node.trick;
  }
  if (!node.trick_full.empty()) {
    j["trick_full"] = node.trick_full;
  }
  if (!node.condition.empty()) {
    j["condition"] = node.condition;
  }
  if (!node.boards.empty() || node.type == "action" ||
      node.type == "condition" || node.type == "outcome") {
    j["boards"] = node.boards;
  }
  if (!node.board_to_trick.empty()) {
    j["board_to_trick"] = node.board_to_trick;
  }

  nlohmann::json children = nlohmann::json::array();
  for (const auto &child : node.children) {
    if (child) {
      children.push_back(*child);
    }
  }
  j["children"] = children;
}

uint64_t ComputeUnknownCards(const std::vector<uint64_t> &hands) {
  int n_cnt = CountCardsInHand(hands[0]);
  int e_cnt = CountCardsInHand(hands[1]);
  int s_cnt = CountCardsInHand(hands[2]);
  int w_cnt = CountCardsInHand(hands[3]);

  int max_cnt = std::max({n_cnt, e_cnt, s_cnt, w_cnt});
  if (max_cnt == 0) max_cnt = 13;

  int needed = (4 * max_cnt) - (n_cnt + e_cnt + s_cnt + w_cnt);
  if (needed <= 0) return 0;

  uint64_t total_known = hands[0] | hands[1] | hands[2] | hands[3];
  uint64_t unknown_cards = 0;

  int added = 0;
  for (int s = 0; s < 4 && added < needed; ++s) {
    Suit suit = static_cast<Suit>(s);
    uint16_t suit_known = (total_known >> (s * 16)) & 0xffff;
    if (suit_known > 0) {
      for (int r = 12; r >= 0 && added < needed; --r) {
        uint64_t card_bit = Card::GetSuitedCard(suit, r);
        if ((total_known & card_bit) == 0 && (unknown_cards & card_bit) == 0) {
          unknown_cards |= card_bit;
          added++;
        }
      }
    }
  }

  for (int s = 0; s < 4 && added < needed; ++s) {
    Suit suit = static_cast<Suit>(s);
    for (int r = 12; r >= 0 && added < needed; --r) {
      uint64_t card_bit = Card::GetSuitedCard(suit, r);
      if ((total_known & card_bit) == 0 && (unknown_cards & card_bit) == 0) {
        unknown_cards |= card_bit;
        added++;
      }
    }
  }

  return unknown_cards;
}

void ExtractHandWildcards(const std::string &hand_str,
                          std::vector<std::pair<Suit, char>> *wildcards) {
  Suit cur_suit = SPADES;
  for (size_t i = 0; i < hand_str.size(); ++i) {
    char c = hand_str[i];
    if (c == 'S' || c == 's') {
      cur_suit = SPADES;
    } else if (c == 'H' || c == 'h') {
      cur_suit = HEARTS;
    } else if (c == 'D' || c == 'd') {
      cur_suit = DIAMONDS;
    } else if (c == 'C' || c == 'c') {
      cur_suit = CLUBS;
    } else if (c == 'x' || c == 'X') {
      wildcards->emplace_back(cur_suit, c);
    }
  }
}

bool ResolveHandsAndUnknownCards(
    const std::vector<std::string> &hand_strs, const std::string &unknown_str,
    std::vector<uint64_t> *hands, uint64_t *unknown_cards_out,
    std::vector<CardType> (*resolved_hand_wildcards)[4][4],
    std::string *error_msg) {
  std::vector<std::pair<Suit, char>> hand_wildcards[4];
  for (int p = 0; p < 4; ++p) {
    ExtractHandWildcards(hand_strs[p], &hand_wildcards[p]);
  }

  uint64_t combined_known_cards =
      (*hands)[0] | (*hands)[1] | (*hands)[2] | (*hands)[3];
  uint64_t unknown_cards = 0;
  std::vector<std::pair<Suit, char>> unknown_wildcards;

  if (!unknown_str.empty()) {
    Suit cur_suit = SPADES;
    for (size_t i = 0; i < unknown_str.size(); ++i) {
      char c = unknown_str[i];
      if (c == '1' && i + 1 < unknown_str.size() && unknown_str[i + 1] == '0') {
        c = 'T';
        i++;
      }
      if (std::isspace(static_cast<unsigned char>(c)) || c == ';' ||
          c == ',' || c == '-') {
        continue;
      }
      if (c == 'S' || c == 's') {
        cur_suit = SPADES;
        continue;
      }
      if (c == 'H' || c == 'h') {
        cur_suit = HEARTS;
        continue;
      }
      if (c == 'D' || c == 'd') {
        cur_suit = DIAMONDS;
        continue;
      }
      if (c == 'C' || c == 'c') {
        cur_suit = CLUBS;
        continue;
      }

      int card_val = -1;
      if (c == 'A' || c == 'a') {
        card_val = 12;
      } else if (c == 'K' || c == 'k') {
        card_val = 11;
      } else if (c == 'Q' || c == 'q') {
        card_val = 10;
      } else if (c == 'J' || c == 'j') {
        card_val = 9;
      } else if (c == 'T' || c == 't') {
        card_val = 8;
      } else if (c >= '2' && c <= '9') {
        card_val = c - '2';
      }

      if (card_val >= 0) {
        uint64_t card_bit = Card::GetSuitedCard(cur_suit, card_val);
        if ((combined_known_cards & card_bit) != 0) {
          *error_msg = "Card " + Card::PrintCard(card_bit) +
                       " in unknown cards conflicts with a known hand card.";
          return false;
        }
        if ((unknown_cards & card_bit) != 0) {
          *error_msg = "Duplicate card " + Card::PrintCard(card_bit) +
                       " in unknown cards specification.";
          return false;
        }
        unknown_cards |= card_bit;
        continue;
      }

      if (c == 'x' || c == 'X') {
        unknown_wildcards.emplace_back(cur_suit, c);
        continue;
      }

      *error_msg = std::string("Unrecognized character '") + c +
                   "' in unknown cards specification.";
      return false;
    }
  }

  std::vector<int> available_ranks[4];
  uint64_t all_explicit = combined_known_cards | unknown_cards;
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    for (int r = 0; r <= 12; ++r) {
      uint64_t card_bit = Card::GetSuitedCard(suit, r);
      if ((all_explicit & card_bit) == 0) {
        available_ranks[s].push_back(r);
      }
    }
  }

  auto pick_wildcard_card = [&](Suit suit, char wc,
                                CardType *out_bit) -> bool {
    auto &ranks = available_ranks[static_cast<int>(suit)];
    if (ranks.empty()) {
      *error_msg = "Not enough outstanding cards available in " +
                   SuitDebugString(suit) + " for wildcard 'x'.";
      return false;
    }
    int picked_rank = 0;
    if (wc == 'X') {
      picked_rank = ranks.back();
      ranks.pop_back();
    } else {
      picked_rank = ranks.front();
      ranks.erase(ranks.begin());
    }
    *out_bit = Card::GetSuitedCard(suit, picked_rank);
    return true;
  };

  for (int p = 0; p < 4; ++p) {
    for (const auto &[suit, wc] : hand_wildcards[p]) {
      CardType card_bit = 0;
      if (!pick_wildcard_card(suit, wc, &card_bit)) {
        return false;
      }
      (*hands)[p] |= card_bit;
      if (resolved_hand_wildcards != nullptr) {
        (*resolved_hand_wildcards)[p][static_cast<int>(suit)].push_back(
            card_bit);
      }
    }
  }

  if (!unknown_str.empty()) {
    for (const auto &[suit, wc] : unknown_wildcards) {
      CardType card_bit = 0;
      if (!pick_wildcard_card(suit, wc, &card_bit)) {
        return false;
      }
      unknown_cards |= card_bit;
    }

    int parsed_count = CountCardsInHand(unknown_cards);
    int n_cnt = CountCardsInHand((*hands)[0]);
    int e_cnt = CountCardsInHand((*hands)[1]);
    int s_cnt = CountCardsInHand((*hands)[2]);
    int w_cnt = CountCardsInHand((*hands)[3]);

    int max_cnt = std::max({n_cnt, e_cnt, s_cnt, w_cnt});
    if (max_cnt == 0) max_cnt = 13;
    int expected = (4 * max_cnt) - (n_cnt + e_cnt + s_cnt + w_cnt);

    if (parsed_count != expected) {
      *error_msg = "Invalid unknown cards count: specified " +
                   std::to_string(parsed_count) +
                   " unknown card(s), but expected " +
                   std::to_string(expected) + " based on hand sizes.";
      return false;
    }
    *unknown_cards_out = unknown_cards;
  } else {
    *unknown_cards_out = ComputeUnknownCards(*hands);
  }

  return true;
}

void to_json(nlohmann::json &j, const CrossHandSummaryReport &summary) {
  j["total_deals"] = summary.total_deals;
  j["cards_per_hand"] = summary.cards_per_hand;
  j["unknown_cards"] = summary.unknown_cards;
  j["invariant_rules"] = summary.invariant_rules;

  nlohmann::json stats_json = nlohmann::json::array();
  for (const auto &st : summary.invariant_stats) {
    nlohmann::json item;
    item["rule"] = st.rule;
    item["formatted_rule"] = st.FormatRuleWithStats();
    item["holds_count"] = st.holds_count;
    item["total_bifurcations"] = st.total_bifurcations;
    item["holds_percent"] = st.holds_percent();
    item["has_line_stats"] = st.has_line_stats;
    item["is_failure_mode"] = st.is_failure_mode;
    if (st.has_line_stats) {
      item["success_when_holds_count"] = st.success_when_holds_count;
      item["success_when_holds_percent"] = st.success_when_holds_percent();
    }
    stats_json.push_back(std::move(item));
  }
  j["invariant_stats"] = std::move(stats_json);

  nlohmann::json suit_bounds_json = nlohmann::json::object();
  for (const auto &[suit, bounds] : summary.suit_bounds) {
    std::string suit_key = SuitDebugString(suit);
    nlohmann::json b;
    b["min_hand1"] = bounds.min_hand1;
    b["max_hand1"] = bounds.max_hand1;
    b["min_hand2"] = bounds.min_hand2;
    b["max_hand2"] = bounds.max_hand2;
    b["min_combined"] = bounds.min_combined;
    b["max_combined"] = bounds.max_combined;
    suit_bounds_json[suit_key] = b;
  }
  j["suit_bounds"] = suit_bounds_json;

  nlohmann::json honor_stats_json = nlohmann::json::object();
  for (const auto &[suit, rank_map] : summary.honor_stats) {
    std::string suit_key = SuitDebugString(suit);
    nlohmann::json rank_json = nlohmann::json::object();
    for (const auto &[rank, stats] : rank_map) {
      std::string card_val = Card::PrintCardValue(rank);
      nlohmann::json s;
      s["hand1_count"] = stats.hand1_count;
      s["hand2_count"] = stats.hand2_count;
      s["both_count"] = stats.both_count;
      s["neither_count"] = stats.neither_count;
      rank_json[card_val] = s;
    }
    honor_stats_json[suit_key] = rank_json;
  }
  j["honor_stats"] = honor_stats_json;
}

WebServer::WebServer(const std::string &host, int port)
    : host_(host), port_(port) {
#ifndef __EMSCRIPTEN__
  RegisterRoutes();
#endif
}

WebServer::~WebServer() {
#ifndef __EMSCRIPTEN__
  Stop();
#endif
}

std::string WebServer::GetSolveStatusJson() const {
  nlohmann::json response;
  response["status"] = LineFinder::GetSolveStatus();
  double progress = LineFinder::GetSolveProgress();
  if (progress >= 0.0) {
    response["progress"] = progress;
  }
  return response.dump();
}

std::string WebServer::StopSolveJson() {
  LineFinder::RequestStop();
  LineFinder::SetSolveStatus("Stopping solve...");
  nlohmann::json response;
  response["status"] = "stopping";
  return response.dump();
}

#ifndef __EMSCRIPTEN__
void WebServer::RegisterRoutes() {
  svr_.Get("/", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(web_assets::INDEX_HTML, "text/html");
  });

  svr_.Get("/index.html", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(web_assets::INDEX_HTML, "text/html");
  });

  svr_.Get("/help.html", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(web_assets::HELP_HTML, "text/html");
  });

  svr_.Get("/styles.css", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(web_assets::STYLES_CSS, "text/css");
  });

  svr_.Get("/app.js", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(web_assets::APP_JS, "application/javascript");
  });

  svr_.Get("/presets.json", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(web_assets::PRESETS_JSON, "application/json");
  });

  svr_.Get("/api/presets", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(web_assets::PRESETS_JSON, "application/json");
  });

  svr_.Get("/bridge-strategy.md", [](const httplib::Request &,
                                     httplib::Response &res) {
    res.set_content(web_assets::BRIDGE_STRATEGY_MD,
                    "text/markdown; charset=utf-8");
  });

  svr_.Get("/user-guide.md", [](const httplib::Request &,
                                httplib::Response &res) {
    res.set_content(web_assets::USER_GUIDE_MD,
                    "text/markdown; charset=utf-8");
  });

  auto serve_doc_image = [](const httplib::Request &req,
                            httplib::Response &res) {
    if (req.matches.size() < 2) {
      res.status = 404;
      return;
    }
    std::string name = req.matches[1].str();
    const char *candidates[] = {"docs/images/", "../docs/images/"};
    for (const char *dir : candidates) {
      std::ifstream ifs(std::string(dir) + name, std::ios::binary);
      if (ifs) {
        std::ostringstream ss;
        ss << ifs.rdbuf();
        res.set_content(ss.str(), "image/png");
        return;
      }
    }
    res.status = 404;
  };
  svr_.Get(R"(/images/([A-Za-z0-9_.-]+\.png))", serve_doc_image);
  svr_.Get(R"(/docs/images/([A-Za-z0-9_.-]+\.png))", serve_doc_image);

  svr_.Get("/api/health", [this](const httplib::Request &req, httplib::Response &res) {
    HandleHealth(req, res);
  });

  svr_.Post("/api/solve", [this](const httplib::Request &req, httplib::Response &res) {
    HandleSolve(req, res);
  });

  svr_.Get("/api/solve_status", [this](const httplib::Request &req, httplib::Response &res) {
    HandleSolveStatus(req, res);
  });

  svr_.Post("/api/stop_solve", [this](const httplib::Request &req, httplib::Response &res) {
    HandleStopSolve(req, res);
  });
}

void WebServer::HandleHealth(const httplib::Request &, httplib::Response &res) {
  nlohmann::json response;
  response["status"] = "ok";
  response["service"] = "bridge_solver_http_server";
  res.set_content(response.dump(), "application/json");
}

void WebServer::HandleSolveStatus(const httplib::Request &,
                                  httplib::Response &res) {
  res.set_content(GetSolveStatusJson(), "application/json");
}

void WebServer::HandleStopSolve(const httplib::Request &,
                                httplib::Response &res) {
  res.set_content(StopSolveJson(), "application/json");
}

void WebServer::HandleSolve(const httplib::Request &req,
                            httplib::Response &res) {
  int status = 200;
  std::string body = SolveJson(req.body, &status);
  res.status = status;
  res.set_content(body, "application/json");
}
#endif  // __EMSCRIPTEN__

namespace {

std::vector<std::vector<std::shared_ptr<HandRequirements>>>
ParseHandRequirements(const nlohmann::json &reqs_json) {
  std::vector<std::vector<std::shared_ptr<HandRequirements>>> reqs(4);
  if (!reqs_json.is_object()) return reqs;
  for (int i = 0; i < 4; ++i) {
    std::string p_key = (i == 0 ? "N" : (i == 1 ? "E" : (i == 2 ? "S" : "W")));
    if (!reqs_json.contains(p_key)) continue;
    const auto &p_req = reqs_json[p_key];
    if (p_req.is_array()) {
      for (const auto &item : p_req) {
        if (item.is_string()) {
          auto r = HandRequirements::FromString(item.get<std::string>());
          if (r) reqs[i].push_back(std::move(r));
        }
      }
      continue;
    }
    if (!p_req.is_object()) continue;
    if (p_req.contains("hcp_low") && p_req.contains("hcp_high")) {
      reqs[i].push_back(std::make_shared<HighCardRange>(
          p_req["hcp_low"].get<int>(), p_req["hcp_high"].get<int>()));
    }
    if (p_req.contains("spades_min")) {
      reqs[i].push_back(std::make_shared<SuitAtLeast>(
          SPADES, p_req["spades_min"].get<int>()));
    }
    if (p_req.contains("suit_max") && p_req["suit_max"].is_object()) {
      for (const auto &[s_key, max_val] : p_req["suit_max"].items()) {
        if (max_val.is_number_integer()) {
          Suit s = Card::ParseSuit(s_key);
          if (s != NOTRUMP) {
            reqs[i].push_back(std::make_shared<SuitLength>(
                s, 0, std::max(0, max_val.get<int>())));
          }
        }
      }
    }
    if (p_req.contains("would_bid")) {
      const auto &wb = p_req["would_bid"];
      auto add_wb_obj = [&](const nlohmann::json &obj) {
        if (obj.is_object() && obj.contains("bid") && obj["bid"].is_string()) {
          std::string bid = obj["bid"].get<std::string>();
          std::string after =
              (obj.contains("after") && obj["after"].is_string())
                  ? obj["after"].get<std::string>()
                  : "";
          if (!bid.empty()) {
            reqs[i].push_back(std::make_shared<WouldBid>(bid, after));
          }
        } else if (obj.is_string()) {
          auto r = HandRequirements::FromString(obj.get<std::string>());
          if (r) reqs[i].push_back(std::move(r));
        }
      };
      if (wb.is_array()) {
        for (const auto &item : wb) {
          add_wb_obj(item);
        }
      } else {
        add_wb_obj(wb);
      }
    }
    if (p_req.contains("exprs") && p_req["exprs"].is_array()) {
      for (const auto &expr : p_req["exprs"]) {
        if (expr.is_string()) {
          auto r = HandRequirements::FromString(expr.get<std::string>());
          if (r) reqs[i].push_back(std::move(r));
        }
      }
    }
  }
  return reqs;
}

class FatalLogCaptureSink : public google::LogSink {
 public:
  void send(google::LogSeverity severity, const char * /*full_filename*/,
            const char *base_filename, int line,
            const google::LogMessageTime & /*time*/, const char *message,
            size_t message_len) override {
    if (severity == google::GLOG_FATAL) {
      std::lock_guard<std::mutex> lock(mu_);
      last_fatal_ = std::string("Fatal check failure at ") +
                    (base_filename ? base_filename : "unknown") + ":" +
                    std::to_string(line) + ": " +
                    std::string(message, message_len);
    }
  }

  std::string GetLastFatal() const {
    std::lock_guard<std::mutex> lock(mu_);
    return last_fatal_;
  }

 private:
  mutable std::mutex mu_;
  std::string last_fatal_;
};

FatalLogCaptureSink &GetFatalLogSink() {
  static FatalLogCaptureSink sink;
  return sink;
}

#if defined(__GNUC__)
__attribute__((noreturn))
#else
[[noreturn]]
#endif
void ThrowOnGlogFatal() {
  std::string msg = GetFatalLogSink().GetLastFatal();
  if (msg.empty()) {
    msg = "Fatal internal solver error (LOG(FATAL) / CHECK failure)";
  }
  throw std::runtime_error(msg);
}

void EnsureGlogFatalHandlerInstalled() {
  static std::once_flag once;
  std::call_once(once, []() {
    google::AddLogSink(&GetFatalLogSink());
    google::InstallFailureFunction(
        reinterpret_cast<google::logging_fail_func_t>(&ThrowOnGlogFatal));
  });
}

}  // namespace

std::unique_ptr<LineFinder> WebServer::CheckoutLineFinder(
    int num_iters, const std::vector<uint64_t> &hands,
    uint64_t unknown_cards, Suit trump_suit, int leader,
    const std::vector<CardType> &cur_trick,
    const nlohmann::json &reqs_json, bool *reused_lf) {
  *reused_lf = false;
  {
    std::lock_guard<std::mutex> lock(cached_lf_mu_);
    if (cached_lf_ && !cur_trick.empty() &&
        cached_num_iters_ == num_iters && cached_hands_ == hands &&
        cached_unknown_cards_ == unknown_cards &&
        cached_trump_ == trump_suit && cached_leader_ == leader &&
        cached_reqs_json_ == reqs_json) {
      *reused_lf = true;
      return std::move(cached_lf_);
    }
  }
  auto lf = std::make_unique<LineFinder>(num_iters);
  if (!reqs_json.is_null() && !reqs_json.empty()) {
    lf->SetRequirements(ParseHandRequirements(reqs_json));
  }
  return lf;
}

void WebServer::CheckinLineFinder(
    std::unique_ptr<LineFinder> lf, int num_iters,
    const std::vector<uint64_t> &hands, uint64_t unknown_cards,
    Suit trump_suit, int leader, nlohmann::json reqs_json) {
  std::lock_guard<std::mutex> lock(cached_lf_mu_);
  cached_lf_ = std::move(lf);
  cached_num_iters_ = num_iters;
  cached_hands_ = hands;
  cached_unknown_cards_ = unknown_cards;
  cached_trump_ = trump_suit;
  cached_leader_ = leader;
  cached_reqs_json_ = std::move(reqs_json);
}

std::string WebServer::SolveJson(const std::string &req_body,
                                 int *http_status_out) {
  EnsureGlogFatalHandlerInstalled();
  if (http_status_out) *http_status_out = 200;
  nlohmann::json response;
  try {
    auto start_time = std::chrono::high_resolution_clock::now();
    LineFinder::ClearStop();
    LineFinder::SetSolveStatus("Starting solve...");

    nlohmann::json req_json = nlohmann::json::parse(req_body);

    std::string north_str = "";
    std::string east_str = "";
    std::string south_str = "";
    std::string west_str = "";

    if (req_json.contains("hands")) {
      auto &h = req_json["hands"];
      if (h.contains("N")) north_str = h["N"].get<std::string>();
      if (h.contains("E")) east_str = h["E"].get<std::string>();
      if (h.contains("S")) south_str = h["S"].get<std::string>();
      if (h.contains("W")) west_str = h["W"].get<std::string>();
    }

    std::vector<uint64_t> hands(4, 0);
    hands[0] = ParseHand(north_str);
    hands[1] = ParseHand(east_str);
    hands[2] = ParseHand(south_str);
    hands[3] = ParseHand(west_str);

    int num_iters = 100;
    if (req_json.contains("num_iters")) {
      num_iters = req_json["num_iters"].get<int>();
    }

    int num_threads = 16;
    if (req_json.contains("num_threads") &&
        req_json["num_threads"].is_number()) {
      num_threads = std::max(1, req_json["num_threads"].get<int>());
    }

    std::string unknown_str = "";
    if (req_json.contains("unknown_cards") &&
        req_json["unknown_cards"].is_string()) {
      unknown_str = req_json["unknown_cards"].get<std::string>();
    }

    uint64_t unknown_cards = 0;
    std::string unknown_err = "";
    std::vector<CardType> resolved_hand_wildcards[4][4];
    std::vector<std::string> hand_strs = {north_str, east_str, south_str,
                                          west_str};

    if (!ResolveHandsAndUnknownCards(hand_strs, unknown_str, &hands,
                                     &unknown_cards, &resolved_hand_wildcards,
                                     &unknown_err)) {
      response["success"] = false;
      response["error"] = unknown_err;
      if (http_status_out) *http_status_out = 400;
      return response.dump();
    }

    bool reuse_deals = req_json.value("reuse_deals", false);
    bool is_fuzzy = false;
    if (unknown_cards != 0 || req_json.contains("requirements") ||
        !unknown_str.empty() || reuse_deals) {
      is_fuzzy = true;
    }

    Suit trump_suit = NOTRUMP;
    if (req_json.contains("trump")) {
      trump_suit = Card::ParseSuit(req_json["trump"].get<std::string>());
    }

    int leader = NORTH;
    if (req_json.contains("leader")) {
      if (req_json["leader"].is_number()) {
        leader = req_json["leader"].get<int>() % 4;
      } else {
        leader = Card::ParsePlayerDirection(
            req_json["leader"].get<std::string>());
      }
    }

    int min_tricks = 1;
    if (req_json.contains("min_tricks")) {
      min_tricks = req_json["min_tricks"].get<int>();
    }

    int start_trick_num = 1;
    if (req_json.contains("start_trick_num") &&
        req_json["start_trick_num"].is_number()) {
      start_trick_num = std::max(1, req_json["start_trick_num"].get<int>());
    }

    int initial_ns_tricks = 0;
    if (req_json.contains("initial_ns_tricks") &&
        req_json["initial_ns_tricks"].is_number()) {
      initial_ns_tricks = std::max(0, req_json["initial_ns_tricks"].get<int>());
    }

    std::vector<CardType> cur_trick;
    if (req_json.contains("cur_trick") && req_json["cur_trick"].is_array()) {
      for (size_t k = 0; k < req_json["cur_trick"].size(); ++k) {
        const auto &c_json = req_json["cur_trick"][k];
        if (!c_json.is_string()) continue;
        std::string c_str = c_json.get<std::string>();
        if (c_str.size() < 2) continue;
        Suit s = Card::ParseSuit(static_cast<char>(std::toupper(c_str[0])));
        char rch = c_str[1];
        if (c_str.size() >= 3 && c_str[1] == '1' && c_str[2] == '0') {
          rch = 'T';
        }
        int seat = (leader + static_cast<int>(k)) % 4;
        if (s != NOTRUMP &&
            (rch == 'x' || rch == 'X' || rch == '_' || rch == '<')) {
          auto &vec = resolved_hand_wildcards[seat][static_cast<int>(s)];
          if (!vec.empty()) {
            CardType ct = vec.front();
            vec.erase(vec.begin());
            cur_trick.push_back(ct);
          } else {
            std::vector<CardType> suit_cards =
                CardsInHandInSuit(hands[seat], s, /*played_cards=*/0);
            if (!suit_cards.empty()) {
              CardType ct =
                  (rch == 'X') ? suit_cards.back() : suit_cards.front();
              cur_trick.push_back(ct);
            }
          }
        } else {
          CardType ct = Card::ParseStringCard(c_str);
          cur_trick.push_back(ct);
        }
      }
    }

    std::string best_seq;
    OptimalLines optimal_lines;
    int tricks_won = 0;
    std::string best_lead;

    std::shared_ptr<DecisionTreeNode> tree_node;
    int node_counter = 0;
    int covered_boards = 1;
    int total_boards = 1;
    int success_boards = 1;
    int failure_boards = 0;

    double solve_ms = 0.0;
    double invariant_ms = 0.0;
    std::vector<std::vector<std::string>> replay_tricks;
    if (req_json.contains("replay_tricks") &&
        req_json["replay_tricks"].is_array()) {
      for (const auto &t_json : req_json["replay_tricks"]) {
        if (t_json.is_array()) {
          std::vector<std::string> cards;
          for (const auto &c_json : t_json) {
            if (c_json.is_string()) {
              cards.push_back(c_json.get<std::string>());
            }
          }
          replay_tricks.push_back(std::move(cards));
        }
      }
    }
    std::set<int> branch_board_ids;
    if (req_json.contains("branch_board_ids") &&
        req_json["branch_board_ids"].is_array()) {
      for (const auto &id_json : req_json["branch_board_ids"]) {
        if (id_json.is_number_integer()) {
          int bid = id_json.get<int>();
          if (bid >= 0) branch_board_ids.insert(bid);
        }
      }
    }
    std::vector<std::map<int, std::string>> replay_board_tricks;
    if (req_json.contains("replay_board_tricks") &&
        req_json["replay_board_tricks"].is_array()) {
      for (const auto &m_json : req_json["replay_board_tricks"]) {
        std::map<int, std::string> bmap;
        if (m_json.is_object()) {
          for (auto it = m_json.begin(); it != m_json.end(); ++it) {
            if (!it.value().is_string()) continue;
            try {
              int bid = std::stoi(it.key());
              if (bid >= 0) bmap[bid] = it.value().get<std::string>();
            } catch (...) {
            }
          }
        }
        replay_board_tricks.push_back(std::move(bmap));
      }
    }

    bool has_actual_deal = false;
    BoardState actual_init_bs;
    BoardState actual_bs;
    if (is_fuzzy && req_json.contains("actual_deal") &&
        req_json["actual_deal"].is_object()) {
      const auto &ad = req_json["actual_deal"];
      std::string ad_e = ad.value("E", "");
      std::string ad_w = ad.value("W", "");
      if (!ad_e.empty() || !ad_w.empty()) {
        std::string ad_n = ad.value("N", north_str);
        std::string ad_s = ad.value("S", south_str);
        std::vector<uint64_t> actual_hands(4, 0);
        actual_hands[0] = ParseHand(ad_n);
        actual_hands[1] = ParseHand(ad_e);
        actual_hands[2] = ParseHand(ad_s);
        actual_hands[3] = ParseHand(ad_w);
        uint64_t dummy_unknown = 0;
        std::string dummy_err;
        std::vector<std::string> actual_strs = {ad_n, ad_e, ad_s, ad_w};
        bool resolved_ok = ResolveHandsAndUnknownCards(
            actual_strs, "", &actual_hands, &dummy_unknown, nullptr,
            &dummy_err);
        if (actual_hands[0] == 0) actual_hands[0] = hands[0];
        if (actual_hands[2] == 0) actual_hands[2] = hands[2];
        int n_cards = CountCardsInHand(actual_hands[0]);
        bool valid_counts =
            resolved_ok && n_cards > 0 &&
            CountCardsInHand(actual_hands[1]) == n_cards &&
            CountCardsInHand(actual_hands[2]) == n_cards &&
            CountCardsInHand(actual_hands[3]) == n_cards;
        if (valid_counts) {
          int init_lead = leader;
          if (req_json.contains("initial_leader") &&
              req_json["initial_leader"].is_string()) {
            std::string il_str = req_json["initial_leader"].get<std::string>();
            if (!il_str.empty()) {
              init_lead = Card::ParsePlayerDirection(il_str);
            }
          }
          actual_init_bs.SetHands(actual_hands);
          actual_init_bs.SetTrump(trump_suit);
          actual_init_bs.SetLead(init_lead);
          actual_init_bs.MarkMissingCardsAsPlayed();
          actual_init_bs.set_board_id(0xAC70A100u);

          actual_bs = actual_init_bs;
          bool replay_ok = true;
          for (const auto &trick_cards : replay_tricks) {
            for (const std::string &c_str : trick_cards) {
              if (!PlayReplayCardOnBoard(c_str, actual_bs)) {
                replay_ok = false;
                break;
              }
            }
            if (!replay_ok) break;
          }
          if (replay_ok) {
            actual_bs.SetLead(leader);
            for (CardType ct : cur_trick) {
              actual_bs.PlayCard(actual_bs.CurPlayer(), ct);
            }
            has_actual_deal = true;
          }
        }
      }
    }

    if (is_fuzzy) {
      total_boards = num_iters;
      nlohmann::json reqs_json =
          req_json.contains("requirements") ? req_json["requirements"]
                                            : nlohmann::json();
      bool reused_lf = false;
      std::unique_ptr<LineFinder> lf = CheckoutLineFinder(
          num_iters, hands, unknown_cards, trump_suit, leader,
          (replay_tricks.empty() && branch_board_ids.empty())
              ? cur_trick
              : std::vector<CardType>{},
          reqs_json, &reused_lf);
      if (reused_lf) {
        lf->SetPredealtBoards({}, {});
      } else if (reuse_deals) {
        std::lock_guard<std::mutex> lock(cached_lf_mu_);
        if (static_cast<int>(last_initial_boards_.size()) == num_iters) {
          lf->SetPredealtBoards(last_initial_boards_, replay_tricks,
                                branch_board_ids, replay_board_tricks);
          if (last_solver_cache_) {
            lf->SetSharedCache(last_solver_cache_);
          }
        }
      }

      std::vector<ConditionalLine> lines =
          lf->Solve(min_tricks, hands, unknown_cards, leader, trump_suit,
                    cur_trick, num_threads);
      if (LineFinder::IsStopRequested()) {
        if (reused_lf) {
          CheckinLineFinder(std::move(lf), num_iters, hands, unknown_cards,
                            trump_suit, leader, std::move(reqs_json));
        }
        LineFinder::SetSolveStatus("Solve stopped.");
        response["success"] = false;
        response["stopped"] = true;
        response["error"] = "Solve stopped by user.";
        return response.dump();
      }
      solve_ms = lf->solve_ms();
      invariant_ms = lf->invariant_ms();

      std::vector<BoardState> boards = lf->boards();
      std::vector<BoardState> display_boards =
          !lf->initial_boards().empty() ? lf->initial_boards() : boards;
      std::set<int> active_offsets = lf->board_offsets();
      if (!active_offsets.empty()) {
        total_boards = static_cast<int>(active_offsets.size());
      }
      if (!reuse_deals && !reused_lf && !lf->initial_boards().empty()) {
        std::lock_guard<std::mutex> lock(cached_lf_mu_);
        last_initial_boards_ = lf->initial_boards();
        last_solver_cache_ = lf->shared_cache();
      }
      if (reused_lf ||
          (!reuse_deals && cur_trick.empty() && replay_tricks.empty() &&
           branch_board_ids.empty())) {
        CheckinLineFinder(std::move(lf), num_iters, hands, unknown_cards,
                          trump_suit, leader, std::move(reqs_json));
      }
      std::set<int> covered_set;
      if (!lines.empty()) {
        covered_set = lines[0].CoveredBoardsSet();
      }
      response["covered_board_ids"] =
          std::vector<int>(covered_set.begin(), covered_set.end());
      response["active_board_ids"] =
          std::vector<int>(active_offsets.begin(), active_offsets.end());

      constexpr size_t kMaxSampleHands = 100;
      nlohmann::json sample_hands_json = nlohmann::json::array();
      nlohmann::json failure_sample_hands_json = nlohmann::json::array();
      nlohmann::json all_hands_json = nlohmann::json::array();
      std::vector<int> failed_board_ids;
      for (int b_idx = 0; b_idx < static_cast<int>(boards.size()); ++b_idx) {
        bool is_active =
            active_offsets.empty() || (active_offsets.count(b_idx) > 0);
        bool is_covered = is_active && (covered_set.count(b_idx) > 0);
        if (is_active && !is_covered) {
          failed_board_ids.push_back(b_idx);
        }
        const BoardState &disp_b =
            b_idx < static_cast<int>(display_boards.size())
                ? display_boards[b_idx]
                : boards[b_idx];
        nlohmann::json hand_entry = {
            {"board_id", b_idx},
            {"N", HandDebugString(disp_b.GetHand(0), true)},
            {"E", HandDebugString(disp_b.GetHand(1), true)},
            {"S", HandDebugString(disp_b.GetHand(2), true)},
            {"W", HandDebugString(disp_b.GetHand(3), true)},
            {"succeeded", is_covered},
            {"active", is_active},
        };
        all_hands_json.push_back(hand_entry);
        if (is_covered && sample_hands_json.size() < kMaxSampleHands) {
          sample_hands_json.push_back(hand_entry);
        } else if (is_active && !is_covered &&
                   failure_sample_hands_json.size() < kMaxSampleHands) {
          failure_sample_hands_json.push_back(hand_entry);
        }
      }
      response["sample_hands"] = sample_hands_json;
      response["failure_sample_hands"] = failure_sample_hands_json;
      response["all_hands"] = all_hands_json;

      if (!lines.empty()) {
        if (!failed_board_ids.empty()) {
          LineFinder::SetSolveStatus("Indexing failure deals...");
        } else {
          LineFinder::SetSolveStatus("Building decision tree...");
        }
        std::string start_tag = "#" + std::to_string(start_trick_num);
        auto extract_trick_tag = [](const std::string &lbl) -> std::string {
          size_t colon = lbl.find(':');
          if (colon != std::string::npos && !lbl.empty() && lbl[0] == '#') {
            return lbl.substr(0, colon);
          }
          return "";
        };

        auto summarize_trick1 =
            [](const std::shared_ptr<DecisionTreeNode> &root) -> std::string {
          if (!root) return "-";
          if (!root->trick.empty()) return root->trick;
          std::string joined;
          for (const auto &child : root->children) {
            if (!child || child->failure || child->trick.empty()) continue;
            if (!joined.empty()) joined += " | ";
            joined += child->trick;
          }
          return joined.empty() ? root->label : joined;
        };

        std::function<void(const std::shared_ptr<DecisionTreeNode> &, int,
                           std::vector<std::string> &)>
            collect_steps = [&](const std::shared_ptr<DecisionTreeNode> &n,
                                int depth,
                                std::vector<std::string> &steps_out) {
              if (!n || n->failure) return;
              std::string tag = extract_trick_tag(n->label);
              if (depth > 0 && !n->trick.empty() && tag != start_tag) {
                std::string step = tag.empty() ? "" : tag;
                if (!n->condition.empty()) {
                  step += (step.empty() ? "[" : " [") + n->condition + "]";
                }
                if (!step.empty()) step += ": ";
                step += n->trick;
                steps_out.push_back(step);
              }
              for (const auto &c : n->children) {
                collect_steps(c, depth + 1, steps_out);
              }
            };

        std::vector<std::shared_ptr<DecisionTreeNode>> line_trees(lines.size());
        std::vector<std::string> line_best_leads(lines.size());
        std::vector<std::set<int>> line_covered_sets(lines.size());
        std::vector<std::string> trick1_summaries(lines.size());
        std::vector<std::vector<std::string>> line_steps(lines.size());
        std::vector<nlohmann::json> line_actual_hands(lines.size());

        for (size_t i = 0; i < lines.size(); ++i) {
          line_covered_sets[i] = lines[i].CoveredBoardsSet();
          std::vector<int> line_failed_ids;
          line_failed_ids.reserve(boards.size());
          for (int b_idx = 0; b_idx < static_cast<int>(boards.size());
               ++b_idx) {
            bool is_active =
                active_offsets.empty() || (active_offsets.count(b_idx) > 0);
            if (is_active && line_covered_sets[i].count(b_idx) == 0) {
              line_failed_ids.push_back(b_idx);
            }
          }
          FailureIndex line_failures = IndexFailures(
              lines[i], boards, line_failed_ids, num_threads, min_tricks);
          if (has_actual_deal) {
            ConditionalLine::BoardPlayTrace actual_trace =
                AttachActualBoardToFailures(lines[i], actual_bs, min_tricks,
                                            &line_failures);
            int actual_tricks = actual_trace.final_ns_tricks;
            if (!line_failures.actual_succeeded && min_tricks > 0 &&
                actual_tricks >= min_tricks) {
              actual_tricks = min_tricks - 1;
            }
            line_actual_hands[i] = {
                {"board_id", -1},
                {"N", HandDebugString(actual_init_bs.GetHand(0), true)},
                {"E", HandDebugString(actual_init_bs.GetHand(1), true)},
                {"S", HandDebugString(actual_init_bs.GetHand(2), true)},
                {"W", HandDebugString(actual_init_bs.GetHand(3), true)},
                {"succeeded", line_failures.actual_succeeded},
                {"ns_tricks", initial_ns_tricks + actual_tricks},
            };
          }
          if (initial_ns_tricks > 0) {
            auto offset_group = [&](FailureIndex::Group &grp) {
              if (!grp.boards.empty()) {
                grp.min_ns_tricks += initial_ns_tricks;
                grp.max_ns_tricks += initial_ns_tricks;
              }
            };
            offset_group(line_failures.unplaced);
            for (auto &[_node, grp_map] : line_failures.leaves) {
              for (auto &[_k, grp] : grp_map) offset_group(grp);
            }
            for (auto &[_node, grp_map] : line_failures.unmatched_leaves) {
              for (auto &[_k, grp] : grp_map) offset_group(grp);
            }
          }
          int line_node_counter = 0;
          auto ltree = BuildTreeFromConditionalLine(
              lines[i], line_node_counter, start_trick_num, total_boards,
              initial_ns_tricks, &line_failures);
          if (ltree && !line_failures.unplaced.boards.empty()) {
            ltree->children.push_back(MakeFailureLeaf(
                line_failures.unplaced, line_failures.unplaced.condition,
                line_node_counter, total_boards));
          }
          AccumulateFailureCounts(ltree);
          if (ltree) {
            if (has_actual_deal) {
              PropagateActualBoardId(ltree);
            }
            PruneEmptyBranches(ltree);
            CollapseRedundantNodes(ltree);
            SetDefaultCollapsedState(ltree);
          }
          line_trees[i] = ltree;
          if (!lines[i].do_first().Empty()) {
            line_best_leads[i] =
                ExtractFirstCard(lines[i].do_first().ToString(true));
          } else if (!cur_trick.empty()) {
            line_best_leads[i] = Card::PrintCard(cur_trick[0]);
          } else if (ltree) {
            line_best_leads[i] = GetFirstCardFromNode(ltree);
          }
          trick1_summaries[i] = summarize_trick1(ltree);
          collect_steps(ltree, 0, line_steps[i]);
        }

        nlohmann::json lines_json = nlohmann::json::array();
        for (size_t i = 0; i < lines.size(); ++i) {
          std::string desc = trick1_summaries[i];
          std::vector<size_t> peers;
          for (size_t j = 0; j < lines.size(); ++j) {
            if (j != i && trick1_summaries[j] == trick1_summaries[i]) {
              peers.push_back(j);
            }
          }
          int added_steps = 0;
          while (!peers.empty() && added_steps < 2) {
            std::string best_step;
            size_t best_eliminated = 0;
            for (const std::string &cand : line_steps[i]) {
              size_t eliminated = 0;
              for (size_t p : peers) {
                if (std::find(line_steps[p].begin(), line_steps[p].end(),
                              cand) == line_steps[p].end()) {
                  ++eliminated;
                }
              }
              if (eliminated > best_eliminated) {
                best_eliminated = eliminated;
                best_step = cand;
              }
            }
            if (best_eliminated == 0) break;
            desc += " -> " + best_step;
            std::vector<size_t> remaining;
            for (size_t p : peers) {
              if (std::find(line_steps[p].begin(), line_steps[p].end(),
                            best_step) != line_steps[p].end()) {
                remaining.push_back(p);
              }
            }
            peers = std::move(remaining);
            ++added_steps;
          }
          std::string line_label =
              "Line " + std::to_string(i + 1) + ": " + desc;

          nlohmann::json line_entry = {
              {"index", static_cast<int>(i)},
              {"label", line_label},
              {"best_lead", line_best_leads[i]},
              {"covered_boards", lines[i].CoveredBoards()},
              {"covered_board_ids",
               std::vector<int>(line_covered_sets[i].begin(),
                                line_covered_sets[i].end())},
              {"tree", line_trees[i] ? nlohmann::json(*line_trees[i])
                                     : nlohmann::json::object()},
          };
          if (has_actual_deal && !line_actual_hands[i].is_null()) {
            line_entry["actual_hand"] = line_actual_hands[i];
          }
          if (lines[i].has_summary()) {
            line_entry["distribution_summary"] = lines[i].summary();
          }
          if (lines[i].has_failure_summary()) {
            line_entry["failure_distribution_summary"] =
                lines[i].failure_summary();
          }
          lines_json.push_back(std::move(line_entry));
        }

        tree_node = line_trees[0];
        best_lead = line_best_leads[0];
        success_boards = lines[0].CoveredBoards();
        failure_boards = std::max(0, total_boards - success_boards);
        covered_boards = success_boards;
        tricks_won = initial_ns_tricks + min_tricks;
        // All returned lines cover the same number of boards; the first is
        // shown by default, and all equivalent lines are included in `lines`.
        response["num_lines"] = lines.size();
        response["lines"] = std::move(lines_json);
        if (has_actual_deal && !line_actual_hands[0].is_null()) {
          response["actual_hand"] = line_actual_hands[0];
        }
        if (lines[0].has_summary()) {
          response["distribution_summary"] = lines[0].summary();
        }
        if (lines[0].has_failure_summary()) {
          response["failure_distribution_summary"] = lines[0].failure_summary();
        }
      } else {
        success_boards = 0;
        failure_boards = total_boards;
        covered_boards = 0;
        std::vector<int> all_failed_ids;
        all_failed_ids.reserve(boards.size());
        for (int b_idx = 0; b_idx < static_cast<int>(boards.size()); ++b_idx) {
          bool is_active =
              active_offsets.empty() || (active_offsets.count(b_idx) > 0);
          if (is_active) {
            all_failed_ids.push_back(b_idx);
          }
        }
        ConditionalLine empty_line;
        FailureIndex line_failures =
            IndexFailures(empty_line, boards, all_failed_ids, num_threads);
        if (has_actual_deal) {
          ConditionalLine::BoardPlayTrace actual_trace =
              AttachActualBoardToFailures(empty_line, actual_bs, min_tricks,
                                          &line_failures);
          response["actual_hand"] = {
              {"board_id", -1},
              {"N", HandDebugString(actual_init_bs.GetHand(0), true)},
              {"E", HandDebugString(actual_init_bs.GetHand(1), true)},
              {"S", HandDebugString(actual_init_bs.GetHand(2), true)},
              {"W", HandDebugString(actual_init_bs.GetHand(3), true)},
              {"succeeded", false},
              {"ns_tricks", initial_ns_tricks + actual_trace.final_ns_tricks},
          };
        }
        if (initial_ns_tricks > 0 && !line_failures.unplaced.boards.empty()) {
          line_failures.unplaced.min_ns_tricks += initial_ns_tricks;
          line_failures.unplaced.max_ns_tricks += initial_ns_tricks;
        }
        if (!line_failures.unplaced.boards.empty()) {
          tree_node = MakeFailureLeaf(line_failures.unplaced, "", node_counter,
                                      total_boards);
        } else {
          tree_node = BuildTreeFromOptimalLines(
              optimal_lines, tricks_won, node_counter, BoardState(),
              start_trick_num, initial_ns_tricks);
        }
      }
    } else {
      BoardState bs;
      bs.SetHands(hands);
      bs.SetTrump(trump_suit);
      bs.SetLead(leader);
      bs.MarkMissingCardsAsPlayed();
      for (CardType ct : cur_trick) {
        bs.PlayCard(bs.CurPlayer(), ct);
      }

      LineFinder::SetSolveStatus("Solving double-dummy tree...");
      auto solve_start = std::chrono::high_resolution_clock::now();
      int pair_tricks =
          MostTricksCanBeWon(bs, 0, &best_seq, &optimal_lines);
      tricks_won = initial_ns_tricks + pair_tricks;
      best_lead = ExtractFirstCard(best_seq);
      LineFinder::SetSolveStatus("Building decision tree...");
      tree_node = BuildTreeFromOptimalLines(
          optimal_lines, pair_tricks, node_counter, bs, start_trick_num,
          initial_ns_tricks);
      auto solve_end = std::chrono::high_resolution_clock::now();
      solve_ms = std::chrono::duration<double, std::milli>(solve_end - solve_start).count();
      invariant_ms = 0.0;

      total_boards = 1;
      success_boards = (tricks_won >= initial_ns_tricks + min_tricks) ? 1 : 0;
      failure_boards = 1 - success_boards;
      covered_boards = success_boards;
      nlohmann::json sample_hands_json = nlohmann::json::array();
      sample_hands_json.push_back({
          {"board_id", 0},
          {"N", HandDebugString(hands[0], true)},
          {"E", HandDebugString(hands[1], true)},
          {"S", HandDebugString(hands[2], true)},
          {"W", HandDebugString(hands[3], true)},
          {"succeeded", success_boards > 0},
      });
      response["sample_hands"] = sample_hands_json;
      response["failure_sample_hands"] = nlohmann::json::array();
      response["all_hands"] = sample_hands_json;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms =
        std::chrono::duration<double, std::milli>(end_time - start_time).count();

    response["success"] = true;
    response["tricks_won"] = tricks_won;
    response["best_lead"] = best_lead;
    response["sequence"] = best_seq;
    response["total_boards"] = total_boards;
    response["success_boards"] = success_boards;
    response["failure_boards"] = failure_boards;
    response["covered_boards"] = covered_boards;
    response["num_threads"] = num_threads;
    response["solve_ms"] = solve_ms;
    response["invariant_ms"] = invariant_ms;
    if (tree_node) {
      if (!is_fuzzy || !response.contains("lines")) {
        LineFinder::SetSolveStatus("Formatting decision tree...");
        CollapseRedundantNodes(tree_node);
        SetDefaultCollapsedState(tree_node);
      }
      response["tree"] = *tree_node;
    } else {
      response["tree"] = nlohmann::json::object();
    }
    response["elapsed_ms"] = elapsed_ms;

    LineFinder::SetSolveStatus("Solve complete.");
    return response.dump();
  } catch (const std::exception &e) {
    LineFinder::SetSolveStatus("Idle");
    LOG(ERROR) << "Exception in SolveJson: " << e.what();
    response["success"] = false;
    response["error"] = e.what();
    if (http_status_out) *http_status_out = 400;
    return response.dump();
  }
}

#ifndef __EMSCRIPTEN__
bool WebServer::Start(bool async) {
  if (running_) return true;

  if (async) {
    running_ = true;
    server_thread_ = std::make_unique<std::thread>([this]() {
      LOG(INFO) << "Starting WebServer on http://" << host_ << ":" << port_;
      svr_.listen(host_.c_str(), port_);
      running_ = false;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return true;
  } else {
    LOG(INFO) << "Starting WebServer on http://" << host_ << ":" << port_;
    running_ = true;
    bool ok = svr_.listen(host_.c_str(), port_);
    running_ = false;
    return ok;
  }
}

void WebServer::Stop() {
  if (running_ || svr_.is_running()) {
    svr_.stop();
    try {
      httplib::Client dummy_client(host_, port_);
      dummy_client.set_connection_timeout(0, 100000);
      dummy_client.Get("/api/health");
    } catch (...) {}

    if (server_thread_ && server_thread_->joinable()) {
      server_thread_->join();
    }
    running_ = false;
  }
}
#endif  // __EMSCRIPTEN__
