#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "conditional_line.h"
#include "third_party/httplib.h"
#include "third_party/nlohmann/json.hpp"

struct DecisionTreeNode {
  std::string id;
  std::string type;  // "action", "condition", "outcome"
  std::string label;
  std::string description;
  std::string trick;
  std::string trick_full;
  std::string condition;
  bool collapsed = false;
  int ns_tricks = 0;
  int success_boards = 0;
  int failure_boards = 0;
  int total_boards = 0;
  int covered_boards = 0;
  std::vector<int> boards;
  std::map<std::string, std::string> board_to_trick;
  std::vector<std::shared_ptr<DecisionTreeNode>> children;

  std::map<std::string, std::shared_ptr<DecisionTreeNode>> child_map;
};

std::string ExtractFirstCard(const std::string &seq);
std::string FormatWinnerCondition(const PlayCondition *cond_ptr);
std::string FormatBranchCondition(const PlayCondition *cond_ptr, bool only_choice = false);
std::shared_ptr<DecisionTreeNode> BuildTreeFromConditionalLine(
    const ConditionalLine &line, int &node_id, int trick_num = 1, int root_total = 1, int ns_tricks = 0);
void to_json(nlohmann::json &j, const DecisionTreeNode &node);

class WebServer {
 public:
  WebServer(const std::string &host = "0.0.0.0", int port = 8080);
  ~WebServer();

  bool Start(bool async = false);
  void Stop();

  int port() const { return port_; }
  std::string host() const { return host_; }

 private:
  void RegisterRoutes();
  void HandleHealth(const httplib::Request &req, httplib::Response &res);
  void HandleSolve(const httplib::Request &req, httplib::Response &res);
  void HandleSolveStatus(const httplib::Request &req, httplib::Response &res);

  std::string host_;
  int port_;
  httplib::Server svr_;
  std::unique_ptr<std::thread> server_thread_;
  std::atomic<bool> running_{false};
};
