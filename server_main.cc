#include <iostream>
#include <string>
#include <cstdlib>

#include "glog/logging.h"
#include "web_server.h"

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  std::string host = "0.0.0.0";
  int port = 8080;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    } else if ((arg == "-h" || arg == "--host") && i + 1 < argc) {
      host = argv[++i];
    } else if (arg == "--help") {
      std::cout << "Usage: " << argv[0] << " [-p PORT] [-h HOST]\n"
                << "Starts embedded Bridge SPA HTTP server.\n"
                << "  -p, --port   Port number (default: 8080)\n"
                << "  -h, --host   Host address (default: 0.0.0.0)\n";
      return 0;
    }
  }

  std::cout << "=========================================================\n"
            << " Bridge Deal Solver & Interactive Decision Tree Server\n"
            << "=========================================================\n"
            << " Server listening at: http://localhost:" << port << "\n"
            << " Press Ctrl+C to stop.\n"
            << "=========================================================\n";

  WebServer server(host, port);
  if (!server.Start(/*async=*/false)) {
    std::cerr << "Failed to start HTTP server on " << host << ":" << port << "\n";
    return 1;
  }

  return 0;
}
