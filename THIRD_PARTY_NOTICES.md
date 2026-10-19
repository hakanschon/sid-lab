# Third-party notices

Pinned dependencies retain original headers and licenses. This file and [`licenses/`](licenses/) also accompany the AU and standalone under `Contents/Resources/Legal/`. The copied notices are unmodified upstream texts.

| Project | Attribution | License / notice copy |
| --- | --- | --- |
| JUCE 9.0.4 | Raw Material Software Limited; Julian Storer, JUCE team and contributors | [JUCE notice](licenses/JUCE-LICENSE.md); AGPLv3 option |
| libresidfp 1.2.3 | Dag Lem, Antti S. Lankila, Ken Händel and Leandro Nini | [AUTHORS](licenses/libresidfp-AUTHORS.txt), [GPLv2](licenses/libresidfp-GPL-2.0.txt), [GPLv3](licenses/GPL-3.0.txt); GPL-2.0-or-later |
| zlib 1.3.2 | Jean-loup Gailly and Mark Adler | [Zlib](licenses/juce/zlib-LICENSE.txt) |
| HarfBuzz 14.2.1 | HarfBuzz authors, including Behdad Esfahbod, David Turner, Werner Lemberg and contributors listed in COPYING | [MIT Modern Variant / COPYING](licenses/juce/HarfBuzz-COPYING.txt) |
| SheenBidi 2.9.0 | Copyright (C) 2016-2025 Muhammad Tayyab Akram | [Apache-2.0](licenses/juce/SheenBidi-LICENSE.txt) |
| LunaSVG 3.5.0 | Samuel Ugochukwu | [MIT](licenses/juce/LunaSVG-LICENSE.txt) |
| PlutoVG 1.3.2 | Samuel Ugochukwu | [MIT](licenses/juce/PlutoVG-LICENSE.txt) |
| libpng 1.6.58 | The PNG Reference Library authors | [libpng-2.0](licenses/juce/libpng-LICENSE.txt) |
| IJG JPEG 10.0 | The Independent JPEG Group | [README with legal terms](licenses/juce/IJG-README.txt) |
| libwebp 1.6.0 | Google and WebP contributors | [BSD-3-Clause](licenses/juce/libwebp-COPYING.txt), [AUTHORS](licenses/juce/libwebp-AUTHORS.txt), [PATENTS](licenses/juce/libwebp-PATENTS.txt) |
| AudioUnitSDK 1.1.0 | Copyright © 2000-2021 Apple Inc. All rights reserved; contributors | [Apache-2.0](licenses/juce/AudioUnitSDK-LICENSE.txt) |
| libogg 1.3.6 | Copyright (c) 2002-2020 Xiph.org Foundation | [BSD-3-Clause](licenses/juce/libogg-COPYING.txt) |
| Opus 1.6.1 | Xiph.Org, Skype Limited, Octasic, Jean-Marc Valin, Timothy B. Terriberry, CSIRO, Gregory Maxwell, Mark Borgerding, Erik de Castro Lopo, Mozilla, Amazon | [BSD-3-Clause and original notices](licenses/juce/Opus-COPYING.txt), [additional licensing information](licenses/juce/Opus-LICENSE-PLEASE-READ.txt) |
| opusfile 0.12-59-g6dfd29e | Copyright (c) 1994-2013 Xiph.Org Foundation and contributors | [BSD-3-Clause](licenses/juce/opusfile-COPYING.txt) |
| libopusenc 0.3 | Copyright (c) 1994-2013 Xiph.Org Foundation and contributors; Copyright (c) 2017 Jean-Marc Valin | [BSD-3-Clause](licenses/juce/libopusenc-COPYING.txt) |

**This software is based in part on the work of the Independent JPEG Group.**

Versions and provenance come from the pinned JUCE source and its [SPDX inventory](licenses/JUCE.spdx.json). The full inventory covers more platforms/features than this build enables, rather than claiming every package is linked into this instrument. macOS supplies system frameworks and Helvetica Neue/Menlo; their font files are not redistributed.

JUCE's Opus codec stack is enabled by default in this configuration, including Ogg container support, even though the SID voices do not use sampled audio. Its notices are included above. FLAC and Vorbis decoding are explicitly disabled by this project's build flags.

Upstream submodules are unmodified. SID Lab's build-copy dither change is documented in [LICENSES.md](LICENSES.md) and the CMake adapter. The complete source archive also includes JUCE's vendor provenance files and original source notices.
