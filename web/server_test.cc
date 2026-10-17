#include <catch2/catch_all.hpp>
#include <chrono>
#include <functional>
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
  REQUIRE(res_index->body.find("traceHandInput") != std::string::npos);
  REQUIRE(res_index->body.find("traceHandBtn") != std::string::npos);
  REQUIRE(res_index->body.find("arrow-traced") != std::string::npos);
  REQUIRE(res_index->body.find("computeNewLineBtn") != std::string::npos);
  REQUIRE(res_index->body.find("resetLineBtn") != std::string::npos);
  REQUIRE(res_index->body.find("equivalentLinesBar") != std::string::npos);
  REQUIRE(res_index->body.find("equivalentLineSelect") != std::string::npos);
  REQUIRE(res_index->body.find("tabCompareLinesBtn") != std::string::npos);
  REQUIRE(res_index->body.find("panelCompareLines") != std::string::npos);
  REQUIRE(res_index->body.find("compareLinesList") != std::string::npos);
  REQUIRE(res_index->body.find("compareLinesResults") != std::string::npos);
  REQUIRE(res_index->body.find("Rand deals:") != std::string::npos);
  REQUIRE(res_index->body.find("openFeedbackBtn") != std::string::npos);
  REQUIRE(res_index->body.find("feedbackModal") != std::string::npos);

  // Test CSS serving
  auto res_css = client.Get("/styles.css");
  REQUIRE(res_css);
  REQUIRE(res_css->status == 200);
  REQUIRE(res_css->body.find(".traced-branch-node") != std::string::npos);
  REQUIRE(res_css->body.find(".traced-branch-link") != std::string::npos);
  REQUIRE(res_css->body.find(".trick-columns-header") != std::string::npos);
  REQUIRE(res_css->body.find(".card-chip.card-lead") != std::string::npos);
  REQUIRE(res_css->body.find(".card-chip.card-winner") != std::string::npos);
  REQUIRE(res_css->body.find("border-radius: 9999px") != std::string::npos);
  REQUIRE(res_css->body.find(".card-rank-playable") != std::string::npos);
  REQUIRE(res_css->body.find(".equivalent-lines-bar") != std::string::npos);
  REQUIRE(res_css->body.find(".compare-line-row") != std::string::npos);
  REQUIRE(res_css->body.find(".compare-stacked-bar") != std::string::npos);
  REQUIRE(res_css->body.find(".feedback-modal-overlay") != std::string::npos);

  // Test JS serving
  auto res_js = client.Get("/app.js");
  REQUIRE(res_js);
  REQUIRE(res_js->status == 200);
  REQUIRE(res_js->body.find("const nodeDetailBanner = document.getElementById('nodeDetailBanner');") != std::string::npos);
  REQUIRE(res_js->body.find("const nodeDetailContent = document.getElementById('nodeDetailContent');") != std::string::npos);
  REQUIRE(res_js->body.find("function traceHandByNumber") != std::string::npos);
  REQUIRE(res_js->body.find("function selectEquivalentLine") != std::string::npos);
  REQUIRE(res_js->body.find("function formatLineSuccessPctPrefix") != std::string::npos);
  REQUIRE(res_js->body.find("function buildSelectableLinesList") != std::string::npos);
  REQUIRE(res_js->body.find("function renderCompareLinesTab") != std::string::npos);
  REQUIRE(res_js->body.find("function renderComparePairSection") != std::string::npos);
  REQUIRE(res_js->body.find("function formatProposedCardsDescriptor") != std::string::npos);
  REQUIRE(res_js->body.find("Original line") != std::string::npos);
  REQUIRE(res_js->body.find("Alt line") != std::string::npos);
  REQUIRE(res_js->body.find("leadSeatIdx") != std::string::npos);
  REQUIRE(res_js->body.find("trick-columns-header") != std::string::npos);
  REQUIRE(res_js->body.find("function handleProposeCardClick") != std::string::npos);
  REQUIRE(res_js->body.find("function zoomAroundPoint") != std::string::npos);
  REQUIRE(res_js->body.find("function submitSentryFeedback") != std::string::npos);

  // Test Presets Endpoints
  auto res_presets_file = client.Get("/presets.json");
  REQUIRE(res_presets_file);
  REQUIRE(res_presets_file->status == 200);
  nlohmann::json presets_file_json = nlohmann::json::parse(res_presets_file->body);
  REQUIRE(presets_file_json.contains("finesse"));
  REQUIRE(presets_file_json.contains("nine_card_hearts"));
  REQUIRE(presets_file_json.contains("safety_play"));
  REQUIRE(presets_file_json.contains("twelve_card_spades"));
  REQUIRE(presets_file_json.contains("restricted_choice"));
  REQUIRE(presets_file_json.contains("mollo_6s"));
  REQUIRE(presets_file_json["mollo_6s"]["cur_trick"] == nlohmann::json::array({"DQ"}));
  REQUIRE(presets_file_json.contains("two_way_finesse"));
  REQUIRE(presets_file_json.contains("hand_8_261014"));
  REQUIRE(presets_file_json["hand_8_261014"]["cur_trick"] ==
          nlohmann::json::array({"CJ"}));
  for (const auto& [key, preset] : presets_file_json.items()) {
    REQUIRE(preset["iters"].get<int>() >= 100);
  }

  auto res_presets_api = client.Get("/api/presets");
  REQUIRE(res_presets_api);
  REQUIRE(res_presets_api->status == 200);
  nlohmann::json presets_api_json = nlohmann::json::parse(res_presets_api->body);
  REQUIRE(presets_api_json.contains("endplay"));
  REQUIRE(presets_api_json.contains("nine_card_hearts"));
  REQUIRE(presets_api_json.contains("safety_play"));
  REQUIRE(presets_api_json.contains("twelve_card_spades"));
  REQUIRE(presets_api_json.contains("restricted_choice"));
  REQUIRE(presets_api_json.contains("mollo_6s"));
  REQUIRE(presets_api_json.contains("two_way_finesse"));
  REQUIRE(presets_api_json.contains("hand_8_261014"));
  for (const auto& [key, preset] : presets_api_json.items()) {
    REQUIRE(preset["iters"].get<int>() >= 100);
  }

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
  // The progress estimate only exists while a play-tree search is running.
  REQUIRE(!status_json.contains("progress"));
  REQUIRE(LineFinder::GetSolveProgress() < 0.0);

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
  auto &trick1_node = (ep_root_label.find("#1:") != std::string::npos) ? tree : tree["children"][0];

  // Trick 1 (collapsed by default)
  REQUIRE(trick1_node["label"].get<std::string>().find("#1:") != std::string::npos);
  REQUIRE(trick1_node["collapsed"] == true);
  REQUIRE(trick1_node["children"].size() >= 1);

  // Trick 2
  auto &trick2_node = trick1_node["children"][0];
  REQUIRE(trick2_node["label"].get<std::string>().find("#2:") != std::string::npos);
  REQUIRE(trick2_node["children"].size() >= 1);

  // Trick 3
  auto &trick3_node = trick2_node["children"][0];
  REQUIRE(trick3_node["label"].get<std::string>().find("#3:") != std::string::npos);
  REQUIRE(trick3_node["children"].size() >= 1);

  // Trick 4
  auto &trick4_node = trick3_node["children"][0];
  REQUIRE(trick4_node["label"].get<std::string>().find("#4:") != std::string::npos);
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
  finesse_req["num_iters"] = 25;
  finesse_req["unknown_cards"] = "S Kxxx";

  auto res_finesse = client.Post("/api/solve", finesse_req.dump(), "application/json");
  REQUIRE(res_finesse);
  REQUIRE(res_finesse->status == 200);
  nlohmann::json finesse_json = nlohmann::json::parse(res_finesse->body);
  REQUIRE(finesse_json["success"] == true);
  REQUIRE(finesse_json.contains("tree"));
  REQUIRE(finesse_json.contains("sample_hands"));
  REQUIRE(finesse_json["sample_hands"].is_array());
  REQUIRE(!finesse_json["sample_hands"].empty());
  REQUIRE(finesse_json["sample_hands"].size() <= 100);
  REQUIRE(finesse_json["sample_hands"][0].contains("N"));
  REQUIRE(finesse_json["sample_hands"][0].contains("E"));
  REQUIRE(finesse_json["sample_hands"][0].contains("S"));
  REQUIRE(finesse_json["sample_hands"][0].contains("W"));
  REQUIRE(finesse_json.contains("failure_sample_hands"));
  REQUIRE(finesse_json["failure_sample_hands"].is_array());
  REQUIRE(finesse_json.contains("all_hands"));
  REQUIRE(finesse_json["all_hands"].is_array());
  REQUIRE(static_cast<int>(finesse_json["all_hands"].size()) ==
          finesse_json["total_boards"].get<int>());
  for (int b = 0; b < finesse_json["total_boards"].get<int>(); ++b) {
    REQUIRE(finesse_json["all_hands"][b]["board_id"] == b);
    REQUIRE(finesse_json["all_hands"][b].contains("E"));
    REQUIRE(finesse_json["all_hands"][b].contains("W"));
    REQUIRE(finesse_json["all_hands"][b].contains("succeeded"));
  }
  std::set<int> leaf_boards;
  std::function<void(const nlohmann::json &)> collect_leaf_boards =
      [&](const nlohmann::json &n) {
    if (!n.contains("children") || n["children"].empty()) {
      REQUIRE(n.contains("boards"));
      for (const auto &b : n["boards"]) {
        leaf_boards.insert(b.get<int>());
      }
      return;
    }
    for (const auto &c : n["children"]) {
      collect_leaf_boards(c);
    }
  };
  collect_leaf_boards(finesse_json["tree"]);
  REQUIRE(static_cast<int>(leaf_boards.size()) ==
          finesse_json["total_boards"].get<int>());
  REQUIRE(!finesse_json.contains("failure_tree"));
  // Failed boards are attached to the single tree as failure leaves; together
  // they account for every failed board exactly once.
  std::function<int(const nlohmann::json &)> count_failure_leaf_boards =
      [&](const nlohmann::json &n) -> int {
    int sum = 0;
    if (n.value("failure", false)) {
      REQUIRE(n["type"] == "outcome");
      REQUIRE(n["label"].get<std::string>().rfind("Hand Failed", 0) == 0);
      REQUIRE(n["children"].empty());
      sum += static_cast<int>(n["boards"].size());
    }
    if (n.contains("children")) {
      for (const auto &c : n["children"]) {
        sum += count_failure_leaf_boards(c);
      }
    }
    return sum;
  };
  if (finesse_json["failure_boards"].get<int>() > 0) {
    REQUIRE(!finesse_json["failure_sample_hands"].empty());
    REQUIRE(count_failure_leaf_boards(finesse_json["tree"]) ==
            finesse_json["failure_boards"].get<int>());
    REQUIRE(finesse_json["tree"]["failure_boards"].get<int>() ==
            finesse_json["failure_boards"].get<int>());
  } else {
    REQUIRE(count_failure_leaf_boards(finesse_json["tree"]) == 0);
  }
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

  // Test 5b: 9-Card Hearts Deal solve (default 16 threads and explicit thread counts)
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
  REQUIRE(hearts9_json["num_threads"] == 16);

  for (int threads : {1, 2, 4, 8, 16}) {
    hearts9_req["num_threads"] = threads;
    hearts9_req["num_iters"] = 4;
    auto res_t = client.Post("/api/solve", hearts9_req.dump(), "application/json");
    REQUIRE(res_t);
    REQUIRE(res_t->status == 200);
    nlohmann::json t_json = nlohmann::json::parse(res_t->body);
    REQUIRE(t_json["success"] == true);
    REQUIRE(t_json["num_threads"] == threads);
  }

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
  REQUIRE(preset_json.contains("num_lines"));
  REQUIRE(preset_json["num_lines"].get<int>() >= 2);
  REQUIRE(preset_json.contains("lines"));
  REQUIRE(preset_json["lines"].is_array());
  REQUIRE(static_cast<int>(preset_json["lines"].size()) ==
          preset_json["num_lines"].get<int>());
  std::set<std::string> seen_line_labels;
  for (size_t i = 0; i < preset_json["lines"].size(); ++i) {
    const auto &line_obj = preset_json["lines"][i];
    REQUIRE(line_obj["index"].get<size_t>() == i);
    REQUIRE(line_obj.contains("label"));
    std::string lbl = line_obj["label"].get<std::string>();
    REQUIRE(!lbl.empty());
    seen_line_labels.insert(lbl);
    REQUIRE(line_obj.contains("best_lead"));
    REQUIRE(!line_obj["best_lead"].get<std::string>().empty());
    REQUIRE(line_obj.contains("covered_boards"));
    REQUIRE(line_obj["covered_boards"] == preset_json["covered_boards"]);
    REQUIRE(line_obj.contains("covered_board_ids"));
    REQUIRE(line_obj["covered_board_ids"].is_array());
    REQUIRE(line_obj.contains("tree"));
    REQUIRE(line_obj["tree"].contains("id"));
  }
  REQUIRE(seen_line_labels.size() == preset_json["lines"].size());

  auto &ptree = preset_json["tree"];
  // Trick nodes may be wrapped in a "variants" condition node when several
  // generalized tricks share a branch; descend through those to the next
  // labelled trick.
  auto find_trick = [](nlohmann::json &start, const std::string &tag)
      -> nlohmann::json & {
    nlohmann::json *cur = &start;
    while (cur->contains("label") &&
           (*cur)["label"].get<std::string>().find(tag) == std::string::npos &&
           cur->contains("children") && !(*cur)["children"].empty()) {
      cur = &((*cur)["children"][0]);
    }
    return *cur;
  };
  auto &p_trick1 = find_trick(ptree, "#1:");

  // Depth 1: Trick 1
  REQUIRE(p_trick1["label"].get<std::string>().find("#1:") != std::string::npos);
  REQUIRE(p_trick1["children"].size() >= 1);

  // Depth 2: Trick 2
  auto &p_trick2 = find_trick(p_trick1["children"][0], "#2:");
  REQUIRE(p_trick2["label"].get<std::string>().find("#2:") != std::string::npos);
  REQUIRE(p_trick2["children"].size() >= 1);

  // Depth 3: Trick 3
  auto &p_trick3 = find_trick(p_trick2["children"][0], "#3:");
  REQUIRE(p_trick3["label"].get<std::string>().find("#3:") != std::string::npos);
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
  REQUIRE((*curr)["label"].get<std::string>().find("#") != std::string::npos);

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
  REQUIRE(node->label == "H7 is led");

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
  REQUIRE(node_diff->label == "Defender leads");
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

