#!/bin/bash
set -euo pipefail
TARGET="$HOME/Library/Audio/Plug-Ins/Components/SID Lab.component"
[[ -e "$TARGET" ]] || { echo 'SID Lab is not installed in the user Components folder.'; exit 0; }
ID="$(/usr/libexec/PlistBuddy -c Print:CFBundleIdentifier "$TARGET/Contents/Info.plist" 2>/dev/null || true)"
[[ "$ID" == org.sidlab.instrument ]] || { echo 'Refusing removal: bundle is not owned by SID Lab.' >&2; exit 1; }
rm -rf "$TARGET"
echo 'Removed only the SID Lab component. Project sources and saved presets are preserved.'
