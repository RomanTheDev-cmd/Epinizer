#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${EPINIZER_LLVM_ROOT:?Set EPINIZER_LLVM_ROOT to LLVM with clang-cl and lld-link}"
: "${EPINIZER_WINSDK:?Set EPINIZER_WINSDK to the xwin splat output}"
CMAKE="${CMAKE:-cmake}"
"$CMAKE" -S . -B build-windows-v2-msvc -DCMAKE_TOOLCHAIN_FILE=cmake/windows-clang-msvc.cmake -DCMAKE_BUILD_TYPE=Release
"$CMAKE" --build build-windows-v2-msvc --parallel 4
if [[ -n "${EPINIZER_WINE:-}" ]]; then
  mkdir -p build-windows-v2-msvc/Epinizer_artefacts/Release/VST3/Epinizer.vst3/Contents/Resources
  "$EPINIZER_WINE" build-windows-v2-msvc/Epinizer_vst3_helper.exe > build-windows-v2-msvc/Epinizer_artefacts/Release/VST3/Epinizer.vst3/Contents/Resources/moduleinfo.json
fi
python3 scripts/package-windows-cross.py
