# Playing SID Lab

## Midnight Lab interface

The current design groups the preset, category and next seed in the top display, alongside a live output display. Click **TRACE / SPECTRUM / ORBIT** in the OUTPUT / LIVE screen to cycle styles. Spectrum and Orbit also replace the small routing diagram in Play view with a larger display. Spectrum uses a 1024-point, Hann-windowed FFT with 32 logarithmic bands and peak hold; Orbit draws three rotating delay portraits of the actual waveform. Both center their analysis to remove SID output DC. The FFT and drawing run on the GUI thread. The style is saved with the host state and stays selected when changing sound presets. The cyan, amber and violet oscillator panels in Chip view correspond to the routing diagram in Play view. The waveform symbols show settings; the scope shows actual audio. Synthesis controls and MIDI mappings follow the existing instrument; older preset formats remain readable. See [Architecture](docs/ARCHITECTURE.md) for implementation details.

## Quick start

Load a factory preset, select Classic or Enhanced Poly, and play your MIDI keyboard or click the on-screen keys. Start with Master at 70%. Brightness and Movement are the fastest ways to explore a patch. For independent use, launch the standalone app and open **Options > Audio/MIDI Settings** to select an output device and enable your controller’s MIDI input. In GarageBand the host handles MIDI routing and audio devices.

The browser filters the 104 embedded factory sounds by category. The small **< / >** buttons browse the selected category and wrap at either end. Start in **Disco / Sequencer** with Crystal Cartridge II, Munich Midnight, Neon Autobahn or Velvet Motorik; the Arpeggio category adds Quartz Cartridge and Crystal Rain. All patches are original SID Lab sounds, created in this project. The 1.2 Disco bank uses independent tonal recipes: Munich Midnight is a rounded bass, Neon Autobahn a dry pulse, Transistor Nights a nasal band-pass tone, Disco Databus a brass pluck, Glass Transmission a metallic mallet, and Silver Boulevard a string texture. See [Sound design](docs/SOUND_DESIGN.md) for the musical influences and bank overview. **IMPORT PRESET** loads a user `.sidlab.json` file. **SAVE / EXPORT** opens a native file dialog; the filename becomes the name inside the exported preset. No folder is created or indexed automatically. User files can be organized anywhere you choose. Project recall uses a separate embedded host-state format and does not depend on the saved user file.

## Architecture

**Classic mono** uses one real emulated SID with three oscillators and one shared filter. Last-note priority returns to a previously held key when the most recent key is released. Layers can use different tuning, ratios, waveforms and envelopes.

**Enhanced Poly** assigns a complete three-oscillator SID to each note, up to the **Poly Chips** setting (1–4, initially 4). It prefers free chips, then releasing chips, then the oldest active chip. This is a deliberately expanded architecture; a single original SID did not have twelve independent oscillators. The filter is shared within each chip, never split into three independent filters.

The two chip models use the actual MOS6581/CSG8580 emulation, including model-specific DAC, combined waveform, mixer and analog filter behavior. PAL is 985248 Hz; NTSC is 1022730 Hz. Note frequency is converted to the nearest 16-bit frequency register using the selected clock. Extreme notes/tunings saturate at the hardware's frequency-register ceiling.

## Play macros

| Control | Actual sound parameter |
| --- | --- |
| Brightness | 11-bit SID filter cutoff, smoothed over approximately 8 ms |
| Resonance | SID feedback nibble, 0–15 |
| Grit | Adds up to four resonance steps and raises the actual SID four-bit mixer volume from 65% to 100% of the Chip Volume setting; no cosmetic model EQ |
| Movement | Scales the per-note LFO's PWM and cutoff modulation |
| Attack | Adds 0–15 native attack steps to each oscillator, clamped at 15 |
| Release | Adds 0–15 native release steps to each oscillator, clamped at 15 |
| Master | Smoothed output gain after SID emulation |

Grit can change both tone and loudness. If a patch has every voice routed dry, Brightness and Resonance have little effect until a voice is routed into the shared filter. Movement needs nonzero PWM Depth or Filter Motion. PWM acts on waveforms that include Pulse. These are hardware/signal-routing consequences, not disconnected controls.

Double-click a knob to reset it; drag to adjust or edit its numeric value. Percentage fields accept a percentage number such as `50`. Discrete ADSR controls use authentic SID nibbles, not linear millisecond approximations.

## Chip view

Each oscillator has Enable, waveform, transpose (±36 semitones), fine tune (±100 cents), frequency ratio (0.25–8), 12-bit pulse width, attack/decay/sustain/release nibbles, ring modulation, sync and filter routing. Waveforms include Triangle, Saw, Pulse, Noise and four tonal combinations. Noise/tonal combined waveforms are intentionally excluded from the musical menu because SID noise shift-register behavior can lock them into silence.

