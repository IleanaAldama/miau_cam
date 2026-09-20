#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
source "${EMSDK:-$HOME/emsdk}/emsdk_env.sh" > /dev/null
emcmake cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
