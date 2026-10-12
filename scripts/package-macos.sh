#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
SOURCE="build/Epinizer_artefacts/Release/AU/Epinizer.component"
[[ -d "$SOURCE" ]] || { echo 'Build the AU first.'; exit 1; }
codesign --force --sign - "$SOURCE"
codesign --verify --strict "$SOURCE"
mkdir -p dist
STAGE="$(mktemp -d /private/tmp/epinizer-package.XXXXXX)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/Epinizer-macOS" "$STAGE/pkg/Library/Audio/Plug-Ins/Components"
ditto --norsrc --noextattr "$SOURCE" "$STAGE/Epinizer-macOS/Epinizer.component"
cp scripts/install-macos.command "$STAGE/Epinizer-macOS/Install Epinizer.command"
cp README.md VALIDATION.md CHANGELOG.md THIRD_PARTY.md "$STAGE/Epinizer-macOS/"
ditto --norsrc --noextattr "$SOURCE" "$STAGE/pkg/Library/Audio/Plug-Ins/Components/Epinizer.component"
COPYFILE_DISABLE=1 pkgbuild --root "$STAGE/pkg" --identifier audio.epinizer.effect --version 2.0.0 --install-location / dist/Epinizer-2.0.0-macOS.pkg
# The installer is unsigned. The AU carries a local ad-hoc signature, not a Developer ID signature.
ditto --norsrc --noextattr -c -k --keepParent "$STAGE/Epinizer-macOS" dist/Epinizer-2.0.0-macOS.zip
shasum -a 256 dist/Epinizer-2.0.0-macOS.pkg dist/Epinizer-2.0.0-macOS.zip > dist/SHA256SUMS.txt