TEST_CASE("BuildTreeFromConditionalLine Preserves Trick Leader and Winner") {
  std::set<int> boards = {0, 1};
  ConditionalLine leaf_line(boards);

  // Trick 2: West (3) leads (S_), North plays S5, East plays (Sx), South wins SK
  ConditionalLine trick2_line;
  trick2_line.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("S_", "S5", boards),
      GeneralizedDemiStruct("Sx", "SK", boards),
      boards, /*ldr=*/3));
  trick2_line.AddCondition(new WinnerPlayCondition(2), leaf_line);

  // Trick 1: North (0) leads DT, East plays (Dx), South plays D8, West wins (DX)
  ConditionalLine trick1_line;
  trick1_line.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("Dx", "DT", boards),
      GeneralizedDemiStruct("DX", "D8", boards),
      boards, /*ldr=*/0));
  trick1_line.AddCondition(
      new WinnerPlayCondition(3, std::vector<std::string>{"DJ", "DQ", "DK"}),
      trick2_line);

  int node_id = 0;
  auto tree = BuildTreeFromConditionalLine(trick1_line, node_id);
  REQUIRE(tree != nullptr);
  REQUIRE(tree->leader == 0);
  REQUIRE(tree->winner == 3);
  REQUIRE(tree->children.size() == 1);

  auto trick2_node = tree->children[0];
  REQUIRE(trick2_node != nullptr);
  REQUIRE(trick2_node->leader == 3);
  REQUIRE(trick2_node->winner == 2);
  REQUIRE(trick2_node->branch_winner == 3);

  nlohmann::json j = *tree;
  REQUIRE(j["leader"] == 0);
  REQUIRE(j["winner"] == 3);
  REQUIRE(j["children"][0]["leader"] == 3);
  REQUIRE(j["children"][0]["winner"] == 2);
  REQUIRE(j["children"][0]["branch_winner"] == 3);
}