The ring/sync source chain is **3 > 1 > 2 > 3**. Ring modulation acts on Triangle. Source oscillators still run when their audio waveform is muted. Enable controls whether that oscillator's waveform contributes audio; it does not remove the physical oscillator from the chip.

The bottom global panel controls one shared filter per SID, cutoff, resonance, native chip volume, PAL/NTSC, poly capacity, stereo spread and Master. Filter modes combine LP/BP/HP bits. **Mode 0 disables all filter outputs; a voice routed to the filter is then silent.** Uncheck its **To filter** box for dry output. This preserves SID routing semantics.

The SID's ADSR timing is nonlinear and quantized. Attack nibble 0 is approximately 2 ms, with the longest attacks measured in seconds. Release steps range roughly from 6 ms to 24 seconds; natural release tails are retained. Long releases can keep CPU in use after keys are released.

Model, clock, architecture and discrete patch changes use a short output fade before applying a prepared bank and replaying held keys. Retrigger/steal residuals fade over roughly 3 ms. This adds several milliseconds around such changes and avoids hard discontinuities; no expensive chip/model construction happens in playback.

## Discovery and recall

Choose Bass, Lead, Arpeggio, Pad / Atmosphere, Sound FX, Wild / Experimental, Disco / Sequencer, Drums / Kits or 80s / Alphaville. **RANDOMIZE** uses category-specific envelopes, waveform choices, tuned layers, resonance and modulation limits. It preserves master gain. A new Drums / Kits discovery selects one of the three kit architectures; a melodic discovery leaves an existing kit for Enhanced Poly. Other melodic discovery preserves Classic/Poly. **MUTATE** changes a smaller part of the current patch.

The four checkboxes lock Oscillators, Filter/Chip, Envelopes or Modulation. Filter/Chip includes model/clock/filter/volume, and Oscillators includes filter routing. The editable **NEXT SEED** is consumed and incremented after each generation. Re-enter a seed with the same SID Lab version, starting patch, category, locks and operation to reproduce a discovery. Preset files store the resulting values and do not depend on reproducing that seed. The status line records the actual current seed.

Every Randomize, Mutate or preset load captures the previous sound before replacing it. **UNDO** swaps the current sound with that previous snapshot, so it also permits redo. This is one reversible history step, not an unlimited history stack.

**A > B** saves current edits to A and recalls B; **B > A** saves edits to B and recalls A. Both slots initially contain the initial factory patch. They become independent as you edit. Host state preserves both slots, the current slot, locks, seed and undo snapshot. Save a user preset to transfer a single sound to another project.

## Save a good discovery and import it later

1. Randomize, Mutate or edit a sound until you like it.
2. Click **SAVE / EXPORT**. Choose a folder you keep, such as `Documents/SID Lab Presets`, and a filename such as `My Melody Keeper.sidlab.json`.
3. The footer shows **SAVED / EXPORTED** and the full file path after success. The filename names the exported sound. Saving keeps your current sound, A/B and Undo history intact.
4. Later, click **IMPORT PRESET**, select the file, and play. The footer confirms the imported path. This loads the sound independently of which factory preset was its starting point.
5. Copy those files to a backup, another folder or another Mac to transfer them. App builds and the AU installer do not manage or replace files you save in your own folder.

Each file stores the full 64 synthesis parameters plus its name, category and seed, including chip model, playing mode, oscillators, envelopes, filter, modulation, arp and manual edits. You do not have to reproduce the original Randomize seed. The snapshot is taken when you press Save; later MIDI CC movement while the dialog is open does not change that exported snapshot. Recent controller changes are included even before the GUI parameter timer has caught up.

The file contains one sound. A/B slots, locks, Undo history and visual style are part of the separate host/project state. Arp tempo follows the current host as usual. Imported sounds appear as the current sound; user files are opened through Import and are not automatically added to the factory list. The chooser remembers the last used folder while this editor is open. Current releases read schemas 1, 2 and 3; keep user files outside the app bundle and generated build folders.

## MIDI and modulation

All 16 MIDI channels are accepted. Velocity controls the output level of each complete chip, leaving the chip's native envelopes intact. Pitch bend initially spans ±2 semitones; its range is also a host parameter. Sustain holds released keys until pedal-up. CC123 releases notes; CC120 / PANIC uses a short hard-stop fade. CC121 resets bend/pedal state. Repeated same-note events are reference-counted.

### Connect eight rotary knobs

