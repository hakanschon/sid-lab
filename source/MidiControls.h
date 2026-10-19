// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Parameters.h"

namespace sidlab {
inline constexpr std::array<int,8> midiKnobParams{Cutoff,Resonance,Grit,Movement,Attack,Release,LfoRate,Master};
inline constexpr std::array<int,8> defaultKnobCC{74,71,70,76,73,72,77,7};
inline constexpr std::array<const char*,8> midiKnobNames{"Brightness","Resonance","Grit","Movement","Attack","Release","LFO speed","Master"};
// Channel zero means any channel; learned bindings use the actual input channel.
constexpr int midiBinding(int channel,int cc) noexcept {return channel*128+cc;}
constexpr bool learnableCC(int cc) noexcept {
    // Keep bank selection, data-entry/RPN/NRPN, pedals and channel-mode commands
    // available to their normal MIDI functions while learning rotary controls.
    return cc>0&&cc<120&&cc!=6&&cc!=32&&cc!=38&&cc!=64&&!(cc>=96&&cc<=101);
}
constexpr bool validMidiBinding(int binding) noexcept {
    return binding>=0&&binding<17*128&&learnableCC(binding%128);
}
constexpr bool overlappingMidiBindings(int a,int b) noexcept {
    return a>=0&&b>=0&&a%128==b%128&&(a/128==0||b/128==0||a/128==b/128);
}
} // namespace sidlab
