#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
CMAKE="${CMAKE:-cmake}"
"$CMAKE" -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
"$CMAKE" --build build --config Release --parallel 4
"$CMAKE" --build build --target test --config Release
bash scripts/package-macos.sh