TEST_CASE("Unmatched Defender Play Attaches to Condition Node") {
  // North (0) leads C6; South (2) holds CJ.
  // Variant 1: C6 (Cx) CJ (C_) -> when S wins (board 0)
  // Variant 2: C6 (CA) CJ (C_) -> when E wins (board 1)
  // Failed board 2: East plays C2 (matches Cx), West plays CQ (matches C_) and
  // wins -> winner failure ("if W wins the CQ"), must attach under Variant 1.
  // Failed board 3: East is void in clubs and discards D6 -> unmatched defender
  // discard ("if E discards"), must attach to "C6 is led" as a
  // sibling of Variant 1, NOT as a child of Variant 1.
  // Failed board 4: East is void in clubs and discards S7 -> consolidated into
  // the same "if E discards" sibling failure leaf with board 3.
  // Failed board 5: East follows suit with CK (neither Cx nor CA) -> unmatched
  // non-discard play ("if E plays CK").
  std::set<int> b0 = {0};
  std::set<int> b1 = {1};
  ConditionalLine leaf0(b0);
  ConditionalLine leaf1(b1);

  ConditionalLine var1;
  var1.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("Cx", "C6", b0),
      GeneralizedDemiStruct("C_", "CJ", b0), b0, /*ldr=*/0));
  var1.AddCondition(new WinnerPlayCondition(2), leaf0);

  ConditionalLine var2;
  var2.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("CA", "C6", b1),
      GeneralizedDemiStruct("C_", "CJ", b1), b1, /*ldr=*/0));
  var2.AddCondition(new WinnerPlayCondition(1), leaf1);

  ConditionalLine multi_root;
  multi_root.set_total_boards(6);
  multi_root.AddCondition(new TrickPlayCondition("C6 (Cx) CJ (C_)"), var1);
  multi_root.AddCondition(new TrickPlayCondition("C6 (CA) CJ (C_)"), var2);

  auto make_board = [](const std::string &n, const std::string &e,
                       const std::string &s, const std::string &w) {
    BoardState bs;
    bs.SetHands({ParseHand(n), ParseHand(e), ParseHand(s), ParseHand(w)});
    bs.SetTrump(NOTRUMP);
    bs.SetLead(NORTH);
    bs.MarkMissingCardsAsPlayed();
    return bs;
  };

  std::vector<BoardState> boards = {
      make_board("C6", "C2", "CJ", "C3"),  // board 0: S wins CJ
      make_board("C6", "CA", "CJ", "C3"),  // board 1: E wins CA
      make_board("C6", "C2", "CJ", "CQ"),  // board 2: W wins CQ (matches C_)
      make_board("C6", "D6", "CJ", "C3"),  // board 3: E discards D6
      make_board("C6", "S7", "CJ", "C3"),  // board 4: E discards S7
      make_board("C6", "CK", "CJ", "C3"),  // board 5: E plays CK (unmatched)
  };

  FailureIndex failures = IndexFailures(multi_root, boards, {2, 3, 4, 5}, 1);
  int node_id = 0;
  auto tree =
      BuildTreeFromConditionalLine(multi_root, node_id, 1, 6, 0, &failures);
  REQUIRE(tree != nullptr);
  REQUIRE(tree->type == "condition");
  REQUIRE(tree->label == "C6 is led");
  // 2 trick variants + 2 sibling failure leaves ("if E plays CK",
  // "if E discards")
  REQUIRE(tree->children.size() == 4);

  auto var1_node = tree->children[0];
  REQUIRE(var1_node->label == "#1: C6 (Cx) CJ (C_)");
  // Variant 1 has 1 success child + 1 winner-failure child ("if W wins the CQ")
  REQUIRE(var1_node->children.size() == 2);
  REQUIRE(var1_node->children[1]->failure == true);
  REQUIRE(var1_node->children[1]->condition == "if W wins the CQ");

  auto ck_fail = tree->children[2];
  REQUIRE(ck_fail->failure == true);
  REQUIRE(ck_fail->condition == "if E plays CK");
  REQUIRE(ck_fail->trick == "C6 CK CJ C3");
  REQUIRE(ck_fail->leader == 0);
  REQUIRE(ck_fail->winner == 1);
  REQUIRE(ck_fail->failure_boards == 1);

  auto discard_fail = tree->children[3];
  REQUIRE(discard_fail->failure == true);
  REQUIRE(discard_fail->condition == "if E discards");
  REQUIRE(discard_fail->trick == "C6 (R_) CJ C3");
  REQUIRE(discard_fail->board_to_trick.at("3") == "C6 D6 CJ C3");
  REQUIRE(discard_fail->board_to_trick.at("4") == "C6 S7 CJ C3");
  REQUIRE(discard_fail->leader == 0);
  REQUIRE(discard_fail->winner == 2);
  REQUIRE(discard_fail->failure_boards == 2);

  // Also verify single-variant trick node wraps in "C6 is led" when an
  // unmatched defender play occurs.
  FailureIndex single_failures = IndexFailures(var1, boards, {3}, 1);
  int single_node_id = 0;
  auto single_tree = BuildTreeFromConditionalLine(var1, single_node_id, 1, 6, 0,
                                                  &single_failures);
  REQUIRE(single_tree != nullptr);
  REQUIRE(single_tree->type == "condition");
  REQUIRE(single_tree->label == "C6 is led");
  REQUIRE(single_tree->children.size() == 2);
  REQUIRE(single_tree->children[0]->label == "#1: C6 (Cx) CJ (C_)");
  REQUIRE(single_tree->children[0]->condition == "C6 (Cx) CJ (C_)");
  REQUIRE(single_tree->children[1]->failure == true);
  REQUIRE(single_tree->children[1]->condition == "if E discards");
  REQUIRE(single_tree->children[1]->trick == "C6 D6 CJ C3");
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

