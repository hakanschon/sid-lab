// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "SidEngine.h"
#include "Presets.h"
#include "MidiControls.h"
#include <mutex>
namespace sidlab {
class Processor final : public juce::AudioProcessor, private juce::Timer, private juce::MidiKeyboardStateListener {
public:
    Processor();~Processor() override;
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    const juce::String getName() const override {return "SID Lab";}
    bool acceptsMidi() const override {return true;}
    bool producesMidi() const override {return false;}
    double getTailLengthSeconds() const override {return 36.1;}
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    int getNumPrograms() override {return int(factoryPresets().size());}
    int getCurrentProgram() override {return currentProgram.load();}
    void setCurrentProgram(int i) override {if(i>=0&&i<int(factoryPresets().size())) requestedProgram.store(i);}
    const juce::String getProgramName(int i) override;
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboard;
    SidEngine engine;
    std::atomic<int> voices{0};
    std::atomic<int> visualStyle{0};
    void setVisualStyle(int style);
    std::atomic<bool> panicRequested{false};
    Patch snapshot() const noexcept;
    Preset currentPreset() const;
    void load(const Preset&);
    void loadFactory(int);
    void generate(int category,uint64_t seed,bool mutate);
    void undo();
    void compare();
    bool canUndo() const;
    bool isB() const;
    void setLocks(unsigned);
    unsigned getLocks() const;
    void setValue(int param,float value);
    // Message-thread setup; synthesis reads only fixed-size atomic state.
    void beginMidiLearn(int knob=-1);
    void cancelMidiLearn();
    void resetMidiKnobs();
    int midiLearnKnob() const noexcept {return learningActive.load()?learningKnob.load():-1;}
    int midiKnobBinding(int knob) const noexcept;
    int lastMidiCC() const noexcept {return lastController.load();}
    std::atomic<unsigned> revision{0};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::array<std::atomic<float>*,parameterCount> raw{};
    Patch lastPatch=defaultPatch();
    std::atomic<unsigned> transaction{0};
    mutable std::mutex stateMutex;
    Preset metadata;
    std::array<Preset,2> ab;
    Preset previous;
    bool undoAvailable=false, comparisonB=false;
    unsigned locks=0;
    std::atomic<int> currentProgram{0},requestedProgram{-1};
    // GUI -> audio, fixed-size SPSC MIDI queue. No MidiKeyboardState locks in render.
    std::array<MidiEvent,256> guiMidi{};
    std::atomic<unsigned> midiWrite{0},midiRead{0};
    std::array<std::atomic<float>,parameterCount> pendingCC;
    std::array<std::atomic<int>,8> knobBindings;
    std::atomic<int> lastController{-1};
    std::atomic<bool> learningActive{false};
    std::atomic<unsigned> learnRequest{0};
    std::atomic<uint64_t> learnResult{0};
    std::array<std::atomic<uint64_t>,8> learnIgnore{};
    std::array<int,8> learningDraft{};
    std::atomic<unsigned> learnGeneration{0};
    std::atomic<int> learningKnob{0};
    bool learningAll=false;
    void armMidiLearn();
    bool captureMidiLearn(int binding) noexcept;
    void finishMidiLearn();
    void timerCallback() override;
    void handleNoteOn(juce::MidiKeyboardState*,int,int,float) override;
    void handleNoteOff(juce::MidiKeyboardState*,int,int,float) override;
    void enqueue(MidiEvent) noexcept;
    void apply(const Preset&);
    void event(MidiEvent,Patch&) noexcept;
};
} // namespace sidlab
