#include "web_server.h"

#include <chrono>
#include <iostream>
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
#include "solver.h"
#include "web_assets.h"

namespace {

Suit ParseSuitName(const std::string &s) {
  return Card::ParseSuit(s);
}

int ParsePlayerDirection(const std::string &d) {
  if (d == "N" || d == "NORTH" || d == "0" || d == "north") return NORTH; // 0
  if (d == "E" || d == "EAST" || d == "1" || d == "east") return EAST;   // 1
  if (d == "S" || d == "SOUTH" || d == "2" || d == "south") return SOUTH; // 2
  if (d == "W" || d == "WEST" || d == "3" || d == "west") return WEST;   // 3
  return NORTH;
}

std::string ExtractFirstCard(const std::string &seq) {
  std::stringstream ss(seq);
  std::string token;
  while (ss >> token) {
    if (token != "/" && token.size() >= 2) {
      return token;
    }
  }
  return "";
}

struct DecisionTreeNode {
  std::string id;
  std::string type;  // "action", "condition", "outcome"
  std::string label;
  std::string description;
  std::string trick;
  std::string trick_full;
  bool collapsed = false;
  int success_boards = 0;
  int failure_boards = 0;
  int total_boards = 0;
  int covered_boards = 0;
  std::vector<int> boards;
  std::vector<std::shared_ptr<DecisionTreeNode>> children;

