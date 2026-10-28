#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
SOURCE="$PWD/Epinizer.component"
if [[ ! -d "$SOURCE" ]]; then
  SOURCE="$PWD/../build-macos/Epinizer_artefacts/Release/AU/Epinizer.component"
fi
[[ -d "$SOURCE" ]] || { echo 'Epinizer.component not found. Build the AU first.'; exit 1; }
DEST="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$DEST"
if [[ -e "$DEST/Epinizer.component" ]]; then
  BACKUP="$HOME/Library/Application Support/Epinizer/Backups/$(date +%Y%m%d-%H%M%S)"
  mkdir -p "$BACKUP"
  mv "$DEST/Epinizer.component" "$BACKUP/"
fi
/usr/bin/ditto --norsrc --noextattr "$SOURCE" "$DEST/Epinizer.component"
echo 'Epinizer AU installed. Restart your DAW and rescan Audio Units if needed.'
