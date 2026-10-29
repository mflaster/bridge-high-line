#include <gtest/gtest.h>
#include <chrono>
#include <functional>
#include <thread>

#include "line_finder.h"
#include "third_party/httplib.h"
#include "third_party/nlohmann/json.hpp"
#include "web_server.h"

TEST(ServerTest, WebServerHealthAndStaticAssets) {
  int port = 18080;
  WebServer server("127.0.0.1", port);
  ASSERT_TRUE(server.Start(/*async=*/true));

  // Allow server thread to initialize
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  httplib::Client client("127.0.0.1", port);
  client.set_keep_alive(false);

  // Test static HTML serving
  auto res_index = client.Get("/");
  ASSERT_TRUE(res_index);
  ASSERT_TRUE(res_index->status == 200);
  ASSERT_TRUE(res_index->body.find("Bridge High Line Solver") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("traceHandInput") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("traceHandBtn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("arrow-traced") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("computeNewLineBtn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("resetLineBtn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("equivalentLinesBar") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("equivalentLineSelect") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("tabCompareLinesBtn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("panelCompareLines") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("compareLinesList") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("compareLinesResults") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("Rand deals:") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("openHelpBtn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("helpModal") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("helpTopicsNav") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("openFeedbackBtn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("feedbackModal") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("header-left") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("header-import-btn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("importHandModal") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("importIntroBox") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("Hand record preferred") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("importSampleSheetBtn") != std::string::npos);

  auto res_help_html = client.Get("/help.html");
  ASSERT_TRUE(res_help_html);
  ASSERT_TRUE(res_help_html->status == 200);
  ASSERT_TRUE(res_help_html->body.find("helpPageTopicsNav") != std::string::npos);
  ASSERT_TRUE(res_help_html->body.find(
              "Bridge Deal Solver — Help &amp; Strategy Guide") !=
          std::string::npos);

  auto res_strategy = client.Get("/bridge-strategy.md");
  ASSERT_TRUE(res_strategy);
  ASSERT_TRUE(res_strategy->status == 200);
  ASSERT_TRUE(res_strategy->body.find("How the Solver Thinks About Bridge") !=
          std::string::npos);

  auto res_guide = client.Get("/user-guide.md");
  ASSERT_TRUE(res_guide);
  ASSERT_TRUE(res_guide->status == 200);
  ASSERT_TRUE(res_guide->body.find("Loading Hands & Setting Up a Deal") !=
          std::string::npos);
  ASSERT_TRUE(res_guide->body.find("images/hand-record-scanner.png") !=
          std::string::npos);
  ASSERT_TRUE(res_guide->body.find("images/decision-tree-inspector.png") !=
          std::string::npos);
  ASSERT_TRUE(res_guide->body.find("images/propose-and-compare-lines.png") !=
          std::string::npos);
  ASSERT_TRUE(res_guide->body.find("images/distribution-summary.png") !=
          std::string::npos);

  // Test CSS serving
  auto res_css = client.Get("/styles.css");
  ASSERT_TRUE(res_css);
  ASSERT_TRUE(res_css->status == 200);
  ASSERT_TRUE(res_css->body.find(".traced-branch-node") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".traced-branch-link") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".trick-columns-header") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".card-chip.card-lead") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".card-chip.card-winner") != std::string::npos);
  ASSERT_TRUE(res_css->body.find("border-radius: 9999px") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".card-rank-playable") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".card-rank-current.card-rank-playable") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".equivalent-lines-bar") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".compare-line-row") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".compare-stacked-bar") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".feedback-modal-overlay") != std::string::npos);
  ASSERT_TRUE(res_css->body.find(".import-intro-box") != std::string::npos);

  // Test JS serving across modular JS files
  auto res_js = client.Get("/app.js");
  ASSERT_TRUE(res_js);
  ASSERT_TRUE(res_js->status == 200);
  ASSERT_TRUE(res_js->body.find(
                  "const nodeDetailBanner = "
                  "document.getElementById('nodeDetailBanner');") !=
              std::string::npos);
  ASSERT_TRUE(res_js->body.find(
                  "const nodeDetailContent = "
                  "document.getElementById('nodeDetailContent');") !=
              std::string::npos);
  ASSERT_TRUE(res_js->body.find("function formatProposedCardsDescriptor") !=
              std::string::npos);
  ASSERT_TRUE(res_js->body.find("function openImportHandModal") !=
              std::string::npos);

  auto res_solver_api_js = client.Get("/solver_api.js");
  ASSERT_TRUE(res_solver_api_js);
  ASSERT_TRUE(res_solver_api_js->status == 200);
  ASSERT_TRUE(res_solver_api_js->body.find("const WASM_NUM_THREADS =") !=
              std::string::npos);
  ASSERT_TRUE(res_solver_api_js->body.find(
                  "function ensureCrossOriginIsolation") != std::string::npos);

  auto res_deal_svg_js = client.Get("/deal_svg.js");
  ASSERT_TRUE(res_deal_svg_js);
  ASSERT_TRUE(res_deal_svg_js->status == 200);
  ASSERT_TRUE(res_deal_svg_js->body.find("function handleProposeCardClick") !=
              std::string::npos);

  auto res_analysis_tabs_js = client.Get("/analysis_tabs.js");
  ASSERT_TRUE(res_analysis_tabs_js);
  ASSERT_TRUE(res_analysis_tabs_js->status == 200);
  ASSERT_TRUE(res_analysis_tabs_js->body.find(
                  "function selectEquivalentLine") != std::string::npos);
  ASSERT_TRUE(res_analysis_tabs_js->body.find(
                  "function formatLineSuccessPctPrefix") != std::string::npos);
  ASSERT_TRUE(res_analysis_tabs_js->body.find(
                  "function buildSelectableLinesList") != std::string::npos);
  ASSERT_TRUE(res_analysis_tabs_js->body.find("Original line") !=
              std::string::npos);
  ASSERT_TRUE(res_analysis_tabs_js->body.find("Alt line") != std::string::npos);
  ASSERT_TRUE(res_analysis_tabs_js->body.find(
                  "function renderCompareLinesTab") != std::string::npos);
  ASSERT_TRUE(res_analysis_tabs_js->body.find(
                  "function renderComparePairSection") != std::string::npos);

  auto res_photo_scanner_js = client.Get("/photo_scanner.js");
  ASSERT_TRUE(res_photo_scanner_js);
  ASSERT_TRUE(res_photo_scanner_js->status == 200);
  ASSERT_TRUE(res_photo_scanner_js->body.find("parseModalFirstRoundCalls") !=
              std::string::npos);
  ASSERT_TRUE(res_photo_scanner_js->body.find("function parseLinBoardsFromText") !=
              std::string::npos);

  auto res_decision_tree_js = client.Get("/decision_tree.js");
  ASSERT_TRUE(res_decision_tree_js);
  ASSERT_TRUE(res_decision_tree_js->status == 200);
  ASSERT_TRUE(res_decision_tree_js->body.find("function traceHandByNumber") !=
              std::string::npos);
  ASSERT_TRUE(res_decision_tree_js->body.find("leadSeatIdx") !=
              std::string::npos);
  ASSERT_TRUE(res_decision_tree_js->body.find("trick-columns-header") !=
              std::string::npos);
  ASSERT_TRUE(res_decision_tree_js->body.find("function zoomAroundPoint") !=
              std::string::npos);
  ASSERT_TRUE(res_decision_tree_js->body.find(
                  "function getNodeFillProportions") != std::string::npos);
  ASSERT_TRUE(res_decision_tree_js->body.find("node-fill-pat-") !=
              std::string::npos);
  ASSERT_TRUE(res_decision_tree_js->body.find("selectedTrickStates") !=
              std::string::npos);

  auto res_telemetry_js = client.Get("/telemetry_and_help.js");
  ASSERT_TRUE(res_telemetry_js);
  ASSERT_TRUE(res_telemetry_js->status == 200);
  ASSERT_TRUE(res_telemetry_js->body.find("function submitSentryFeedback") !=
              std::string::npos);
  ASSERT_TRUE(
      res_telemetry_js->body.find(
          "const feedbackMessage = (message || '').slice(0, 4096);") !=
      std::string::npos);
  ASSERT_TRUE(res_telemetry_js->body.find(
                  "function getActivePresetAndHandInfo") != std::string::npos);
  ASSERT_TRUE(res_telemetry_js->body.find("Preset / Hand:") !=
              std::string::npos);

  ASSERT_TRUE(res_index->body.find("saveAnalysisBtn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("pdf_export.js") != std::string::npos);

  auto res_pdf_export_js = client.Get("/pdf_export.js");
  ASSERT_TRUE(res_pdf_export_js);
  ASSERT_TRUE(res_pdf_export_js->status == 200);
  ASSERT_TRUE(res_pdf_export_js->body.find(
                  "function generateAnalysisPdfPages") != std::string::npos);
  ASSERT_TRUE(res_pdf_export_js->body.find(
                  "function buildPdfFromPageCanvases") != std::string::npos);
  ASSERT_TRUE(res_pdf_export_js->body.find("function saveAnalysisAsPdf") !=
              std::string::npos);

  // Test Presets Endpoints
  auto res_presets_file = client.Get("/presets.json");
  ASSERT_TRUE(res_presets_file);
  ASSERT_TRUE(res_presets_file->status == 200);
  nlohmann::json presets_file_json = nlohmann::json::parse(res_presets_file->body);
  ASSERT_TRUE(presets_file_json.contains("finesse"));
  ASSERT_TRUE(presets_file_json.contains("nine_card_hearts"));
  ASSERT_TRUE(presets_file_json.contains("safety_play"));
  ASSERT_TRUE(presets_file_json.contains("twelve_card_spades"));
  ASSERT_TRUE(presets_file_json.contains("restricted_choice"));
  ASSERT_TRUE(presets_file_json.contains("mollo_6s"));
  ASSERT_TRUE(presets_file_json["mollo_6s"]["cur_trick"] == nlohmann::json::array({"DQ"}));
  ASSERT_TRUE(presets_file_json.contains("two_way_finesse"));
  ASSERT_TRUE(presets_file_json.contains("hand_8_261014"));
  ASSERT_TRUE(presets_file_json["hand_8_261014"]["cur_trick"] ==
          nlohmann::json::array({"CJ"}));
  ASSERT_TRUE(presets_file_json.contains("hand_6_261014"));
  const auto &h6 = presets_file_json["hand_6_261014"];
  ASSERT_TRUE(h6["cur_trick"] == nlohmann::json::array({"HA"}));
  ASSERT_TRUE(h6.contains("requirements"));
  ASSERT_TRUE(h6["requirements"]["E"]["would_bid"][0]["bid"] == "X");
  ASSERT_TRUE(h6["requirements"]["E"]["would_bid"][0]["after"] == "P 1S");
  ASSERT_TRUE(h6["requirements"]["W"]["would_bid"][0]["bid"] == "P");
  for (const auto& [key, preset] : presets_file_json.items()) {
    ASSERT_TRUE(preset["iters"].get<int>() >= 100);
  }

  auto res_presets_api = client.Get("/api/presets");
  ASSERT_TRUE(res_presets_api);
  ASSERT_TRUE(res_presets_api->status == 200);
  nlohmann::json presets_api_json = nlohmann::json::parse(res_presets_api->body);
  ASSERT_TRUE(presets_api_json.contains("endplay"));
  ASSERT_TRUE(presets_api_json.contains("nine_card_hearts"));
  ASSERT_TRUE(presets_api_json.contains("safety_play"));
  ASSERT_TRUE(presets_api_json.contains("twelve_card_spades"));
  ASSERT_TRUE(presets_api_json.contains("restricted_choice"));
  ASSERT_TRUE(presets_api_json.contains("mollo_6s"));
  ASSERT_TRUE(presets_api_json.contains("two_way_finesse"));
  ASSERT_TRUE(presets_api_json.contains("hand_8_261014"));
  ASSERT_TRUE(presets_api_json.contains("hand_6_261014"));
  for (const auto& [key, preset] : presets_api_json.items()) {
    ASSERT_TRUE(preset["iters"].get<int>() >= 100);
  }

  // Test Health Endpoint
  auto res_health = client.Get("/api/health");
  ASSERT_TRUE(res_health);
  ASSERT_TRUE(res_health->status == 200);
  nlohmann::json health_json = nlohmann::json::parse(res_health->body);
  ASSERT_TRUE(health_json["status"] == "ok");

  // Test Solve Status Endpoint
  LineFinder::SetSolveStatus("Analyzing trick 1...");
  auto res_status = client.Get("/api/solve_status");
  ASSERT_TRUE(res_status);
  ASSERT_TRUE(res_status->status == 200);
  nlohmann::json status_json = nlohmann::json::parse(res_status->body);
  ASSERT_TRUE(status_json["status"] == "Analyzing trick 1...");
  // The progress estimate only exists while a play-tree search is running.
  ASSERT_TRUE(!status_json.contains("progress"));
  ASSERT_TRUE(LineFinder::GetSolveProgress() < 0.0);

  server.Stop();
}

TEST(ServerTest, WebServerRESTAPIApiSolveIntegration) {
  int port = 18081;
  WebServer server("127.0.0.1", port);
  ASSERT_TRUE(server.Start(/*async=*/true));

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
  ASSERT_TRUE(res_solve);
  ASSERT_TRUE(res_solve->status == 200);

  nlohmann::json res_json = nlohmann::json::parse(res_solve->body);
  ASSERT_TRUE(res_json["success"] == true);
  ASSERT_TRUE(res_json["tricks_won"] == 1);
  ASSERT_TRUE(res_json["best_lead"] == "SA");
  ASSERT_TRUE(res_json.contains("tree"));
  ASSERT_TRUE(res_json["tree"].contains("id"));
  ASSERT_TRUE(res_json.contains("solve_ms"));
  ASSERT_TRUE(res_json.contains("invariant_ms"));
  ASSERT_TRUE(res_json["solve_ms"].get<double>() >= 0.0);
  ASSERT_TRUE(res_json["invariant_ms"].get<double>() >= 0.0);

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
  ASSERT_TRUE(res_endplay);
  ASSERT_TRUE(res_endplay->status == 200);

  nlohmann::json endplay_json = nlohmann::json::parse(res_endplay->body);
  ASSERT_TRUE(endplay_json["success"] == true);
  ASSERT_TRUE(endplay_json["tricks_won"] == 4);

  // Check decision tree hierarchy down to 4 tricks
  auto &tree = endplay_json["tree"];
  std::string ep_root_label = tree["label"].get<std::string>();
  auto &trick1_node = (ep_root_label.find("#1:") != std::string::npos) ? tree : tree["children"][0];

  // Trick 1 (collapsed by default)
  ASSERT_TRUE(trick1_node["label"].get<std::string>().find("#1:") != std::string::npos);
  ASSERT_TRUE(trick1_node["collapsed"] == true);
  ASSERT_TRUE(trick1_node["children"].size() >= 1);

  // Trick 2
  auto &trick2_node = trick1_node["children"][0];
  ASSERT_TRUE(trick2_node["label"].get<std::string>().find("#2:") != std::string::npos);
  ASSERT_TRUE(trick2_node["children"].size() >= 1);

  // Trick 3
  auto &trick3_node = trick2_node["children"][0];
  ASSERT_TRUE(trick3_node["label"].get<std::string>().find("#3:") != std::string::npos);
  ASSERT_TRUE(trick3_node["children"].size() >= 1);

  // Trick 4
  auto &trick4_node = trick3_node["children"][0];
  ASSERT_TRUE(trick4_node["label"].get<std::string>().find("#4:") != std::string::npos);
  ASSERT_TRUE(trick4_node["type"] == "outcome");

  // Test 3: Invalid JSON payload handling
  auto res_bad = client.Post("/api/solve", "{invalid json}", "application/json");
  ASSERT_TRUE(res_bad);
  ASSERT_TRUE(res_bad->status == 400);

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
  ASSERT_TRUE(res_fuzzy);
  ASSERT_TRUE(res_fuzzy->status == 200);

  nlohmann::json fuzzy_json = nlohmann::json::parse(res_fuzzy->body);
  ASSERT_TRUE(fuzzy_json["success"] == true);
  ASSERT_TRUE(fuzzy_json.contains("solve_ms"));
  ASSERT_TRUE(fuzzy_json.contains("invariant_ms"));
  ASSERT_TRUE(fuzzy_json["solve_ms"].get<double>() >= 0.0);
  ASSERT_TRUE(fuzzy_json["invariant_ms"].get<double>() >= 0.0);
  ASSERT_TRUE(fuzzy_json.contains("distribution_summary"));
  auto &dist_sum = fuzzy_json["distribution_summary"];
  ASSERT_TRUE(dist_sum.contains("invariant_rules"));
  ASSERT_TRUE(dist_sum.contains("suit_bounds"));
  ASSERT_TRUE(dist_sum.contains("honor_stats"));

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
  ASSERT_TRUE(res_finesse);
  ASSERT_TRUE(res_finesse->status == 200);
  nlohmann::json finesse_json = nlohmann::json::parse(res_finesse->body);
  ASSERT_TRUE(finesse_json["success"] == true);
  ASSERT_TRUE(finesse_json.contains("tree"));
  ASSERT_TRUE(finesse_json.contains("sample_hands"));
  ASSERT_TRUE(finesse_json["sample_hands"].is_array());
  ASSERT_TRUE(!finesse_json["sample_hands"].empty());
  ASSERT_TRUE(finesse_json["sample_hands"].size() <= 100);
  ASSERT_TRUE(finesse_json["sample_hands"][0].contains("N"));
  ASSERT_TRUE(finesse_json["sample_hands"][0].contains("E"));
  ASSERT_TRUE(finesse_json["sample_hands"][0].contains("S"));
  ASSERT_TRUE(finesse_json["sample_hands"][0].contains("W"));
  ASSERT_TRUE(finesse_json.contains("failure_sample_hands"));
  ASSERT_TRUE(finesse_json["failure_sample_hands"].is_array());
  ASSERT_TRUE(finesse_json.contains("all_hands"));
  ASSERT_TRUE(finesse_json["all_hands"].is_array());
  ASSERT_TRUE(static_cast<int>(finesse_json["all_hands"].size()) ==
          finesse_json["total_boards"].get<int>());
  for (int b = 0; b < finesse_json["total_boards"].get<int>(); ++b) {
    ASSERT_TRUE(finesse_json["all_hands"][b]["board_id"] == b);
    ASSERT_TRUE(finesse_json["all_hands"][b].contains("E"));
    ASSERT_TRUE(finesse_json["all_hands"][b].contains("W"));
    ASSERT_TRUE(finesse_json["all_hands"][b].contains("succeeded"));
  }
  std::set<int> leaf_boards;
  std::function<void(const nlohmann::json &)> collect_leaf_boards =
      [&](const nlohmann::json &n) {
    if (!n.contains("children") || n["children"].empty()) {
      ASSERT_TRUE(n.contains("boards"));
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
  ASSERT_TRUE(static_cast<int>(leaf_boards.size()) ==
          finesse_json["total_boards"].get<int>());
  ASSERT_TRUE(!finesse_json.contains("failure_tree"));
  // Failed boards are attached to the single tree as failure leaves; together
  // they account for every failed board exactly once.
  std::function<int(const nlohmann::json &)> count_failure_leaf_boards =
      [&](const nlohmann::json &n) -> int {
    int sum = 0;
    if (n.value("failure", false)) {
      EXPECT_TRUE(n["type"] == "outcome");
      EXPECT_TRUE(n["label"].get<std::string>().rfind("Hand Failed", 0) == 0);
      EXPECT_TRUE(n["children"].empty());
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
    ASSERT_TRUE(!finesse_json["failure_sample_hands"].empty());
    ASSERT_TRUE(count_failure_leaf_boards(finesse_json["tree"]) ==
            finesse_json["failure_boards"].get<int>());
    ASSERT_TRUE(finesse_json["tree"]["failure_boards"].get<int>() ==
            finesse_json["failure_boards"].get<int>());
  } else {
    ASSERT_TRUE(count_failure_leaf_boards(finesse_json["tree"]) == 0);
  }
  auto &finesse_tree = finesse_json["tree"];
  ASSERT_TRUE(finesse_tree.contains("label"));
  // Ensure the tree is collapsed and does not contain redundant 'Hand Succeeded' or 'after ...' single-child wrappers
  std::string root_label = finesse_tree["label"].get<std::string>();
  ASSERT_TRUE(root_label.find("Hand Succeeded") == std::string::npos);
  if (finesse_tree.contains("children") && !finesse_tree["children"].empty()) {
    for (const auto &c : finesse_tree["children"]) {
      std::string child_label = c["label"].get<std::string>();
      ASSERT_TRUE(child_label.find("Hand Succeeded") == std::string::npos);
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
  ASSERT_TRUE(res_hearts9);
  ASSERT_TRUE(res_hearts9->status == 200);
  nlohmann::json hearts9_json = nlohmann::json::parse(res_hearts9->body);
  ASSERT_TRUE(hearts9_json["success"] == true);
  ASSERT_TRUE(hearts9_json["num_threads"] == 16);

  for (int threads : {1, 2, 4, 8, 16}) {
    hearts9_req["num_threads"] = threads;
    hearts9_req["num_iters"] = 4;
    auto res_t = client.Post("/api/solve", hearts9_req.dump(), "application/json");
    ASSERT_TRUE(res_t);
    ASSERT_TRUE(res_t->status == 200);
    nlohmann::json t_json = nlohmann::json::parse(res_t->body);
    ASSERT_TRUE(t_json["success"] == true);
    ASSERT_TRUE(t_json["num_threads"] == threads);
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
  ASSERT_TRUE(res_wildcard);
  ASSERT_TRUE(res_wildcard->status == 200);
  nlohmann::json wildcard_json = nlohmann::json::parse(res_wildcard->body);
  ASSERT_TRUE(wildcard_json["success"] == true);

  // Test 7: Error handling - conflicting unknown card
  nlohmann::json conflict_req;
  conflict_req["hands"]["N"] = "S AQ";
  conflict_req["hands"]["S"] = "S 87";
  conflict_req["unknown_cards"] = "S SAxx"; // SA already in North
  auto res_conflict = client.Post("/api/solve", conflict_req.dump(), "application/json");
  ASSERT_TRUE(res_conflict);
  ASSERT_TRUE(res_conflict->status == 400);
  nlohmann::json conflict_json = nlohmann::json::parse(res_conflict->body);
  ASSERT_TRUE(conflict_json["success"] == false);
  ASSERT_TRUE(conflict_json["error"].get<std::string>().find("conflicts") != std::string::npos);

  // Test 8: Error handling - invalid unknown card count (5 provided vs 4 expected)
  nlohmann::json count_err_req;
  count_err_req["hands"]["N"] = "S AQ";
  count_err_req["hands"]["S"] = "S 87";
  count_err_req["unknown_cards"] = "S Kxxxx"; // 5 cards provided, but 4 expected for 2-card ending
  auto res_count_err = client.Post("/api/solve", count_err_req.dump(), "application/json");
  ASSERT_TRUE(res_count_err);
  ASSERT_TRUE(res_count_err->status == 400);
  nlohmann::json count_err_json = nlohmann::json::parse(res_count_err->body);
  ASSERT_TRUE(count_err_json["success"] == false);
  ASSERT_TRUE(count_err_json["error"].get<std::string>().find("Invalid unknown cards count") != std::string::npos);

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
  ASSERT_TRUE(res_preset);
  ASSERT_TRUE(res_preset->status == 200);
  nlohmann::json preset_json = nlohmann::json::parse(res_preset->body);
  ASSERT_TRUE(preset_json["success"] == true);
  ASSERT_TRUE(preset_json.contains("tree"));
  ASSERT_TRUE(preset_json.contains("num_lines"));
  ASSERT_TRUE(preset_json["num_lines"].get<int>() >= 1);
  ASSERT_TRUE(preset_json.contains("lines"));
  ASSERT_TRUE(preset_json["lines"].is_array());
  ASSERT_TRUE(static_cast<int>(preset_json["lines"].size()) ==
          preset_json["num_lines"].get<int>());
  std::set<std::string> seen_line_labels;
  for (size_t i = 0; i < preset_json["lines"].size(); ++i) {
    const auto &line_obj = preset_json["lines"][i];
    ASSERT_TRUE(line_obj["index"].get<size_t>() == i);
    ASSERT_TRUE(line_obj.contains("label"));
    std::string lbl = line_obj["label"].get<std::string>();
    ASSERT_TRUE(!lbl.empty());
    seen_line_labels.insert(lbl);
    ASSERT_TRUE(line_obj.contains("best_lead"));
    ASSERT_TRUE(!line_obj["best_lead"].get<std::string>().empty());
    ASSERT_TRUE(line_obj.contains("covered_boards"));
    ASSERT_TRUE(line_obj["covered_boards"] == preset_json["covered_boards"]);
    ASSERT_TRUE(line_obj.contains("covered_board_ids"));
    ASSERT_TRUE(line_obj["covered_board_ids"].is_array());
    ASSERT_TRUE(line_obj.contains("tree"));
    ASSERT_TRUE(line_obj["tree"].contains("id"));
  }
  ASSERT_TRUE(seen_line_labels.size() == preset_json["lines"].size());

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
  ASSERT_TRUE(p_trick1["label"].get<std::string>().find("#1:") != std::string::npos);
  ASSERT_TRUE(p_trick1["children"].size() >= 1);

  // Depth 2: Trick 2
  auto &p_trick2 = find_trick(p_trick1["children"][0], "#2:");
  ASSERT_TRUE(p_trick2["label"].get<std::string>().find("#2:") != std::string::npos);
  ASSERT_TRUE(p_trick2["children"].size() >= 1);

  // Depth 3: Trick 3
  auto &p_trick3 = find_trick(p_trick2["children"][0], "#3:");
  ASSERT_TRUE(p_trick3["label"].get<std::string>().find("#3:") != std::string::npos);
  if (p_trick3["children"].size() > 1) {
    bool found_branch_cond = false;
    for (const auto &c : p_trick3["children"]) {
      if (c.contains("condition") && !c["condition"].get<std::string>().empty()) {
        found_branch_cond = true;
        std::string cond = c["condition"].get<std::string>();
        ASSERT_TRUE(cond.find("0 wins") == std::string::npos);
        ASSERT_TRUE(cond.find("1 wins") == std::string::npos);
        ASSERT_TRUE((cond.find("N wins") != std::string::npos || cond.find("E wins") != std::string::npos ||
                 cond.find("S wins") != std::string::npos || cond.find("W wins") != std::string::npos));
        ASSERT_TRUE(cond.find(" wins the ") != std::string::npos);
      }
    }
    ASSERT_TRUE(found_branch_cond);
  }

  // Find outcome leaf at depth 4
  nlohmann::json *curr = &p_trick1;
  while (curr->contains("children") && !(*curr)["children"].empty()) {
    curr = &((*curr)["children"][0]);
  }
  ASSERT_TRUE((*curr)["type"] == "outcome");
  ASSERT_TRUE((*curr)["success_boards"] > 0);
  ASSERT_TRUE((*curr).contains("ns_tricks"));
  ASSERT_TRUE((*curr)["label"].get<std::string>().find("#") != std::string::npos);

  server.Stop();
}

TEST(ServerTest, BuildTreeFromConditionalLineCommonLeadCardLabel) {
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
  ASSERT_TRUE(node != nullptr);
  ASSERT_TRUE(node->label == "H7 is led");

  // Test fallback when declarer children have different lead cards (e.g.
  // IsCardGoodCondition in squeeze deals)
  ConditionalLine root_diff;
  ConditionalLine child_diff_1;
  child_diff_1.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("__", "HK", boards),
      GeneralizedDemiStruct("__", "S3", boards),
      boards, 2));

  ConditionalLine child_diff_2;
  child_diff_2.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("S_", "S2", boards),
      GeneralizedDemiStruct("__", "SA", boards),
      boards, 2));

  CardType hk = Card::ParseStringCard("HK");
  root_diff.AddCondition(new IsCardGoodCondition(hk, true), child_diff_1);
  root_diff.AddCondition(new IsCardGoodCondition(hk, false), child_diff_2);

  int node_id2 = 0;
  auto node_diff = BuildTreeFromConditionalLine(root_diff, node_id2);
  ASSERT_TRUE(node_diff != nullptr);
  ASSERT_EQ(node_diff->label, "Declarer leads");

  // Test state condition (e.g. PlayerDiscardedCondition) where both branches
  // happen to lead the same card (CJ): must still use "Declarer leads" rather
  // than "CJ is led".
  ConditionalLine root_state_same_lead;
  ConditionalLine child_state_1;
  child_state_1.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("C_", "CJ", boards),
      GeneralizedDemiStruct("C_", "CA", boards),
      boards, 0));
  ConditionalLine child_state_2;
  child_state_2.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("C_", "CJ", boards),
      GeneralizedDemiStruct("C_", "CQ", boards),
      boards, 0));
  root_state_same_lead.AddCondition(
      new PlayerDiscardedCondition(1, SPADES, true), child_state_1);
  root_state_same_lead.AddCondition(
      new PlayerDiscardedCondition(1, SPADES, false), child_state_2);

  int node_id_state = 0;
  auto node_state =
      BuildTreeFromConditionalLine(root_state_same_lead, node_id_state);
  ASSERT_TRUE(node_state != nullptr);
  ASSERT_EQ(node_state->label, "Declarer leads");

  // Test when a defender is on lead across branches
  ConditionalLine root_def;
  ConditionalLine child_def_1;
  child_def_1.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("H_", "H7", boards),
      GeneralizedDemiStruct("H_", "HK", boards),
      boards, 1));

  ConditionalLine child_def_2;
  child_def_2.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("S_", "SA", boards),
      GeneralizedDemiStruct("S_", "SK", boards),
      boards, 1));

  root_def.AddCondition(new TrickPlayCondition("(H_) H7 (H_) HK"), child_def_1);
  root_def.AddCondition(new TrickPlayCondition("(S_) SA (S_) SK"), child_def_2);

  int node_id3 = 0;
  auto node_def = BuildTreeFromConditionalLine(root_def, node_id3);
  ASSERT_TRUE(node_def != nullptr);
  ASSERT_EQ(node_def->label, "Defender leads");
}

