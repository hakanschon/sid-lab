// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Parameters.h"
namespace sidlab {
inline constexpr int synthpopPatchCount=24;
// Original SID interpretations of an early-eighties synth-pop palette.
Patch synthpopPatch(int index) noexcept;
}
