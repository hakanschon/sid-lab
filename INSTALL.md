# Install and verify

Build and test first, then install the one owned AU component:

```sh
./scripts/build.sh
./scripts/install.sh
./scripts/validate.sh
```

Installed location:

```text
~/Library/Audio/Plug-Ins/Components/SID Lab.component
```

The public instrument uses manufacturer **SID Lab** and bundle ID `org.sidlab.instrument`. Earlier development builds may have a different AU identity. Export any sounds you want to transfer before switching; existing host projects may still need their original plugin. The installer refuses a destination with a different identity.

The installer checks the source and destination bundle IDs, stages the copy, verifies its ad hoc signature and preserves a rollback copy until the replacement succeeds. It refuses to overwrite an unrelated plugin. No admin privileges or global cache changes are involved. The standalone app remains in `build/SIDLab_artefacts/Release/Standalone/SID Lab.app` and can be opened directly.

The validation script verifies the arm64 executable and signature, records `auval -a`, and executes:

```sh
auval -v aumu Sidl Sdlb
```

Read the actual result in `evidence/auval.log`. If the new component is not listed yet, quit and reopen GarageBand. A logout/login is a fallback for macOS registration. Do not delete shared Audio Unit caches or disable security protections. For a failure, preserve the logs and diagnose the reported error first.

Remove only this project's installed component with:

```sh
./scripts/uninstall.sh
```

It verifies ownership and keeps the repository, app and user presets. Quit hosts using SID Lab before upgrading or removing it.

## GarageBand acceptance checklist

**Requires a real listening check and MIDI controller confirmation. Automated host-state tests do not replace saving and reopening a GarageBand project.**

1. Connect a MIDI controller using its normal instrument/MIDI port. Open **GarageBand**. GarageBand must receive standard MIDI note/CC messages from the controller.
2. Create an **Empty Project**, then a **Software Instrument** track.
3. In **GarageBand > Settings > Audio/MIDI**, verify **Enable Audio Units** is selected. Check your chosen audio output device and a sensible listening level. Older GarageBand versions call Settings “Preferences.”
4. Select the instrument track and press **B** for **Smart Controls**. Expand the **Plug-ins** area if needed. Use the instrument slot (the first slot), not an audio-effect slot.
5. Click the right side of the current instrument slot and choose **AU Instruments > SID Lab > SID Lab > Stereo**. Depending on host version, the manufacturer and instrument may appear without a separate Stereo submenu.
6. Click the center of the **SID Lab** instrument slot to open its custom graphical editor. Verify Play and Chip views, model and architecture selectors, large knobs, factory browser and keyboard are visible.
7. Play your MIDI controller. Verify notes, chords in Enhanced Poly, pitch bend, sustain down/up and the MIDI indicator. Release every key and pedal; check for stuck notes. **PANIC** should fade and clear all voices.
   For rotary controls, configure absolute MIDI CC messages. In SID Lab choose **MIDI KNOBS > Learn all 8 knobs** and turn knobs 1–8, waiting for each prompt. Check Brightness, Resonance, Grit, Movement, Attack, Release, LFO speed and Master. Save/reopen the project and verify the learned assignments remain.
8. Select factory presets from all nine categories. Try Dirty C64 Bass, Arcade Lead, Classic SID Arpeggio, PWM Dream Pad, Noisy Percussion and Ring Modulator FX. Also try Crystal Cartridge II in Disco / Sequencer and Disco Circuit Kit in Drums / Kits. Try Japan Fifth Bass, Tokyo Neon Pluck and Melody String Machine in 80s / Alphaville; hold three-note chords on the string patch. Also try Melody Hook Strings and Melody Octave Drive. Save a Randomize result with SAVE / EXPORT, change the sound, then use IMPORT PRESET to restore your file. Click the kit pads or use MIDI notes 36/38/42/46. Switch OUTPUT / LIVE between Trace, Spectrum and Orbit. Verify the sounds differ and the visual change preserves the sound. Classic patches are deliberately monophonic.
9. Choose a category, lock Oscillators, then click **RANDOMIZE**. Confirm the oscillators stay fixed while unlocked sections change. Try **MUTATE**, **UNDO**, and **A > B / B > A**. Change Brightness in each comparison slot and check both versions survive switching.
10. Click **SAVE / EXPORT**, choose a descriptive `.sidlab.json` filename in a folder you own, and save it. Change the sound, then **IMPORT PRESET** and select that file. Check the sound, name and parameters return.
11. Close the plugin window, record a short MIDI region with GarageBand's **Record** button or **R**, stop, and play it back. Verify SID Lab renders the recorded region. When using GarageBand Musical Typing, close the plugin editor if text entry is taking keyboard focus.
12. Set an unmistakable patch (for example 8580, Enhanced Poly, Brightness 777, PWM Dream Pad), save the GarageBand project, close it, reopen it, and inspect/play the sound again. Check all synthesis parameters, locks and comparison sounds. The `.sidlab.json` file should not be needed to reopen that project because host state is embedded.

Result to record: GarageBand version, output device, sample rate/buffer if known, controller model and MIDI connection, steps passed/failed and any audible problems. Merely opening a window does not verify physical MIDI, speakers, recording, or project recall.

Apple's official reference: [Use Audio Units plug-ins with GarageBand](https://support.apple.com/guide/garageband/use-audio-units-plug-ins-gbnde06a4e4d/mac).
