#include <string>

#include "line_finder.h"
#include "omp_compat.h"
#include "web_server.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>

EM_JS(void, js_notify_progress, (const char *status_ptr, double progress), {
  if (typeof self !== 'undefined' &&
      typeof self.onBridgeSolverProgress === 'function') {
    self.onBridgeSolverProgress(UTF8ToString(status_ptr), progress);
  }
});
#else
#define EMSCRIPTEN_KEEPALIVE
inline void js_notify_progress(const char *, double) {}
#endif

namespace {

WebServer &GetSolverEngine() {
  static WebServer *engine = []() {
    LineFinder::SetProgressCallback(
        [](const std::string &status, double progress) {
          if (omp_get_thread_num() == 0) {
            js_notify_progress(status.c_str(), progress);
          }
        });
    return new WebServer();
  }();
  return *engine;
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE
const char *wasm_solve_json(const char *req_json) {
  static std::string last_response;
  last_response =
      GetSolverEngine().SolveJson(req_json != nullptr ? req_json : "{}");
  return last_response.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char *wasm_get_solve_status() {
  static std::string last_status;
  last_status = GetSolverEngine().GetSolveStatusJson();
  return last_status.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char *wasm_stop_solve() {
  static std::string last_stop;
  last_stop = GetSolverEngine().StopSolveJson();
  return last_stop.c_str();
}

}  // extern "C"
