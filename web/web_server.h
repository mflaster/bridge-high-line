#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <set>
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
  // True for outcome leaves where the contract failed.
  bool failure = false;
  // Seat index (0=N, 1=E, 2=S, 3=W) of the trick leader, or -1 if not a trick.
  int leader = -1;
  // Seat index (0=N, 1=E, 2=S, 3=W) of the trick winner at this trick node,
  // or -1 if not a trick node.
  int winner = -1;
  // Seat index (0=N, 1=E, 2=S, 3=W) of the parent trick's winner along this
  // branch, or -1 if not a winner-conditioned branch.
  int branch_winner = -1;
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

// Where the failed boards leave the line of play. Each failed board follows
// the line's prescribed cards until the defenders' play reaches a position the
// line has no answer for; it is attached there as a failure leaf.
struct FailureIndex {
  struct Group {
    // Branch label explaining how these boards left the line, e.g.
    // "if W plays DK" (a card outside every pattern) or "if E wins the SA"
    // (a trick winner the line has no continuation for).
    std::string condition;
    int winner = -1;
    std::vector<int> boards;
    int min_ns_tricks = 0;
    int max_ns_tricks = 0;
  };
  // Concrete trick played on each failed board at each line node it passed.
  std::map<const ConditionalLine *, std::map<std::string, std::string>> tricks;
  // Failed boards that left the line after the given node, keyed by a
  // condition key that is unique per (reason, seat, card).
  std::map<const ConditionalLine *, std::map<std::string, Group>> leaves;
  // Failed boards for which no trick of the line could be played at all.
  Group unplaced;
};

std::string ExtractFirstCard(const std::string &seq);
std::string FormatWinnerCondition(const PlayCondition *cond_ptr);
std::string FormatBranchCondition(const PlayCondition *cond_ptr, bool only_choice = false);
FailureIndex IndexFailures(const ConditionalLine &line,
                           const std::vector<BoardState> &boards,
                           const std::vector<int> &failed_board_ids,
                           int num_threads = 16);
std::shared_ptr<DecisionTreeNode> BuildTreeFromConditionalLine(
    const ConditionalLine &line, int &node_id, int trick_num = 1,
    int root_total = 1, int ns_tricks = 0,
    const FailureIndex *failures = nullptr);
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
