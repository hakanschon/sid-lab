// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "PluginProcessor.h"
#include "AudioDisplay.h"
namespace sidlab {
class LabLook final : public juce::LookAndFeel_V4 {
public:
    LabLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    juce::Font getTextButtonFont(juce::TextButton&,int) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&,juce::Label&) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
};
class Editor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit Editor(Processor&);~Editor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Control;
    Processor& synth;
    AudioDisplay display;
    LabLook look;
    std::vector<std::unique_ptr<Control>> controls;
    juce::ComboBox category,presets;
    juce::TextButton view{"CHIP MODE"},random{"RANDOMIZE"},mutate{"MUTATE"},undo{"UNDO"},ab{"A / B"},save{"SAVE / EXPORT"},load{"IMPORT PRESET"},panic{"PANIC"};
    juce::TextButton visualButton{"TRACE"},previousPreset{"<"},nextPreset{">"};
    juce::TextButton midiKnobs{"MIDI KNOBS"};
    std::array<juce::TextButton,8> drumPads;
    std::array<juce::ToggleButton,4> lockButtons;
    juce::TextEditor seed;
    juce::Label status;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File lastPresetDirectory;
    juce::String fileStatus;
    int fileStatusFrames=0;
    bool chipView=false;
    unsigned seenRevision=~0u,seenActivity=0;
    int activityFrames=0,seenMode=-1;
    juce::Rectangle<int> scale(juce::Rectangle<int>) const;
    void addKnob(int param,const juce::String& title,juce::Rectangle<int> bounds,int page,bool large=false);
    void addChoice(int param,const juce::String& title,const juce::StringArray&,juce::Rectangle<int>,int page);
    void addToggle(int param,const juce::String& title,juce::Rectangle<int>,int page);
    void addSlider(int,const juce::String&,juce::Rectangle<int>,int);
    void refreshPresets();
    void browse(int direction);
    void generate(bool small);
    void chooseFile(bool saving);
    void showMidiKnobs();
    void timerCallback() override;
};
} // namespace sidlab
