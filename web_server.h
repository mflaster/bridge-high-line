#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "third_party/httplib.h"
#include "third_party/nlohmann/json.hpp"

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

  std::string host_;
  int port_;
  httplib::Server svr_;
  std::unique_ptr<std::thread> server_thread_;
  std::atomic<bool> running_{false};
};
