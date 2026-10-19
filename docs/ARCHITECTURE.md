# Architecture

SID Lab separates the native host wrapper, versioned sound data and cycle-based SID engine. JUCE provides audio/MIDI, APVTS parameters, UI and the AU/standalone wrappers. libresidfp supplies the emulator; see [Credits](../CREDITS.md).

`Parameters` defines 64 stable synthesis values: 20 original global values, 13 per oscillator for three oscillators, and five appended extensions. Existing parameter IDs/indices stay stable. Schema 1 (59 values) and schema 2 (62 values) load with compatible defaults; schema 3 stores all 64. New required fields and unsupported versions are validated before application.

`SidEngine` prepares sixteen complete SIDs: four note slots for each 6581/8580 × PAL/NTSC combination. A complete SID has three oscillators, one shared filter, its native mixer and resampler. Only the selected bank's active chips render. Model tables, filters and resamplers are prepared outside audio processing.

Host event offsets divide each buffer into exact spans; modulation uses chunks of at most 32 samples inside those spans. The emulator's sample-count API carries its own cycle/resampler phase. Register writes are cached. Structural changes fade for 4 ms, switch prepared banks at zero gain and replay held keys. Steals/retriggers retain a short decaying residual. Drum transition events use a bounded queue to avoid lost short hits.

Fixed 16×128 key records track repeats, sustain and age. Classic mono uses last-note priority; enhanced polyphony allocates free/releasing/oldest chips. Drum recipes use real SID waveforms, envelopes and pitch sweeps. They are generated percussion rather than sample playback.

GUI notes enter through a fixed SPSC queue; overflow requests panic. Incoming controller values affect the engine at their event offsets and enter atomic pending storage. A 30 Hz message-thread timer publishes host parameter changes and advances MIDI Learn. Controller capture uses fixed atomic storage, generation-tagged requests and no audio-thread file operations or host notifications. Current-sound snapshots include pending controller values for immediate saving.

Versioned JSON user files contain one complete sound, name, category and seed. Host state additionally retains A/B, Undo, locks, visualization and rotary assignments. Temporary-file replacement protects preset writes. Changing sound presets retains controller setup. Audio display samples are atomic; FFT and drawing run on the GUI thread.

The allocation guard intercepts malloc/calloc/realloc/free and C++ new/delete in the verification executable. Tests cover ordinary, cold-thread and structural-transition callbacks. This establishes evidence for the exercised paths, rather than proving every possible host/OS path allocates nothing. Offline CPU ratios are not device dropout measurements.
