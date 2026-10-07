#include <catch2/catch_all.hpp>
#include <chrono>
#include <thread>

#include "line_finder.h"
#include "third_party/httplib.h"
#include "third_party/nlohmann/json.hpp"
#include "web_server.h"

TEST_CASE("WebServer Health and Static Assets") {
  int port = 18080;
  WebServer server("127.0.0.1", port);
  REQUIRE(server.Start(/*async=*/true));

  // Allow server thread to initialize
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  httplib::Client client("127.0.0.1", port);
  client.set_keep_alive(false);

  // Test static HTML serving
  auto res_index = client.Get("/");
  REQUIRE(res_index);
  REQUIRE(res_index->status == 200);
  REQUIRE(res_index->body.find("Bridge Deal Solver") != std::string::npos);

  // Test CSS serving
  auto res_css = client.Get("/styles.css");
  REQUIRE(res_css);
  REQUIRE(res_css->status == 200);

  // Test JS serving
  auto res_js = client.Get("/app.js");
  REQUIRE(res_js);
  REQUIRE(res_js->status == 200);
  REQUIRE(res_js->body.find("const nodeDetailBanner = document.getElementById('nodeDetailBanner');") != std::string::npos);
  REQUIRE(res_js->body.find("const nodeDetailContent = document.getElementById('nodeDetailContent');") != std::string::npos);

  // Test Presets Endpoints
  auto res_presets_file = client.Get("/presets.json");
  REQUIRE(res_presets_file);
  REQUIRE(res_presets_file->status == 200);
  nlohmann::json presets_file_json = nlohmann::json::parse(res_presets_file->body);
  REQUIRE(presets_file_json.contains("finesse"));
  REQUIRE(presets_file_json.contains("nine_card_hearts"));

  auto res_presets_api = client.Get("/api/presets");
  REQUIRE(res_presets_api);
  REQUIRE(res_presets_api->status == 200);
  nlohmann::json presets_api_json = nlohmann::json::parse(res_presets_api->body);
  REQUIRE(presets_api_json.contains("endplay"));
  REQUIRE(presets_api_json.contains("nine_card_hearts"));

  // Test Health Endpoint
  auto res_health = client.Get("/api/health");
  REQUIRE(res_health);
  REQUIRE(res_health->status == 200);
  nlohmann::json health_json = nlohmann::json::parse(res_health->body);
  REQUIRE(health_json["status"] == "ok");

  // Test Solve Status Endpoint
  LineFinder::SetSolveStatus("Analyzing trick 1...");
  auto res_status = client.Get("/api/solve_status");
  REQUIRE(res_status);
  REQUIRE(res_status->status == 200);
  nlohmann::json status_json = nlohmann::json::parse(res_status->body);
  REQUIRE(status_json["status"] == "Analyzing trick 1...");

  server.Stop();
}

