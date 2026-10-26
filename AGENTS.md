# Developer & Agent Guidelines

## 1. Prerequisites

Before building or testing the codebase, ensure the following tools and packages are installed:

- **CMake**: Version 3.20 or higher
- **C++ Compiler**: C++17 compliant compiler (e.g., `g++-13` or `clang++`)
- **Build Tool**: `make` or `ninja`
- **Python 3**: Required for build configuration script steps (generating `sayc_generated.h` and `web_assets.h`)
- **Node.js**: Required for WebAssembly smoke testing
- **Emscripten SDK (emsdk)**: Required for WebAssembly compilation (`bridge_wasm`)

> **Note on Script Generation:** Ensure `python3` and `emsdk` are installed before building WebAssembly targets to avoid script errors during code and header generation (`sayc_generated.h`, `web_assets.h`).

---

## 2. Native C++ Build and Test Instructions

### Building Native Targets

To configure and compile all native C++ build targets:

```bash
cmake -B build
cmake --build build -j4
```

Or use the default `make` convenience target:

```bash
make
```

### Running Test Suites

Execute all unit tests using CTest:

```bash
ctest --test-dir build --output-on-failure
```

Or via the convenience `make` target:

```bash
make run_all_tests
```

### Running Individual Tests

Individual test executables can be run directly from the `build` directory or via specific `make` targets:

- `./build/basic_test` (or `make run_basic_test`)
- `./build/board_state_test` (or `make run_board_state_test`)
- `./build/conditional_line_test` (or `make run_conditional_line_test`)
- `./build/dds_test` (or `make run_dds_test`)
- `./build/deal_test` (or `make run_deal_test`)
- `./build/hand_requirements_test` (or `make run_requirements_test`)
- `./build/interface_dds_test` (or `make run_interface_dds_test`)
- `./build/line_finder_test` (or `make run_line_finder_test`)
- `./build/server_test` (or `make run_server_test`)
- `./build/cross_hand_summarizer_test` (or `make run_cross_hand_summarizer_test`)
- `./build/search_progress_test`
- `./build/string_util_test`

---

## 3. Web Server Execution

To run the local C++ HTTP web server (`bridge_server`):

```bash
./build/bridge_server
```

Or via the `make` shortcut:

```bash
make run_server
```

By default, the server listens at `http://0.0.0.0:8080`. Additional CLI flags can be passed, such as `-p` / `--port` to change the listening port or `-h` / `--host` to specify the binding address.

---

## 4. WebAssembly Build and Smoke Testing

### Setting Up Emscripten SDK

Ensure Emscripten environment variables are loaded in your shell:

```bash
source /path/to/emsdk/emsdk_env.sh
```

### Compiling WebAssembly Binary (`bridge_wasm`)

Configure and build the WebAssembly target into `build-wasm`:

```bash
emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm -j4
```

### Running WebAssembly Smoke Test

Run the Node.js smoke test script to verify that `build-wasm/bridge_wasm.js` compiles and solves deals correctly:

```bash
node -e '(() => {
  const fs = require("fs");
  const path = require("path");
  const vm = require("vm");
  let ran = false;
  process.on("exit", () => {
    if (!ran) {
      console.error("Wasm smoke test did not run!");
      process.exitCode = 1;
    }
  });
  const wasmJs = path.resolve("build-wasm/bridge_wasm.js");
  global.require = require;
  global.__filename = wasmJs;
  global.__dirname = path.dirname(wasmJs);
  global.self = global;
  global.Module = {
    locateFile: (p) => path.resolve("build-wasm", p),
    onRuntimeInitialized() {
      setImmediate(() => {
        const req1 = JSON.stringify({
          hands: {
            N: "S AQ; H -; D -; C -;",
            E: "S 54; H -; D -; C -;",
            S: "S 87; H -; D -; C -;",
            W: "S KJ; H -; D -; C -;"
          },
          trump: "SPADES",
          leader: "NORTH",
          min_tricks: 1,
          num_iters: 4,
          num_threads: 4
        });
        const out1 = JSON.parse(global.Module.ccall(
          "wasm_solve_json", "string", ["string"], [req1]
        ));
        if (!out1.success || out1.tricks_won !== 1 || out1.best_lead !== "SA") {
          console.error("Unexpected Wasm solve output:", out1);
          process.exit(1);
        }
        ran = true;
        console.log("Wasm smoke test passed:", out1.best_lead);
        process.exit(0);
      });
    }
  };
  vm.runInThisContext(fs.readFileSync(wasmJs, "utf8"), { filename: wasmJs });
})()'
```

---

## 5. Web Frontend Change Policy

Any time you change the web frontend, you must include screenshots in your PR description.
