// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
namespace sidlab {
enum Param : int { Model, Clock, PlayMode, Polyphony, Cutoff, Resonance, Grit, Movement,
    Attack, Release, Master, ChipVolume, FilterMode, LfoRate, PwmDepth, FilterDepth,
    Arp, ArpRate, Stereo, BendRange, GlobalCount };
enum VoiceParam : int { Enabled, Wave, Tune, Fine, Ratio, Pulse, VAttack, Decay,
    Sustain, VRelease, Ring, Sync, Route, VoiceCount };
constexpr int legacyParameterCount = GlobalCount + 3 * VoiceCount;
// Append IDs: existing AU automation indices and v1 sound values stay stable.
enum Extension : int { PitchSweep = legacyParameterCount, SweepTime, ArpGate, FilterEnvAmount, FilterEnvDecay, parameterCount };
constexpr int schema2ParameterCount = FilterEnvAmount;
struct ArpPattern { int length; std::array<int,16> notes; };
const ArpPattern& arpeggioPattern(int index) noexcept;
constexpr int vp(int voice, VoiceParam p) { return GlobalCount + voice * VoiceCount + p; }
struct ParamSpec { std::string id, name, unit; float min, max, step, initial; };
const std::array<ParamSpec, parameterCount>& parameterSpecs();
using Patch = std::array<float, parameterCount>;
Patch defaultPatch();
Patch sanitize(Patch p) noexcept;
constexpr double palClock = 985248.0, ntscClock = 1022730.0;
uint16_t frequencyRegister(double midiNote, double clockHz, double ratio = 1.0) noexcept;
uint8_t waveformBits(int wave) noexcept;
std::array<uint8_t, 3> filterRegisters(const Patch& p, float cutoff, float resonance) noexcept;
inline constexpr std::array<const char*, 9> categories { "Bass", "Lead", "Arpeggio", "Pad / Atmosphere", "Sound FX", "Wild / Experimental", "Disco / Sequencer", "Drums / Kits", "80s / Alphaville" };
inline constexpr std::array<const char*,12> arpNames {"Off","Major","Minor","Octaves","Motorik","Disco Minor","Disco Major","Berlin","Pulse Race","Crystal Steps","Minor Cascade","Octave Drive"};
inline constexpr std::array<const char*, 8> waves { "Triangle", "Saw", "Pulse", "Noise", "Tri + Saw", "Tri + Pulse", "Saw + Pulse", "Tri + Saw + Pulse" };
} // namespace sidlab