TEST_CASE("WebServer Solve Status Lifecycle Test") {
  int port = 18086;
  WebServer server("127.0.0.1", port);
  REQUIRE(server.Start(/*async=*/true));
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  httplib::Client client("127.0.0.1", port);
  client.set_keep_alive(false);
  client.set_read_timeout(60, 0);

  nlohmann::json req;
  req["hands"]["N"] = "S AQ; H -; D -; C -;";
  req["hands"]["E"] = "";
  req["hands"]["S"] = "S 87; H -; D -; C -;";
  req["hands"]["W"] = "";
  req["trump"] = "SPADES";
  req["leader"] = "SOUTH";
  req["min_tricks"] = 2;
  req["num_iters"] = 10;
  req["unknown_cards"] = "S Kxxx";

  auto res = client.Post("/api/solve", req.dump(), "application/json");
  REQUIRE(res);
  REQUIRE(res->status == 200);

  auto res_status = client.Get("/api/solve_status");
  REQUIRE(res_status);
  REQUIRE(res_status->status == 200);
  nlohmann::json status_json = nlohmann::json::parse(res_status->body);
  REQUIRE(status_json["status"] == "Solve complete.");

  // Test mid-trick solve with cur_trick, start_trick_num, and initial_ns_tricks
  nlohmann::json mid_req;
  mid_req["hands"]["N"] = "S AQ; H -; D -; C -;";
  mid_req["hands"]["E"] = "";
  mid_req["hands"]["S"] = "S 87; H -; D -; C -;";
  mid_req["hands"]["W"] = "S 4; H -; D -; C -;";
  mid_req["trump"] = "SPADES";
  mid_req["leader"] = "SOUTH";
  mid_req["min_tricks"] = 1;
  mid_req["num_iters"] = 15;
  mid_req["unknown_cards"] = "S Kxx";
  mid_req["cur_trick"] = nlohmann::json::array({"S8", "S4"});
  mid_req["start_trick_num"] = 2;
  mid_req["initial_ns_tricks"] = 1;

  auto res_mid = client.Post("/api/solve", mid_req.dump(), "application/json");
  REQUIRE(res_mid);
  REQUIRE(res_mid->status == 200);
  nlohmann::json mid_json = nlohmann::json::parse(res_mid->body);
  REQUIRE(mid_json["success"] == true);
  REQUIRE(mid_json["best_lead"] == "S8");
  REQUIRE(mid_json["tricks_won"] == 2);
  REQUIRE(mid_json.contains("tree"));
  std::string root_lbl = mid_json["tree"].value("label", "");
  if (root_lbl.find("#") == std::string::npos &&
      mid_json["tree"].contains("children") &&
      !mid_json["tree"]["children"].empty()) {
    root_lbl = mid_json["tree"]["children"][0].value("label", "");
  }
  REQUIRE(root_lbl.find("#2:") != std::string::npos);

  // Test suit_max requirement (e.g. East showed out of Diamonds on a prior
  // trick, so East must have 0 Diamonds in all simulated hands).
  nlohmann::json void_req;
  void_req["hands"]["N"] = "S A; D A;";
  void_req["hands"]["E"] = "";
  void_req["hands"]["S"] = "S K; D K;";
  void_req["hands"]["W"] = "";
  void_req["trump"] = "NOTRUMP";
  void_req["leader"] = "NORTH";
  void_req["min_tricks"] = 2;
  void_req["num_iters"] = 20;
  void_req["unknown_cards"] = "S QJ D QJ";
  void_req["requirements"]["E"]["suit_max"]["D"] = 0;

  auto res_void = client.Post("/api/solve", void_req.dump(), "application/json");
  REQUIRE(res_void);
  REQUIRE(res_void->status == 200);
  nlohmann::json void_json = nlohmann::json::parse(res_void->body);
  REQUIRE(void_json["success"] == true);
  for (const auto &h : void_json["all_hands"]) {
    std::string e_hand = h["E"].get<std::string>();
    REQUIRE(e_hand.find("D ") == std::string::npos);
  }

  // Test reuse_deals + replay_tricks: a subsequent manual alternate line solve
  // operates on the exact same dealt hands (0..N-1) and returns top-level
  // covered_board_ids.
  nlohmann::json base_req;
  base_req["hands"]["N"] = "S AQ; H 2; D -; C -;";
  base_req["hands"]["E"] = "";
  base_req["hands"]["S"] = "S 87; H A; D -; C -;";
  base_req["hands"]["W"] = "";
  base_req["trump"] = "SPADES";
  base_req["leader"] = "SOUTH";
  base_req["min_tricks"] = 3;
  base_req["num_iters"] = 16;
  base_req["unknown_cards"] = "S K432 H 43";

  auto res_base = client.Post("/api/solve", base_req.dump(), "application/json");
  REQUIRE(res_base);
  REQUIRE(res_base->status == 200);
  nlohmann::json base_json = nlohmann::json::parse(res_base->body);
  REQUIRE(base_json["success"] == true);
  REQUIRE(base_json.contains("covered_board_ids"));
  REQUIRE(base_json["all_hands"].size() == 16);

  // Now compute an alternate line with reuse_deals=true after Trick 1 (HA H3 H2 H4)
  nlohmann::json alt_req;
  alt_req["hands"]["N"] = "S AQ; H -; D -; C -;";
  alt_req["hands"]["E"] = "";
  alt_req["hands"]["S"] = "S 87; H -; D -; C -;";
  alt_req["hands"]["W"] = "";
  alt_req["trump"] = "SPADES";
  alt_req["leader"] = "SOUTH";
  alt_req["min_tricks"] = 2;
  alt_req["num_iters"] = 16;
  alt_req["unknown_cards"] = "S K432";
  alt_req["start_trick_num"] = 2;
  alt_req["initial_ns_tricks"] = 1;
  alt_req["reuse_deals"] = true;
  alt_req["replay_tricks"] =
      nlohmann::json::array({nlohmann::json::array({"HA", "Hx", "H2", "Hx"})});

  auto res_alt = client.Post("/api/solve", alt_req.dump(), "application/json");
  REQUIRE(res_alt);
  REQUIRE(res_alt->status == 200);
  nlohmann::json alt_json = nlohmann::json::parse(res_alt->body);
  REQUIRE(alt_json["success"] == true);
  REQUIRE(alt_json.contains("covered_board_ids"));
  REQUIRE(alt_json["all_hands"].size() == 16);
  for (int i = 0; i < 16; ++i) {
    REQUIRE(alt_json["all_hands"][i]["E"] == base_json["all_hands"][i]["E"]);
    REQUIRE(alt_json["all_hands"][i]["W"] == base_json["all_hands"][i]["W"]);
  }

  // Test /api/stop_solve endpoint and subsequent solve recovery
  auto res_stop = client.Post("/api/stop_solve", "", "application/json");
  REQUIRE(res_stop);
  REQUIRE(res_stop->status == 200);
  nlohmann::json stop_json = nlohmann::json::parse(res_stop->body);
  REQUIRE(stop_json["status"] == "stopping");
  REQUIRE(LineFinder::IsStopRequested());

  // Next /api/solve clears the stop flag automatically and succeeds
  auto res_after_stop =
      client.Post("/api/solve", base_req.dump(), "application/json");
  REQUIRE(res_after_stop);
  REQUIRE(res_after_stop->status == 200);
  nlohmann::json after_stop_json = nlohmann::json::parse(res_after_stop->body);
  REQUIRE(after_stop_json["success"] == true);
  REQUIRE(!LineFinder::IsStopRequested());

  auto res_index = client.Get("/");
  REQUIRE(res_index);
  REQUIRE(res_index->body.find("stopSolveBtn") != std::string::npos);
  REQUIRE(res_index->body.find("stopSolveActionBtn") != std::string::npos);
  REQUIRE(res_index->body.find("photoScanModal") != std::string::npos);
  REQUIRE(res_index->body.find("declarerRotateBar") != std::string::npos);
  REQUIRE(res_index->body.find("pdfPageBar") != std::string::npos);
  REQUIRE(res_index->body.find(
              "image/*,.pdf,application/pdf,.pbn,.txt,.deal") !=
          std::string::npos);

  auto res_app_js = client.Get("/app.js");
  REQUIRE(res_app_js);
  REQUIRE(res_app_js->body.find("recognizeBoardFromImageData") !=
          std::string::npos);
  REQUIRE(res_app_js->body.find("applyDeclarerRotation") != std::string::npos);
  REQUIRE(res_app_js->body.find("openPhotoScanModalWithPdfFile") !=
          std::string::npos);
  REQUIRE(res_app_js->body.find("extractPbnBoardsFromPdfBuffer") !=
          std::string::npos);

  // Test partial-trick proposal with wildcard 'x' moved from Unknown Cards
  // into West's hand (e.g. Simple Finesse preset: S7, Sx, SA).
  auto res_fin_base = client.Post("/api/solve", req.dump(), "application/json");
  REQUIRE(res_fin_base);
  REQUIRE(res_fin_base->status == 200);
  nlohmann::json fin_base_json = nlohmann::json::parse(res_fin_base->body);
  REQUIRE(fin_base_json["success"] == true);

  nlohmann::json fin_partial_req;
  fin_partial_req["hands"]["N"] = "S AQ; H -; D -; C -;";
  fin_partial_req["hands"]["E"] = "";
  fin_partial_req["hands"]["S"] = "S 87; H -; D -; C -;";
  fin_partial_req["hands"]["W"] = "S x; H -; D -; C -;";
  fin_partial_req["trump"] = "SPADES";
  fin_partial_req["leader"] = "SOUTH";
  fin_partial_req["min_tricks"] = 2;
  fin_partial_req["num_iters"] = 10;
  fin_partial_req["unknown_cards"] = "S Kxx";
  fin_partial_req["cur_trick"] = nlohmann::json::array({"S7", "Sx", "SA"});
  fin_partial_req["start_trick_num"] = 1;
  fin_partial_req["initial_ns_tricks"] = 0;
  fin_partial_req["reuse_deals"] = true;
  fin_partial_req["replay_tricks"] = nlohmann::json::array();

  auto res_fin_partial =
      client.Post("/api/solve", fin_partial_req.dump(), "application/json");
  REQUIRE(res_fin_partial);
  REQUIRE(res_fin_partial->status == 200);
  nlohmann::json fin_partial_json =
      nlohmann::json::parse(res_fin_partial->body);
  REQUIRE(fin_partial_json["success"] == true);
  REQUIRE(fin_partial_json["total_boards"] == 10);
  REQUIRE(fin_partial_json["success_boards"] == 0);
  REQUIRE(fin_partial_json["failure_boards"] == 10);
  REQUIRE(fin_partial_json.contains("tree"));
  REQUIRE(fin_partial_json["tree"]["label"].get<std::string>().find(
              "#1: S7 (S") != std::string::npos);
  REQUIRE(fin_partial_json["tree"]["label"].get<std::string>().find(
              ") SA (S") != std::string::npos);
  REQUIRE(fin_partial_json["tree"].contains("children"));
  REQUIRE(!fin_partial_json["tree"]["children"].empty());
  REQUIRE(fin_partial_json["tree"]["children"][0]["failure"] == true);
  for (int i = 0; i < 10; ++i) {
    REQUIRE(fin_partial_json["all_hands"][i]["E"] ==
            fin_base_json["all_hands"][i]["E"]);
    REQUIRE(fin_partial_json["all_hands"][i]["W"] ==
            fin_base_json["all_hands"][i]["W"]);
  }

  server.Stop();
}

