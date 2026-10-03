# Bridge Solver & Line Finder

A C++17 library and test suite for double-dummy bridge analysis, deal generation, line finding, and integration with the Double Dummy Solver (DDS) engine.

## Features & Third-Party Dependencies

All external dependencies are automatically fetched, configured, and built via CMake `FetchContent`. No manual pre-installation of libraries is required:

- **Double Dummy Solver (`dds`)**: v2.9.0 static library build with OpenMP support.
- **Catch2**: v3.5.2 test framework.
- **glog**: Google Logging library (v0.6.0).
- **Boost**: C++ Header libraries (v1.83.0).

## Prerequisites

- **CMake**: 3.20 or newer
- **C++ Compiler**: C++17 support (e.g. GCC 8+, Clang 7+, MSVC 2019+)
- **OpenMP**: C++ OpenMP runtime (installed with standard GCC/Clang development toolchains)

## How to Build

### Using CMake (Linux & macOS)

```bash
# Configure the build directory
cmake -B build

# Build all libraries and executables
cmake --build build -j4
```

### Windows 10 Build Instructions

There are three primary ways to build and run the project on Windows 10:

#### Option 1: Visual Studio / MSVC (Recommended Native Build)
1. Install **Visual Studio 2019** or **Visual Studio 2022** with the **"Desktop development with C++"** workload.
2. Open **Developer Command Prompt for VS** (or PowerShell/CMD) and navigate to the project directory:
   ```cmd
   # Configure using the Visual Studio generator
   cmake -B build -G "Visual Studio 17 2022"

   # Build Release binaries
   cmake --build build --config Release -j4

   # Run test suite
   ctest --test-dir build -C Release --output-on-failure
   ```
   *Note: All C++ dependencies (Catch2, glog, Boost headers, DDS) are automatically downloaded and built via CMake `FetchContent`.*

#### Option 2: MSYS2 / MinGW-w64 (`g++`)
1. Install [MSYS2](https://www.msys2.org/) and launch the **MINGW64** shell.
2. Install build dependencies:
   ```bash
   pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake make
   ```
3. Build and test using standard CMake or Makefile commands:
   ```bash
   cmake -B build
   cmake --build build -j4
   ctest --test-dir build --output-on-failure
   ```

#### Option 3: WSL2 (Windows Subsystem for Linux)
1. Install Ubuntu via WSL2 (`wsl --install`).
2. Build and test following standard Linux instructions (`cmake -B build && cmake --build build` or `make run_all_tests`).

### Using Makefile (Linux / macOS Convenience Wrapper)

A legacy `Makefile` is provided as a thin wrapper over CMake:

```bash
# Build all targets
make

# Clean build directory
make clean
```

## Important Build Targets

### Core Library
- **`bridge_lib`**: Static library containing card representations, hand requirements, deal generators/parsers, solver logic, and DDS interface routines.

### Test Executables
- **`basic_test`**: Card representation, hand evaluation, and basic solver behavior.
- **`board_state_test`**: Board state management, move validation, and PBN parsing.
- **`conditional_line_test`**: Conditional line of play evaluation.
- **`dds_test`**: Low-level integration tests with the Double Dummy Solver.
- **`deal_test`**: Hand dealing algorithms, card masking, and key-value deal file parsing.
- **`hand_requirements_test`**: High card points, suit length, and hand shape constraint checkers.
- **`interface_dds_test`**: Higher-level DDS C++ wrapper tests and play tree evaluations.
- **`line_finder_test`**: Line-finding searches across simulated deals.

### Utility Executables
- **`test_play`**: Executable for testing interactive play lines.

## Running Tests

### Running the Entire Test Suite

Using `ctest`:
```bash
ctest --test-dir build --output-on-failure
```

Or using the Makefile wrapper:
```bash
make run_all_tests
```

### Running Individual Tests

You can run individual Catch2 test binaries directly from `build/` with optional Catch2 flags:

```bash
# Run deal tests
./build/deal_test

# Run basic tests with compact reporter
./build/basic_test --reporter compact --success -d yes

# Skip slow tests in line_finder_test using Catch2 tags
./build/line_finder_test "~[.slow]"
```

Or run via Makefile convenience targets:

```bash
make run_basic_test
make run_requirements_test
make run_deal_test
make run_dds_test
make run_interface_dds_test
make run_line_finder_test
make run_board_state_test
make run_conditional_line_test
```

## Plain-Text Deal File Format

Deal configurations can be loaded from key-value text files (such as `test_data/sample.deal`):

```text
# Sample deal configuration
north  = S AKQ2 H JT9 D 876 C 543
east   = S JT9 H 8762 D 543 C AKQ
south  = S 876 H AKQ D AKQ2 C 876
west   = S 543 H 543 D JT9 C JT92
trump  = HEARTS
leader = NORTH
```
