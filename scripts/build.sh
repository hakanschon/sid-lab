#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
[[ "$(uname -m)" == arm64 ]] || { echo 'SID Lab requires a native arm64 shell on Apple Silicon.' >&2; exit 1; }
for tool in cmake python3 xcrun codesign; do command -v "$tool" >/dev/null || { echo "Missing $tool. See BUILDING.md, then retry." >&2; exit 1; }; done
xcrun --find clang++ >/dev/null
if [[ -e .git && ( ! -e third_party/JUCE/.git || ! -e third_party/libresidfp/.git ) ]]; then
    git submodule update --init --recursive || { echo 'Cannot fetch pinned dependencies. Check GitHub/network access and retry git submodule update --init --recursive.' >&2; exit 1; }
fi
python3 scripts/verify_dependencies.py
mkdir -p evidence
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DBUILD_TESTING=ON 2>&1 | tee evidence/configure.log
cmake --build build --parallel "${SIDLAB_JOBS:-6}" 2>&1 | tee evidence/build.log
# Temporary packaging is outside file-provider managed Documents. All durable
# artifacts/source stay in this repository; installation remains a separate step.
SIGN_STAGE="$(mktemp -d "${TMPDIR:-/tmp}/sidlab-sign.XXXXXX")"
trap 'rm -rf "$SIGN_STAGE"' EXIT
: > evidence/signing.log
for kind in AU Standalone; do
    if [[ "$kind" == AU ]]; then base='SID Lab.component'; else base='SID Lab.app'; fi
    bundle="build/SIDLab_artefacts/Release/$kind/$base"
    staged="$SIGN_STAGE/$base"
    ditto --norsrc "$bundle" "$staged"
    xattr -dr com.apple.ResourceFork "$staged" 2>/dev/null || true
    xattr -dr com.apple.FinderInfo "$staged" 2>/dev/null || true
    codesign --force --sign - --timestamp=none "$staged" 2>&1 | tee -a evidence/signing.log
    codesign --verify --strict --verbose=2 "$staged" 2>&1 | tee -a evidence/signing.log
    ditto -c -k --norsrc --keepParent "$staged" "build/SIDLab-$kind-arm64.zip"
    ditto --norsrc "$staged" "$bundle"
done
ctest --test-dir build --output-on-failure 2>&1 | tee evidence/ctest.log
cp build/Testing/Temporary/LastTest.log evidence/tests.log
python3 scripts/package_source.py build/SIDLab-source.tar.gz
echo "Built and tested AU: $ROOT/build/SIDLab_artefacts/Release/AU/SID Lab.component"
echo "Standalone: $ROOT/build/SIDLab_artefacts/Release/Standalone/SID Lab.app"