TEST_CASE("WebServer Direct SolveJson and ProgressCallback") {
  WebServer engine;
  std::vector<std::string> progress_statuses;
  LineFinder::SetProgressCallback(
      [&progress_statuses](const std::string &status, double) {
        progress_statuses.push_back(status);
      });

  nlohmann::json req;
  req["hands"]["N"] = "S AQ; H -; D -; C -;";
  req["hands"]["E"] = "S 54; H -; D -; C -;";
  req["hands"]["S"] = "S 87; H -; D -; C -;";
  req["hands"]["W"] = "S KJ; H -; D -; C -;";
  req["trump"] = "SPADES";
  req["leader"] = "NORTH";
  req["min_tricks"] = 1;
  req["num_iters"] = 1;
  req["num_threads"] = 1;

  int status_code = 0;
  std::string body = engine.SolveJson(req.dump(), &status_code);
  LineFinder::SetProgressCallback(nullptr);

  REQUIRE(status_code == 200);
  nlohmann::json res = nlohmann::json::parse(body);
  REQUIRE(res["success"] == true);
  REQUIRE(res["tricks_won"] == 1);
  REQUIRE(res["best_lead"] == "SA");
  REQUIRE(!progress_statuses.empty());

  nlohmann::json st = nlohmann::json::parse(engine.GetSolveStatusJson());
  REQUIRE(st["status"] == "Solve complete.");
}

