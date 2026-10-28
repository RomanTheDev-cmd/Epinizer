#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
SOURCE="build-macos/Epinizer_artefacts/Release/AU/Epinizer.component"
[[ -d "$SOURCE" ]] || { echo 'Build the AU first.'; exit 1; }
mkdir -p dist
STAGE="$(mktemp -d /private/tmp/epinizer-package.XXXXXX)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/Epinizer-macOS" "$STAGE/pkg/Library/Audio/Plug-Ins/Components"
ditto --norsrc --noextattr "$SOURCE" "$STAGE/Epinizer-macOS/Epinizer.component"
codesign --force --sign - "$STAGE/Epinizer-macOS/Epinizer.component"
codesign --verify --strict "$STAGE/Epinizer-macOS/Epinizer.component"
cp scripts/install-macos.command "$STAGE/Epinizer-macOS/Install Epinizer.command"
cp README.md VALIDATION.md CHANGELOG.md THIRD_PARTY.md "План следующей версии.md" "$STAGE/Epinizer-macOS/"
mkdir -p "$STAGE/Epinizer-macOS/Assets"
cp Assets/README.md "$STAGE/Epinizer-macOS/Assets/"
ditto --norsrc --noextattr "$STAGE/Epinizer-macOS/Epinizer.component" "$STAGE/pkg/Library/Audio/Plug-Ins/Components/Epinizer.component"
# Avoid the ownership-mapping diagnostics on this macOS; set archive ownership explicitly.
COPYFILE_DISABLE=1 pkgbuild --ownership preserve --root "$STAGE/pkg" --identifier audio.epinizer.effect --version 2.1.0 --install-location / "$STAGE/preserved.pkg"
pkgutil --expand "$STAGE/preserved.pkg" "$STAGE/expanded"
python3 scripts/root-pkg-ownership.py "$STAGE/expanded"
pkgutil --flatten "$STAGE/expanded" "$STAGE/final.pkg"
mv "$STAGE/final.pkg" dist/Epinizer-2.1.0-macOS.pkg
# The installer is unsigned. The AU carries a local ad-hoc signature, not a Developer ID signature.
ditto --norsrc --noextattr -c -k --keepParent "$STAGE/Epinizer-macOS" dist/Epinizer-2.1.0-macOS.zip
shasum -a 256 dist/Epinizer-2.1.0-macOS.pkg dist/Epinizer-2.1.0-macOS.zip > dist/SHA256SUMS.txt
