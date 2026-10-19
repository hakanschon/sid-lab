#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="$ROOT/build/SIDLab_artefacts/Release/AU/SID Lab.component"
TARGET="$HOME/Library/Audio/Plug-Ins/Components/SID Lab.component"
ID='org.sidlab.instrument'
[[ -d "$SOURCE" ]] || { echo 'Build first: ./scripts/build.sh' >&2; exit 1; }
source_id="$(/usr/libexec/PlistBuddy -c Print:CFBundleIdentifier "$SOURCE/Contents/Info.plist")"
[[ "$source_id" == "$ID" ]] || { echo 'Refusing installation: source bundle identity mismatch.' >&2; exit 1; }
if [[ -e "$TARGET" ]]; then
    target_id="$(/usr/libexec/PlistBuddy -c Print:CFBundleIdentifier "$TARGET/Contents/Info.plist" 2>/dev/null || true)"
    [[ "$target_id" == "$ID" ]] || { echo "Refusing to overwrite an unrelated component at $TARGET" >&2; exit 1; }
fi
mkdir -p "$(dirname "$TARGET")"
STAGING="$(dirname "$TARGET")/.SIDLab-install-$$.component"
BACKUP="$(dirname "$TARGET")/.SIDLab-previous-$$.component"
cleanup(){ rm -rf "$STAGING"; if [[ -e "$BACKUP" && ! -e "$TARGET" ]]; then mv "$BACKUP" "$TARGET"; fi; }
trap cleanup EXIT
ditto --norsrc "$SOURCE" "$STAGING"
xattr -dr com.apple.FinderInfo "$STAGING" 2>/dev/null || true
xattr -dr com.apple.ResourceFork "$STAGING" 2>/dev/null || true
codesign --force --sign - --timestamp=none "$STAGING"
codesign --verify --strict "$STAGING"
if [[ -e "$TARGET" ]]; then mv "$TARGET" "$BACKUP"; fi
mv "$STAGING" "$TARGET"
rm -rf "$BACKUP"
echo "Installed $TARGET"
echo 'Quit and reopen GarageBand if it was open. Then run ./scripts/validate.sh.'