TEST(ServerTest, FormatWinnerConditionAndFormatBranchConditionFormatting) {
  WinnerPlayCondition cond_no_cards(0);
  ASSERT_TRUE(FormatWinnerCondition(&cond_no_cards) == "North wins");
  ASSERT_TRUE(FormatBranchCondition(&cond_no_cards, false) == "if N wins");
  ASSERT_TRUE(FormatBranchCondition(&cond_no_cards, true) == "when N wins");

  WinnerPlayCondition cond_with_cards(1, std::vector<std::string>{"SK", "SQ"});
  ASSERT_TRUE(FormatWinnerCondition(&cond_with_cards) == "East wins the SK,SQ");
  ASSERT_TRUE(FormatBranchCondition(&cond_with_cards, false) == "if E wins the SK,SQ");
  ASSERT_TRUE(FormatBranchCondition(&cond_with_cards, true) == "when E wins the SK,SQ");
}

TEST(ServerTest, BuildTreeFromConditionalLinePreservesTrickLeaderAndWinner) {
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
  ASSERT_TRUE(tree != nullptr);
  ASSERT_TRUE(tree->leader == 0);
  ASSERT_TRUE(tree->winner == 3);
  ASSERT_TRUE(tree->children.size() == 1);

  auto trick2_node = tree->children[0];
  ASSERT_TRUE(trick2_node != nullptr);
  ASSERT_TRUE(trick2_node->leader == 3);
  ASSERT_TRUE(trick2_node->winner == 2);
  ASSERT_TRUE(trick2_node->branch_winner == 3);

  nlohmann::json j = *tree;
  ASSERT_TRUE(j["leader"] == 0);
  ASSERT_TRUE(j["winner"] == 3);
  ASSERT_TRUE(j["children"][0]["leader"] == 3);
  ASSERT_TRUE(j["children"][0]["winner"] == 2);
  ASSERT_TRUE(j["children"][0]["branch_winner"] == 3);
}

