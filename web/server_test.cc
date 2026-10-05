#include <catch2/catch_all.hpp>
#include <chrono>
#include <thread>

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

  // Test Presets Endpoints
  auto res_presets_file = client.Get("/presets.json");
  REQUIRE(res_presets_file);
  REQUIRE(res_presets_file->status == 200);
  nlohmann::json presets_file_json = nlohmann::json::parse(res_presets_file->body);
  REQUIRE(presets_file_json.contains("finesse"));

  auto res_presets_api = client.Get("/api/presets");
  REQUIRE(res_presets_api);
  REQUIRE(res_presets_api->status == 200);
  nlohmann::json presets_api_json = nlohmann::json::parse(res_presets_api->body);
  REQUIRE(presets_api_json.contains("endplay"));

  // Test Health Endpoint
  auto res_health = client.Get("/api/health");
  REQUIRE(res_health);
  REQUIRE(res_health->status == 200);
  nlohmann::json health_json = nlohmann::json::parse(res_health->body);
  REQUIRE(health_json["status"] == "ok");

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
  REQUIRE(tree["collapsed"] == false);
  REQUIRE(tree["children"].size() >= 1);

  // Trick 1
  auto &trick1_node = tree["children"][0];
  REQUIRE(trick1_node["label"].get<std::string>().find("Trick 1:") != std::string::npos);
  REQUIRE(trick1_node["collapsed"] == true);
  REQUIRE(trick1_node["children"].size() >= 1);

  // Trick 2
  auto &trick2_node = trick1_node["children"][0];
  REQUIRE(trick2_node["label"].get<std::string>().find("Trick 2:") != std::string::npos);
  REQUIRE(trick2_node["collapsed"] == true);

  // Trick 3
  auto &trick3_node = trick2_node["children"][0];
  REQUIRE(trick3_node["label"].get<std::string>().find("Trick 3:") != std::string::npos);

  // Trick 4
  auto &trick4_node = trick3_node["children"][0];
  REQUIRE(trick4_node["label"].get<std::string>().find("Trick 4:") != std::string::npos);

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

  // Test 9: Decision Tree Node Collapsing Verification
  nlohmann::json collapse_req;
  collapse_req["hands"]["N"] = "S AQ";
  collapse_req["hands"]["S"] = "S 32";
  collapse_req["trump"] = "SPADES";
  collapse_req["leader"] = "NORTH";
  collapse_req["min_tricks"] = 1;
  collapse_req["num_iters"] = 1;

  auto res_collapse = client.Post("/api/solve", collapse_req.dump(), "application/json");
  REQUIRE(res_collapse);
  REQUIRE(res_collapse->status == 200);
  nlohmann::json collapse_json = nlohmann::json::parse(res_collapse->body);
  REQUIRE(collapse_json["success"] == true);
  REQUIRE(collapse_json.contains("tree"));
  auto &collapsed_tree = collapse_json["tree"];
  // Verify that single-path redundant wrapper nodes and win condition nodes are merged cleanly
  // into node labels formatted like "after ... (N wins)"
  bool found_merged_label = false;
  std::function<void(const nlohmann::json &)> check_node = [&](const nlohmann::json &node) {
    if (node.contains("label") && node["label"].is_string()) {
      std::string l = node["label"].get<std::string>();
      if (l.find("after ") == 0 && l.find("(N wins)") != std::string::npos) {
        found_merged_label = true;
      }
    }
    if (node.contains("children") && node["children"].is_array()) {
      for (const auto &c : node["children"]) {
        check_node(c);
      }
    }
  };
  check_node(collapsed_tree);
  REQUIRE(found_merged_label);

  // Test 10: 4-Card Ending Decision Tree Node Depth Verification
  // Ensure different tricks are never collapsed together, yielding depth 4 for 4 tricks.
  nlohmann::json req_4card;
  req_4card["hands"]["N"] = "S AKQJ";
  req_4card["hands"]["E"] = "S 9876";
  req_4card["hands"]["S"] = "S 5432";
  req_4card["hands"]["W"] = "H AKQJ";
  req_4card["trump"] = "SPADES";
  req_4card["leader"] = "NORTH";

  auto res_4card = client.Post("/api/solve", req_4card.dump(), "application/json");
  REQUIRE(res_4card);
  REQUIRE(res_4card->status == 200);
  nlohmann::json json_4card = nlohmann::json::parse(res_4card->body);
  REQUIRE(json_4card["success"] == true);
  REQUIRE(json_4card.contains("tree"));

  // Traverse tree to count trick nodes along primary path
  int trick_count = 0;
  nlohmann::json curr = json_4card["tree"];
  while (true) {
    if (curr.contains("type") && curr["type"] == "action") {
      if (curr.contains("label") && curr["label"].get<std::string>().rfind("Optimal Lines", 0) != 0) {
        trick_count++;
      }
    }
    if (curr.contains("children") && !curr["children"].empty()) {
      curr = curr["children"][0];
    } else {
      break;
    }
  }
  REQUIRE(trick_count == 4);

  server.Stop();
}
