#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
TARGET="$HOME/Library/Audio/Plug-Ins/Components/SID Lab.component"
[[ -d "$TARGET" ]] || { echo 'Install first: ./scripts/install.sh' >&2; exit 1; }
mkdir -p evidence
file "$TARGET/Contents/MacOS/SID Lab" | tee evidence/architecture.log
codesign --verify --strict --verbose=2 "$TARGET" 2>&1 | tee evidence/signature.log
auval -a > evidence/au-list.log 2>&1
if ! /usr/bin/grep -q 'Sidl.*Sdlb' evidence/au-list.log; then
    echo 'SID Lab is not registered yet. Quit/reopen GarageBand or log out/in, then retry. No global cache was changed.' >&2
    exit 1
fi
auval -v aumu Sidl Sdlb 2>&1 | tee evidence/auval.log