TEST(ServerTest, UnmatchedDefenderPlayAttachesToConditionNode) {
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
  ASSERT_TRUE(tree != nullptr);
  ASSERT_TRUE(tree->type == "condition");
  ASSERT_TRUE(tree->label == "C6 is led");
  // 2 trick variants + 2 sibling failure leaves ("if E plays CK",
  // "if E discards")
  ASSERT_TRUE(tree->children.size() == 4);

  auto var1_node = tree->children[0];
  ASSERT_TRUE(var1_node->label == "#1: C6 (Cx) CJ (C_)");
  // Variant 1 has 1 success child + 1 winner-failure child ("if W wins the CQ")
  ASSERT_TRUE(var1_node->children.size() == 2);
  ASSERT_TRUE(var1_node->children[1]->failure == true);
  ASSERT_TRUE(var1_node->children[1]->condition == "if W wins the CQ");
  ASSERT_TRUE(var1_node->children[1]->board_to_trick.at("2") == "C6 C2 CJ CQ");

  auto ck_fail = tree->children[2];
  ASSERT_TRUE(ck_fail->failure == true);
  ASSERT_TRUE(ck_fail->condition == "if E plays CK");
  ASSERT_TRUE(ck_fail->trick == "C6 CK CJ C3");
  ASSERT_TRUE(ck_fail->leader == 0);
  ASSERT_TRUE(ck_fail->winner == 1);
  ASSERT_TRUE(ck_fail->failure_boards == 1);

  auto discard_fail = tree->children[3];
  ASSERT_TRUE(discard_fail->failure == true);
  ASSERT_TRUE(discard_fail->condition == "if E discards");
  ASSERT_TRUE(discard_fail->trick == "C6 (R_) CJ C3");
  ASSERT_TRUE(discard_fail->board_to_trick.at("3") == "C6 D6 CJ C3");
  ASSERT_TRUE(discard_fail->board_to_trick.at("4") == "C6 S7 CJ C3");
  ASSERT_TRUE(discard_fail->leader == 0);
  ASSERT_TRUE(discard_fail->winner == 2);
  ASSERT_TRUE(discard_fail->failure_boards == 2);

  // Also verify single-variant trick node wraps in "C6 is led" when an
  // unmatched defender play occurs.
  FailureIndex single_failures = IndexFailures(var1, boards, {3}, 1);
  int single_node_id = 0;
  auto single_tree = BuildTreeFromConditionalLine(var1, single_node_id, 1, 6, 0,
                                                  &single_failures);
  ASSERT_TRUE(single_tree != nullptr);
  ASSERT_TRUE(single_tree->type == "condition");
  ASSERT_TRUE(single_tree->label == "C6 is led");
  ASSERT_TRUE(single_tree->children.size() == 2);
  ASSERT_TRUE(single_tree->children[0]->label == "#1: C6 (Cx) CJ (C_)");
  ASSERT_TRUE(single_tree->children[0]->condition == "C6 (Cx) CJ (C_)");
  ASSERT_TRUE(single_tree->children[1]->failure == true);
  ASSERT_TRUE(single_tree->children[1]->condition == "if E discards");
  ASSERT_TRUE(single_tree->children[1]->trick == "C6 D6 CJ C3");
}