Configure your MIDI controller’s knobs to send absolute 0–127 MIDI CC messages. Many controllers have a separate instrument/MIDI port or mode; a DAW control-surface port can send mixer or relative messages instead. Consult your controller’s manual for its CC assignments and mode selection. SID Lab learns the incoming CC and channel without changing controller presets or firmware.

1. Open SID Lab's own window. In GarageBand, click the middle of the SID Lab instrument slot under **Plug-ins**.
2. Click **MIDI KNOBS** near the top right, then **Learn all 8 knobs (turn 1 to 8)**.
3. Turn physical knob 1 a little. Wait for **TURN KNOB 2**, then turn knob 2. Continue through knob 8. Follow the footer prompt; continued messages from a previously learned knob are ignored.
4. After knob 8, the button returns to **MIDI KNOBS** and the complete map becomes active. Choose **Current assignments** to inspect each learned CC and MIDI channel.
5. Save your GarageBand project, or quit the standalone app normally to retain its state. Configure each separate app/plugin instance once; MIDI assignments are stored with host state rather than individual sound preset files.

| Physical knob | Destination | Standard CC before learning |
| --- | --- | --- |
| 1 | Brightness | 74 |
| 2 | Resonance | 71 |
| 3 | Grit | 70 |
| 4 | Movement | 76 |
| 5 | Attack | 73 |
| 6 | Release | 72 |
| 7 | LFO speed | 77 |
| 8 | Master | 7 |

The standard CC numbers are SID Lab's defaults, controller presets may use different assignments. The large controls show their knob numbers; knob 7 is the smaller LFO control in the Motion area. **Learn one knob** reassigns an individual destination. Reusing another knob's binding swaps the two assignments. **Cancel learning** retains the previous complete map, including if only some knobs were turned. **Restore standard CC assignments** returns to the table above. Changing sounds, Randomize, Undo and A/B retain your controller setup.

Learned bindings include the incoming MIDI channel. Standard assignments accept all channels. CC1 still controls Movement and CC7 still controls Master as aliases when that channel/CC has not been explicitly assigned elsewhere. CC64 remains sustain. Pedals, bank selection, data-entry/RPN/NRPN and channel-mode commands are excluded from learning. The footer shows the latest incoming CC, channel and value to help check routing. Learning requires absolute 0–127 CC messages, not relative encoders, NRPNs or a DAW control-surface port.

CC changes affect audio at the event's block offset, then synchronize the visible/host parameter on the message thread. Recent changes are included in Save/Export and project state before that GUI synchronization. GarageBand can consume controller messages itself; if the learning prompt does not advance, check the controller’s absolute CC mode, the selected instrument track and the incoming CC display. In the standalone app, enable your controller’s input under **Options > Audio/MIDI Settings**. Restart GarageBand after installing an update to load the new AU.

The LFO drives PWM and filter motion. Per-note arpeggio choices are Off, Major, Minor, Octaves, Motorik, Disco Minor, Disco Major, Berlin, Pulse Race, Crystal Steps, Minor Cascade and Octave Drive, at 1/8, 1/16, 1/32 or 1/48 divisions. They follow the host BPM (120 BPM when no tempo is available). **Step Length** controls each step’s SID gate duration, from 5% to 100%. Full length retains the original continuous gate. **Sweep** starts each note up to 48 semitones above or below its played pitch; **Time** sets its exponential return time constant (5–600 ms). **Pluck** adds a signed envelope to the actual shared-filter cutoff (−1800 to +1800 register steps). **Decay** is its exponential time constant (5–1500 ms). Positive Pluck opens then closes the filter; negative Pluck starts dark and rises toward Brightness. It starts at note-on with arp off and retriggers at every arp step with arp on. Zero keeps the previous filter behavior. Dry oscillators bypass it. These five modulation parameters are included in the Modulation lock. Phase starts on a note and runs independently per chip; it is not locked to host song position/transport. LFO and arp register updates occur in chunks of at most 32 host samples. MIDI events split those chunks at the exact host event offset.

Poly spread pans entire emulated chips and can be set to zero. Classic output is dual mono. No chorus, delay, reverb or portamento is included. Those are optional external synth effects rather than original SID hardware, and can be added in GarageBand.

## 80s / Alphaville

Select **80s / Alphaville** in the category browser for twenty-four original SID interpretations of the *Forever Young* era. Start with **Japan Fifth Bass**, which pairs a low pulse with a saw a fifth above it, and **Tokyo Neon Pluck** for short, bright keyboard parts. **Forever Brass**, **Melody String Machine**, **Berlin Glass Bells**, **Diamond Pad**, **Harbour Electric Keys**, **Young Horizon Choir**, **Japan Brass Stabs** and **Forever Fade Strings** support up to four-note chords. Hold the string and pad sounds to hear their slower attack and filter movement.

