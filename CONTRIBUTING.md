# Contributing

Issues and pull requests are welcome. Include your macOS version, Mac architecture, host/version, SID Lab version, exact reproduction steps and whether the same issue occurs standalone. For MIDI problems, include the incoming CC/channel shown in the footer, controller model and mode. Redact local usernames, device serials, private project paths and credentials from logs.

Start with [BUILDING.md](BUILDING.md). Run `python3 scripts/check_public.py`, `./scripts/build.sh` and relevant manual checks. Audit release archives and reachable Git history with `scripts/audit_release.py` as described in [Publishing](docs/PUBLISHING.md). Keep changes focused; explain the behavior, validation and remaining uncertainty. Report physical listening/host acceptance separately from automated tests.

Preserve parameter IDs and existing indices. Append extensions with preset-schema compatibility, protect old projects and include meaningful regression coverage for DSP/MIDI/state changes. Keep audio callbacks bounded and free of heap allocation, file operations, blocking locks and GUI/host notification calls.

Keep upstream submodules pinned and unmodified. Document build-copy patches. An intentional dependency upgrade must update `dependencies.lock`, Git links, copied licenses/attributions and verification together. Do not add music recordings, ROMs, commercial patches, unlicensed artwork or bundled system fonts.

Contributions are submitted under the project's AGPL-3.0-or-later license. Preserve your copyright and all third-party notices; do not submit material you lack permission to share. No separate contributor agreement is required. See [LICENSES.md](LICENSES.md).
