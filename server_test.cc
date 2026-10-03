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

  // Test 2: Invalid JSON payload handling
  auto res_bad = client.Post("/api/solve", "{invalid json}", "application/json");
  REQUIRE(res_bad);
  REQUIRE(res_bad->status == 400);

  server.Stop();
}