TEST(ServerTest, WebServerConcurrentMultithreadedApiSolveRequests) {
  int port = 18082;
  WebServer server("127.0.0.1", port);
  ASSERT_TRUE(server.Start(/*async=*/true));

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
  ASSERT_TRUE(res_baseline);
  ASSERT_TRUE(res_baseline->status == 200);

  nlohmann::json baseline_json = nlohmann::json::parse(res_baseline->body);
  ASSERT_TRUE(baseline_json["success"] == true);
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

  ASSERT_TRUE(all_passed.load());

  server.Stop();
}

TEST(ServerTest, WebServerSolveStatusLifecycleTest) {
  int port = 18086;
  WebServer server("127.0.0.1", port);
  ASSERT_TRUE(server.Start(/*async=*/true));
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
  ASSERT_TRUE(res);
  ASSERT_TRUE(res->status == 200);

  auto res_status = client.Get("/api/solve_status");
  ASSERT_TRUE(res_status);
  ASSERT_TRUE(res_status->status == 200);
  nlohmann::json status_json = nlohmann::json::parse(res_status->body);
  ASSERT_TRUE(status_json["status"] == "Solve complete.");

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
  ASSERT_TRUE(res_mid);
  ASSERT_TRUE(res_mid->status == 200);
  nlohmann::json mid_json = nlohmann::json::parse(res_mid->body);
  ASSERT_TRUE(mid_json["success"] == true);
  ASSERT_TRUE(mid_json["best_lead"] == "S8");
  ASSERT_TRUE(mid_json["tricks_won"] == 2);
  ASSERT_TRUE(mid_json.contains("tree"));
  std::string root_lbl = mid_json["tree"].value("label", "");
  if (root_lbl.find("#") == std::string::npos &&
      mid_json["tree"].contains("children") &&
      !mid_json["tree"]["children"].empty()) {
    root_lbl = mid_json["tree"]["children"][0].value("label", "");
  }
  ASSERT_TRUE(root_lbl.find("#2:") != std::string::npos);

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
  ASSERT_TRUE(res_void);
  ASSERT_TRUE(res_void->status == 200);
  nlohmann::json void_json = nlohmann::json::parse(res_void->body);
  ASSERT_TRUE(void_json["success"] == true);
  for (const auto &h : void_json["all_hands"]) {
    std::string e_hand = h["E"].get<std::string>();
    ASSERT_TRUE(e_hand.find("D ") == std::string::npos);
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
  ASSERT_TRUE(res_base);
  ASSERT_TRUE(res_base->status == 200);
  nlohmann::json base_json = nlohmann::json::parse(res_base->body);
  ASSERT_TRUE(base_json["success"] == true);
  ASSERT_TRUE(base_json.contains("covered_board_ids"));
  ASSERT_TRUE(base_json["all_hands"].size() == 16);

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
  ASSERT_TRUE(res_alt);
  ASSERT_TRUE(res_alt->status == 200);
  nlohmann::json alt_json = nlohmann::json::parse(res_alt->body);
  ASSERT_TRUE(alt_json["success"] == true);
  ASSERT_TRUE(alt_json.contains("covered_board_ids"));
  ASSERT_TRUE(alt_json["all_hands"].size() == 16);
  for (int i = 0; i < 16; ++i) {
    ASSERT_TRUE(alt_json["all_hands"][i]["E"] == base_json["all_hands"][i]["E"]);
    ASSERT_TRUE(alt_json["all_hands"][i]["W"] == base_json["all_hands"][i]["W"]);
  }

  // Regression test: alternate line with reuse_deals=true, replay_tricks where
  // some boards cannot follow the branch (!valid_for_branch), and initial-hand
  // requirements (e.g. HighCardRange / WouldBid / contradictory SuitLength on
  // remaining cards) does not crash in FuzzyBoardState::Randomize.
  nlohmann::json alt_reqs_req = alt_req;
  alt_reqs_req["replay_tricks"] =
      nlohmann::json::array({nlohmann::json::array({"HA", "H4", "H2", "H3"})});
  alt_reqs_req["requirements"]["W"]["hcp_low"] = 10;
  alt_reqs_req["requirements"]["W"]["hcp_high"] = 37;
  alt_reqs_req["requirements"]["W"]["would_bid"] =
      nlohmann::json::array({{{"bid", "1NT"}, {"after", ""}}});
  alt_reqs_req["requirements"]["E"]["suit_max"]["S"] = 0;

  auto res_alt_reqs =
      client.Post("/api/solve", alt_reqs_req.dump(), "application/json");
  ASSERT_TRUE(res_alt_reqs);
  ASSERT_TRUE(res_alt_reqs->status == 200);
  nlohmann::json alt_reqs_json = nlohmann::json::parse(res_alt_reqs->body);
  ASSERT_TRUE(alt_reqs_json["success"] == true);

  // Regression test: if a solve request triggers a glog LOG(FATAL) (e.g. an
  // invalid trump string passed to Card::ParseSuit), WebServer::SolveJson
  // intercepts the fatal log and returns a clean HTTP 400 JSON error with the
  // file/line/message instead of dumping core and killing the server.
  nlohmann::json fatal_req = base_req;
  fatal_req["trump"] = "INVALID_SUIT_STRING";
  auto res_fatal =
      client.Post("/api/solve", fatal_req.dump(), "application/json");
  ASSERT_TRUE(res_fatal);
  ASSERT_TRUE(res_fatal->status == 400);
  nlohmann::json fatal_json = nlohmann::json::parse(res_fatal->body);
  ASSERT_TRUE(fatal_json["success"] == false);
  std::string fatal_err = fatal_json.value("error", "");
  ASSERT_TRUE(fatal_err.find("Unknown suit string") != std::string::npos);

  // Test /api/stop_solve endpoint and subsequent solve recovery
  auto res_stop = client.Post("/api/stop_solve", "", "application/json");
  ASSERT_TRUE(res_stop);
  ASSERT_TRUE(res_stop->status == 200);
  nlohmann::json stop_json = nlohmann::json::parse(res_stop->body);
  ASSERT_TRUE(stop_json["status"] == "stopping");
  ASSERT_TRUE(LineFinder::IsStopRequested());

  // Next /api/solve clears the stop flag automatically and succeeds
  auto res_after_stop =
      client.Post("/api/solve", base_req.dump(), "application/json");
  ASSERT_TRUE(res_after_stop);
  ASSERT_TRUE(res_after_stop->status == 200);
  nlohmann::json after_stop_json = nlohmann::json::parse(res_after_stop->body);
  ASSERT_TRUE(after_stop_json["success"] == true);
  ASSERT_TRUE(!LineFinder::IsStopRequested());

  auto res_index = client.Get("/");
  ASSERT_TRUE(res_index);
  ASSERT_TRUE(res_index->body.find("stopSolveBtn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("stopSolveActionBtn") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("photoScanModal") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("declarerRotateBar") != std::string::npos);
  ASSERT_TRUE(res_index->body.find("pdfPageBar") != std::string::npos);
  ASSERT_TRUE(res_index->body.find(
              "image/*,.pdf,application/pdf,.pbn,.lin,.txt,.deal") !=
          std::string::npos);

  auto res_app_js = client.Get("/app.js");
  ASSERT_TRUE(res_app_js);
  ASSERT_TRUE(res_app_js->body.find("applyDeclarerRotation") !=
              std::string::npos);

  auto res_scanner_js = client.Get("/photo_scanner.js");
  ASSERT_TRUE(res_scanner_js);
  ASSERT_TRUE(res_scanner_js->body.find("recognizeBoardFromImageData") !=
              std::string::npos);
  ASSERT_TRUE(res_scanner_js->body.find("openPhotoScanModalWithPdfFile") !=
              std::string::npos);
  ASSERT_TRUE(res_scanner_js->body.find("extractPbnBoardsFromPdfBuffer") !=
              std::string::npos);

  // Test partial-trick proposal with wildcard 'x' moved from Unknown Cards
  // into West's hand (e.g. Simple Finesse preset: S7, Sx, SA).
  auto res_fin_base = client.Post("/api/solve", req.dump(), "application/json");
  ASSERT_TRUE(res_fin_base);
  ASSERT_TRUE(res_fin_base->status == 200);
  nlohmann::json fin_base_json = nlohmann::json::parse(res_fin_base->body);
  ASSERT_TRUE(fin_base_json["success"] == true);

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
  ASSERT_TRUE(res_fin_partial);
  ASSERT_TRUE(res_fin_partial->status == 200);
  nlohmann::json fin_partial_json =
      nlohmann::json::parse(res_fin_partial->body);
  ASSERT_TRUE(fin_partial_json["success"] == true);
  ASSERT_TRUE(fin_partial_json["total_boards"] == 10);
  ASSERT_TRUE(fin_partial_json["success_boards"] == 0);
  ASSERT_TRUE(fin_partial_json["failure_boards"] == 10);
  ASSERT_TRUE(fin_partial_json.contains("tree"));
  ASSERT_TRUE(fin_partial_json["tree"]["label"].get<std::string>().find(
              "#1: S7 (S") != std::string::npos);
  ASSERT_TRUE(fin_partial_json["tree"]["label"].get<std::string>().find(
              ") SA (S") != std::string::npos);
  ASSERT_TRUE(fin_partial_json["tree"].contains("children"));
  ASSERT_TRUE(!fin_partial_json["tree"]["children"].empty());
  ASSERT_TRUE(fin_partial_json["tree"]["children"][0]["failure"] == true);
  for (int i = 0; i < 10; ++i) {
    ASSERT_TRUE(fin_partial_json["all_hands"][i]["E"] ==
            fin_base_json["all_hands"][i]["E"]);
    ASSERT_TRUE(fin_partial_json["all_hands"][i]["W"] ==
            fin_base_json["all_hands"][i]["W"]);
  }

  server.Stop();
}

TEST(ServerTest, WebServerDirectSolveJsonAndProgressCallback) {
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

  ASSERT_TRUE(status_code == 200);
  nlohmann::json res = nlohmann::json::parse(body);
  ASSERT_TRUE(res["success"] == true);
  ASSERT_TRUE(res["tricks_won"] == 1);
  ASSERT_TRUE(res["best_lead"] == "SA");
  ASSERT_TRUE(!progress_statuses.empty());

  nlohmann::json st = nlohmann::json::parse(engine.GetSolveStatusJson());
  ASSERT_TRUE(st["status"] == "Solve complete.");
}

TEST(ServerTest, WebServerSolveJsonActualDealEvaluationBoard0) {
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
  ASSERT_TRUE(status_make == 200);
  nlohmann::json res_make = nlohmann::json::parse(body_make);
  ASSERT_TRUE(res_make["success"] == true);
  ASSERT_TRUE(res_make["total_boards"] == 6);
  ASSERT_TRUE(res_make.contains("actual_hand"));
  ASSERT_TRUE(res_make["actual_hand"]["board_id"] == -1);
  ASSERT_TRUE(res_make["actual_hand"]["succeeded"] == true);
  ASSERT_TRUE(res_make["actual_hand"]["ns_tricks"] == 2);
  ASSERT_TRUE(res_make["tree"].contains("boards"));
  const auto &root_boards_make = res_make["tree"]["boards"];
  ASSERT_TRUE(std::find(root_boards_make.begin(), root_boards_make.end(), -1) !=
          root_boards_make.end());

  // Case 2: Actual layout has SK offside in East -> finesse fails (1 trick).
  nlohmann::json req_fail = req_make;
  req_fail["actual_deal"]["E"] = "S Kx; H -; D -; C -;";
  req_fail["actual_deal"]["W"] = "S x5; H -; D -; C -;";

  int status_fail = 0;
  std::string body_fail = engine.SolveJson(req_fail.dump(), &status_fail);
  ASSERT_TRUE(status_fail == 200);
  nlohmann::json res_fail = nlohmann::json::parse(body_fail);
  ASSERT_TRUE(res_fail["success"] == true);
  ASSERT_TRUE(res_fail["total_boards"] == 6);
  ASSERT_TRUE(res_fail.contains("actual_hand"));
  ASSERT_TRUE(res_fail["actual_hand"]["board_id"] == -1);
  ASSERT_TRUE(res_fail["actual_hand"]["succeeded"] == false);
  ASSERT_TRUE(res_fail["actual_hand"]["ns_tricks"] == 1);
  const auto &root_boards_fail = res_fail["tree"]["boards"];
  ASSERT_TRUE(std::find(root_boards_fail.begin(), root_boards_fail.end(), -1) !=
          root_boards_fail.end());
}

TEST(ServerTest, WebServerSolveJsonWouldBidRequirements) {
  WebServer engine;

  // Base E (11 cards): S 4; H QT; D KJT7; C T754 (6 HCP).
  // Base W (11 cards): S Q32; H 7642; D 9642; C - (2 HCP).
  // Unknown cards (4 cards): H AK C 32.
  // For E to satisfy WouldBid(X, P 1S), E MUST receive both HA and HK (6+7=13
  // HCP, 1-4-4-4), giving W C32 (2 HCP, passes).
  nlohmann::json req;
  req["hands"]["N"] = "S AJ8765; H J85; D -; C KJ86;";
  req["hands"]["S"] = "S KT9; H 93; D AQ853; C AQ9;";
  req["hands"]["E"] = "S 4; H QT; D KJT7; C T754;";
  req["hands"]["W"] = "S Q32; H 7642; D 9642; C -;";
  req["unknown_cards"] = "H AK C 32";
  req["trump"] = "SPADES";
  req["leader"] = "NORTH";
  req["min_tricks"] = 12;
  req["num_iters"] = 4;
  req["num_threads"] = 1;
  req["requirements"]["E"]["would_bid"] = nlohmann::json::array(
      {{{"bid", "X"}, {"after", "P 1S"}}});
  req["requirements"]["W"]["would_bid"] = nlohmann::json::array(
      {{{"bid", "P"}, {"after", ""}}});

  int status_code = 0;
  std::string body = engine.SolveJson(req.dump(), &status_code);
  ASSERT_EQ(status_code, 200);
  nlohmann::json res = nlohmann::json::parse(body);
  ASSERT_TRUE(res["success"] == true);
  ASSERT_TRUE(res.contains("all_hands"));
  ASSERT_GE(res["all_hands"].size(), 1u);
  WouldBid east_x("X", "P 1S");
  WouldBid west_p("P", "");
  for (const auto &board : res["all_hands"]) {
    uint64_t eh = ParseHand(board["E"].get<std::string>());
    uint64_t wh = ParseHand(board["W"].get<std::string>());
    EXPECT_TRUE(east_x.MeetsRequirement(eh)) << "East hand: " << board["E"];
    EXPECT_TRUE(west_p.MeetsRequirement(wh)) << "West hand: " << board["W"];
  }
}

TEST(ServerTest, WebServerSolveJsonNineCardHeartsNoFalseFailureNodes) {
  WebServer engine;
  nlohmann::json req = {
      {"hands",
       {{"N", "S 532; H T7; D T32; C 6;"},
        {"E", ""},
        {"S", "S K9; H -; D A854; C AKJ;"},
        {"W", ""}}},
      {"unknown_cards", "S AJTQ64 D KQJ976 C T43Q98"},
      {"trump", "HEARTS"},
      {"leader", "NORTH"},
      {"min_tricks", 6},
      {"num_iters", 200},
      {"num_threads", 16}};

  int status_code = 0;
  std::string body = engine.SolveJson(req.dump(), &status_code);
  ASSERT_EQ(status_code, 200);
  nlohmann::json res = nlohmann::json::parse(body);
  ASSERT_TRUE(res["success"] == true);
  ASSERT_TRUE(res.contains("tree"));

  int min_tricks = res["tricks_won"].get<int>();
  ASSERT_EQ(min_tricks, 6);

  int failure_leaf_boards = 0;
  std::function<void(const nlohmann::json &)> verify_node =
      [&](const nlohmann::json &n) {
        bool is_fail = n.value("failure", false);
        int ns = n.value("ns_tricks", 0);
        std::string lbl = n.value("label", "");
        std::string cond = n.value("condition", "");
        if (is_fail) {
          EXPECT_LT(ns, min_tricks)
              << "Failure node claims N/S won >= target tricks (" << ns
              << " >= " << min_tricks << "): label=" << lbl
              << " cond=" << cond;
          if (n.contains("boards")) {
            failure_leaf_boards += static_cast<int>(n["boards"].size());
          }
        }
        if (n.contains("children")) {
          // Sibling branches must not have contradictory/duplicate conditions
          // where one says "if <seat> plays <CARD>" and a sibling trick node
          // starts with "(<CARD>)".
          std::vector<std::string> fail_play_cards;
          std::vector<std::string> action_lead_cards;
          for (const auto &c : n["children"]) {
            std::string c_cond = c.value("condition", "");
            if (c.value("failure", false) &&
                c_cond.find(" plays ") != std::string::npos) {
              size_t pos = c_cond.rfind(' ');
              if (pos != std::string::npos) {
                fail_play_cards.push_back(c_cond.substr(pos + 1));
              }
            } else if (!c.value("failure", false) &&
                       c_cond.size() >= 4 && c_cond[0] == '(' &&
                       c_cond[3] == ')') {
              action_lead_cards.push_back(c_cond.substr(1, 2));
            }
          }
          for (const auto &fc : fail_play_cards) {
            for (const auto &ac : action_lead_cards) {
              EXPECT_NE(fc, ac)
                  << "Failure condition 'plays " << fc
                  << "' contradicts sibling action condition starting with '("
                  << ac << ")' under node: " << lbl;
            }
          }
          for (const auto &c : n["children"]) {
            verify_node(c);
          }
        }
      };

  verify_node(res["tree"]);
  EXPECT_EQ(failure_leaf_boards, res["failure_boards"].get<int>());

  // Verify Trick 1 children have winner conditions ("if ... wins ...") rather
  // than raw trick strings like "(D_) DA (D_) D3", and "if S wins the CJ" has
  // a single collapsed CK -> CA line without splitting on equivalent spot cards
  // S5/S3/S2.
  const auto &tree_root = res["tree"];
  ASSERT_TRUE(tree_root.contains("children"));
  ASSERT_FALSE(tree_root["children"].empty());
  auto check_t1_node = [](const nlohmann::json &t1) {
    if (!t1.contains("children") || t1["children"].size() <= 1) return;
    for (const auto &branch : t1["children"]) {
      std::string b_cond = branch.value("condition", "");
      EXPECT_NE(b_cond.find("wins"), std::string::npos)
          << "Expected winner condition under " << t1.value("label", "")
          << ", got: " << b_cond;
      if (b_cond == "if S wins the CJ") {
        // Because Trick 2 is collapsed directly into the 'if S wins the CJ'
        // node rather than splitting into S3 vs S5 spot-card variants,
        // branch itself is the #2 trick node (not an '<Card> is led' container).
        std::string b_label = branch.value("label", "");
        EXPECT_EQ(b_label.rfind("#2: ", 0), 0u)
            << "Expected single collapsed #2 node under 'if S wins the CJ'"
            << ", got: " << b_label;
      }
    }
  };
  if (tree_root.value("label", "").rfind("#1: ", 0) == 0) {
    check_t1_node(tree_root);
  } else {
    for (const auto &rc : tree_root["children"]) {
      if (rc.value("label", "").rfind("#1: ", 0) == 0) {
        check_t1_node(rc);
      }
    }
  }
}

TEST(ServerTest,
     CollapseRedundantNodesPreservesConditionWhenChildConditionIsRedundant) {
  std::set<int> b0 = {0};
  std::set<int> b1 = {1};
  ConditionalLine leaf0(b0);
  ConditionalLine leaf1(b1);

  // Under "if E wins the CQ", a container ConditionalLine has a single
  // TrickPlayCondition("(D_) DA (D_) D3") child.
  ConditionalLine trick2_e;
  trick2_e.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("D_", "DA", b0),
      GeneralizedDemiStruct("D_", "D3", b0), b0, /*ldr=*/1));
  trick2_e.AddCondition(new WinnerPlayCondition(2), leaf0);

  ConditionalLine e_wins_container;
  e_wins_container.AddCondition(
      new TrickPlayCondition("(D_) DA (D_) D3"), trick2_e);

  ConditionalLine trick2_s;
  trick2_s.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("__", "CK", b1),
      GeneralizedDemiStruct("C_", "S2", b1), b1, /*ldr=*/2));
  trick2_s.AddCondition(new WinnerPlayCondition(2), leaf1);

  ConditionalLine trick1;
  trick1.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("C_", "C6", {0, 1}),
      GeneralizedDemiStruct("__", "CJ", {0, 1}), {0, 1}, /*ldr=*/0));
  trick1.AddCondition(
      new WinnerPlayCondition(1, std::vector<std::string>{"CQ"}),
      e_wins_container);
  trick1.AddCondition(
      new WinnerPlayCondition(2, std::vector<std::string>{"CJ"}),
      trick2_s);

  int node_id = 0;
  auto tree = BuildTreeFromConditionalLine(trick1, node_id);
  ASSERT_NE(tree, nullptr);
  ASSERT_EQ(tree->children.size(), 2u);
  EXPECT_EQ(tree->children[0]->label, "#2: (D_) DA (D_) D3");
  EXPECT_EQ(tree->children[0]->condition, "if E wins the CQ");
  EXPECT_EQ(tree->children[1]->label, "#2: CK (__) S2 (C_)");
  EXPECT_EQ(tree->children[1]->condition, "if S wins the CJ");
}

