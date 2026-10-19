// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginEditor.h"
#include <cmath>
namespace sidlab {
namespace {
const auto ink = juce::Colour(0xff10191d);
const auto panel = juce::Colour(0xff1b272d);
const auto line = juce::Colour(0xff35444a);
const auto text = juce::Colour(0xffeee9db);
const auto muted = juce::Colour(0xff94a6a8);
const auto cyan = juce::Colour(0xff72ded2);
const auto gold = juce::Colour(0xffefbc77);
const auto violet = juce::Colour(0xffb8ace0);
constexpr float pi = juce::MathConstants<float>::pi;

juce::Font font(float size, bool bold = false) {
    return juce::Font(juce::FontOptions("Helvetica Neue", size,
        bold ? juce::Font::bold : juce::Font::plain));
}
juce::Font mono(float size) {
    return juce::Font(juce::FontOptions("Menlo", size, juce::Font::plain));
}
juce::Colour voiceColour(int voice) {
    return voice == 0 ? cyan : (voice == 1 ? gold : violet);
}
void caption(juce::Graphics& g, const juce::String& label,
             juce::Rectangle<int> bounds, juce::Colour colour = muted) {
    g.setColour(colour);
    g.setFont(font(9.5f, true).withExtraKerningFactor(0.11f));
    g.drawText(label, bounds, juce::Justification::centredLeft);
}
void plate(juce::Graphics& g, juce::Rectangle<float> bounds, float corner = 8.0f) {
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.translated(0, 2), corner);
    g.setGradientFill(juce::ColourGradient(panel.brighter(0.045f), bounds.getTopLeft(),
        panel.darker(0.09f), bounds.getBottomLeft(), false));
    g.fillRoundedRectangle(bounds, corner);
    g.setColour(line.withAlpha(0.8f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1);
    g.setColour(juce::Colours::white.withAlpha(0.035f));
    g.drawHorizontalLine(int(bounds.getY()) + 1, bounds.getX() + corner, bounds.getRight() - corner);
}
void screw(juce::Graphics& g, float x, float y) {
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillEllipse(x - 4, y - 3, 8, 8);
    g.setColour(juce::Colour(0xff455052));
    g.fillEllipse(x - 3, y - 3, 6, 6);
    g.setColour(ink);
    g.drawLine(x - 1.7f, y + 1.7f, x + 1.7f, y - 1.7f, 1.2f);
}
void waveGlyph(juce::Graphics& g, juce::Rectangle<float> bounds, int waveform,
               juce::Colour colour) {
    // Parameter symbols, not a substitute audio oscillator or an audio preview.
    juce::Path path;
    const float mid = bounds.getCentreY(), amplitude = bounds.getHeight() * 0.34f;
    for (int i = 0; i <= 48; ++i) {
        const float t = i / 48.0f;
        float y = 0;
        if (waveform == 0) y = 1.0f - 4.0f * std::abs(t - 0.5f);
        else if (waveform == 1) y = 2.0f * t - 1.0f;
        else if (waveform == 2) y = (t < 0.5f ? 0.8f : -0.8f);
        else if (waveform == 3) y = std::sin(t * 83) * std::cos(t * 37);
        else y = std::sin(t * 2 * pi) * (0.65f + 0.3f * std::cos(t * 6 * pi));
        const float x = bounds.getX() + t * bounds.getWidth();
        if (i == 0) path.startNewSubPath(x, mid - y * amplitude);
        else path.lineTo(x, mid - y * amplitude);
    }
    g.setColour(colour);
    g.strokePath(path, juce::PathStrokeType(1.5f));
}
} // namespace

LabLook::LabLook() {
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::rotarySliderFillColourId, cyan);
    setColour(juce::ComboBox::backgroundColourId, ink);
    setColour(juce::ComboBox::textColourId, text);
    setColour(juce::ComboBox::outlineColourId, line);
    setColour(juce::PopupMenu::backgroundColourId, ink);
    setColour(juce::PopupMenu::textColourId, text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, line);
    setColour(juce::PopupMenu::highlightedTextColourId, cyan);
    setColour(juce::TextButton::buttonColourId, panel);
    setColour(juce::TextButton::textColourOffId, text);
    setColour(juce::TextButton::textColourOnId, ink);
    setColour(juce::ToggleButton::textColourId, muted);
    setColour(juce::ToggleButton::tickColourId, cyan);
    setColour(juce::Label::textColourId, muted);
}

void LabLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                              float value, float start, float end, juce::Slider& slider) {
    const bool large = w > 120 && h > 105;
    const auto bounds = juce::Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(large ? 8.0f : 2.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = start + value * (end - start);
    const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);

    for (int i = 0; i <= 10; ++i) {
        const float a = start + (end - start) * i / 10.0f;
        const float outer = radius, inner = radius * (i % 5 == 0 ? 0.88f : 0.94f);
        g.setColour(i / 10.0f <= value ? accent.withAlpha(0.7f) : line.brighter(0.1f));
        g.drawLine(c.x + std::sin(a) * inner, c.y - std::cos(a) * inner,
                   c.x + std::sin(a) * outer, c.y - std::cos(a) * outer, 1);
    }
    const float trackRadius = radius * 0.87f;
    juce::Path track, arc;
    track.addCentredArc(c.x, c.y, trackRadius, trackRadius, 0, start, end, true);
    arc.addCentredArc(c.x, c.y, trackRadius, trackRadius, 0, start, angle, true);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.strokePath(track, juce::PathStrokeType(3));
    g.setColour(accent.withAlpha(0.08f));
    g.strokePath(arc, juce::PathStrokeType(7));
    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(2.2f));

    const float dial = radius * 0.70f;
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillEllipse(c.x - dial - 1, c.y - dial + 2, dial * 2 + 2, dial * 2 + 2);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff536066), c.x, c.y - dial,
        juce::Colour(0xff111a1e), c.x, c.y + dial, false));
    g.fillEllipse(c.x - dial, c.y - dial, dial * 2, dial * 2);
    g.setColour(juce::Colour(0xff0d1518));
    g.fillEllipse(c.x - dial + 2, c.y - dial + 2, dial * 2 - 4, dial * 2 - 4);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b383e), c.x, c.y - dial,
        juce::Colour(0xff172126), c.x, c.y + dial, false));
    g.fillEllipse(c.x - dial + 3, c.y - dial + 3, dial * 2 - 6, dial * 2 - 6);
    juce::Path highlight;
    highlight.addCentredArc(c.x, c.y, dial - 4, dial - 4, 0, -0.8f, 0.8f, true);
    g.setColour(juce::Colours::white.withAlpha(0.075f));
    g.strokePath(highlight, juce::PathStrokeType(1));
    g.setColour(text);
    g.drawLine(c.x + std::sin(angle) * dial * (large ? 0.48f : 0.30f), c.y - std::cos(angle) * dial * (large ? 0.48f : 0.30f),
               c.x + std::sin(angle) * dial * 0.82f, c.y - std::cos(angle) * dial * 0.82f,
               large ? 2.4f : 1.8f);
}

