// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Randomizer.h"
#include <juce_core/juce_core.h>
#include <vector>
namespace sidlab {
struct Preset { juce::String name; int category=0; uint64_t seed=0; Patch patch=defaultPatch(); };
const std::vector<Preset>& factoryPresets();
juce::var presetJson(const Preset&);
bool readPreset(const juce::var&,Preset&,juce::String& error);
juce::Result savePresetFile(const juce::File&,const Preset&);
juce::Result loadPresetFile(const juce::File&,Preset&);
} // namespace sidlab
