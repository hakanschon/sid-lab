// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "SidEngine.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_graphics/juce_graphics.h>
namespace sidlab {
class AudioDisplay {
public:
    explicit AudioDisplay(SidEngine&);
    void update(); // Message thread only; FFT never runs in the audio callback.
    void paint(juce::Graphics&,juce::Rectangle<float>,int style,bool large=false) const;
private:
    SidEngine& engine;
    juce::dsp::FFT fft{10};
    std::array<float,2048> fftData{};
    std::array<float,1024> window{};
    std::array<float,256> wave{};
    std::array<float,32> bands{},peaks{};
    float orbitAngle=0,signalPeak=0,orbitCentre=0;
    double rate=48000;
};
} // namespace sidlab