TEST(ServerTest, BranchSolveUsesBranchDealCountAsDenominator) {
  WebServer engine;
  nlohmann::json base_req = {
      {"hands",
       {{"N", "S A2; H A2; D -; C -;"},
        {"E", ""},
        {"S", "S 43; H 43; D -; C -;"},
        {"W", ""}}},
      {"unknown_cards", "S KQJT H KQJT"},
      {"trump", "NOTRUMP"},
      {"leader", "SOUTH"},
      {"min_tricks", 2},
      {"num_iters", 20},
      {"num_threads", 4}};

  int status_code = 0;
  std::string base_body = engine.SolveJson(base_req.dump(), &status_code);
  ASSERT_EQ(status_code, 200);
  nlohmann::json base_res = nlohmann::json::parse(base_body);
  ASSERT_TRUE(base_res["success"] == true);
  EXPECT_EQ(base_res["total_boards"].get<int>(), 20);

  // Pick a branch subset of 10 boards out of the 20 deals.
  std::vector<int> branch_ids = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18};
  std::set<int> branch_set(branch_ids.begin(), branch_ids.end());

  nlohmann::json branch_req = {
      {"hands",
       {{"N", "S 2; H A2; D -; C -;"},
        {"E", ""},
        {"S", "S 4; H 43; D -; C -;"},
        {"W", ""}}},
      {"unknown_cards", "S JT H KQJT"},
      {"trump", "NOTRUMP"},
      {"leader", "NORTH"},
      {"min_tricks", 1},
      {"num_iters", 20},
      {"num_threads", 4},
      {"start_trick_num", 2},
      {"initial_ns_tricks", 1},
      {"reuse_deals", true},
      {"replay_tricks", nlohmann::json::array({{"S3", "__", "SA", "__"}})},
      {"branch_board_ids", branch_ids},
      {"cur_trick", {"HA"}}};

  status_code = 0;
  std::string branch_body = engine.SolveJson(branch_req.dump(), &status_code);
  ASSERT_EQ(status_code, 200) << branch_body;
  nlohmann::json branch_res = nlohmann::json::parse(branch_body);
  ASSERT_TRUE(branch_res["success"] == true);
  EXPECT_EQ(branch_res["total_boards"].get<int>(), 10);
  EXPECT_EQ(branch_res["success_boards"].get<int>() +
                branch_res["failure_boards"].get<int>(),
            10);
  ASSERT_TRUE(branch_res.contains("active_board_ids"));
  EXPECT_EQ(branch_res["active_board_ids"].get<std::vector<int>>(), branch_ids);
  ASSERT_TRUE(branch_res.contains("tree"));
  EXPECT_EQ(branch_res["tree"]["total_boards"].get<int>(), 10);

  for (const auto &fh : branch_res["failure_sample_hands"]) {
    int bid = fh["board_id"].get<int>();
    EXPECT_GT(branch_set.count(bid), 0u)
        << "Non-branch board " << bid << " included in failure_sample_hands";
  }
}