TEST_CASE("WebServer SolveJson Actual Deal Evaluation (Board #0)") {
  WebServer engine;

  // Simple finesse: North has S AQ, South has S 87, unknown S Kxx5.
  // South leads S7 to finesse SQ.
  // Case 1: Actual layout has SK onside in West -> should succeed (2 tricks).
  nlohmann::json req_make;
  req_make["hands"]["N"] = "S AQ; H -; D -; C -;";
  req_make["hands"]["E"] = "";
  req_make["hands"]["S"] = "S 87; H -; D -; C -;";
  req_make["hands"]["W"] = "";
  req_make["trump"] = "SPADES";
  req_make["leader"] = "SOUTH";
  req_make["min_tricks"] = 2;
  req_make["num_iters"] = 6;
  req_make["num_threads"] = 1;
  req_make["unknown_cards"] = "S Kxx5";
  req_make["actual_deal"]["N"] = "S AQ; H -; D -; C -;";
  req_make["actual_deal"]["E"] = "S x5; H -; D -; C -;";
  req_make["actual_deal"]["S"] = "S 87; H -; D -; C -;";
  req_make["actual_deal"]["W"] = "S Kx; H -; D -; C -;";

  int status_make = 0;
  std::string body_make = engine.SolveJson(req_make.dump(), &status_make);
  REQUIRE(status_make == 200);
  nlohmann::json res_make = nlohmann::json::parse(body_make);
  REQUIRE(res_make["success"] == true);
  REQUIRE(res_make["total_boards"] == 6);
  REQUIRE(res_make.contains("actual_hand"));
  REQUIRE(res_make["actual_hand"]["board_id"] == -1);
  REQUIRE(res_make["actual_hand"]["succeeded"] == true);
  REQUIRE(res_make["actual_hand"]["ns_tricks"] == 2);
  REQUIRE(res_make["tree"].contains("boards"));
  const auto &root_boards_make = res_make["tree"]["boards"];
  REQUIRE(std::find(root_boards_make.begin(), root_boards_make.end(), -1) !=
          root_boards_make.end());

  // Case 2: Actual layout has SK offside in East -> finesse fails (1 trick).
  nlohmann::json req_fail = req_make;
  req_fail["actual_deal"]["E"] = "S Kx; H -; D -; C -;";
  req_fail["actual_deal"]["W"] = "S x5; H -; D -; C -;";

  int status_fail = 0;
  std::string body_fail = engine.SolveJson(req_fail.dump(), &status_fail);
  REQUIRE(status_fail == 200);
  nlohmann::json res_fail = nlohmann::json::parse(body_fail);
  REQUIRE(res_fail["success"] == true);
  REQUIRE(res_fail["total_boards"] == 6);
  REQUIRE(res_fail.contains("actual_hand"));
  REQUIRE(res_fail["actual_hand"]["board_id"] == -1);
  REQUIRE(res_fail["actual_hand"]["succeeded"] == false);
  REQUIRE(res_fail["actual_hand"]["ns_tricks"] == 1);
  const auto &root_boards_fail = res_fail["tree"]["boards"];
  REQUIRE(std::find(root_boards_fail.begin(), root_boards_fail.end(), -1) !=
          root_boards_fail.end());
}