  std::map<std::string, std::shared_ptr<DecisionTreeNode>> child_map;
};

std::string NormalizeDirectionAndCondition(const std::string &s);

std::shared_ptr<DecisionTreeNode> BuildTreeFromConditionalLine(
    const ConditionalLine &line, int &node_id, int depth = 0, int root_total = 1) {
  int total = line.total_boards() > 0 ? line.total_boards() : root_total;
  if (depth > 20) {
    auto leaf = std::make_shared<DecisionTreeNode>();
    leaf->id = "node_" + std::to_string(++node_id);
    leaf->type = "outcome";
    leaf->label = "Max depth reached";
    leaf->success_boards = 0;
    leaf->failure_boards = total;
    leaf->total_boards = total;
    leaf->covered_boards = 0;
    return leaf;
  }

  auto node = std::make_shared<DecisionTreeNode>();
  node->id = "node_" + std::to_string(++node_id);
  node->collapsed = (depth >= 1);

  const auto &final_boards = line.final_covered_boards();
  if (!final_boards.empty()) {
    int success_cnt = final_boards.size();
    int failure_cnt = std::max(0, total - success_cnt);
    int pct = total > 0 ? (100 * success_cnt) / total : 0;
    node->type = "outcome";
    node->label = "Hand Succeeded (" + std::to_string(success_cnt) + "/" +
                  std::to_string(total) + " boards, " + std::to_string(pct) + "%)";
    node->success_boards = success_cnt;
    node->failure_boards = failure_cnt;
    node->total_boards = total;
    node->covered_boards = success_cnt;
    node->boards = std::vector<int>(final_boards.begin(), final_boards.end());
    return node;
  }

  node->type = "action";
  std::set<int> boards_set = line.CoveredBoardsSet();
  int success_cnt = boards_set.size();
  int failure_cnt = std::max(0, total - success_cnt);
  int pct = total > 0 ? (100 * success_cnt) / total : 0;

  node->success_boards = success_cnt;
  node->failure_boards = failure_cnt;
  node->total_boards = total;
  node->covered_boards = success_cnt;
  node->boards = std::vector<int>(boards_set.begin(), boards_set.end());

  const auto &gt = line.do_first();
  if (!gt.Empty() && !gt.demi1.opp_card.empty() && !gt.demi2.opp_card.empty()) {
    node->trick = gt.ToString(true);
    node->trick_full = gt.ToString(false);
    node->label = gt.ToString(true);
  } else {
    node->label = "Success: " + std::to_string(success_cnt) + "/" +
                  std::to_string(total) + " (" + std::to_string(pct) + "%)";
  }

  const auto &conds = line.conditions();
  for (size_t i = 0; i < conds.size(); ++i) {
    const auto &[cond_ptr, child_line_ptr] = conds[i];
    auto cond_node = std::make_shared<DecisionTreeNode>();
    cond_node->id = "node_" + std::to_string(++node_id);
    cond_node->type = "condition";
    std::string cond_desc = cond_ptr ? cond_ptr->Describe(conds.size() == 1) : "";
    cond_desc = NormalizeDirectionAndCondition(cond_desc);
    cond_node->label = cond_desc;
    cond_node->description = cond_desc;
    cond_node->collapsed = true;

    if (child_line_ptr) {
      auto child_node = BuildTreeFromConditionalLine(*child_line_ptr, node_id, depth + 1, total);
      cond_node->success_boards = child_node->success_boards;
      cond_node->failure_boards = child_node->failure_boards;
      cond_node->total_boards = child_node->total_boards;
      cond_node->covered_boards = child_node->covered_boards;
      cond_node->boards = child_node->boards;
      cond_node->children.push_back(child_node);
    }
    node->children.push_back(cond_node);
  }

  return node;
}

bool CanCollapseNodes(const nlohmann::json &parent, const nlohmann::json &child);

std::string NormalizeDirectionAndCondition(const std::string &s) {
  std::string res = s;
  struct Replacement {
    std::string from;
    std::string to;
  };
  static const std::vector<Replacement> repls = {
      {"when 0 wins", "N wins"}, {"when 1 wins", "E wins"},
      {"when 2 wins", "S wins"}, {"when 3 wins", "W wins"},
      {"if 0 wins", "N wins"},   {"if 1 wins", "E wins"},
      {"if 2 wins", "S wins"},   {"if 3 wins", "W wins"},
      {"when N wins", "N wins"}, {"when E wins", "E wins"},
      {"when S wins", "S wins"}, {"when W wins", "W wins"},
      {"if N wins", "N wins"},   {"if E wins", "E wins"},
      {"if S wins", "S wins"},   {"if W wins", "W wins"},
      {"0 wins", "N wins"},      {"1 wins", "E wins"},
      {"2 wins", "S wins"},      {"3 wins", "W wins"}};

  for (const auto &r : repls) {
    size_t pos = 0;
    while ((pos = res.find(r.from, pos)) != std::string::npos) {
      res.replace(pos, r.from.length(), r.to);
      pos += r.to.length();
    }
  }
  return res;
}

struct LabelParts {
  std::string trick;
  std::string cond;
};

LabelParts ParseLabelParts(const std::string &raw) {
  std::string s = NormalizeDirectionAndCondition(raw);
  if (s.rfind("after ", 0) == 0) s = s.substr(6);

  LabelParts parts;
  static const std::vector<std::string> cond_strings = {
      "N wins", "E wins", "S wins", "W wins"};

  for (const auto &c : cond_strings) {
    if (s == c) {
      parts.cond = c;
      return parts;
    }
    std::string parenthesized = "(" + c + ")";
    size_t pos = s.find(parenthesized);
    if (pos != std::string::npos) {
      parts.cond = c;
      std::string t = s;
      t.erase(pos, parenthesized.length());
      size_t last = t.find_last_not_of(" \t");
      if (last != std::string::npos) t = t.substr(0, last + 1);
      parts.trick = t;
      return parts;
    }
    std::string when_cond = "when " + c;
    pos = s.find(when_cond);
    if (pos != std::string::npos) {
      parts.cond = c;
      std::string t = s;
      t.erase(pos, when_cond.length());
      size_t last = t.find_last_not_of(" \t()");
      if (last != std::string::npos) t = t.substr(0, last + 1);
      parts.trick = t;
      return parts;
    }
    std::string trailing_cond = " " + c;
    if (s.length() > trailing_cond.length() &&
        s.rfind(trailing_cond) == s.length() - trailing_cond.length()) {
      parts.cond = c;
      std::string t = s.substr(0, s.length() - trailing_cond.length());
      size_t last = t.find_last_not_of(" \t()");
      if (last != std::string::npos) t = t.substr(0, last + 1);
      parts.trick = t;
      return parts;
    }
  }

  parts.trick = s;
  return parts;
}

std::string ExtractTrickFromNode(const nlohmann::json &node) {
  if (node.contains("trick") && node["trick"].is_string()) {
    std::string t = node["trick"].get<std::string>();
    if (!t.empty()) return t;
  }
  if (!node.contains("label") || !node["label"].is_string()) {
    return "";
  }
  std::string l = node["label"].get<std::string>();
  if (l.rfind("Success:", 0) == 0 ||
      l.rfind("Hand Succeeded", 0) == 0 ||
      l.rfind("Contract Result:", 0) == 0 ||
      l.rfind("Max depth reached", 0) == 0) {
    return "";
  }
  LabelParts parts = ParseLabelParts(l);
  return parts.trick;
}

bool CanCollapseNodes(const nlohmann::json &parent, const nlohmann::json &child) {
  if (!parent.contains("type") || !child.contains("type")) {
    return false;
  }
  std::string p_type = parent["type"].get<std::string>();
  std::string c_type = child["type"].get<std::string>();

  // Never collapse an outcome parent with anything.
  if (p_type == "outcome") {
    return false;
  }

  // An action parent CANNOT collapse directly into an outcome child.
  if (p_type == "action" && c_type == "outcome") {
    return false;
  }

  if (parent.contains("label")) {
    std::string p_label = parent["label"].get<std::string>();
    if (p_label.rfind("Optimal Lines", 0) == 0 || p_label.rfind("Trick ", 0) == 0) {
      return false;
    }
  }
  if (child.contains("label")) {
    std::string c_label = child["label"].get<std::string>();
    if (c_label.rfind("Trick ", 0) == 0) {
      return false;
    }
  }

  std::string p_trick = ExtractTrickFromNode(parent);
  std::string c_trick = ExtractTrickFromNode(child);

  auto is_win_cond = [](const std::string &t) {
    return (t == "N wins" || t == "E wins" || t == "S wins" || t == "W wins" ||
            t == "0 wins" || t == "1 wins" || t == "2 wins" || t == "3 wins" ||
            t.find("wins") != std::string::npos);
  };

  // A condition node describes the outcome/condition of its parent action,
  // NOT its child action. Therefore, a condition parent should NOT collapse
  // an action child into itself, UNLESS parent and child describe the exact same trick.
  if (p_type == "condition" && c_type == "action") {
    if (p_trick.empty() || c_trick.empty() || is_win_cond(p_trick) || is_win_cond(c_trick) || p_trick != c_trick) {
      return false;
    }
  }

  // If both nodes describe trick play steps and the tricks are different,
  // then they represent different tricks/play steps and MUST NOT be collapsed.
  if (!p_trick.empty() && !c_trick.empty() && !is_win_cond(p_trick) && !is_win_cond(c_trick)) {
    if (p_trick != c_trick) {
      return false;
    }
  }

  return true;
}

std::string CombineLabels(const std::string &l1, const std::string &l2) {
  if (l1 == l2) return NormalizeDirectionAndCondition(l1);

  std::string s1 = NormalizeDirectionAndCondition(l1);
  std::string s2 = NormalizeDirectionAndCondition(l2);

  if (s1.empty()) return s2;
  if (s2.empty()) return s1;
  if (s1 == s2) return s1;

  auto is_outcome = [](const std::string &s) {
    return (s.rfind("Success:", 0) == 0 ||
            s.rfind("Hand Succeeded", 0) == 0 ||
            s.rfind("Contract Result:", 0) == 0 ||
            s.rfind("Max depth reached", 0) == 0);
  };

  bool s1_outcome = is_outcome(s1);
  bool s2_outcome = is_outcome(s2);

  if (s1_outcome != s2_outcome) {
    std::string outcome_str = s1_outcome ? s1 : s2;
    std::string cond_str = s1_outcome ? s2 : s1;

    LabelParts cp = ParseLabelParts(cond_str);
    std::string win_cond = !cp.cond.empty() ? cp.cond : cond_str;
    if (win_cond.rfind("if ", 0) == 0) win_cond = win_cond.substr(3);
    if (win_cond.rfind("when ", 0) == 0) win_cond = win_cond.substr(5);

    if (win_cond.empty() || win_cond.rfind("after ", 0) == 0) {
      return outcome_str;
    }

    if (outcome_str.find(win_cond) != std::string::npos) {
      return outcome_str;
    }

    if (outcome_str.rfind("Hand Succeeded (", 0) == 0) {
      return "Hand Succeeded (" + win_cond + ") " + outcome_str.substr(15);
    } else if (outcome_str == "Hand Succeeded") {
      return "Hand Succeeded (" + win_cond + ")";
    } else {
      return outcome_str + " (" + win_cond + ")";
    }
  }

  bool has_after = (s1.rfind("after ", 0) == 0 || s2.rfind("after ", 0) == 0);

  LabelParts p1 = ParseLabelParts(s1);
  LabelParts p2 = ParseLabelParts(s2);

  std::string merged_trick = !p1.trick.empty() ? p1.trick : p2.trick;
  if (!p1.trick.empty() && !p2.trick.empty() && p1.trick != p2.trick) {
    merged_trick = p1.trick + " → " + p2.trick;
  }

  std::string merged_cond = !p1.cond.empty() ? p1.cond : p2.cond;

  if (merged_trick.empty()) {
    if (!merged_cond.empty()) {
      return (has_after ? "after " : "") + merged_cond;
    }
    return "";
  }

  std::string result = merged_trick;
  if (!merged_cond.empty()) {
    result += " (" + merged_cond + ")";
  }
  return result;
}

void CollapseRedundantTreeNodes(nlohmann::json &node) {
  if (!node.contains("children") || !node["children"].is_array()) {
    return;
  }

  while (node["children"].size() == 1) {
    auto &child = node["children"][0];
    if (!CanCollapseNodes(node, child)) {
      break;
    }

    std::string p_label = node.contains("label") ? node["label"].get<std::string>() : "";
    std::string c_label = child.contains("label") ? child["label"].get<std::string>() : "";
    node["label"] = CombineLabels(p_label, c_label);

    if (child.contains("type")) {
      std::string c_type = child["type"].get<std::string>();
      if (c_type == "action" || c_type == "outcome") {
        node["type"] = c_type;
      }
    }

    if (child.contains("trick") && (!node.contains("trick") || node["trick"].get<std::string>().empty())) {
      node["trick"] = child["trick"];
    }
    if (child.contains("trick_full") && (!node.contains("trick_full") || node["trick_full"].get<std::string>().empty())) {
      node["trick_full"] = child["trick_full"];
    }

    if (child.contains("success_boards")) node["success_boards"] = child["success_boards"];
    if (child.contains("failure_boards")) node["failure_boards"] = child["failure_boards"];
    if (child.contains("total_boards")) node["total_boards"] = child["total_boards"];
    if (child.contains("covered_boards")) node["covered_boards"] = child["covered_boards"];
    if (child.contains("boards")) node["boards"] = child["boards"];

    node["children"] = child["children"];
  }

  if (node.contains("children") && node["children"].is_array()) {
    for (auto &child : node["children"]) {
      CollapseRedundantTreeNodes(child);
    }
  }
}

std::vector<std::string> SplitTricks(const std::string &line_str) {
  std::vector<std::string> raw_segments;
  size_t start = 0;
  size_t end = line_str.find('/');
  while (end != std::string::npos) {
    std::string seg = line_str.substr(start, end - start);
    size_t f = seg.find_first_not_of(" \t");
    size_t l = seg.find_last_not_of(" \t");
    if (f != std::string::npos && l != std::string::npos) {
      raw_segments.push_back(seg.substr(f, l - f + 1));
    }
    start = end + 1;
    end = line_str.find('/', start);
  }
  std::string last_seg = line_str.substr(start);
  size_t f = last_seg.find_first_not_of(" \t");
  size_t l = last_seg.find_last_not_of(" \t");
  if (f != std::string::npos && l != std::string::npos) {
    raw_segments.push_back(last_seg.substr(f, l - f + 1));
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
    const OptimalLines &lines, int tricks_won, int &node_id) {
  auto root_node = std::make_shared<DecisionTreeNode>();
  root_node->id = "node_" + std::to_string(++node_id);
  root_node->type = "action";
  root_node->label = "Optimal Lines (" + std::to_string(lines.size()) +
                     " variant" + (lines.size() == 1 ? "" : "s") + ")";
  root_node->collapsed = false;
  root_node->success_boards = 1;
  root_node->failure_boards = 0;
  root_node->total_boards = 1;
  root_node->covered_boards = 1;

  for (const auto &line_str : lines) {
    std::vector<std::string> tricks = SplitTricks(line_str);
    if (tricks.empty()) continue;

    auto current = root_node;
    for (size_t i = 0; i < tricks.size(); ++i) {
      std::string trick_label = "Trick " + std::to_string(i + 1) + ": " + tricks[i];
      auto it = current->child_map.find(trick_label);
      if (it == current->child_map.end()) {
        auto child = std::make_shared<DecisionTreeNode>();
        child->id = "node_" + std::to_string(++node_id);
        child->type = "action";
        child->label = trick_label;
        child->trick = ExtractFirstCard(tricks[i]);
        child->collapsed = true;
        child->success_boards = 1;
        child->failure_boards = 0;
        child->total_boards = 1;
        child->covered_boards = 1;
        current->child_map[trick_label] = child;
        current->children.push_back(child);
        current = child;
      } else {
        current = it->second;
      }
    }

    std::string outcome_label = "Contract Result: " + std::to_string(tricks_won) + " tricks won";
    if (current->child_map.find(outcome_label) == current->child_map.end()) {
      auto outcome = std::make_shared<DecisionTreeNode>();
      outcome->id = "node_" + std::to_string(++node_id);
      outcome->type = "outcome";
      outcome->label = outcome_label;
      outcome->collapsed = true;
      outcome->success_boards = 1;
      outcome->failure_boards = 0;
      outcome->total_boards = 1;
      outcome->covered_boards = 1;
      current->child_map[outcome_label] = outcome;
      current->children.push_back(outcome);
    }
  }

  return root_node;
}

void to_json(nlohmann::json &j, const DecisionTreeNode &node) {
  j = nlohmann::json{
      {"id", node.id},
      {"type", node.type},
      {"label", node.label},
      {"collapsed", node.collapsed},
      {"success_boards", node.success_boards},
      {"failure_boards", node.failure_boards},
      {"total_boards", node.total_boards},
      {"covered_boards", node.covered_boards},
  };
  if (!node.description.empty()) {
    j["description"] = node.description;
  }
  if (!node.trick.empty()) {
    j["trick"] = node.trick;
  }
  if (!node.trick_full.empty()) {
    j["trick_full"] = node.trick_full;
  }
  if (!node.boards.empty() || node.type == "action" || node.type == "condition") {
    j["boards"] = node.boards;
  }

  nlohmann::json children = nlohmann::json::array();
  for (const auto &child : node.children) {
    if (child) {
      children.push_back(*child);
    }
  }
  j["children"] = children;
}

uint64_t ParseUnknownCards(const std::string &unknown_str,
                           const std::vector<uint64_t> &hands,
                           std::string &error_msg) {
  uint64_t combined_known_cards = hands[0] | hands[1] | hands[2] | hands[3];

  std::vector<int> available_ranks[4];
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    for (int r = 12; r >= 0; --r) {
      uint64_t card_bit = Card::GetSuitedCard(suit, r);
      if ((combined_known_cards & card_bit) == 0) {
        available_ranks[s].push_back(r);
      }
    }
  }

  uint64_t unknown_cards = 0;
  Suit cur_suit = SPADES;

  for (size_t i = 0; i < unknown_str.size(); ++i) {
    char c = unknown_str[i];
    if (c == '1' && i + 1 < unknown_str.size() && unknown_str[i + 1] == '0') {
      c = 'T';
      i++;
    }

    if (std::isspace(static_cast<unsigned char>(c)) || c == ';' || c == ',' || c == '-') {
      continue;
    }

    if (c == 'S' || c == 's') { cur_suit = SPADES; continue; }
    if (c == 'H' || c == 'h') { cur_suit = HEARTS; continue; }
    if (c == 'D' || c == 'd') { cur_suit = DIAMONDS; continue; }
    if (c == 'C' || c == 'c') { cur_suit = CLUBS; continue; }

    int card_val = -1;
    if (c == 'A' || c == 'a') card_val = 12;
    else if (c == 'K' || c == 'k') card_val = 11;
    else if (c == 'Q' || c == 'q') card_val = 10;
    else if (c == 'J' || c == 'j') card_val = 9;
    else if (c == 'T' || c == 't') card_val = 8;
    else if (c >= '2' && c <= '9') card_val = c - '2';

    if (card_val >= 0) {
      uint64_t card_bit = Card::GetSuitedCard(cur_suit, card_val);
      if ((combined_known_cards & card_bit) != 0) {
        error_msg = "Card " + Card::PrintCard(card_bit) + " in unknown cards conflicts with a known hand card.";
        return 0;
      }
      if ((unknown_cards & card_bit) != 0) {
        error_msg = "Duplicate card " + Card::PrintCard(card_bit) + " in unknown cards specification.";
        return 0;
      }
      unknown_cards |= card_bit;
      auto &ranks = available_ranks[static_cast<int>(cur_suit)];
      auto it = std::find(ranks.begin(), ranks.end(), card_val);
      if (it != ranks.end()) {
        ranks.erase(it);
      }
      continue;
    }

    if (c == 'x' || c == 'X') {
      auto &ranks = available_ranks[static_cast<int>(cur_suit)];
      if (ranks.empty()) {
        error_msg = "Not enough outstanding cards available in " + SuitDebugString(cur_suit) + " for wildcard 'x'.";
        return 0;
      }
      int picked_rank = ranks.front();
      ranks.erase(ranks.begin());
      uint64_t card_bit = Card::GetSuitedCard(cur_suit, picked_rank);
      unknown_cards |= card_bit;
      continue;
    }

    error_msg = std::string("Unrecognized character '") + c + "' in unknown cards specification.";
    return 0;
  }

  int parsed_count = CountCardsInHand(unknown_cards);
  int n_cnt = CountCardsInHand(hands[0]);
  int e_cnt = CountCardsInHand(hands[1]);
  int s_cnt = CountCardsInHand(hands[2]);
  int w_cnt = CountCardsInHand(hands[3]);

  int max_cnt = std::max({n_cnt, e_cnt, s_cnt, w_cnt});
  if (max_cnt == 0) max_cnt = 13;
  int expected = (4 * max_cnt) - (n_cnt + e_cnt + s_cnt + w_cnt);

  if (parsed_count != expected) {
    error_msg = "Invalid unknown cards count: specified " + std::to_string(parsed_count) +
                " unknown card(s), but expected " + std::to_string(expected) + " based on hand sizes.";
    return 0;
  }

  return unknown_cards;
}

uint64_t ComputeUnknownCards(const std::vector<uint64_t>& hands) {
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

}  // namespace

void to_json(nlohmann::json &j, const CrossHandSummaryReport &summary) {
  j["total_deals"] = summary.total_deals;
  j["cards_per_hand"] = summary.cards_per_hand;
  j["unknown_cards"] = summary.unknown_cards;
  j["invariant_rules"] = summary.invariant_rules;

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
  RegisterRoutes();
}

WebServer::~WebServer() {
  Stop();
}

void WebServer::RegisterRoutes() {
  svr_.Get("/", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(web_assets::INDEX_HTML, "text/html");
  });