TEST(ServerTest, PruneEmptyBranchesClearsSingleSurvivingStateCondition) {
  std::set<int> empty_boards;
  std::set<int> b0 = {0};
  std::set<int> b1 = {1};
  std::set<int> all_boards = {0, 1};

  ConditionalLine leaf_empty(empty_boards);
  ConditionalLine leaf0(b0);
  ConditionalLine leaf1(b1);

  // Empty branch: [if DQ is good] -> #9: DQ (D_) D2 (D_) (0 boards after Step 2)
  ConditionalLine trick9_dq;
  trick9_dq.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("D_", "DQ", empty_boards),
      GeneralizedDemiStruct("D_", "D2", empty_boards), empty_boards,
      /*ldr=*/0));
  trick9_dq.AddCondition(new WinnerPlayCondition(0), leaf_empty);

  // Surviving branch: [if DQ is not good] -> Declarer leads:
  //   [if E discarded on S] #9: SJ (__) D5 (S_) (board 0)
  //   [if E did not discard on S] #9: CK (C_) C9 (C_) (board 1)
  ConditionalLine trick9_sj;
  trick9_sj.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("__", "SJ", b0),
      GeneralizedDemiStruct("S_", "D5", b0), b0, /*ldr=*/0));
  trick9_sj.AddCondition(new WinnerPlayCondition(0), leaf0);

  ConditionalLine trick9_ck;
  trick9_ck.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("C_", "CK", b1),
      GeneralizedDemiStruct("C_", "C9", b1), b1, /*ldr=*/0));
  trick9_ck.AddCondition(new WinnerPlayCondition(0), leaf1);

  ConditionalLine disc_split;
  disc_split.AddCondition(new PlayerDiscardedCondition(1, SPADES, true),
                          trick9_sj);
  disc_split.AddCondition(new PlayerDiscardedCondition(1, SPADES, false),
                          trick9_ck);

  ConditionalLine dq_split;
  CardType dq = Card::GetSuitedCard(DIAMONDS, 12);
  dq_split.AddCondition(new IsCardGoodCondition(dq, true), trick9_dq);
  dq_split.AddCondition(new IsCardGoodCondition(dq, false), disc_split);

  ConditionalLine trick8;
  trick8.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("S_", "S9", all_boards),
      GeneralizedDemiStruct("__", "SA", all_boards), all_boards, /*ldr=*/2));
  trick8.AddCondition(new WinnerPlayCondition(0), dq_split);

  int node_id = 0;
  auto tree = BuildTreeFromConditionalLine(trick8, node_id, /*trick_num=*/8,
                                           /*root_total=*/2);
  ASSERT_NE(tree, nullptr);
  AccumulateFailureCounts(tree);
  PruneEmptyBranches(tree);
  CollapseRedundantNodes(tree);

  // Under #8, the vacuous "[if DQ is not good]" condition should be cleared and
  // collapsed so the single "Declarer leads" node has an empty condition and
  // directly holds the two "[if E discarded on S]" / "[if E did not discard on
  // S]" children.
  ASSERT_EQ(tree->label, "#8: S9 (S_) SA (__)");
  ASSERT_EQ(tree->children.size(), 1u);
  const auto &decl_leads = tree->children[0];
  EXPECT_EQ(decl_leads->type, "condition");
  EXPECT_EQ(decl_leads->label, "Declarer leads");
  EXPECT_EQ(decl_leads->condition, "");
  ASSERT_EQ(decl_leads->children.size(), 2u);
  EXPECT_EQ(decl_leads->children[0]->condition, "if E discarded on S");
  EXPECT_EQ(decl_leads->children[0]->label, "#9: SJ (__) D5 (S_)");
  EXPECT_EQ(decl_leads->children[1]->condition, "if E did not discard on S");
  EXPECT_EQ(decl_leads->children[1]->label, "#9: CK (C_) C9 (C_)");
}

