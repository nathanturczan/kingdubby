#!/bin/bash
# Builds and runs the offline DSP regression test (no JUCE, no DAW).
set -euo pipefail

cd "$(dirname "$0")/.."
OUT="$(mktemp -d)/dsp_regression"

clang++ -std=c++17 -O2 -Wall -Wextra -o "$OUT" Tests/dsp_regression.cpp Source/DubDelay.cpp
"$OUT"
