# Bridge Web Server & Interactive Single-Page Application (SPA)

This document provides complete instructions on how to build, run, and use the embedded C++ Web Server (`bridge_server`) and interactive Single-Page Application (SPA) UI.

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Building the Web Server](#building-the-web-server)
4. [Running the Web Server](#running-the-web-server)
5. [Web Interface User Guide](#web-interface-user-guide)
   - [Interactive Deal Setup & Compass View](#interactive-deal-setup--compass-view)
   - [Contract & Trump Controls](#contract--trump-controls)
   - [Quick-Load Deal Presets](#quick-load-deal-presets)
   - [Hand Requirements & Fuzzy Deal Constraints](#hand-requirements--fuzzy-deal-constraints)
   - [Interactive Decision Tree Explorer](#interactive-decision-tree-explorer)
6. [REST API Reference](#rest-api-reference)
   - [`GET /api/health`](#get-apihealth)
   - [`POST /api/solve`](#post-apisolve)

---

## Overview

The `bridge_server` executable embeds a lightweight HTTP microserver that delivers an interactive Single-Page Application (SPA) directly from memory. It allows users to:
- Enter and visualize bridge deal card distributions across all four hands (North, East, South, West) using standard bridge card notation.
- Select trump suits, lead positions, and target trick counts.
- Specify High Card Point (HCP) ranges and suit length constraints for partial or fuzzy deal solving.
- Compute double-dummy optimal play lines using the Double Dummy Solver (DDS) engine.
- Explore multi-trick optimal decision trees with expandable/collapsible nodes, zoom (+/-), pan (drag & drop), and board coverage analytics.

---

## Architecture

- **Zero Web Runtime Dependencies**: Built entirely in C++17. All HTML, CSS, JavaScript, and SVG assets are compiled directly into the binary header (`web_assets.h`), requiring no Node.js, npm, or external static file servers at runtime.
- **Microserver Backend**: Utilizes embedded `cpp-httplib` and `nlohmann/json` under `third_party/` to handle HTTP requests and JSON REST API endpoints asynchronously or synchronously.
- **Interactive SVG Renderer**: Uses native browser SVG rendering for high-performance rendering of decision trees and deal cards without heavy external JS framework dependencies.

---

## Building the Web Server

### Prerequisites
- **CMake**: 3.20 or newer
- **C++ Compiler**: C++17 support (GCC 8+, Clang 7+, MSVC 2019+)
- **OpenMP**: Standard C++ OpenMP runtime

### Using CMake (Linux & macOS)

```bash
# Configure the build directory (if not already configured)
cmake -B build

# Build the bridge_server binary
cmake --build build --target bridge_server -j4
```

### Using Makefile (Linux / macOS Convenience Wrapper)

```bash
# Build all targets including bridge_server
make

# Or build bridge_server explicitly
make bridge_server
```

### Windows 10 / 11 Build Instructions

#### Option 1: Visual Studio / MSVC (Developer Command Prompt)
```cmd
# Configure using the Visual Studio generator
cmake -B build -G "Visual Studio 17 2022"

# Build bridge_server in Release mode
cmake --build build --config Release --target bridge_server -j4
```

#### Option 2: MSYS2 / MinGW-w64
```bash
cmake -B build
cmake --build build --target bridge_server -j4
```

#### Option 3: WSL2 (Windows Subsystem for Linux)
```bash
cmake -B build && cmake --build build --target bridge_server -j4
```

---

## Running the Web Server

Once built, the binary is located at `./build/bridge_server` (or `./build/Release/bridge_server.exe` on Windows Visual Studio builds).

### Command-Line Usage

```bash
./build/bridge_server [OPTIONS]
```

#### Options:
| Flag | Long Flag | Description | Default |
| --- | --- | --- | --- |
| `-p` | `--port` | HTTP listening port number | `8080` |
| `-h` | `--host` | HTTP host interface IP address | `0.0.0.0` |
| | `--help` | Display usage information and exit | N/A |

### Example Commands

Start server on default port `8080`:
```bash
./build/bridge_server
```

Start server on custom port `9090` listening only on localhost:
```bash
./build/bridge_server -p 9090 -h 127.0.0.1
```

Using Makefile convenience target:
```bash
make run_server
```

When started, the server outputs confirmation to stdout:
```text
=========================================================
 Bridge Deal Solver & Interactive Decision Tree Server
=========================================================
 Server listening at: http://localhost:8080
 Press Ctrl+C to stop.
=========================================================
```

---

## Web Interface User Guide

Open your web browser and navigate to:
```text
http://localhost:8080
```

### Interactive Deal Setup & Compass View

The deal setup panel features a standard four-compass direction layout (**North**, **East**, **South**, **West**):

1. **Card Input Fields**: Enter card distributions for each hand using standard suit rank notation:
   - Example: `S AKQ2 H JT9 D 876 C 543`
   - Suits: `S` (Spades ♠), `H` (Hearts ♥), `D` (Diamonds ♦), `C` (Clubs ♣).
   - Ranks: `A`, `K`, `Q`, `J`, `T` (10), `9` down to `2`.
   - Cards within a suit can be separated by spaces or concatenated (e.g. `S AKQ2` or `S A K Q 2`).
2. **Real-time Card Display**: As cards are entered, suit symbols are color-coded (Spades/Clubs in black, Hearts/Diamonds in red).

### Contract & Trump Controls

- **Trump Suit**: Select the contract trump suit (**Notrump**, **Spades ♠**, **Hearts ♥**, **Diamonds ♦**, **Clubs ♣**).
- **Leader**: Select the opening lead hand (**North**, **East**, **South**, **West**).
- **Target Tricks Slider**: Adjust the minimum trick target (1 to 13) for line finding and analysis.

### Quick-Load Deal Presets

Click any preset button at the top of the deal setup panel to quickly populate test hands:
- **4-Card Endplay**: A classic 4-card endplay testing squeeze and trump elimination lines.
- **Full 13-Card Deal**: A complete 52-card deal configuration.
- **Partial Hand**: A 3-card or 5-card partial deal to test short-card double-dummy line combinations.

### Hand Requirements & Fuzzy Deal Constraints

When dealing with unknown or partially specified cards (fewer than 52 total known cards):
- Expand the **Hand Requirements** section.
- **HCP Range**: Specify minimum and maximum High Card Points (`hcp_low`, `hcp_high`) expected for each defender hand.
- **Suit Minimums**: Specify minimum expected length for specific suits.
- The solver uses these constraints during Monte Carlo line-finding simulation to evaluate success probabilities across matching candidate boards.

### Interactive Decision Tree Explorer

Click **Solve Deal** to execute the double-dummy solver. The **Decision Tree Explorer** will render the decision tree:

1. **Multi-Trick Lines**:
   - For full or partial deal solves, optimal play sequences are structured in a multi-level hierarchy:
     `Trick 1` $\rightarrow$ `Trick 2` $\rightarrow$ `Trick 3` $\rightarrow$ `Trick 4` $\rightarrow$ `Contract Outcome`.
2. **Expanding & Collapsing Nodes**:
   - Sub-tricks are collapsed by default (`depth >= 1`) to keep complex decision trees neat.
   - Click any trick node to expand its child tricks going down.
   - Use the **Expand All** and **Collapse All** toolbar buttons to toggle the entire tree state.
3. **Interactive Viewport Controls**:
   - **Zoom In (`+`) / Zoom Out (`-`)**: Adjust the tree zoom level. You can also use mouse wheel scrolling over the canvas.
   - **Pan (Drag & Drop)**: Click and drag anywhere on the tree canvas to move the view.
   - **Reset Viewport**: Restores the default centering and zoom level.
4. **Summary & Analytics**:
   - Displays maximum tricks won for declarer, best opening lead card, execution time (in milliseconds), and total boards covered.

---

## REST API Reference

The server exposes REST JSON endpoints for programmatic integration.

### `GET /api/health`

Checks server health status.

#### Response Example
```json
{
  "service": "bridge_solver_http_server",
  "status": "ok"
}
```

#### Example Request (`curl`)
```bash
curl http://localhost:8080/api/health
```

---

### `POST /api/solve`

Solves a deal configuration and generates the interactive decision tree structure.

#### Request Body Schema (JSON)

| Field | Type | Description |
| --- | --- | --- |
| `hands` | `object` | Object with keys `"N"`, `"E"`, `"S"`, `"W"` containing card string notation. |
| `trump` | `string` | Trump suit: `"NOTRUMP"`, `"SPADES"`, `"HEARTS"`, `"DIAMONDS"`, `"CLUBS"`. |
| `leader` | `string` or `int` | Opening lead direction: `"NORTH"`, `"EAST"`, `"SOUTH"`, `"WEST"` (or `0`..`3`). |
| `min_tricks` | `int` | Minimum target tricks (default: `1`). |
| `num_iters` | `int` | Simulation iterations for fuzzy/partial solves (default: `10`). |
| `requirements` | `object` | (Optional) Hand requirement constraints object per player. |

#### Example Request Body
```json
{
  "hands": {
    "N": "S A4 H A D A C A",
    "E": "S K H K D K C K",
    "S": "S Q H Q D Q C Q",
    "W": "S J H J D J C J"
  },
  "trump": "NOTRUMP",
  "leader": "NORTH",
  "min_tricks": 1
}
```

#### Example Request (`curl`)
```bash
curl -X POST http://localhost:8080/api/solve \
  -H "Content-Type: application/json" \
  -d '{
    "hands": {
      "N": "S A4 H A D A C A",
      "E": "S K H K D K C K",
      "S": "S Q H Q D Q C Q",
      "W": "S J H J D J C J"
    },
    "trump": "NOTRUMP",
    "leader": "NORTH"
  }'
```

#### Response Body Schema (JSON)
```json
{
  "success": true,
  "tricks_won": 4,
  "best_lead": "SA",
  "sequence": "SA SK SQ SJ / HA HK HQ HJ / DA DK DQ DJ / CA CK CQ CJ",
  "covered_boards": 1,
  "elapsed_ms": 1.42,
  "tree": {
    "id": "node_1",
    "type": "action",
    "label": "Optimal Lines (1 variant)",
    "collapsed": false,
    "children": [
      {
        "id": "node_2",
        "type": "action",
        "label": "Trick 1: SA SK SQ SJ",
        "trick": "SA",
        "collapsed": false,
        "children": [
          {
            "id": "node_3",
            "type": "action",
            "label": "Trick 2: HA HK HQ HJ",
            "trick": "HA",
            "collapsed": true,
            "children": [ ... ]
          }
        ]
      }
    ]
  }
}
```