**Summer Voltage Lead**, **Jet Set Sync Lead** and **Arcade Romance Lead** are melodic mono patches. **Midnight Marimba** is a short mallet tone. **Night Drive Octaves**, **1984 Spark Arp** and **Melody Octave Drive** provide automatic arpeggios; the other twenty-one are played directly from your keys. These are original sound-design approximations on SID rather than exact emulations of Alphaville's original synths or recordings.

Randomize and Mutate work in the new category and use its own tonal recipes. Master and the selected mono/poly architecture follow the existing discovery rules; choose Enhanced Poly when exploring chord sounds through Randomize. The original ninety-six factory programs retain their values and indices in 1.4. The additional Melody sounds are appended as programs 96–103 (zero-based), category 8. Their category label requires 1.3; synthesis remains on the existing 64-parameter schema 3.

The eight **Melody** patches target the fast string and synth textures around “Sounds Like a Melody.” Try **Melody Hook Strings** for a played string line, **Melody Staccato Strings** for short chord hits, **Melody Finale Ensemble** for sustained chords, **Melody Octave Drive** for pulse bass and **Melody Bright Lead** for a focused keyboard lead. **Melody Glass Pulse**, **Melody Bandpass Keys** and **Melody Warm Backdrop** add chiming, nasal and soft-pad variations. The record includes orchestral strings; these remain SID interpretations.

## Retro SID drum kits

The **Drums / Kits** bank contains BitBox 64, Disco Circuit, Chrome Arcade, Soft Tape, Industrial Raster, Neon Club, Tiny Cartridge and Darkroom. These are eight patch variations built from three synthesized recipe families, using actual SID triangle/pulse/noise, pitch drops, envelopes and shared filters. There are no drum samples or external libraries.

Loading a kit selects BitBox, Disco or Chrome kit architecture. Each hit receives a complete SID, up to the Poly Chips capacity. Short MIDI note-offs do not cut one-shot hits; the native release tail retires automatically. A closed hat chokes an open hat. PANIC/CC120 still stops all hits. The eight pads in Play view send actual MIDI events and work without an external controller. Select a melodic preset to return to a melodic architecture.

| MIDI notes | Instrument |
| --- | --- |
| 35, 36 | Kick |
| 37 | Rim |
| 38, 40 | Snare |
| 39 | Clap |
| 42, 44 | Closed hat |
| 46 | Open hat |
| 41, 43, 45 | Low tom |
| 47, 48, 50 | High tom |
| 56 | Cowbell |
| 49, 52, 55, 57 | Crash |
| 51, 53, 59 | Ride |

All MIDI channels work. Other notes repeat a playable layout each octave: C kick, C# rim, D snare, D# clap, E closed hat, F low tom, F# closed hat, G high tom, G# cowbell, A crash, A# open hat, B ride. The GM assignments above take precedence. Note numbers avoid host-dependent octave labels.

Brightness, Resonance, Grit and envelope macros shape newly triggered hits; Master affects the current output. Keep Attack at zero for sharp transients: long attack offsets can make short gated hits quiet. Kit recipes supply oscillator settings, arp-off, pitch sweeps, filter mode and routing for each instrument; the individual oscillator controls in Chip view describe melodic synthesis and do not override those recipes. Clock/model, chip volume, poly capacity and stereo spread remain shared kit settings. Kit variations intentionally share instrument families with different tuning, filtering, grit and model choices.

New user presets use schema 3 with 64 parameters. Original schema 1 presets and host states still load with zero pitch sweep and a fully open step gate. Schema 1 and 2 default Filter Pluck to zero, preserving their saved tone. The original 24 factory patches retain their values and ordering. Schema 2 files need SID Lab 1.1; schema 3 files need 1.2. Project recall keeps the saved parameters, including older versions of factory sounds. After updating, browse to a factory preset with the arrows to load its current recipe.

## Practical limits

Four notes are the tested maximum. All four model/clock banks are preallocated, though only selected active chips render. Sinc resampling is provided by libresidfp's sample-count clock API; the app does not round an independent cycles-per-block accumulator. Prepare/sample-rate changes rebuild tables outside the callback.

A conservative output scale leaves headroom: Classic uses one-chip gain, Enhanced reserves room for four chips. Single notes in Enhanced can be quieter. Raise the GarageBand track level as needed. The output trace auto-scales visually by a fixed factor; it is not a calibrated peak meter.

The synth safely bounds parameters, but extreme ring/sync/combined-waveform settings may be quiet or unusual because they represent real SID behavior. Release all keys or press PANIC if a controller disconnects while notes are held. Automatic physical-device-disconnection detection belongs to the host.
