#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$REPO_DIR"

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target line_finder_perf -j4

./build/line_finder_perf "$@"
