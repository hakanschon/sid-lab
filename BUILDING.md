# Building SID Lab

## Tools

Use an Apple Silicon Mac and a native arm64 shell, with macOS 12 or later as the deployment target. You need Apple Command Line Tools or Xcode, CMake 3.25+, Python 3.9+ and Git for a repository checkout. Source-archive builds include dependencies and do not need Git or a network connection. Autotools and an Apple Developer membership are not needed.

Install Apple's tools with `xcode-select --install` if needed. Install CMake from [cmake.org](https://cmake.org/download/) and Python from [python.org](https://www.python.org/downloads/macos/), or use your existing package manager. Check `cmake --version`, `python3 --version`, `git --version` and `xcrun --find clang++` before building. Review any Apple license prompt yourself; the scripts do not accept licenses automatically.

If full Xcode is selected but you intend to use an already installed Command Line Tools toolchain, set `DEVELOPER_DIR=/Library/Developer/CommandLineTools` for the build command. This does not change the global developer-directory selection. Use a normal Finder/Terminal session for GUI/audio validation; a restricted sandbox may not see registered Audio Units.

## Recursive clone

```sh
git clone --recurse-submodules https://github.com/hakanschon/sid-lab.git
cd sid-lab
./scripts/build.sh
```

For an existing checkout, run `git submodule update --init --recursive`. `dependencies.lock` and the Git links pin these exact revisions:

| Component | Version | Commit |
| --- | --- | --- |
| JUCE | 9.0.4 | `3fb5cbc0340758596dac39f7c8de47d562b4896e` |
| libresidfp | 1.2.3 | `32c8525418a643eaed8dfafa3d7039e250050cfa` |

The dependency verifier rejects another revision or changed dependency sources. Update pins, notices and tests together when intentionally upgrading.

## Complete source archive

Use `SIDLab-source.tar.gz` from the same release as the binaries. It contains project source, complete dependency sources, original notices and `DEPENDENCY_SOURCES.json`. Unpack it and run `./scripts/build.sh` inside `sid-lab/`. The verifier checks the manifest revision, file inventory and SHA-256 of every dependency file. It detects changes relative to that manifest; the manifest is not an independent signature of publisher identity. Release checksums and the source repository establish provenance separately.

GitHub's automatic repository ZIP/tarball omits submodule contents. Use a recursive clone or the complete source archive. To make a complete archive from an initialized checkout:

```sh
python3 scripts/package_source.py build/SIDLab-source.tar.gz
```

## Build outputs

The script configures Release/arm64, builds the instrument and verification executable, signs temporary clean bundle copies ad hoc, checks their signatures, packages them and runs CTest. `SIDLAB_JOBS=3 ./scripts/build.sh` limits parallel compiler jobs.

| Output | Location |
| --- | --- |
| Standalone | `build/SIDLab_artefacts/Release/Standalone/SID Lab.app` |
| AUv2 | `build/SIDLab_artefacts/Release/AU/SID Lab.component` |
| Binary ZIPs | `build/SIDLab-AU-arm64.zip`, `build/SIDLab-Standalone-arm64.zip` |
| Complete source | `build/SIDLab-source.tar.gz` |
| Local test/render logs | `evidence/` (generated; not part of the public Git snapshot) |

Both bundles carry copyright and license notices in `Contents/Resources/Legal/`. Publish the complete source archive with any binary release; [release procedure](docs/PUBLISHING.md). Local and CI binaries are ad hoc signed, not notarized. This workflow does not disable Gatekeeper, strip quarantine, use administrator privileges or alter Audio Unit caches.

macOS file providers can attach FinderInfo/ResourceFork metadata that codesign rejects. The build signs and verifies disposable copies outside managed Documents storage, then returns signed bundles/ZIPs to `build/`. Prefer verifying the installed component or extracted archive outside file-provider storage if metadata is reattached.

## Tests and focused targets

```sh
cmake --build build --target SIDLab_AU SIDLab_Standalone sid_tests --parallel 3
ctest --test-dir build --output-on-failure
./build/sid_tests "$PWD/evidence"
```

The suite exercises actual SID synthesis, register behavior, sample-offset MIDI, state and preset compatibility, all factory sounds, eight-knob learning, callback allocation and GUI construction. It generates PNG/WAV evidence and refreshes factory JSON exports. These automated checks do not replace listening or a real GarageBand record/save/reopen test. `scripts/validate.sh` separately checks the installed AU with Apple's `auval`.

For a focused engine build:

```sh
cmake -S . -B build-core -DCMAKE_BUILD_TYPE=Release -DSIDLAB_ENGINE_ONLY=ON
cmake --build build-core --parallel 3
./build-core/engine_probe build-core/sid-engine.wav
```

## Library adapter

libresidfp uses Autotools upstream. `cmake/Libresidfp.cmake` uses the pinned 1.2.3 source list and configures upstream templates for this C++20 build. It copies sources into `build/resid-source`, retaining notices, then changes the shared filter-dither counter to `std::atomic<unsigned>` with relaxed increments and a compile-time lock-free assertion. This removes shared mutable-index races without per-thread lazy allocation. No upstream submodule is edited. Fast-math is disabled.

Filter tables, model configuration, SID objects and sinc resamplers are prepared outside the callback. Sixteen complete SIDs cover four slots × two models × two clocks; only the selected bank renders. See [Architecture](docs/ARCHITECTURE.md).

AU identity is `aumu / Sidl / Sdlb`, bundle ID `org.sidlab.instrument`. The install script uses the default `build/` outputs. Xcode IDE builds can use a separate `build-xcode/` directory with `cmake -G Xcode`, but the command-line script is the reference packaging path.
