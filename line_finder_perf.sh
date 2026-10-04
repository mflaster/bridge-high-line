#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

cmake -B build
cmake --build build --target line_finder_perf -j4

./build/line_finder_perf | tee line_finder_perf.log
