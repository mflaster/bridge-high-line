#include "web_server.h"

#include <chrono>
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

std::string PlayerDirectionName(int dir) {
  switch ((dir % 4 + 4) % 4) {
    case NORTH: return "North";
    case EAST: return "East";
    case SOUTH: return "South";
    case WEST: return "West";
    default: return "Unknown";
  }
}

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
    desc.replace(match.position(0), match.length(0), PlayerDirectionName(player) + " wins");
  }
  return desc;
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
    std::string dir;
    switch ((player % 4 + 4) % 4) {
      case NORTH: dir = "N"; break;
      case EAST:  dir = "E"; break;
      case SOUTH: dir = "S"; break;
      case WEST:  dir = "W"; break;
      default:    dir = std::to_string(player); break;
    }
    desc.replace(match.position(0), match.length(0), dir + " wins");
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
  if (node->label.rfind("Trick ", 0) == 0) {
    std::string card = ExtractFirstCard(node->label);
    if (!card.empty()) return card;
  }
  for (const auto &child : node->children) {
    std::string card = GetFirstCardFromNode(child);
    if (!card.empty()) return card;
  }
  return "";
}

std::shared_ptr<DecisionTreeNode> BuildTreeFromConditionalLine(
    const ConditionalLine &line, int &node_id, int trick_num, int root_total, int ns_tricks) {
  int total = line.total_boards() > 0 ? line.total_boards() : root_total;

  if (line.do_first().Empty()) {
    const auto &conds = line.conditions();
    if (!conds.empty()) {
      if (conds.size() == 1 && conds[0].second) {
        return BuildTreeFromConditionalLine(*conds[0].second, node_id, trick_num, total, ns_tricks);
      }
      auto root_node = std::make_shared<DecisionTreeNode>();
      root_node->id = "node_" + std::to_string(++node_id);
      root_node->type = "action";
      root_node->collapsed = false;
      root_node->total_boards = total;
      for (const auto &[cond_ptr, child_ptr] : conds) {
        if (child_ptr) {
          auto child = BuildTreeFromConditionalLine(*child_ptr, node_id, trick_num, total, 0);
          if (child && cond_ptr) {
            child->condition = FormatBranchCondition(cond_ptr.get(), false);
          }
          root_node->children.push_back(child);
        }
      }

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
        root_node->label = common_card + " is led (" + std::to_string(root_node->children.size()) + " variants)";
      } else {
        root_node->label = "Solve Lines (" + std::to_string(conds.size()) + " variants)";
      }

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
    return leaf;
  }

  const auto &gt = line.do_first();
  std::string cards_played = gt.ToString(true);
  std::set<int> boards_set = line.CoveredBoardsSet();
  int success_cnt = boards_set.size();
  int failure_cnt = std::max(0, total - success_cnt);

  const auto &conds = line.conditions();
  if (conds.size() == 1) {
    const auto &[cond_ptr, child_line_ptr] = conds[0];
    std::string winner_str = FormatWinnerCondition(cond_ptr.get());
    int add_ns = 0;
    if (auto w = dynamic_cast<const WinnerPlayCondition*>(cond_ptr.get())) {
      if (w->who() == NORTH || w->who() == SOUTH) {
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
    node->trick = cards_played;
    node->trick_full = gt.ToString(false);

    std::string label_str = "Trick " + std::to_string(trick_num) + ": " + cards_played;
    if (!winner_str.empty()) {
      label_str += " (" + winner_str + ")";
    }

    if (is_final_trick) {
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
      node->total_boards = total;
      node->label = label_str + " (" + std::to_string(current_ns) + " tricks won by N/S)";
      return node;
    } else {
      node->type = "action";
      node->ns_tricks = current_ns;
      node->label = label_str;
      node->success_boards = success_cnt;
      node->failure_boards = failure_cnt;
      node->total_boards = total;
      node->covered_boards = success_cnt;
      node->boards = std::vector<int>(boards_set.begin(), boards_set.end());

      auto child_node = BuildTreeFromConditionalLine(*child_line_ptr, node_id, trick_num + 1, total, current_ns);
      if (child_node) {
        node->children.push_back(child_node);
      }
      return node;
    }
  } else {
    auto node = std::make_shared<DecisionTreeNode>();
    node->id = "node_" + std::to_string(++node_id);
    node->type = "action";
    node->collapsed = false;
    node->ns_tricks = ns_tricks;
    node->trick = cards_played;
    node->trick_full = gt.ToString(false);
    node->label = "Trick " + std::to_string(trick_num) + ": " + cards_played;
    node->success_boards = success_cnt;
    node->failure_boards = failure_cnt;
    node->total_boards = total;
    node->covered_boards = success_cnt;
    node->boards = std::vector<int>(boards_set.begin(), boards_set.end());

    for (const auto &[cond_ptr, child_line_ptr] : conds) {
      if (child_line_ptr) {
        int branch_add_ns = 0;
        if (auto w = dynamic_cast<const WinnerPlayCondition*>(cond_ptr.get())) {
          if (w->who() == NORTH || w->who() == SOUTH) {
            branch_add_ns = 1;
          }
        }
        int branch_ns = ns_tricks + branch_add_ns;
        auto child = BuildTreeFromConditionalLine(*child_line_ptr, node_id, trick_num + 1, total, branch_ns);
        if (child) {
          if (cond_ptr) {
            child->condition = FormatBranchCondition(cond_ptr.get(), false);
          }
          node->children.push_back(child);
        }
      }
    }
    return node;
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
    const OptimalLines &lines, int tricks_won, int &node_id, const BoardState &initial_bs) {
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

    for (size_t i = 0; i < tricks.size(); ++i) {
      sim_bs.PlayTrick(tricks[i]);
      int winner = sim_bs.CurPlayer();
      std::string winner_name = PlayerDirectionName(winner);

      auto node = std::make_shared<DecisionTreeNode>();
      node->id = "node_" + std::to_string(++node_id);
      node->collapsed = false;
      node->trick = tricks[i];
      node->trick_full = tricks[i];
      node->success_boards = 1;
      node->failure_boards = 0;
      node->total_boards = 1;
      node->covered_boards = 1;

      std::string label_str = "Trick " + std::to_string(i + 1) + ": " + tricks[i] + " (" + winner_name + " wins)";

      if (i == tricks.size() - 1) {
        node->type = "outcome";
        node->label = label_str;
        node->description = std::to_string(tricks_won) + " tricks won";
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
    }

    if (first_trick_node) {
      line_roots.push_back(first_trick_node);
    }
  }

  if (line_roots.empty()) {
    auto root_node = std::make_shared<DecisionTreeNode>();
    root_node->id = "node_" + std::to_string(++node_id);
    root_node->type = "outcome";
    root_node->label = "Contract Result: " + std::to_string(tricks_won) + " tricks won";
    return root_node;
  }

  if (line_roots.size() == 1) {
    return line_roots[0];
  }

  auto root_node = std::make_shared<DecisionTreeNode>();
  root_node->id = "node_" + std::to_string(++node_id);
  root_node->type = "action";
  root_node->collapsed = false;
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

  if (all_same && !common_card.empty()) {
    root_node->label = common_card + " is led (" + std::to_string(line_roots.size()) + " variants)";
  } else {
    root_node->label = "Optimal Lines (" + std::to_string(line_roots.size()) + " variants)";
  }

  return root_node;
}

bool IsGenericOutcomeLabel(const std::string &l) {
  if (l.empty()) return true;
  if (l.rfind("Hand Succeeded", 0) == 0) return true;
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

    // Don't merge two distinct Trick steps into each other
    if (node->label.rfind("Trick ", 0) == 0 && child->label.rfind("Trick ", 0) == 0) {
      break;
    }
    if (node->label.rfind("Trick ", 0) == 0) {
      break;
    }

    if (node->condition.empty() && !child->condition.empty()) {
      node->condition = child->condition;
    }

    node = child;
  }
}

void SetDefaultCollapsedState(std::shared_ptr<DecisionTreeNode> &node, bool is_root = true) {
  if (!node) return;

  // A container root node (e.g. "Solve Lines (N variants)") has no trick of its own
  // and multiple child branches. It remains expanded (collapsed = false) so its child
  // branches (the first trick in each branch) are visible.
  if (is_root && node->trick.empty() && node->children.size() > 1 &&
      (node->label.find("Solve Lines") != std::string::npos ||
       node->label.find("Optimal Lines") != std::string::npos ||
       node->label.find("variants") != std::string::npos)) {
    node->collapsed = false;
  } else {
    // All trick nodes (e.g. Trick 1) and sub-nodes are collapsed by default.
    node->collapsed = true;
  }

  for (auto &child : node->children) {
    SetDefaultCollapsedState(child, false);
  }
}

void to_json(nlohmann::json &j, const DecisionTreeNode &node) {
  j = nlohmann::json{
      {"id", node.id},
      {"type", node.type},
      {"label", node.label},
      {"collapsed", node.collapsed},
      {"ns_tricks", node.ns_tricks},
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
  if (!node.condition.empty()) {
    j["condition"] = node.condition;
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
        tree_node = BuildTreeFromOptimalLines(optimal_lines, tricks_won, node_counter, BoardState());
      }
    } else {
      BoardState bs;
      bs.SetHands(hands);
      bs.SetTrump(trump_suit);
      bs.SetLead(leader);
      bs.MarkMissingCardsAsPlayed();

      tricks_won = MostTricksCanBeWon(bs, leader % 2, &best_seq, &optimal_lines);
      best_lead = ExtractFirstCard(best_seq);
      tree_node = BuildTreeFromOptimalLines(optimal_lines, tricks_won, node_counter, bs);
      total_boards = 1;
      success_boards = 1;
      failure_boards = 0;
      covered_boards = 1;
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
    if (tree_node) {
      CollapseRedundantNodes(tree_node);
      SetDefaultCollapsedState(tree_node);
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
