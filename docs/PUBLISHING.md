# Publishing and releases

## Prepare a public repository

Review the source, dependency notices and images before publishing. Use a project identity for release commits if desired; Git configuration applies only to the exported repository. A GitHub no-reply email hides an email address but still links commits to its account. The repository's owner is always visible. Public repositories owned by the same account can be associated through its profile; private repository contents remain private. See [repository visibility](https://docs.github.com/en/repositories/creating-and-managing-repositories/about-repositories) and [commit email settings](https://docs.github.com/en/account-and-profile/how-tos/email-preferences/setting-your-commit-email-address).

For a first publication from a development checkout, export a reviewed snapshot with fresh history:

```sh
python3 scripts/check_public.py
python3 scripts/export_public.py build/public-repo \
  --author-name 'SID Lab contributors' \
  --author-email 'contributors@sidlab.invalid'
cd build/public-repo
python3 scripts/audit_release.py --history
```

The exporter copies only `.public-files.json`'s file set, retains pinned dependency Git links and starts one new `main` commit. It excludes development history and generated build/evidence folders. Existing destinations are never overwritten. It does not create a GitHub repository or push anything. Inspect `.public-files.json` whenever adding files that should be distributed.

1. Sign in to the intended GitHub account and open [New repository](https://github.com/new).
2. Choose the owner, name it **sid-lab**, select **Public**, and leave README, license and `.gitignore` initialization off. Create the empty repository.
3. Copy its HTTPS URL. In the exported folder, set that exact repository as `origin`:

   ```sh
   git remote add origin https://github.com/OWNER/sid-lab.git
   git remote -v
   git log --format=fuller --all
   git status --short
   git push -u origin main
   ```

   Replace `OWNER` with the chosen account. Authenticate through a credential helper or GitHub Desktop. Do not embed credentials in a URL or paste tokens into project files.
4. Inspect the published files and initial commit. Wait for the Actions build to pass. Enable Issues and private vulnerability reporting; consider a branch ruleset after CI is working.

Push only the reviewed `main` branch from the export. Avoid mirror/all-branch pushes or copying a development `.git` directory. These commands target this repository only; they do not change visibility or settings on other repositories. A public project under an existing account is visibly associated with that account.

Suggested description: **A playable C64 SID synthesizer for Apple Silicon — standalone and Audio Unit, powered by libresidfp and JUCE.** Suggested topics: `sid`, `c64`, `synthesizer`, `audio-unit`, `juce`, `libresidfp`, `midi`, `macos`, `apple-silicon`.

## Verify a release

1. Work from the public repository, initialize submodules, commit the release revision, then run `python3 scripts/check_public.py` and `./scripts/build.sh`.
2. Run the audit on the history and all three release archives:

   ```sh
   python3 scripts/audit_release.py --history \
     build/SIDLab-AU-arm64.zip build/SIDLab-Standalone-arm64.zip \
     build/SIDLab-source.tar.gz
   ```

   The audit scans reachable Git history, archive entries and decompressed content for credential patterns, sensitive filenames and local home paths. Inspect any findings before publication. Four unmodified JUCE example/documentation files have reviewed exceptions pinned to exact file hashes in `scripts/upstream_audit_exceptions.json`: one illustrative home path and three copies of upstream Firebase client configuration, unused by SID Lab. Firebase client identifiers are [public configuration](https://firebase.google.com/docs/projects/learn-more), not SID Lab credentials. Any byte change removes the exception. No project files are exempt. Pattern checks cannot guarantee that every possible secret is absent. If a real credential was exposed, revoke it before publishing a repaired snapshot.
3. Inspect test output, the interface images and both bundles' `Contents/Resources/Legal/` notices. Run installed AU validation separately and complete the manual host/MIDI/listening checks you intend to claim.
4. Record the source commit. Create the release tag only after verification. Never describe an ad hoc build as Developer ID signed or notarized.
5. Upload **all three matching files**: `SIDLab-AU-arm64.zip`, `SIDLab-Standalone-arm64.zip`, **`SIDLab-source.tar.gz`**. The source archive includes complete dependency sources, licenses and build scripts. GitHub's automatic source download omits submodules.
6. Include `shasum -a 256` results, supported architecture/format, signature status, dependencies, changes and unverified manual checks in release notes.

The CI workflow builds/tests on a macOS arm64 runner with read-only repository permissions. It does not publish releases automatically. Local verification does not establish hosted CI success.

The complete source archive supports offline rebuilds: extract it, run `python3 scripts/verify_dependencies.py`, then `./scripts/build.sh`. Its dependency checksum manifest detects changes relative to the manifest; it is not a maintainer signature. Keep the exact archive distributed with the binaries rather than regenerating it from a later revision. See [LICENSES.md](../LICENSES.md) for redistribution terms and upstream notices.
