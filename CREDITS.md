# Credits

SID Lab builds on decades of emulation and audio-framework work. Its integration and interface are new; the SID emulator and framework are the work of the people below.

## The SID sound: reSID and reSIDfp / libresidfp

The engine is **[libresidfp](https://github.com/libsidplayfp/libresidfp)**, a descendant of **Dag Lem's reSID**. Upstream's [AUTHORS](licenses/libresidfp-AUTHORS.txt) credits:

- **Dag Lem** — designed and programmed the complete emulation engine.
- **Antti S. Lankila** — distortion simulation and calculation of combined waveforms; began reSIDfp with a focus on the 6581 filter.
- **Ken Händel** — source conversion to Java in the jsidplay2 lineage.
- **Leandro Nini** — port back to C++, merge with reSID 1.0 and further improvements.

Upstream records copyright 2000–2011 Dag Lem, 2007–2010 Antti Lankila and 2010–2026 Leandro Nini. Individual source files preserve their more specific notices. SID Lab uses libresidfp **1.2.3**, under **GPL-2.0-or-later**, and does not claim to have written this emulator.

## The framework: JUCE

**[JUCE](https://juce.com/)** was created by **[Julian “Jules” Storer](https://github.com/julianstorer)** and is developed by **Raw Material Software Limited, the JUCE team and contributors**. It supplies native audio/MIDI integration, plugin wrappers, parameters, graphics, widgets and DSP utilities.

SID Lab pins **JUCE 9.0.4** and uses its **AGPLv3** option. The [license notice](licenses/JUCE-LICENSE.md) and [complete upstream dependency inventory](licenses/JUCE.spdx.json) are preserved unchanged.

## Additional work through JUCE

Thank you to **Jean-loup Gailly and Mark Adler (zlib)**; **the HarfBuzz authors**; **Muhammad Tayyab Akram (SheenBidi)**; **Samuel Ugochukwu (LunaSVG and PlutoVG)**; **the PNG Reference Library authors**; **the Independent JPEG Group**; **Google and WebP contributors**; and **Apple's AudioUnitSDK contributors**. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [`licenses/juce/`](licenses/juce/) for copyright and license texts. JUCE's complete inventory also covers optional components not enabled in this build.

JUCE's enabled codec support also includes **Xiph.Org and the Ogg, Opus, opusfile and libopusenc contributors**, including **Jean-Marc Valin, Timothy B. Terriberry, Gregory Maxwell and the other copyright holders named in the original notices**. Those notices accompany the instrument even though SID Lab's voices are synthesized rather than sampled.

**This software is based in part on the work of the Independent JPEG Group.**

## SID Lab

**SID Lab contributors** — instrument integration, MIDI/voice management, interface, factory sound design, discovery, visualizations, user presets, tests and documentation. Copyright 2026; AGPL-3.0-or-later.

Musical influences include Giorgio Moroder and Alphaville. These are original SID Lab interpretations; no artist recordings or commercial patches are copied and no endorsement is implied. [Sound-design notes](docs/SOUND_DESIGN.md).

Thank you to all upstream contributors, maintainers and testers. Their notices remain authoritative; this page acknowledges their work and does not replace their licenses.
