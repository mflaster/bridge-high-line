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
  if (s == "SPADES" || s == "S" || s == "spades") return SPADES;
  if (s == "HEARTS" || s == "H" || s == "hearts") return HEARTS;
  if (s == "DIAMONDS" || s == "D" || s == "diamonds") return DIAMONDS;
  if (s == "CLUBS" || s == "C" || s == "clubs") return CLUBS;
  return NOTRUMP;
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

nlohmann::json ConditionalLineToJSON(const ConditionalLine &line, int &node_id, int depth = 0) {
  if (depth > 20) {
    nlohmann::json leaf;
    leaf["id"] = "node_" + std::to_string(++node_id);
    leaf["type"] = "outcome";
    leaf["label"] = "Max depth reached";
    leaf["covered_boards"] = 0;
    leaf["children"] = nlohmann::json::array();
    return leaf;
  }
  nlohmann::json node;
  node["id"] = "node_" + std::to_string(++node_id);
  node["collapsed"] = (depth >= 1);

  const auto &final_boards = line.final_covered_boards();
  if (!final_boards.empty()) {
    node["type"] = "outcome";
    node["label"] = "Hand Succeeded (" + std::to_string(final_boards.size()) + " boards)";
    node["covered_boards"] = final_boards.size();
    node["boards"] = std::vector<int>(final_boards.begin(), final_boards.end());
    node["children"] = nlohmann::json::array();
    return node;
  }

  node["type"] = "action";
  std::set<int> boards_set = line.CoveredBoardsSet();
  node["covered_boards"] = boards_set.size();
  node["boards"] = std::vector<int>(boards_set.begin(), boards_set.end());

  const auto &gt = line.do_first();
  if (!gt.Empty() && !gt.demi1.opp_card.empty() && !gt.demi2.opp_card.empty()) {
    node["trick"] = gt.ToString(true);
    node["trick_full"] = gt.ToString(false);
    node["label"] = gt.ToString(true);
  } else {
    node["label"] = "Covers " + std::to_string(boards_set.size()) + " board(s)";
  }

  nlohmann::json children = nlohmann::json::array();
  const auto &conds = line.conditions();
  for (size_t i = 0; i < conds.size(); ++i) {
    const auto &[cond_ptr, child_line_ptr] = conds[i];
    nlohmann::json cond_node;
    cond_node["id"] = "node_" + std::to_string(++node_id);
    cond_node["type"] = "condition";
    std::string cond_desc = cond_ptr ? cond_ptr->Describe(conds.size() == 1) : "";
    cond_node["label"] = cond_desc;
    cond_node["description"] = cond_desc;
    cond_node["collapsed"] = true;

    if (child_line_ptr) {
      nlohmann::json child_json = ConditionalLineToJSON(*child_line_ptr, node_id, depth + 1);
      cond_node["covered_boards"] = child_json["covered_boards"];
      cond_node["boards"] = child_json["boards"];
      cond_node["children"] = nlohmann::json::array({child_json});
    } else {
      cond_node["children"] = nlohmann::json::array();
    }
    children.push_back(cond_node);
  }
  node["children"] = children;

  return node;
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

struct LineTreeNode {
  std::string label;
  std::string trick_first_card;
  std::map<std::string, std::shared_ptr<LineTreeNode>> children;
  bool is_outcome = false;
  int tricks_won = 0;
};

nlohmann::json LineTreeNodeToJSON(const std::shared_ptr<LineTreeNode> &node, int depth, int &node_id) {
  nlohmann::json json_node;
  json_node["id"] = "node_" + std::to_string(++node_id);
  if (node->is_outcome) {
    json_node["type"] = "outcome";
    json_node["label"] = node->label;
    json_node["covered_boards"] = 1;
    json_node["collapsed"] = true;
    json_node["children"] = nlohmann::json::array();
    return json_node;
  }

  json_node["type"] = "action";
  json_node["label"] = node->label;
  if (!node->trick_first_card.empty()) {
    json_node["trick"] = node->trick_first_card;
  }
  json_node["covered_boards"] = 1;
  json_node["collapsed"] = (depth >= 1);

  nlohmann::json children = nlohmann::json::array();
  for (const auto &[key, child_ptr] : node->children) {
    children.push_back(LineTreeNodeToJSON(child_ptr, depth + 1, node_id));
  }
  json_node["children"] = children;
  return json_node;
}

nlohmann::json BuildTreeFromOptimalLines(const OptimalLines &lines, int tricks_won, int &node_id) {
  auto root_node = std::make_shared<LineTreeNode>();
  root_node->label = "Optimal Lines (" + std::to_string(lines.size()) + " variant" + (lines.size() == 1 ? "" : "s") + ")";

  for (const auto &line_str : lines) {
    std::vector<std::string> tricks = SplitTricks(line_str);
    if (tricks.empty()) continue;

    auto current = root_node;
    for (size_t i = 0; i < tricks.size(); ++i) {
      std::string trick_label = "Trick " + std::to_string(i + 1) + ": " + tricks[i];
      if (current->children.find(trick_label) == current->children.end()) {
        auto child = std::make_shared<LineTreeNode>();
        child->label = trick_label;
        child->trick_first_card = ExtractFirstCard(tricks[i]);
        current->children[trick_label] = child;
      }
      current = current->children[trick_label];
    }

    std::string outcome_label = "Contract Result: " + std::to_string(tricks_won) + " tricks won";
    if (current->children.find(outcome_label) == current->children.end()) {
      auto outcome = std::make_shared<LineTreeNode>();
      outcome->label = outcome_label;
      outcome->is_outcome = true;
      outcome->tricks_won = tricks_won;
      current->children[outcome_label] = outcome;
    }
  }

  return LineTreeNodeToJSON(root_node, 0, node_id);
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

nlohmann::json CrossHandSummaryToJSON(const CrossHandSummaryReport &summary) {
  nlohmann::json j;
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

  return j;
}

}  // namespace

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

    uint64_t unknown_cards = ComputeUnknownCards(hands);

    bool is_fuzzy = false;
    if (unknown_cards != 0 || req_json.contains("requirements")) {
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

    nlohmann::json tree_json;
    int node_counter = 0;
    int covered_boards = 1;

    if (is_fuzzy) {
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
        tree_json = ConditionalLineToJSON(lines[0], node_counter);
        covered_boards = lines[0].CoveredBoards();
        tricks_won = min_tricks;
        if (!lines[0].do_first().Empty()) {
          best_lead = lines[0].do_first().ToString(true);
        }
        response["distribution_summary"] = CrossHandSummaryToJSON(lines[0].summary());
      } else {
        tree_json = BuildTreeFromOptimalLines(optimal_lines, tricks_won, node_counter);
      }
    } else {
      BoardState bs;
      bs.SetHands(hands);
      bs.SetTrump(trump_suit);
      bs.SetLead(leader);
      bs.MarkMissingCardsAsPlayed();

      tricks_won = MostTricksCanBeWon(bs, leader % 2, &best_seq, &optimal_lines);
      best_lead = ExtractFirstCard(best_seq);
      tree_json = BuildTreeFromOptimalLines(optimal_lines, tricks_won, node_counter);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms =
        std::chrono::duration<double, std::milli>(end_time - start_time).count();

    response["success"] = true;
    response["tricks_won"] = tricks_won;
    response["best_lead"] = best_lead;
    response["sequence"] = best_seq;
    response["covered_boards"] = covered_boards;
    response["tree"] = tree_json;
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
