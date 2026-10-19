# Licensing

SID Lab's original source, scripts, documentation, interface artwork and factory patches are copyright 2026 **SID Lab contributors** and licensed **AGPL-3.0-or-later**, unless a file states otherwise. The full, unmodified license is in [LICENSE](LICENSE). Contributions retain their authors' copyrights and use the same project license. Third-party files keep their own notices.

## Main dependencies

| Component | Pinned version | License used here | Creators and notices |
| --- | --- | --- | --- |
| JUCE | 9.0.4, `3fb5cbc0340758596dac39f7c8de47d562b4896e` | AGPL-3.0-only; upstream also offers commercial terms | Julian Storer, Raw Material Software Limited and JUCE contributors; [upstream notice](licenses/JUCE-LICENSE.md), [inventory](licenses/JUCE.spdx.json) |
| libresidfp | 1.2.3, `32c8525418a643eaed8dfafa3d7039e250050cfa` | GPL-2.0-or-later | Dag Lem, Antti S. Lankila, Ken Händel and Leandro Nini; [AUTHORS](licenses/libresidfp-AUTHORS.txt), [GPLv2](licenses/libresidfp-GPL-2.0.txt), [GPLv3](licenses/GPL-3.0.txt) |

The original JUCE declaration is preserved: **AGPL-3.0-only OR LicenseRef-JUCE-Commercial**. This project uses its open-source option and makes no claim of a commercial JUCE license. libresidfp's later-version permission permits GPLv3 terms for the combination; GPLv3 and AGPLv3 each contain a section 13 provision for combining the two. This does not transfer authorship or relicense upstream files as SID Lab code. The [GNU license FAQ](https://www.gnu.org/licenses/gpl-faq.html#v3Notwithstanding) describes the provision.

JUCE includes additional third-party software. [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) identifies the notices carried with this macOS build. The complete upstream inventory remains in `third_party/JUCE/JUCE.spdx.json`, copied unchanged to `licenses/JUCE.spdx.json`. Source headers and vendor provenance files remain in the submodules and complete source archive.

## Source changes and distribution

The pinned upstream repositories are unmodified. `cmake/Libresidfp.cmake` copies libresidfp into the build directory, then changes its shared filter-dither index to a lock-free atomic counter. The exact transformation and purpose are visible in that adapter and [BUILDING.md](BUILDING.md). Derived build copies retain upstream copyright headers. JUCE's own vendor changes are recorded in its `JUCE_UPSTREAM.txt` files.

When redistributing the instrument, retain the project/upstream notices, preserve recipients' rights and provide complete corresponding source and build scripts for the binaries distributed, including dependencies and local modifications. `scripts/build.sh` creates `SIDLab-source.tar.gz` with dependency sources and a checksum manifest. Publish it alongside both binary ZIPs and identify the source revision. Automatic GitHub archives omit submodules and are not a substitute. Consult the full licenses for the applicable source and installation-information obligations.

Both bundles carry notices under `Contents/Resources/Legal/`. The license texts control over this summary. A commercial JUCE license alone would not remove libresidfp's GPL obligations or the project license. This repository grants no proprietary-distribution license.

## Sounds, performances and names

The 104 factory sounds and demonstration MIDI sequences were created for SID Lab. No commercial preset banks, Commodore ROMs, SID songs, recorded music, product artwork or artist samples are bundled. Musical references describe original sound-design interpretations, not official patches or exact reconstructions. Software licensing does not claim ownership of music you perform or render with the instrument; see the [GNU FAQ on program output](https://www.gnu.org/licenses/gpl-faq.html#WhatCaseIsOutputGPL).

Product names identify compatibility or inspiration. There is no endorsement by upstream authors, Apple, Commodore, M-Audio or referenced artists.