TEST_CASE("WebServer REST API /api/solve Integration") {
  int port = 18081;
  WebServer server("127.0.0.1", port);
  REQUIRE(server.Start(/*async=*/true));

  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  httplib::Client client("127.0.0.1", port);
  client.set_keep_alive(false);
  client.set_read_timeout(60, 0);

  // Test 1: Simple 2-Card Deal Finesse
  nlohmann::json solve_req;
  solve_req["hands"]["N"] = "S AQ; H -; D -; C -;";
  solve_req["hands"]["E"] = "S 54; H -; D -; C -;";
  solve_req["hands"]["S"] = "S 87; H -; D -; C -;";
  solve_req["hands"]["W"] = "S KJ; H -; D -; C -;";
  solve_req["trump"] = "SPADES";
  solve_req["leader"] = "NORTH";
  solve_req["min_tricks"] = 1;
  solve_req["num_iters"] = 1;

  auto res_solve = client.Post("/api/solve", solve_req.dump(), "application/json");
  REQUIRE(res_solve);
  REQUIRE(res_solve->status == 200);

  nlohmann::json res_json = nlohmann::json::parse(res_solve->body);
  REQUIRE(res_json["success"] == true);
  REQUIRE(res_json["tricks_won"] == 1);
  REQUIRE(res_json["best_lead"] == "SA");
  REQUIRE(res_json.contains("tree"));
  REQUIRE(res_json["tree"].contains("id"));
  REQUIRE(res_json.contains("solve_ms"));
  REQUIRE(res_json.contains("invariant_ms"));
  REQUIRE(res_json["solve_ms"].get<double>() >= 0.0);
  REQUIRE(res_json["invariant_ms"].get<double>() >= 0.0);

  // Test 2: 4-Card Endplay Deal Multi-Trick Tree Verification
  nlohmann::json endplay_req;
  endplay_req["hands"]["N"] = "S AK; H K2; D -; C -;";
  endplay_req["hands"]["E"] = "S 54; H QJ; D -; C -;";
  endplay_req["hands"]["S"] = "S QJ; H A3; D -; C -;";
  endplay_req["hands"]["W"] = "S 87; H T9; D -; C -;";
  endplay_req["trump"] = "HEARTS";
  endplay_req["leader"] = "NORTH";
  endplay_req["min_tricks"] = 3;

  auto res_endplay = client.Post("/api/solve", endplay_req.dump(), "application/json");
  REQUIRE(res_endplay);
  REQUIRE(res_endplay->status == 200);

  nlohmann::json endplay_json = nlohmann::json::parse(res_endplay->body);
  REQUIRE(endplay_json["success"] == true);
  REQUIRE(endplay_json["tricks_won"] == 4);

  // Check decision tree hierarchy down to 4 tricks
  auto &tree = endplay_json["tree"];
  std::string ep_root_label = tree["label"].get<std::string>();
  auto &trick1_node = (ep_root_label.find("Trick 1:") != std::string::npos) ? tree : tree["children"][0];

  // Trick 1 (collapsed by default)
  REQUIRE(trick1_node["label"].get<std::string>().find("Trick 1:") != std::string::npos);
  REQUIRE(trick1_node["label"].get<std::string>().find("wins") != std::string::npos);
  REQUIRE(trick1_node["collapsed"] == true);
  REQUIRE(trick1_node["children"].size() >= 1);

  // Trick 2
  auto &trick2_node = trick1_node["children"][0];
  REQUIRE(trick2_node["label"].get<std::string>().find("Trick 2:") != std::string::npos);
  REQUIRE(trick2_node["label"].get<std::string>().find("wins") != std::string::npos);
  REQUIRE(trick2_node["children"].size() >= 1);

  // Trick 3
  auto &trick3_node = trick2_node["children"][0];
  REQUIRE(trick3_node["label"].get<std::string>().find("Trick 3:") != std::string::npos);
  REQUIRE(trick3_node["label"].get<std::string>().find("wins") != std::string::npos);
  REQUIRE(trick3_node["children"].size() >= 1);

  // Trick 4
  auto &trick4_node = trick3_node["children"][0];
  REQUIRE(trick4_node["label"].get<std::string>().find("Trick 4:") != std::string::npos);
  REQUIRE(trick4_node["label"].get<std::string>().find("wins") != std::string::npos);
  REQUIRE(trick4_node["type"] == "outcome");

  // Test 3: Invalid JSON payload handling
  auto res_bad = client.Post("/api/solve", "{invalid json}", "application/json");
  REQUIRE(res_bad);
  REQUIRE(res_bad->status == 400);

  // Test 4: Fuzzy solve returning distribution summary
  nlohmann::json fuzzy_req;
  fuzzy_req["hands"]["N"] = "S AQ";
  fuzzy_req["hands"]["E"] = "";
  fuzzy_req["hands"]["S"] = "S 32";
  fuzzy_req["hands"]["W"] = "";
  fuzzy_req["trump"] = "SPADES";
  fuzzy_req["leader"] = "NORTH";
  fuzzy_req["min_tricks"] = 1;
  fuzzy_req["num_iters"] = 1;

  auto res_fuzzy = client.Post("/api/solve", fuzzy_req.dump(), "application/json");
  REQUIRE(res_fuzzy);
  REQUIRE(res_fuzzy->status == 200);

  nlohmann::json fuzzy_json = nlohmann::json::parse(res_fuzzy->body);
  REQUIRE(fuzzy_json["success"] == true);
  REQUIRE(fuzzy_json.contains("solve_ms"));
  REQUIRE(fuzzy_json.contains("invariant_ms"));
  REQUIRE(fuzzy_json["solve_ms"].get<double>() >= 0.0);
  REQUIRE(fuzzy_json["invariant_ms"].get<double>() >= 0.0);
  REQUIRE(fuzzy_json.contains("distribution_summary"));
  auto &dist_sum = fuzzy_json["distribution_summary"];
  REQUIRE(dist_sum.contains("invariant_rules"));
  REQUIRE(dist_sum.contains("suit_bounds"));
  REQUIRE(dist_sum.contains("honor_stats"));

  // Test 5: Simple Finesse (Spades) fuzzy solve with leader SOUTH and unknown_cards S Kxxx
  nlohmann::json finesse_req;
  finesse_req["hands"]["N"] = "S AQ; H -; D -; C -;";
  finesse_req["hands"]["E"] = "";
  finesse_req["hands"]["S"] = "S 87; H -; D -; C -;";
  finesse_req["hands"]["W"] = "";
  finesse_req["trump"] = "SPADES";
  finesse_req["leader"] = "SOUTH";
  finesse_req["min_tricks"] = 2;
  finesse_req["num_iters"] = 10;
  finesse_req["unknown_cards"] = "S Kxxx";

  auto res_finesse = client.Post("/api/solve", finesse_req.dump(), "application/json");
  REQUIRE(res_finesse);
  REQUIRE(res_finesse->status == 200);
  nlohmann::json finesse_json = nlohmann::json::parse(res_finesse->body);
  REQUIRE(finesse_json["success"] == true);
  REQUIRE(finesse_json.contains("tree"));
  auto &finesse_tree = finesse_json["tree"];
  REQUIRE(finesse_tree.contains("label"));
  // Ensure the tree is collapsed and does not contain redundant 'Hand Succeeded' or 'after ...' single-child wrappers
  std::string root_label = finesse_tree["label"].get<std::string>();
  REQUIRE(root_label.find("Hand Succeeded") == std::string::npos);
  if (finesse_tree.contains("children") && !finesse_tree["children"].empty()) {
    for (const auto &c : finesse_tree["children"]) {
      std::string child_label = c["label"].get<std::string>();
      REQUIRE(child_label.find("Hand Succeeded") == std::string::npos);
    }
  }

  // Test 5b: 9-Card Hearts Deal solve
  nlohmann::json hearts9_req;
  hearts9_req["hands"]["N"] = "S 532; H T7; D T32; C 6;";
  hearts9_req["hands"]["E"] = "";
  hearts9_req["hands"]["S"] = "S K9; H -; D A854; C AKJ;";
  hearts9_req["hands"]["W"] = "";
  hearts9_req["trump"] = "HEARTS";
  hearts9_req["leader"] = "NORTH";
  hearts9_req["min_tricks"] = 6;
  hearts9_req["num_iters"] = 10;
  hearts9_req["unknown_cards"] = "S AJTQ64 D KQJ976 C T43Q98";

  auto res_hearts9 = client.Post("/api/solve", hearts9_req.dump(), "application/json");
  REQUIRE(res_hearts9);
  REQUIRE(res_hearts9->status == 200);
  nlohmann::json hearts9_json = nlohmann::json::parse(res_hearts9->body);
  REQUIRE(hearts9_json["success"] == true);

  // Test 6: Unknown cards with wildcard 'x' format (H Axxxx C xxxxx for 5-card ending)
  nlohmann::json wildcard_req;
  wildcard_req["hands"]["N"] = "H 43 C 432";
  wildcard_req["hands"]["E"] = "";
  wildcard_req["hands"]["S"] = "H 65 C 876";
  wildcard_req["hands"]["W"] = "";
  wildcard_req["trump"] = "HEARTS";
  wildcard_req["leader"] = "SOUTH";
  wildcard_req["min_tricks"] = 1;
  wildcard_req["num_iters"] = 5;
  wildcard_req["unknown_cards"] = "H Axxxx C xxxxx";

  auto res_wildcard = client.Post("/api/solve", wildcard_req.dump(), "application/json");
  REQUIRE(res_wildcard);
  REQUIRE(res_wildcard->status == 200);
  nlohmann::json wildcard_json = nlohmann::json::parse(res_wildcard->body);
  REQUIRE(wildcard_json["success"] == true);

  // Test 7: Error handling - conflicting unknown card
  nlohmann::json conflict_req;
  conflict_req["hands"]["N"] = "S AQ";
  conflict_req["hands"]["S"] = "S 87";
  conflict_req["unknown_cards"] = "S SAxx"; // SA already in North
  auto res_conflict = client.Post("/api/solve", conflict_req.dump(), "application/json");
  REQUIRE(res_conflict);
  REQUIRE(res_conflict->status == 400);
  nlohmann::json conflict_json = nlohmann::json::parse(res_conflict->body);
  REQUIRE(conflict_json["success"] == false);
  REQUIRE(conflict_json["error"].get<std::string>().find("conflicts") != std::string::npos);

  // Test 8: Error handling - invalid unknown card count (5 provided vs 4 expected)
  nlohmann::json count_err_req;
  count_err_req["hands"]["N"] = "S AQ";
  count_err_req["hands"]["S"] = "S 87";
  count_err_req["unknown_cards"] = "S Kxxxx"; // 5 cards provided, but 4 expected for 2-card ending
  auto res_count_err = client.Post("/api/solve", count_err_req.dump(), "application/json");
  REQUIRE(res_count_err);
  REQUIRE(res_count_err->status == 400);
  nlohmann::json count_err_json = nlohmann::json::parse(res_count_err->body);
  REQUIRE(count_err_json["success"] == false);
  REQUIRE(count_err_json["error"].get<std::string>().find("Invalid unknown cards count") != std::string::npos);

  // Test 9: 4-Card Endplay Preset Tree Structure & Depth
  nlohmann::json endplay_preset_req;
  endplay_preset_req["hands"]["N"] = "S AQT; H 2; D -; C -;";
  endplay_preset_req["hands"]["E"] = "";
  endplay_preset_req["hands"]["S"] = "S 876; H A; D -; C -;";
  endplay_preset_req["hands"]["W"] = "";
  endplay_preset_req["trump"] = "HEARTS";
  endplay_preset_req["leader"] = "SOUTH";
  endplay_preset_req["min_tricks"] = 3;
  endplay_preset_req["num_iters"] = 30;
  endplay_preset_req["unknown_cards"] = "S KJ9543 D 34";

  auto res_preset = client.Post("/api/solve", endplay_preset_req.dump(), "application/json");
  REQUIRE(res_preset);
  REQUIRE(res_preset->status == 200);
  nlohmann::json preset_json = nlohmann::json::parse(res_preset->body);
  REQUIRE(preset_json["success"] == true);
  REQUIRE(preset_json.contains("tree"));

  auto &ptree = preset_json["tree"];
  std::string p_root_label = ptree["label"].get<std::string>();
  auto &p_trick1 = (p_root_label.find("Trick 1:") != std::string::npos) ? ptree : ptree["children"][0];

  // Depth 1: Trick 1
  REQUIRE(p_trick1["label"].get<std::string>().find("Trick 1:") != std::string::npos);
  REQUIRE(p_trick1["children"].size() >= 1);

  // Depth 2: Trick 2
  auto &p_trick2 = p_trick1["children"][0];
  REQUIRE(p_trick2["label"].get<std::string>().find("Trick 2:") != std::string::npos);
  REQUIRE(p_trick2["children"].size() >= 1);

  // Depth 3: Trick 3
  auto &p_trick3 = p_trick2["children"][0];
  REQUIRE(p_trick3["label"].get<std::string>().find("Trick 3:") != std::string::npos);
  if (p_trick3["children"].size() > 1) {
    bool found_branch_cond = false;
    for (const auto &c : p_trick3["children"]) {
      if (c.contains("condition") && !c["condition"].get<std::string>().empty()) {
        found_branch_cond = true;
        std::string cond = c["condition"].get<std::string>();
        REQUIRE(cond.find("0 wins") == std::string::npos);
        REQUIRE(cond.find("1 wins") == std::string::npos);
        REQUIRE((cond.find("N wins") != std::string::npos || cond.find("E wins") != std::string::npos ||
                 cond.find("S wins") != std::string::npos || cond.find("W wins") != std::string::npos));
        REQUIRE(cond.find(" wins the ") != std::string::npos);
      }
    }
    REQUIRE(found_branch_cond);
  }

  // Find outcome leaf at depth 4
  nlohmann::json *curr = &p_trick1;
  while (curr->contains("children") && !(*curr)["children"].empty()) {
    curr = &((*curr)["children"][0]);
  }
  REQUIRE((*curr)["type"] == "outcome");
  REQUIRE((*curr)["success_boards"] > 0);
  REQUIRE((*curr).contains("ns_tricks"));
  REQUIRE((*curr)["label"].get<std::string>().find("tricks won by N/S") != std::string::npos);

  server.Stop();
}