  svr_.Get("/index.html", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(web_assets::INDEX_HTML, "text/html");
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

  svr_.Get("/api/health", [this](const httplib::Request &req, httplib::Response &res) {
    HandleHealth(req, res);
  });

  svr_.Post("/api/solve", [this](const httplib::Request &req, httplib::Response &res) {
    HandleSolve(req, res);
  });
}

void WebServer::HandleHealth(const httplib::Request &, httplib::Response &res) {
  nlohmann::json response;
  response["status"] = "ok";
  response["service"] = "bridge_solver_http_server";
  res.set_content(response.dump(), "application/json");
}

void WebServer::HandleSolve(const httplib::Request &req, httplib::Response &res) {
  nlohmann::json response;
  try {
    auto start_time = std::chrono::high_resolution_clock::now();

    nlohmann::json req_json = nlohmann::json::parse(req.body);

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

    int num_iters = 10;
    if (req_json.contains("num_iters")) {
      num_iters = req_json["num_iters"].get<int>();
    }

    int n_cnt = CountCardsInHand(hands[0]);
    int e_cnt = CountCardsInHand(hands[1]);
    int s_cnt = CountCardsInHand(hands[2]);
    int w_cnt = CountCardsInHand(hands[3]);

    std::string unknown_str = "";
    if (req_json.contains("unknown_cards") && req_json["unknown_cards"].is_string()) {
      unknown_str = req_json["unknown_cards"].get<std::string>();
    }

    uint64_t unknown_cards = 0;
    std::string unknown_err = "";

    if (!unknown_str.empty()) {
      unknown_cards = ParseUnknownCards(unknown_str, hands, unknown_err);
      if (!unknown_err.empty()) {
        response["success"] = false;
        response["error"] = unknown_err;
        res.status = 400;
        res.set_content(response.dump(), "application/json");
        return;
      }
    } else {
      unknown_cards = ComputeUnknownCards(hands);
    }

    bool is_fuzzy = false;
    if (unknown_cards != 0 || req_json.contains("requirements") || !unknown_str.empty()) {
      is_fuzzy = true;
    }

    Suit trump_suit = NOTRUMP;
    if (req_json.contains("trump")) {
      trump_suit = ParseSuitName(req_json["trump"].get<std::string>());
    }

    int leader = NORTH;
    if (req_json.contains("leader")) {
      if (req_json["leader"].is_number()) {
        leader = req_json["leader"].get<int>() % 4;
      } else {
        leader = ParsePlayerDirection(req_json["leader"].get<std::string>());
      }
    }

    int min_tricks = 1;
    if (req_json.contains("min_tricks")) {
      min_tricks = req_json["min_tricks"].get<int>();
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

    if (is_fuzzy) {
      total_boards = num_iters;
      LineFinder lf(num_iters);
      if (req_json.contains("requirements")) {
        auto &reqs_json = req_json["requirements"];
        std::vector<std::vector<std::shared_ptr<HandRequirements>>> reqs(4);
        for (int i = 0; i < 4; ++i) {
          std::string p_key = (i == 0 ? "N" : (i == 1 ? "E" : (i == 2 ? "S" : "W")));
          if (reqs_json.contains(p_key)) {
            auto &p_req = reqs_json[p_key];
            if (p_req.contains("hcp_low") && p_req.contains("hcp_high")) {
              reqs[i].push_back(std::make_shared<HighCardRange>(
                  p_req["hcp_low"].get<int>(), p_req["hcp_high"].get<int>()));
            }
            if (p_req.contains("spades_min")) {
              reqs[i].push_back(std::make_shared<SuitAtLeast>(
                  SPADES, p_req["spades_min"].get<int>()));
            }
          }
        }
        lf.SetRequirements(reqs);
      }

      std::vector<ConditionalLine> lines =
          lf.Solve(min_tricks, hands, unknown_cards, leader, trump_suit, {});

      if (!lines.empty()) {
        tree_node = BuildTreeFromConditionalLine(lines[0], node_counter);
        success_boards = lines[0].CoveredBoards();
        failure_boards = std::max(0, total_boards - success_boards);
        covered_boards = success_boards;
        tricks_won = min_tricks;
        if (!lines[0].do_first().Empty()) {
          best_lead = lines[0].do_first().ToString(true);
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
        tree_node = BuildTreeFromOptimalLines(optimal_lines, tricks_won, node_counter);
      }
    } else {
      BoardState bs;
      bs.SetHands(hands);
      bs.SetTrump(trump_suit);
      bs.SetLead(leader);
      bs.MarkMissingCardsAsPlayed();

      tricks_won = MostTricksCanBeWon(bs, leader % 2, &best_seq, &optimal_lines);
      best_lead = ExtractFirstCard(best_seq);
      tree_node = BuildTreeFromOptimalLines(optimal_lines, tricks_won, node_counter);
      total_boards = 1;
      success_boards = 1;
      failure_boards = 0;
      covered_boards = 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms =
        std::chrono::duration<double, std::milli>(end_time - start_time).count();

    CollapseRedundantTreeNodes(tree_json);

    response["success"] = true;
    response["tricks_won"] = tricks_won;
    response["best_lead"] = best_lead;
    response["sequence"] = best_seq;
    response["total_boards"] = total_boards;
    response["success_boards"] = success_boards;
    response["failure_boards"] = failure_boards;
    response["covered_boards"] = covered_boards;
    if (tree_node) {
      response["tree"] = *tree_node;
    } else {
      response["tree"] = nlohmann::json::object();
    }
    response["elapsed_ms"] = elapsed_ms;

    res.set_content(response.dump(), "application/json");
  } catch (const std::exception &e) {
    LOG(ERROR) << "Exception in HandleSolve: " << e.what();
    response["success"] = false;
    response["error"] = e.what();
    res.status = 400;
    res.set_content(response.dump(), "application/json");
  }
}

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
