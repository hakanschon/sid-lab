// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Parameters.h"
namespace sidlab {
enum Lock : unsigned { LockOscillators=1, LockFilter=2, LockEnvelopes=4, LockModulation=8 };
Patch randomize(const Patch& base,int category,uint64_t seed,unsigned locks=0,bool mutate=false) noexcept;
unsigned sectionFor(int parameter) noexcept;
} // namespace sidlab