TEST_CASE("BuildTreeFromConditionalLine Common Lead Card Label") {
  std::set<int> boards = {0, 1, 2};
  ConditionalLine root_line;

  ConditionalLine child1;
  child1.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("H2", "H7", boards),
      GeneralizedDemiStruct("H9", "HK", boards),
      boards, 0));

  ConditionalLine child2;
  child2.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("H3", "H7", boards),
      GeneralizedDemiStruct("HJ", "HQ", boards),
      boards, 0));

  root_line.AddCondition(new WinnerPlayCondition(0), child1);
  root_line.AddCondition(new WinnerPlayCondition(1), child2);

  int node_id = 0;
  auto node = BuildTreeFromConditionalLine(root_line, node_id);
  REQUIRE(node != nullptr);
  REQUIRE(node->label == "H7 is led (2 variants)");

  // Test fallback when children have different lead cards
  ConditionalLine root_diff;
  ConditionalLine child_diff_1;
  child_diff_1.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("H2", "H7", boards),
      GeneralizedDemiStruct("H9", "HK", boards),
      boards, 0));

  ConditionalLine child_diff_2;
  child_diff_2.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("S2", "SA", boards),
      GeneralizedDemiStruct("S9", "SK", boards),
      boards, 0));

  root_diff.AddCondition(new WinnerPlayCondition(0), child_diff_1);
  root_diff.AddCondition(new WinnerPlayCondition(1), child_diff_2);

  int node_id2 = 0;
  auto node_diff = BuildTreeFromConditionalLine(root_diff, node_id2);
  REQUIRE(node_diff != nullptr);
  REQUIRE(node_diff->label == "Solve Lines (2 variants)");
}