TEST(ServerTest, RotatedDeclarerFrontendSeatMappingAndSolve) {
  int port = 18089;
  WebServer server("127.0.0.1", port);
  ASSERT_TRUE(server.Start(/*async=*/true));
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  httplib::Client client("127.0.0.1", port);
  client.set_keep_alive(false);
  client.set_read_timeout(60, 0);

  auto res_index = client.Get("/");
  ASSERT_TRUE(res_index);
  ASSERT_EQ(res_index->status, 200);
  EXPECT_NE(res_index->body.find("N=Decl · E=LHO · S=Dummy · W=RHO"),
            std::string::npos);
  EXPECT_NE(res_index->body.find("summaryBoundsHand1Header"),
            std::string::npos);
  EXPECT_NE(res_index->body.find("summaryBoundsHand2Header"),
            std::string::npos);
  EXPECT_NE(res_index->body.find("summaryHonorHand1Header"),
            std::string::npos);
  EXPECT_NE(res_index->body.find("summaryHonorHand2Header"),
            std::string::npos);

  auto res_app_js = client.Get("/app.js");
  ASSERT_TRUE(res_app_js);
  ASSERT_EQ(res_app_js->status, 200);
  EXPECT_NE(res_app_js->body.find("function getRotationSeatMap"),
            std::string::npos);
  EXPECT_NE(res_app_js->body.find("function getTableCompassDealHands"),
            std::string::npos);
  EXPECT_NE(res_app_js->body.find("function formatInvariantRuleForRotation"),
            std::string::npos);

  auto res_tabs_js = client.Get("/analysis_tabs.js");
  ASSERT_TRUE(res_tabs_js);
  ASSERT_EQ(res_tabs_js->status, 200);
  EXPECT_NE(res_tabs_js->body.find("getTableCompassDealHands"),
            std::string::npos);
  EXPECT_NE(res_tabs_js->body.find("formatInvariantRuleForRotation"),
            std::string::npos);

  auto res_tree_js = client.Get("/decision_tree.js");
  ASSERT_TRUE(res_tree_js);
  ASSERT_EQ(res_tree_js->status, 200);
  EXPECT_NE(res_tree_js->body.find("getTableCompassDealHands(activeTracedDeal)"),
            std::string::npos);

  // Solve a rotated South-declarer finesse deal (South rotated to N, North to S,
  // West/LHO in solver E/hand 1, East/RHO in solver W/hand 2).
  nlohmann::json solve_req = {
      {"hands",
       {{"N", "S AQ; H -; D -; C -;"},
        {"E", ""},
        {"S", "S 87; H -; D -; C -;"},
        {"W", ""}}},
      {"unknown_cards", "S K432"},
      {"trump", "SPADES"},
      {"leader", "SOUTH"},
      {"min_tricks", 2},
      {"num_iters", 20},
      {"num_threads", 2}};
  auto res_solve =
      client.Post("/api/solve", solve_req.dump(), "application/json");
  ASSERT_TRUE(res_solve);
  ASSERT_EQ(res_solve->status, 200);
  nlohmann::json solve_json = nlohmann::json::parse(res_solve->body);
  ASSERT_TRUE(solve_json["success"] == true);
  ASSERT_TRUE(solve_json.contains("distribution_summary"));
  ASSERT_TRUE(solve_json.contains("sample_hands"));
  ASSERT_FALSE(solve_json["sample_hands"].empty());

  server.Stop();

  // Execute the frontend JS functions in Node (when available) to verify:
  // 1) Declarer seat mapping hints for N, S, E, W
  // 2) Reversible single-dummy N/S rotation in applyDeclarerRotation
  // 3) Distribution Summary headers & single-pass composite rule replacement
  // 4) Un-rotated Sample Hands, Compare Lines cards, and Inspector banner
  if (std::system("command -v node >/dev/null 2>&1") == 0) {
    std::string cmd = R"NODE(node -e '
const fs = require("fs");
const vm = require("vm");

function makeEl(id = "") {
  return {
    id,
    value: "",
    textContent: "",
    _innerHTML: "",
    get innerHTML() { return this._innerHTML; },
    set innerHTML(v) {
      this._innerHTML = String(v);
      if (v === "") this.children = [];
    },
    className: "",
    style: { setProperty() {} },
    dataset: {},
    children: [],
    classList: {
      _set: new Set(),
      add(...c) { c.forEach(x => this._set.add(x)); },
      remove(...c) { c.forEach(x => this._set.delete(x)); },
      toggle(c, force) {
        if (force === undefined) {
          if (this._set.has(c)) this._set.delete(c); else this._set.add(c);
        } else if (force) this._set.add(c); else this._set.delete(c);
      },
      contains(c) { return this._set.has(c); }
    },
    appendChild(ch) { this.children.push(ch); return ch; },
    addEventListener() {},
    setAttribute() {},
    getBoundingClientRect() { return { width: 500, height: 500 }; },
    querySelector(sel) {
      if (sel === "tbody") {
        if (!this._tbody) this._tbody = makeEl("tbody");
        return this._tbody;
      }
      return makeEl(sel);
    }
  };
}

const elements = {};
function getEl(id) {
  if (!elements[id]) elements[id] = makeEl(id);
  return elements[id];
}

const sandbox = {
  console,
  setTimeout,
  clearTimeout,
  setInterval,
  clearInterval,
  performance: { now: () => 0 },
  window: {
    location: { search: "", hash: "" },
    addEventListener() {},
    dispatchEvent() {}
  },
  localStorage: { getItem: () => null, setItem: () => {} },
  URLSearchParams,
  document: {
    body: makeEl("body"),
    getElementById: (id) => getEl(id),
    querySelector: () => makeEl(),
    querySelectorAll: () => [],
    createElement: (tag) => makeEl(tag),
    createElementNS: (ns, tag) => makeEl(tag),
    addEventListener: () => {}
  },
  updateDealSvg: () => {},
  updateProposeLineBarUI: () => {},
  updateLeadStat: () => {},
  parseHandToSuits: (str) => {
    const out = { S: "-", H: "-", D: "-", C: "-" };
    (str || "").split(";").forEach(part => {
      const m = part.trim().match(/^([SHDC])\s+(.+)$/i);
      if (m) out[m[1].toUpperCase()] = m[2].trim();
    });
    return out;
  },
  countHandCards: (str) => {
    const suits = sandbox.parseHandToSuits(str);
    let n = 0;
    ["S", "H", "D", "C"].forEach(s => {
      if (suits[s] && suits[s] !== "-") n += suits[s].length;
    });
    return n;
  },
  getSeatIndex: (s) => ({ N: 0, NORTH: 0, E: 1, EAST: 1, S: 2, SOUTH: 2, W: 3, WEST: 3 }[String(s).toUpperCase()] || 0),
  getSeatName: (idx) => ["N", "E", "S", "W"][((idx % 4) + 4) % 4],
  getTrumpCode: () => "S",
  extractTrickString: () => "",
  tokenizeTrick: () => []
};
vm.createContext(sandbox);

const staticDir = process.cwd().endsWith("/build")
  ? "../web/static"
  : "web/static";
vm.runInContext(fs.readFileSync(staticDir + "/app.js", "utf8"), sandbox);
vm.runInContext(fs.readFileSync(staticDir + "/analysis_tabs.js", "utf8"), sandbox);
vm.runInContext(fs.readFileSync(staticDir + "/decision_tree.js", "utf8"), sandbox);

const assert = (cond, msg) => {
  if (!cond) {
    console.error("Assertion failed:", msg);
    process.exit(1);
  }
};

// 1. Test single-dummy N/S rotation reversibility & idempotence
getEl("inputNorth").value = "S AQ; H -; D -; C -;";
getEl("inputSouth").value = "S 87; H -; D -; C -;";
getEl("inputEast").value = "";
getEl("inputWest").value = "";
sandbox.updateDeclarerBarUI("N");
assert(getEl("declarerMappingHint").textContent === "N=Decl · E=LHO · S=Dummy · W=RHO",
       "N declarer hint");

// Toggle N -> S: swaps N and S
sandbox.applyDeclarerRotation("S", false);
assert(getEl("inputNorth").value === "S 87; H -; D -; C -;", "N->S swap North");
assert(getEl("inputSouth").value === "S AQ; H -; D -; C -;", "N->S swap South");
assert(getEl("declarerMappingHint").textContent === "N=S(Decl) · E=W(LHO) · S=N(Dummy) · W=E(RHO)",
       "S declarer hint");
assert(getEl("modalDeclarerHint").textContent === "N=S(Decl) · E=W(LHO) · S=N(Dummy) · W=E(RHO)",
       "S modal declarer hint");

// Clicking S again is a no-op on single-dummy hands
sandbox.applyDeclarerRotation("S", false);
assert(getEl("inputNorth").value === "S 87; H -; D -; C -;", "S->S idempotent North");

// Toggle S -> N: swaps back
sandbox.applyDeclarerRotation("N", false);
assert(getEl("inputNorth").value === "S AQ; H -; D -; C -;", "S->N restore North");
assert(getEl("inputSouth").value === "S 87; H -; D -; C -;", "S->N restore South");

// 2. Test invariant rule single-pass replacement for N, S, E, W
const compositeRule = "OrInvariant(E has the SK, W has the SQ)";
assert(sandbox.formatInvariantRuleForRotation(compositeRule, "N") ===
       "OrInvariant(E has the SK, W has the SQ)", "Composite rule N");
assert(sandbox.formatInvariantRuleForRotation(compositeRule, "S") ===
       "OrInvariant(W has the SK, E has the SQ)", "Composite rule S no collision");
assert(sandbox.formatInvariantRuleForRotation(compositeRule, "E") ===
       "OrInvariant(S has the SK, N has the SQ)", "Composite rule E");
assert(sandbox.formatInvariantRuleForRotation(compositeRule, "W") ===
       "OrInvariant(N has the SK, S has the SQ)", "Composite rule W");

const lengthRule = "OrInvariant(E length in Spades is always 2, hand 1 length in Hearts is between 1 and 3)";
assert(sandbox.formatInvariantRuleForRotation(lengthRule, "S") ===
       "OrInvariant(W length in Spades is always 2, Hand 1 (West) length in Hearts is between 1 and 3)",
       "Length rule S");
assert(sandbox.formatInvariantRuleForRotation("hand 2 always contains the DA", "S") ===
       "Hand 2 (East) always contains the DA", "Hand 2 rule S");

// 3. Test renderDistributionSummary updates headers and rules for N, S, E, W declarers
const distInput = {
  total_deals: 10,
  invariant_rules: [
    "hand 1 always contains the SK",
    "hand 2 length in Spades is always 2",
    "OrInvariant(E has the HA, W has the HK)"
  ],
  suit_bounds: { S: { min_hand1: 1, max_hand1: 3, min_hand2: 2, max_hand2: 2, min_combined: 3, max_combined: 5 } },
  honor_stats: {}
};

for (const [seat, lhoName, rhoName, lhoShort, rhoShort] of [
  ["N", "East", "West", "E", "W"],
  ["S", "West", "East", "W", "E"],
  ["E", "South", "North", "S", "N"],
  ["W", "North", "South", "N", "S"]
]) {
  sandbox.updateDeclarerBarUI(seat);
  sandbox.renderDistributionSummary(distInput);
  assert(getEl("summaryBoundsHand1Header").textContent === `Hand 1 (${lhoName}) Range`, `Bounds H1 header ${seat}`);
  assert(getEl("summaryBoundsHand2Header").textContent === `Hand 2 (${rhoName}) Range`, `Bounds H2 header ${seat}`);
  assert(getEl("summaryHonorHand1Header").textContent === `Hand 1 (${lhoName}) %`, `Honor H1 header ${seat}`);
  assert(getEl("summaryHonorHand2Header").textContent === `Hand 2 (${rhoName}) %`, `Honor H2 header ${seat}`);
  const renderedRules = getEl("summaryRulesList").children.map(li => li.textContent);
  assert(renderedRules[0] === `Hand 1 (${lhoName}) always contains the SK`, `Rendered rule 0 ${seat}`);
  assert(renderedRules[1] === `Hand 2 (${rhoName}) length in Spades is always 2`, `Rendered rule 1 ${seat}`);
  assert(renderedRules[2] === `OrInvariant(${lhoShort} has the HA, ${rhoShort} has the HK)`, `Rendered rule 2 ${seat}`);
}

// 4. Test getTableCompassDealHands, renderSampleHands, createCompareDealCard, and showNodeInspector for N, S, E, W
const rotDeal = {
  board_id: 0,
  N: "S AK; H -; D -; C -;", // Declarer
  E: "S QJ; H -; D -; C -;", // LHO (hand 1)
  S: "S 32; H -; D -; C -;", // Dummy
  W: "S T9; H -; D -; C -;"  // RHO (hand 2/3)
};

const extractBoxSeatMap = (cardEl) => {
  const out = {};
  const walk = (el) => {
    if (!el) return;
    if (el.className === "sample-hand-box") {
      const seat = el.children[0].textContent;
      const spades = el.children[1].children[1].textContent;
      out[seat] = spades;
    }
    (el.children || []).forEach(walk);
  };
  walk(cardEl);
  return out;
};

for (const [seat, expectedBoxes, expectedBanner] of [
  ["N", { NORTH: "AK", EAST: "QJ", SOUTH: "32", WEST: "T9" }, ["W", "T9", "E", "QJ"]],
  ["S", { SOUTH: "AK", WEST: "QJ", NORTH: "32", EAST: "T9" }, ["W", "QJ", "E", "T9"]],
  ["E", { EAST: "AK", SOUTH: "QJ", WEST: "32", NORTH: "T9" }, ["S", "QJ", "N", "T9"]],
  ["W", { WEST: "AK", NORTH: "QJ", EAST: "32", SOUTH: "T9" }, ["N", "QJ", "S", "T9"]]
]) {
  sandbox.updateDeclarerBarUI(seat);
  sandbox.renderSampleHands([rotDeal]);
  const sampleCardMap = extractBoxSeatMap(getEl("sampleHandsGrid").children[0]);
  for (const [dir, ranks] of Object.entries(expectedBoxes)) {
    assert(sampleCardMap[dir] === ranks, `SampleHands ${seat} ${dir} expected ${ranks}, got ${sampleCardMap[dir]}`);
  }
  const cmpCard = sandbox.createCompareDealCard(rotDeal, 0, 0);
  const cmpCardMap = extractBoxSeatMap(cmpCard);
  for (const [dir, ranks] of Object.entries(expectedBoxes)) {
    assert(cmpCardMap[dir] === ranks, `CompareCard ${seat} ${dir} expected ${ranks}, got ${cmpCardMap[dir]}`);
  }
  vm.runInContext("activeTracedBoardId = 0; activeTracedDeal = " + JSON.stringify(rotDeal) + ";", sandbox);
  sandbox.showNodeInspector({ label: "Leaf", boards: [0] });
  const bannerHtml = getEl("inspectorContent").children[0].innerHTML;
  assert(bannerHtml.includes(`<strong>${expectedBanner[0]}:</strong> <span class="card-suit-s">♠</span>${expectedBanner[1]}`),
         `Inspector banner ${seat} first defender: ${bannerHtml}`);
  assert(bannerHtml.includes(`<strong>${expectedBanner[2]}:</strong> <span class="card-suit-s">♠</span>${expectedBanner[3]}`),
         `Inspector banner ${seat} second defender: ${bannerHtml}`);
}
' )NODE";
    EXPECT_EQ(std::system(cmd.c_str()), 0);
  }
}


