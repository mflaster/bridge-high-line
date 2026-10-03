#include "web_server.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "board_state.h"
#include "card.h"
#include "conditional_line.h"
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

nlohmann::json ConditionalLineToJSON(const ConditionalLine &line, int &node_id) {
  nlohmann::json node;
  node["id"] = "node_" + std::to_string(++node_id);

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
  node["covered_boards"] = line.CoveredBoards();
  node["boards"] = std::vector<int>(line.CoveredBoardsSet().begin(), line.CoveredBoardsSet().end());

  const auto &gt = line.do_first();
  if (!gt.Empty()) {
    node["trick"] = gt.ToString(true);
    node["trick_full"] = gt.ToString(false);
    node["label"] = gt.ToString(true);
  } else {
    node["label"] = "Covers " + std::to_string(line.CoveredBoards()) + " board(s)";
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

    if (child_line_ptr) {
      nlohmann::json child_json = ConditionalLineToJSON(*child_line_ptr, node_id);
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

nlohmann::json BuildTreeFromOptimalLines(const OptimalLines &lines, int tricks_won, int &node_id) {
  nlohmann::json root;
  root["id"] = "node_" + std::to_string(++node_id);
  root["type"] = "action";
  root["label"] = "Optimal Lines (" + std::to_string(lines.size()) + " variant" + (lines.size() == 1 ? "" : "s") + ")";
  root["covered_boards"] = 1;

  nlohmann::json children = nlohmann::json::array();
  for (const auto &line_str : lines) {
    nlohmann::json line_node;
    line_node["id"] = "node_" + std::to_string(++node_id);
    line_node["type"] = "action";
    line_node["label"] = line_str;
    line_node["trick"] = ExtractFirstCard(line_str);
    line_node["covered_boards"] = 1;

    // Add Outcome child
    nlohmann::json outcome_node;
    outcome_node["id"] = "node_" + std::to_string(++node_id);
    outcome_node["type"] = "outcome";
    outcome_node["label"] = "Contract Result: " + std::to_string(tricks_won) + " tricks won";
    outcome_node["covered_boards"] = 1;
    outcome_node["children"] = nlohmann::json::array();

    line_node["children"] = nlohmann::json::array({outcome_node});
    children.push_back(line_node);
  }

  root["children"] = children;
  return root;
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

    uint64_t total_known = hands[0] | hands[1] | hands[2] | hands[3];
    uint64_t unknown_cards = 0;

    bool is_fuzzy = false;
    if ((n_cnt + e_cnt + s_cnt + w_cnt) < 52 && (n_cnt != e_cnt || s_cnt != w_cnt)) {
      unknown_cards = ~total_known;
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

    // Solve using BoardState and MostTricksCanBeWon
    BoardState bs;
    bs.SetHands(hands);
    bs.SetTrump(trump_suit);
    bs.SetLead(leader);
    bs.MarkMissingCardsAsPlayed();

    std::string best_seq;
    OptimalLines optimal_lines;
    int tricks_won = MostTricksCanBeWon(bs, leader % 2, &best_seq, &optimal_lines);
    std::string best_lead = ExtractFirstCard(best_seq);

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
      } else {
        tree_json = BuildTreeFromOptimalLines(optimal_lines, tricks_won, node_counter);
      }
    } else {
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
    if (server_thread_ && server_thread_->joinable()) {
      server_thread_->join();
    }
    running_ = false;
  }
}
