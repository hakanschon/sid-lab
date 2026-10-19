// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Parameters.h"
namespace sidlab {
enum class DrumType { Kick, Snare, Clap, ClosedHat, OpenHat, LowTom, HighTom, Rim, Cowbell, Crash, Ride };
struct DrumRecipe {
    Patch patch{};
    DrumType type=DrumType::Kick;
    float rootNote=36;
    double gateSeconds=0.012;
};
DrumType drumForNote(int midiNote) noexcept;
DrumRecipe makeDrum(const Patch& base,int kit,int midiNote) noexcept;
inline constexpr std::array<int,8> drumPadNotes{36,38,39,42,46,41,37,49};
inline constexpr std::array<const char*,8> drumPadNames{"KICK","SNARE","CLAP","CLOSED HAT","OPEN HAT","LOW TOM","RIM","CRASH"};
} // namespace sidlab