TEST_CASE("FormatWinnerCondition and FormatBranchCondition Formatting") {
  WinnerPlayCondition cond_no_cards(0);
  REQUIRE(FormatWinnerCondition(&cond_no_cards) == "North wins");
  REQUIRE(FormatBranchCondition(&cond_no_cards, false) == "if N wins");
  REQUIRE(FormatBranchCondition(&cond_no_cards, true) == "when N wins");

  WinnerPlayCondition cond_with_cards(1, std::vector<std::string>{"SK", "SQ"});
  REQUIRE(FormatWinnerCondition(&cond_with_cards) == "East wins the SK,SQ");
  REQUIRE(FormatBranchCondition(&cond_with_cards, false) == "if E wins the SK,SQ");
  REQUIRE(FormatBranchCondition(&cond_with_cards, true) == "when E wins the SK,SQ");
}

TEST_CASE("WebServer Concurrent Multithreaded /api/solve Requests") {
  int port = 18082;
  WebServer server("127.0.0.1", port);
  REQUIRE(server.Start(/*async=*/true));

  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  // Sequential baseline
  httplib::Client baseline_client("127.0.0.1", port);
  baseline_client.set_keep_alive(false);
  baseline_client.set_read_timeout(60, 0);

  nlohmann::json endplay_req;
  endplay_req["hands"]["N"] = "S AK; H K2; D -; C -;";
  endplay_req["hands"]["E"] = "S 54; H QJ; D -; C -;";
  endplay_req["hands"]["S"] = "S QJ; H A3; D -; C -;";
  endplay_req["hands"]["W"] = "S 87; H T9; D -; C -;";
  endplay_req["trump"] = "HEARTS";
  endplay_req["leader"] = "NORTH";
  endplay_req["min_tricks"] = 3;

  auto res_baseline = baseline_client.Post("/api/solve", endplay_req.dump(), "application/json");
  REQUIRE(res_baseline);
  REQUIRE(res_baseline->status == 200);

  nlohmann::json baseline_json = nlohmann::json::parse(res_baseline->body);
  REQUIRE(baseline_json["success"] == true);
  int baseline_tricks = baseline_json["tricks_won"];

  // Concurrent execution with multiple threads
  constexpr int num_threads = 8;
  constexpr int requests_per_thread = 5;
  std::vector<std::thread> threads;
  std::atomic<bool> all_passed{true};

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([port, &endplay_req, baseline_tricks, &all_passed]() {
      httplib::Client client("127.0.0.1", port);
      client.set_keep_alive(false);
      client.set_read_timeout(60, 0);

      for (int i = 0; i < requests_per_thread; ++i) {
        auto res = client.Post("/api/solve", endplay_req.dump(), "application/json");
        if (!res || res->status != 200) {
          all_passed = false;
          return;
        }
        nlohmann::json res_json = nlohmann::json::parse(res->body);
        if (res_json["success"] != true || res_json["tricks_won"] != baseline_tricks) {
          all_passed = false;
          return;
        }
      }
    });
  }

  for (auto &th : threads) {
    if (th.joinable()) {
      th.join();
    }
  }

  REQUIRE(all_passed.load());

  server.Stop();
}