void LabLook::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                  const juce::Colour& base, bool hover, bool down) {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool primary = base == cyan;
    auto fill = hover ? base.brighter(0.1f) : base;
    if (down) fill = fill.darker(0.14f);
    if (!button.isEnabled()) fill = fill.withMultipliedAlpha(0.35f);
    g.setGradientFill(juce::ColourGradient(fill.brighter(primary ? 0.05f : 0.035f), 0, 0,
        fill.darker(primary ? 0.08f : 0.12f), 0, float(button.getHeight()), false));
    g.fillRoundedRectangle(bounds, 5);
    g.setColour(primary ? cyan.brighter(0.14f) : line);
    g.drawRoundedRectangle(bounds, 5, 1);
    if (primary) {
        g.setColour(ink.withAlpha(0.6f));
        for (int i = 0; i < 3; ++i) g.fillRect(12 + i * 4, 13 + (i % 2) * 3, 2, 8 - (i % 2) * 3);
    }
}
juce::Font LabLook::getTextButtonFont(juce::TextButton& button, int height) {
    if(button.getComponentID()=="visualStyle")return font(8.5f,true);
    if(button.getComponentID()=="midiKnobs")return font(11.0f,true);
    return font(juce::jmin(12.0f, height * 0.37f), true).withExtraKerningFactor(0.035f);
}
juce::Label* LabLook::createSliderTextBox(juce::Slider& slider) {
    auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
    label->setFont(mono(12));
    label->setColour(juce::Label::textColourId, text);
    label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label->setColour(juce::Label::outlineWhenEditingColourId, cyan.withAlpha(0.7f));
    label->setJustificationType(juce::Justification::centred);
    return label;
}
void LabLook::drawComboBox(juce::Graphics& g, int w, int h, bool down,
                          int, int, int, int, juce::ComboBox& box) {
    const bool hero = box.getComponentID() == "presetBrowser";
    if (!hero) {
        const auto bounds = juce::Rectangle<float>(0.5f, 0.5f, w - 1.0f, h - 1.0f);
        g.setColour(ink.darker(0.1f));
        g.fillRoundedRectangle(bounds, 4);
        g.setColour(down || box.isMouseOver() ? cyan.withAlpha(0.7f) : line);
        g.drawRoundedRectangle(bounds, 4, 1);
    }
    juce::Path chevron;
    const float cx = w - 14.0f, cy = h * 0.5f;
    chevron.startNewSubPath(cx - 3, cy - 1.5f);
    chevron.lineTo(cx, cy + 1.5f);
    chevron.lineTo(cx + 3, cy - 1.5f);
    g.setColour(hero ? cyan : muted);
    g.strokePath(chevron, juce::PathStrokeType(1.5f));
}
juce::Font LabLook::getComboBoxFont(juce::ComboBox& box) {
    if (box.getComponentID() == "presetBrowser") return font(22, true);
    return font(box.getComponentID() == "categoryBrowser" ? 11.0f : 12.5f);
}
void LabLook::positionComboBoxText(juce::ComboBox& box, juce::Label& label) {
    const bool hero = box.getComponentID() == "presetBrowser";
    label.setBounds(hero ? 0 : 9, 0, box.getWidth() - (hero ? 28 : 34), box.getHeight());
    label.setFont(getComboBoxFont(box));
    label.setJustificationType(juce::Justification::centredLeft);
}
void LabLook::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                              bool hover, bool down) {
    const bool enabled = button.getToggleState();
    const float y = button.getHeight() * 0.5f;
    const auto accent = button.findColour(juce::ToggleButton::tickColourId);
    g.setColour(enabled ? accent.withAlpha(0.1f) : ink.withAlpha(0.6f));
    g.fillRoundedRectangle(2, y - 7, 24, 14, 7);
    g.setColour(enabled ? accent.withAlpha(0.5f) : (hover ? muted : line));
    g.drawRoundedRectangle(2, y - 7, 24, 14, 7, 1);
    g.setColour(enabled ? accent : muted.withAlpha(down ? 1.0f : 0.55f));
    g.fillEllipse(enabled ? 16.0f : 5.0f, y - 4, 8, 8);
    g.setColour(enabled ? text : muted);
    g.setFont(font(11.5f));
    g.drawText(button.getButtonText(), 33, 0, button.getWidth() - 33,
               button.getHeight(), juce::Justification::centredLeft);
}
struct Editor::Control : juce::Component {
    juce::Label label;
    juce::Slider knob;
    juce::ComboBox choice;
    juce::ToggleButton toggle;
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> sliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> choiceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;
    juce::Rectangle<int> design;int page=0,type=0;bool large=false;
    Control(){label.setJustificationType(juce::Justification::centred);label.setInterceptsMouseClicks(false,false);}
    void resized() override {
        auto b=getLocalBounds();
        if(type==2){toggle.setBounds(b);return;}
        if(type==3){label.setBounds(b.removeFromLeft(getWidth()<180?44:93));knob.setBounds(b);return;}
        label.setBounds(b.removeFromTop(large?30:18));
        if(type==1)choice.setBounds(b.reduced(3,2));else knob.setBounds(b);
    }
};
void Editor::addKnob(int param,const juce::String& title,juce::Rectangle<int> b,int page,bool large){
    auto c=std::make_unique<Control>();c->design=b;c->page=page;c->large=large;
    c->label.setText(title,juce::dontSendNotification);c->label.setFont(font(large?12.5f:10.5f,true).withExtraKerningFactor(large?0.065f:0.015f));
    c->knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);c->knob.setTextBoxStyle(juce::Slider::TextBoxBelow,false,76,20);
    c->knob.setPopupDisplayEnabled(false,false,this);c->knob.setDoubleClickReturnValue(true,parameterSpecs()[param].initial);
    c->knob.setTooltip(parameterSpecs()[param].name+" / Double click to reset");
    if(large)c->knob.setColour(juce::Slider::rotarySliderFillColourId,param==Grit?gold:(param==Master?text:cyan));
    if(page==2 && param>=GlobalCount)c->knob.setColour(juce::Slider::rotarySliderFillColourId,voiceColour((param-GlobalCount)/VoiceCount));
    if(parameterSpecs()[param].max<=1){c->knob.textFromValueFunction=[](double v){return juce::String(int(std::round(v*100)))+" %";};c->knob.valueFromTextFunction=[](const juce::String& v){return v.getDoubleValue()/100.0;};}
    c->addAndMakeVisible(c->label);c->addAndMakeVisible(c->knob);
    c->sliderAttachment=std::make_unique<Control::SliderAttachment>(synth.parameters,parameterSpecs()[param].id,c->knob);
    addAndMakeVisible(*c);controls.push_back(std::move(c));
}
void Editor::addChoice(int param,const juce::String& title,const juce::StringArray& items,juce::Rectangle<int> b,int page){
    auto c=std::make_unique<Control>();c->type=1;c->page=page;c->design=b;
    c->label.setText(title,juce::dontSendNotification);c->label.setFont(font(10,true));
    c->choice.addItemList(items,1);c->addAndMakeVisible(c->label);c->addAndMakeVisible(c->choice);
    c->choiceAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(synth.parameters,parameterSpecs()[param].id,c->choice);
    addAndMakeVisible(*c);controls.push_back(std::move(c));
}
void Editor::addToggle(int param,const juce::String& title,juce::Rectangle<int> b,int page){
    auto c=std::make_unique<Control>();c->type=2;c->page=page;c->design=b;c->toggle.setButtonText(title);
    if(param>=GlobalCount)c->toggle.setColour(juce::ToggleButton::tickColourId,voiceColour((param-GlobalCount)/VoiceCount));
    c->addAndMakeVisible(c->toggle);c->buttonAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(synth.parameters,parameterSpecs()[param].id,c->toggle);
    addAndMakeVisible(*c);controls.push_back(std::move(c));
}
void Editor::addSlider(int parameter,const juce::String& title,juce::Rectangle<int> bounds,int page){
    auto c=std::make_unique<Control>();c->type=3;c->page=page;c->design=bounds;
    c->label.setText(title,juce::dontSendNotification);c->label.setFont(font(9,true));c->label.setJustificationType(juce::Justification::centredLeft);
    c->knob.setSliderStyle(juce::Slider::LinearHorizontal);c->knob.setTextBoxStyle(juce::Slider::TextBoxRight,false,50,18);
    c->knob.setColour(juce::Slider::trackColourId,cyan);c->knob.setColour(juce::Slider::thumbColourId,cyan);
    c->knob.setTooltip(parameterSpecs()[parameter].name+" / "+parameterSpecs()[parameter].unit);
    c->addAndMakeVisible(c->label);c->addAndMakeVisible(c->knob);
    c->sliderAttachment=std::make_unique<Control::SliderAttachment>(synth.parameters,parameterSpecs()[parameter].id,c->knob);
    addAndMakeVisible(*c);controls.push_back(std::move(c));
}
Editor::Editor(Processor& p):AudioProcessorEditor(p),synth(p),display(p.engine),keyboard(p.keyboard,juce::MidiKeyboardComponent::horizontalKeyboard){
    setLookAndFeel(&look);setOpaque(true);setResizable(true,true);setResizeLimits(1000,740,1680,1230);setSize(1120,820);
    category.setComponentID("categoryBrowser");presets.setComponentID("presetBrowser");
    category.addItem("All categories",1);for(int i=0;i<int(categories.size());++i)category.addItem(categories[size_t(i)],i+2);
    category.setSelectedId(1);category.onChange=[this]{refreshPresets();};addAndMakeVisible(category);
    presets.onChange=[this]{if(presets.getSelectedId()>0)synth.loadFactory(presets.getSelectedId()-1);};addAndMakeVisible(presets);refreshPresets();
    for(auto* b:{&view,&random,&mutate,&undo,&ab,&save,&load,&panic})addAndMakeVisible(*b);
    random.setColour(juce::TextButton::buttonColourId,cyan);random.setColour(juce::TextButton::textColourOffId,ink);
    visualButton.setComponentID("visualStyle");visualButton.setTooltip("Switch live visualization: Trace, Spectrum, Orbit. Visual only; the sound is unchanged.");
    visualButton.onClick=[this]{synth.setVisualStyle((synth.visualStyle.load()+1)%3);constexpr const char* names[]{"TRACE","SPECTRUM","ORBIT"};visualButton.setButtonText(names[synth.visualStyle.load()]);display.update();repaint();};addAndMakeVisible(visualButton);
    previousPreset.setTooltip("Previous preset in this category");nextPreset.setTooltip("Next preset in this category");
    previousPreset.onClick=[this]{browse(-1);};nextPreset.onClick=[this]{browse(1);};addAndMakeVisible(previousPreset);addAndMakeVisible(nextPreset);
    midiKnobs.setComponentID("midiKnobs");midiKnobs.setTooltip("Connect all eight MIDI rotary knobs: choose Learn all 8 knobs, then turn knobs 1 through 8 when prompted. Assignments stay with this app/project.");
    midiKnobs.setColour(juce::TextButton::textColourOffId,cyan);
    midiKnobs.onClick=[this]{showMidiKnobs();};addAndMakeVisible(midiKnobs);
    for(size_t i=0;i<drumPads.size();++i){auto& pad=drumPads[i];pad.setButtonText(juce::String(drumPadNotes[i])+"  "+drumPadNames[i]);pad.setComponentID("drumPad"+juce::String(int(i)));pad.setTooltip("Play SID percussion / MIDI note "+juce::String(drumPadNotes[i]));pad.onClick=[this,i]{synth.keyboard.noteOn(10,drumPadNotes[i],0.9f);synth.keyboard.noteOff(10,drumPadNotes[i],0);};addChildComponent(pad);}
    view.onClick=[this]{chipView=!chipView;view.setButtonText(chipView?"PLAY MODE":"CHIP MODE");resized();repaint();};
    random.onClick=[this]{generate(false);};mutate.onClick=[this]{generate(true);};undo.onClick=[this]{synth.undo();};ab.onClick=[this]{synth.compare();};
    save.onClick=[this]{chooseFile(true);};load.onClick=[this]{chooseFile(false);};panic.onClick=[this]{synth.panicRequested.store(true);synth.keyboard.allNotesOff(0);};
    save.setTooltip("Save/export the complete current sound, including edits and Randomize results, to a .sidlab.json file.");
    load.setTooltip("Import a saved .sidlab.json sound. Your user files remain separate from factory presets and app updates.");
    constexpr const char* lockNames[]{"Oscillators","Filter / chip","Envelopes","Modulation"};
    for(int i=0;i<4;++i){auto& b=lockButtons[size_t(i)];b.setButtonText(lockNames[i]);b.setToggleState((synth.getLocks()&(1u<<i))!=0,juce::dontSendNotification);b.onClick=[this]{unsigned mask=0;for(unsigned k=0;k<4;++k)if(lockButtons[k].getToggleState())mask|=1u<<k;synth.setLocks(mask);};addAndMakeVisible(b);}
    seed.setText("104729");seed.setInputRestrictions(18,"0123456789");seed.setTooltip("Seed for the next Randomize or Mutate. Enter the same seed and starting patch to reproduce it.");seed.setColour(juce::TextEditor::backgroundColourId,ink);seed.setColour(juce::TextEditor::outlineColourId,line);seed.setFont(mono(11));seed.setColour(juce::TextEditor::textColourId,text);addAndMakeVisible(seed);
    status.setFont(font(10));addAndMakeVisible(status);
    keyboard.setAvailableRange(24,96);keyboard.setLowestVisibleKey(36);keyboard.setKeyWidth(24);keyboard.setWantsKeyboardFocus(false);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xffe9e3d4));keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,ink);keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,cyan.withAlpha(0.65f));keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId,juce::Colour(0xff8d958e));
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId,juce::Colour(0xff4b5553));
    keyboard.setColour(juce::MidiKeyboardComponent::shadowColourId,juce::Colours::black.withAlpha(0.65f));addAndMakeVisible(keyboard);
    addChoice(Model,"CHIP",{"MOS 6581","CSG 8580"},{944,216,152,63},0);
    addChoice(PlayMode,"ARCHITECTURE",{"Classic mono","Enhanced poly","BitBox kit","Disco kit","Chrome kit"},{754,216,180,63},0);
    constexpr int macroIds[]{Cutoff,Resonance,Grit,Movement,Attack,Release,Master};
    constexpr const char* macroNames[]{"1  BRIGHTNESS","2  RESONANCE","3  GRIT","4  MOVEMENT","5  ATTACK","6  RELEASE","8  MASTER"};
    for(int i=0;i<7;++i)addKnob(macroIds[i],macroNames[i],{32+i*153,326,139,174},1,true);
    addKnob(LfoRate,"7  LFO / Hz",{40,586,105,88},1);addKnob(PwmDepth,"PWM depth",{149,586,105,88},1);addKnob(FilterDepth,"Filter motion",{258,586,105,88},1);
    juce::StringArray patterns;for(auto name:arpNames)patterns.add(name);addChoice(Arp,"TEMPO ARPEGGIO",patterns,{385,591,154,65},1);
    addChoice(ArpRate,"DIVISION",{"1/8","1/16","1/32","1/48"},{551,591,116,65},1);
    addSlider(ArpGate,"STEP LENGTH",{386,662,280,22},1);
    addSlider(FilterEnvAmount,"PLUCK",{42,676,150,20},1);addSlider(FilterEnvDecay,"DECAY",{205,676,150,20},1);
    addSlider(PitchSweep,"SWEEP",{42,698,150,20},1);addSlider(SweepTime,"TIME",{205,698,150,20},1);
    for(int v=0;v<3;++v){
        const int x=30+v*363;
        addToggle(vp(v,Enabled),"ON",{x+202,306,74,26},2);
        juce::StringArray w;for(auto name:waves)w.add(name);
        addChoice(vp(v,Wave),"WAVEFORM",w,{x+12,337,170,61},2);
        addKnob(vp(v,Pulse),"Pulse width",{x+192,334,130,90},2);
        const VoiceParam row1[]{Tune,Fine,Ratio};const char* row1Names[]{"Transpose / st","Fine / ct","Ratio / x"};
        for(int i=0;i<3;++i)addKnob(vp(v,row1[i]),row1Names[i],{x+8+i*110,425,106,91},2);
        const VoiceParam adsr[]{VAttack,Decay,Sustain,VRelease};const char* envNames[]{"A","D","S","R"};
        for(int i=0;i<4;++i)addKnob(vp(v,adsr[i]),envNames[i],{x+6+i*82,522,80,88},2);
        addToggle(vp(v,Ring),"Ring",{x+10,615,78,26},2);addToggle(vp(v,Sync),"Sync",{x+95,615,78,26},2);addToggle(vp(v,Route),"To filter",{x+181,615,138,26},2);
    }
    addChoice(FilterMode,"SHARED FILTER",{"Bypass routes*","Low-pass","Band-pass","LP + BP","High-pass","LP + HP","BP + HP","LP + BP + HP"},{30,673,165,60},2);
    addKnob(Cutoff,"Cutoff",{210,661,105,81},2);addKnob(Resonance,"Resonance",{320,661,105,81},2);addKnob(ChipVolume,"SID volume",{430,661,105,81},2);
    addChoice(Clock,"CLOCK",{"PAL","NTSC"},{560,674,115,59},2);
    addChoice(Polyphony,"POLY CHIPS",{"1","2","3","4"},{687,674,100,59},2);
    addKnob(Stereo,"Poly spread",{810,661,110,81},2);addKnob(Master,"Master",{945,661,110,81},2);
    constexpr const char* labels[]{"TRACE","SPECTRUM","ORBIT"};visualButton.setButtonText(labels[synth.visualStyle.load()]);
    resized();startTimerHz(30);
}
Editor::~Editor(){stopTimer();setLookAndFeel(nullptr);}
juce::Rectangle<int> Editor::scale(juce::Rectangle<int> r) const {
    return {juce::roundToInt(r.getX()*getWidth()/1120.0),
            juce::roundToInt(r.getY()*getHeight()/860.0),
            juce::roundToInt(r.getWidth()*getWidth()/1120.0),
            juce::roundToInt(r.getHeight()*getHeight()/860.0)};
}
void Editor::resized() {
    category.setBounds(scale({304,104,206,24}));
    presets.setBounds(scale({304,55,435,39}));
    view.setBounds(scale({900,178,196,38}));
    random.setBounds(scale({24,178,168,38}));
    mutate.setBounds(scale({204,178,104,38}));
    undo.setBounds(scale({320,178,80,38}));
    ab.setBounds(scale({412,178,80,38}));
    load.setBounds(scale({504,178,126,38}));
    save.setBounds(scale({642,178,126,38}));
    panic.setBounds(scale({780,178,86,38}));
    seed.setBounds(scale({584,105,155,23}));
    for(int i=0;i<4;++i)lockButtons[size_t(i)].setBounds(scale({130+i*148,237,148,26}));
    visualButton.setBounds(scale({961,33,66,20}));previousPreset.setBounds(scale({680,31,26,20}));nextPreset.setBounds(scale({713,31,26,20}));
    midiKnobs.setBounds(scale({944,146,152,24}));
    const bool kit=synth.snapshot()[PlayMode]>=2;
    for(size_t i=0;i<drumPads.size();++i){drumPads[i].setBounds(scale({42+int(i%4)*157,591+int(i/4)*62,145,50}));drumPads[i].setVisible(kit&&!chipView);}
    for(auto& c:controls) {
        c->setBounds(scale(c->design));
        c->setEnabled(!(kit&&c->page==2&&c->design.getY()<650));
        c->setVisible((c->page==0||c->page==(chipView?2:1))&&!(kit&&c->page==1&&!c->large));
    }
    keyboard.setBounds(scale({26,766,1068,60}));
    status.setBounds(scale({28,832,1064,18}));
}
void Editor::paint(juce::Graphics& original) {
    juce::Graphics::ScopedSaveState saved(original);
    auto& g=original;
    g.addTransform(juce::AffineTransform::scale(getWidth()/1120.0f,getHeight()/860.0f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff283339),0,0,
        juce::Colour(0xff141e22),0,860,false));
    g.fillAll();
    // Subtle brushed finish. All graphics are native vectors, without bitmap assets.
    for(int y=3;y<858;y+=3) {
        g.setColour(juce::Colours::white.withAlpha(y%9==0?0.013f:0.005f));
        g.drawHorizontalLine(y,8,1112);
    }
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawRoundedRectangle(3,3,1114,854,10,5);
    g.setColour(juce::Colour(0xff4a5658).withAlpha(0.6f));
    g.drawRoundedRectangle(5.5f,5.5f,1109,849,8,1);
    for(auto p:{juce::Point<float>{15,15},{1105,15},{15,845},{1105,845}})screw(g,p.x,p.y);

    // Brand: an original three-voice mark rather than borrowed computer branding.
    g.setColour(ink);g.fillRoundedRectangle(26,33,42,45,6);
    for(int i=0;i<3;++i) {
        g.setColour(voiceColour(i));
        g.fillRoundedRectangle(35.0f+i*8,44.0f+(i%2)*5,4,24.0f-(i%2)*10,1);
    }
    g.setColour(text);g.setFont(font(36,true).withExtraKerningFactor(-0.025f));
    g.drawText("SID LAB",80,29,190,49,juce::Justification::centredLeft);
    caption(g,"SID SYNTHESIS INSTRUMENT",{82,83,195,17});
    caption(g,"01 / OPEN SOURCE",{28,119,220,16},gold.withAlpha(0.85f));

    // Preset screen and its seed are the visual center of the instrument.
    plate(g,{286,24,471,116});
    g.setColour(cyan.withAlpha(0.65f));g.fillRoundedRectangle(286,43,2,44,1);
    caption(g,"SOUND LIBRARY",{304,33,270,17});
    caption(g,"NEXT SEED",{522,108,62,15});

    const auto patch=synth.snapshot();
    const int visualStyle=synth.visualStyle.load();
    plate(g,{779,24,317,116});
    g.setColour(juce::Colour(0xff0b181b));g.fillRoundedRectangle(788,32,299,100,4);
    caption(g,"OUTPUT / LIVE",{798,35,150,14},cyan.withAlpha(0.8f));
    g.setColour(activityFrames>0?cyan:cyan.withAlpha(0.16f));
    g.fillEllipse(1029,39,5,5);
    caption(g,"MIDI",{1040,35,38,14},muted);
    display.paint(g,{798,55,277,51},visualStyle);
    g.setFont(mono(9));g.setColour(muted);
    const juce::String model=patch[Model]<0.5f?"MOS 6581":"CSG 8580";
    const juce::String clock=patch[Clock]<0.5f?"PAL":"NTSC";
    g.drawText(model+" / "+clock,798,112,170,13,juce::Justification::centredLeft);
    g.drawText(juce::String(synth.voices.load())+" ACTIVE",980,112,95,13,juce::Justification::centredRight);

    caption(g,"DISCOVERY / GENERATE SOMETHING UNEXPECTED",{26,151,600,18});
    caption(g,"KEEP",{27,242,92,16});
    g.setColour(line);g.drawLine(728,232,728,267,1);

    if(!chipView) {
        constexpr const char* hints[]{"11-BIT CUTOFF","SID FEEDBACK","MIXER + RESONANCE",
            "LFO / PWM","ENVELOPE OFFSET","ENVELOPE OFFSET","OUTPUT GAIN"};
        for(int i=0;i<7;++i) {
            const float x=24.0f+i*153;
            plate(g,{x,300,148,235});
            const auto accent=i==2?gold:(i==6?text:cyan);
            g.setColour(accent.withAlpha(0.8f));g.fillRoundedRectangle(x+62,311,24,2,1);
            caption(g,hints[i],{int(x)+8,507,132,15},muted.withAlpha(0.8f));
            g.setColour(line.withAlpha(0.7f));g.drawLine(x+15,501,x+133,501,0.5f);
        }
        plate(g,{24,556,662,168});
        caption(g,patch[PlayMode]>=2?"SID DRUMS / CLICK PADS OR PLAY YOUR KEYS":"MOTION / PATTERN",{42,568,600,17});
        if(patch[PlayMode]<2){g.setColour(line.withAlpha(0.8f));g.drawLine(370,590,370,716,1);
        caption(g,"HOST TEMPO / STANDALONE 120 BPM",{386,699,282,14},muted.withAlpha(0.75f));}

        plate(g,{702,556,394,168});
        if(visualStyle!=0||patch[PlayMode]>=2){
            caption(g,visualStyle==1?"SPECTRAL FIELD / LIVE FFT":(visualStyle==2?"ORBIT FIELD / AUDIO PHASE PORTRAIT":"DRUM OUTPUT / LIVE TRACE"),{720,568,354,17});
            g.setColour(juce::Colour(0xff0b181b));g.fillRoundedRectangle(716,589,366,105,5);
            display.paint(g,{722,595,354,93},visualStyle,true);
            caption(g,visualStyle==1?"35 HZ - 16 KHZ / PEAK HOLD":(visualStyle==2?"AMPLITUDE NORMALIZED / ROTATING DELAY PORTRAIT":"GM NOTES / ONE SHOT SID PERCUSSION"),{720,698,354,14},muted.withAlpha(0.75f));
        }else{
        caption(g,"INSIDE THE CHIP",{720,568,340,17});
        const float busY=660;
        for(int v=0;v<3;++v) {
            const float x=722.0f+v*66;
            const bool enabled=patch[vp(v,Enabled)]>0.5f;
            const auto colour=enabled?voiceColour(v):muted.withAlpha(0.27f);
            g.setColour(ink);g.fillRoundedRectangle(x,594,56,42,4);
            g.setColour(colour.withAlpha(0.45f));g.drawRoundedRectangle(x,594,56,42,4,1);
            waveGlyph(g,{x+9,610,38,18},int(patch[vp(v,Wave)]),colour);
            g.setColour(colour);g.setFont(mono(8));
            g.drawText("OSC 0"+juce::String(v+1),int(x)+4,598,48,11,juce::Justification::centred);
            g.setColour(colour.withAlpha(0.6f));
            if(patch[vp(v,Route)]>0.5f) {
                g.drawLine(x+28,636,x+28,busY,1);
                g.drawLine(x+28,busY,946,busY,1);
            } else {
                g.drawLine(x+28,594,x+28,589,1);
                g.drawLine(x+28,589,1080,589,1);
                g.drawLine(1080,589,1080,busY,1);
            }
        }
        g.setColour(ink);g.fillRoundedRectangle(946,638,116,44,5);
        g.setColour(cyan.withAlpha(0.55f));g.drawRoundedRectangle(946,638,116,44,5,1);
        caption(g,"ONE SHARED",{959,642,96,13},cyan);
        caption(g,"FILTER",{959,656,96,17},text);
        g.setColour(cyan.withAlpha(0.6f));g.drawLine(1062,busY,1080,busY,1);
        g.fillEllipse(1077,657,6,6);
        g.setColour(muted);g.setFont(mono(9));
        g.drawText(patch[PlayMode]<0.5f?"CLASSIC / 1 CHIP, 3 OSCILLATORS":"ENHANCED / UP TO 4 COMPLETE CHIPS",
            720,697,354,14,juce::Justification::centredLeft);
        }
        caption(g,"PLAY THE HARDWARE. EXPLORE THE UNEXPECTED.",{27,738,1066,17},muted.withAlpha(0.7f));
    } else {
        for(int v=0;v<3;++v) {
            const int x=24+v*363;
            plate(g,{float(x),300,347,348});
            g.setColour(voiceColour(v));g.fillRoundedRectangle(float(x),318,2,35,1);
            caption(g,"OSCILLATOR 0"+juce::String(v+1),{x+18,306,195,25},voiceColour(v));
            g.setColour(line.withAlpha(0.7f));g.drawLine(float(x+16),570,float(x+331),570,0.7f);
        }
        plate(g,{24,657,1072,88});
        caption(g,patch[PlayMode]>=2?"KIT OSCILLATORS USE PER-INSTRUMENT RECIPES. SELECT A MELODIC PRESET TO EDIT OSCILLATORS.":"* MODE 0 SILENCES FILTER ROUTES. UNCHECK TO FILTER FOR DRY AUDIO. RING/SYNC SOURCE: 3 > 1 > 2 > 3.",
            {28,747,1064,15},muted.withAlpha(0.75f));
    }
    g.setColour(juce::Colours::black.withAlpha(0.7f));g.fillRoundedRectangle(23,762,1074,67,4);
}
void Editor::refreshPresets(){presets.clear(juce::dontSendNotification);const int cat=category.getSelectedId()-2;const auto& factory=factoryPresets();for(size_t i=0;i<factory.size();++i)if(cat<0||factory[i].category==cat)presets.addItem(factory[i].name,int(i)+1);presets.setText(synth.currentPreset().name,juce::dontSendNotification);}
void Editor::browse(int direction){
    const int count=presets.getNumItems();if(count==0)return;
    int index=presets.getSelectedItemIndex();if(index<0)index=direction>0?-1:0;
    index=(index+direction+count)%count;presets.setSelectedId(presets.getItemId(index),juce::sendNotificationSync);
}
void Editor::generate(bool small){const auto n=static_cast<uint64_t>(seed.getText().getLargeIntValue());const int cat=category.getSelectedId()>1?category.getSelectedId()-2:synth.currentPreset().category;synth.generate(cat,n,small);seed.setText(juce::String(n+1));}
void Editor::showMidiKnobs(){
    juce::PopupMenu menu;
    menu.addItem(1,"Learn all 8 knobs (turn 1 to 8)");
    juce::PopupMenu individual;
    for(size_t i=0;i<midiKnobNames.size();++i)individual.addItem(10+int(i),juce::String(int(i)+1)+"  "+midiKnobNames[i]);
    menu.addSubMenu("Learn one knob",individual);
    if(synth.midiLearnKnob()>=0)menu.addItem(2,"Cancel learning (keep previous assignments)");
    menu.addSeparator();juce::PopupMenu assignments;
    for(int i=0;i<8;++i){const int binding=synth.midiKnobBinding(i);const auto channel=binding/128;
        assignments.addItem(30+i,juce::String(i+1)+"  "+midiKnobNames[size_t(i)]+"  /  CC"+juce::String(binding%128)+(channel==0?" / any channel":" / channel "+juce::String(channel)),false);}
    menu.addSubMenu("Current assignments",assignments);menu.addItem(3,"Restore standard CC assignments");
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&midiKnobs),[safe=juce::Component::SafePointer<Editor>(this)](int result){
        if(safe==nullptr)return;
        if(result==1)safe->synth.beginMidiLearn();
        else if(result==2)safe->synth.cancelMidiLearn();
        else if(result==3)safe->synth.resetMidiKnobs();
        else if(result>=10&&result<18)safe->synth.beginMidiLearn(result-10);
    });
}
void Editor::chooseFile(bool saving){
    // Capture when Save is pressed. MIDI CCs can still arrive while the dialog is open.
    const auto current=synth.currentPreset();const auto directory=lastPresetDirectory.isDirectory()?lastPresetDirectory:juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    const auto start=directory.getChildFile(juce::File::createLegalFileName(current.name)+".sidlab.json");
    chooser=std::make_unique<juce::FileChooser>(saving?"Save / export SID Lab preset":"Import SID Lab preset",start,"*.json",true);
    const auto flags=saving?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting):(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);
    chooser->launchAsync(flags,[safe=juce::Component::SafePointer<Editor>(this),saving,current](const juce::FileChooser& c){
        if(safe==nullptr)return;const auto f=c.getResult();if(f==juce::File())return;
        auto preset=current;juce::Result result=juce::Result::ok();
        if(saving){auto name=f.getFileNameWithoutExtension();if(name.endsWithIgnoreCase(".sidlab"))name=name.dropLastCharacters(7);preset.name=name.isEmpty()?juce::String("Untitled"):name;result=savePresetFile(f,preset);}
        else{result=loadPresetFile(f,preset);if(result.wasOk())safe->synth.load(preset);}
        if(result.failed())juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"SID Lab preset",result.getErrorMessage());
        else{safe->lastPresetDirectory=f.getParentDirectory();safe->fileStatus=(saving?"SAVED / EXPORTED: ":"IMPORTED: ")+f.getFullPathName();safe->fileStatusFrames=180;}
    });
}
void Editor::timerCallback(){
    const auto rev=synth.revision.load();if(rev!=seenRevision){seenRevision=rev;presets.setText(synth.currentPreset().name,juce::dontSendNotification);for(unsigned i=0;i<4;++i)lockButtons[i].setToggleState((synth.getLocks()&(1u<<i))!=0,juce::dontSendNotification);}
    const int mode=int(synth.snapshot()[PlayMode]);if(mode!=seenMode){seenMode=mode;resized();}
    display.update();constexpr const char* visualNames[]{"TRACE","SPECTRUM","ORBIT"};visualButton.setButtonText(visualNames[synth.visualStyle.load()]);
    const auto activity=synth.engine.activity.load();if(activity!=seenActivity){seenActivity=activity;activityFrames=6;}else if(activityFrames>0)--activityFrames;
    undo.setEnabled(synth.canUndo());ab.setButtonText(synth.isB()?"B  >  A":"A  >  B");
    const auto current=synth.currentPreset();auto normalStatus=current.name+"  /  "+categories[size_t(current.category)]+"  /  Seed "+juce::String(current.seed);
    const int cc=synth.lastMidiCC();if(cc>=0)normalStatus+="     MIDI CC"+juce::String((cc>>7)&127)+" / channel "+juce::String(cc>>14)+" / value "+juce::String(cc&127);
    const int learning=synth.midiLearnKnob();
    midiKnobs.setButtonText(learning<0?"MIDI KNOBS":"TURN KNOB "+juce::String(learning+1));
    if(learning>=0)normalStatus="MIDI LEARN: Turn MIDI knob "+juce::String(learning+1)+" for "+midiKnobNames[size_t(learning)]+". Wait for the next prompt. MIDI KNOBS > Cancel keeps your old assignments.";
    status.setText(learning>=0?normalStatus:(fileStatusFrames>0?fileStatus:normalStatus),juce::dontSendNotification);if(fileStatusFrames>0)--fileStatusFrames;
    repaint();
}
} // namespace sidlab
