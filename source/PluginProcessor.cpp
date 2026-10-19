// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
namespace sidlab {
static_assert(std::atomic<uint64_t>::is_always_lock_free);
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    for(const auto& s:parameterSpecs()) {
        juce::NormalisableRange<float> range(s.min,s.max,s.step);
        result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{s.id,1},s.name,range,s.initial,
            juce::AudioParameterFloatAttributes().withLabel(s.unit)
                .withStringFromValueFunction([spec=s](float value,int) {
                    if(spec.unit=="%") return juce::String(int(std::round(value*100)))+" %";
                    return juce::String(value,spec.step>=1.0f?0:2);
                })
                .withValueFromStringFunction([spec=s](const juce::String& value) {
                    return value.getFloatValue()/(spec.unit=="%"?100.0f:1.0f);
                })));
    }
    return result;
}
Processor::Processor():AudioProcessor(BusesProperties().withOutput("Stereo",juce::AudioChannelSet::stereo(),true)),
    parameters(*this,nullptr,"SIDLabParameters",layout()) {
    for(int i=0;i<parameterCount;++i) raw[i]=parameters.getRawParameterValue(parameterSpecs()[i].id);
    for(auto& cc:pendingCC) cc.store(-1);
    for(size_t i=0;i<knobBindings.size();++i)knobBindings[i].store(defaultKnobCC[i]);
    metadata=factoryPresets().front();ab={metadata,metadata};previous=metadata;
    apply(metadata); keyboard.addListener(this);startTimerHz(30);
}
Processor::~Processor(){stopTimer();keyboard.removeListener(this);}
void Processor::prepareToPlay(double sr,int) {lastPatch=snapshot();engine.setPatch(lastPatch);engine.prepare(sr);}
bool Processor::isBusesLayoutSupported(const BusesLayout& l) const {return l.getMainInputChannelSet().isDisabled()&&l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
Patch Processor::snapshot() const noexcept {
    Patch p{};for(int i=0;i<parameterCount;++i)p[i]=raw[i]->load(std::memory_order_relaxed);
    // A preset export must include controller changes already heard by the audio
    // thread, even before the 30 Hz GUI timer publishes their host parameters.
    for(size_t i=0;i<pendingCC.size();++i){const float x=pendingCC[i].load(std::memory_order_acquire);if(x>=0)p[i]=x;}
    return sanitize(p);
}
void Processor::event(MidiEvent e,Patch& p) noexcept {
    if((e.status&0xf0)==0xb0) {
        const int channel=(e.status&15)+1,cc=e.data1;
        lastController.store((channel<<14)|(cc<<7)|e.data2,std::memory_order_relaxed);
        if(captureMidiLearn(midiBinding(channel,cc)))return;
        int parameter=-1;
        for(size_t i=0;i<knobBindings.size();++i) {
            const int binding=knobBindings[i].load(std::memory_order_acquire);
            if(binding>=0&&binding%128==cc&&(binding/128==0||binding/128==channel)){parameter=midiKnobParams[i];break;}
        }
        // Preserve the modulation wheel and volume fader in addition to the knobs.
        if(parameter<0&&cc==1)parameter=Movement;
        if(parameter<0&&cc==7)parameter=Master;
        if(parameter>=0) {
            const auto& s=parameterSpecs()[parameter];
            float value=s.min+(s.max-s.min)*(e.data2/127.0f);
            if(s.step>0)value=s.min+std::round((value-s.min)/s.step)*s.step;
            pendingCC[size_t(parameter)].store(value,std::memory_order_release);p[size_t(parameter)]=value;engine.setPatch(p);
        }
    }
    engine.midi(e);
}
void Processor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    audio.clear();if(audio.getNumChannels()<2)return;
    const auto before=transaction.load(std::memory_order_acquire);
    if((before&1U)==0) {
        const auto p=snapshot();
        if(transaction.load(std::memory_order_acquire)==before) lastPatch=p;
    }
    for(size_t i=0;i<pendingCC.size();++i) {const float x=pendingCC[i].load(std::memory_order_acquire);if(x>=0)lastPatch[i]=x;}
    engine.setPatch(lastPatch);
    if(auto* playhead=getPlayHead()) if(const auto position=playhead->getPosition()) if(const auto bpm=position->getBpm())engine.setTempo(*bpm);
    if(panicRequested.exchange(false))engine.panic();
    unsigned rd=midiRead.load(std::memory_order_relaxed);
    const auto wr=midiWrite.load(std::memory_order_acquire);
    while(rd!=wr) {event(guiMidi[rd%guiMidi.size()],lastPatch);++rd;}
    midiRead.store(rd,std::memory_order_release);
    auto* l=audio.getWritePointer(0);auto* r=audio.getWritePointer(1);
    int offset=0;
    for(const auto metadataEvent:midi) {
        const int pos=juce::jlimit(offset,audio.getNumSamples(),metadataEvent.samplePosition);
        if(pos>offset)engine.render(l+offset,r+offset,pos-offset);
        offset=pos;
        if(metadataEvent.numBytes>=1&&metadataEvent.data[0]<0xf0) {
            MidiEvent e{metadataEvent.data[0],0,0};
            if(metadataEvent.numBytes>1)e.data1=metadataEvent.data[1];if(metadataEvent.numBytes>2)e.data2=metadataEvent.data[2];
            event(e,lastPatch);
        }
    }
    if(offset<audio.getNumSamples())engine.render(l+offset,r+offset,audio.getNumSamples()-offset);
    voices.store(engine.soundingVoices(),std::memory_order_relaxed);midi.clear();
}
const juce::String Processor::getProgramName(int i){return i>=0&&i<int(factoryPresets().size())?factoryPresets()[size_t(i)].name:juce::String();}
void Processor::setVisualStyle(int style) {
    visualStyle.store(juce::jlimit(0,2,style));
    updateHostDisplay();
}
void Processor::setValue(int index,float value) {
    if(index<0||index>=parameterCount)return;
    auto* p=parameters.getParameter(parameterSpecs()[index].id);
    p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();
}
void Processor::apply(const Preset& preset) {
    const auto patch=sanitize(preset.patch);
    transaction.fetch_add(1,std::memory_order_acq_rel);
    for(auto& cc:pendingCC)cc.store(-1);
    for(int i=0;i<parameterCount;++i) {
        auto* p=parameters.getParameter(parameterSpecs()[i].id);
        p->setValueNotifyingHost(p->convertTo0to1(patch[i]));
    }
    metadata=preset;metadata.patch=patch;
    transaction.fetch_add(1,std::memory_order_release);revision.fetch_add(1);
}
Preset Processor::currentPreset() const {std::lock_guard guard(stateMutex);auto p=metadata;p.patch=snapshot();return p;}
void Processor::load(const Preset& p) {std::lock_guard guard(stateMutex);previous=metadata;previous.patch=snapshot();undoAvailable=true;apply(p);}
void Processor::loadFactory(int i){if(i<0||i>=int(factoryPresets().size()))return;load(factoryPresets()[size_t(i)]);currentProgram.store(i);}
void Processor::generate(int category,uint64_t seed,bool mutate) {
    std::lock_guard guard(stateMutex);previous=metadata;previous.patch=snapshot();undoAvailable=true;
    Preset p{(mutate?"Mutation ":"Discovery ")+juce::String(seed),juce::jlimit(0,int(categories.size())-1,category),seed,
        randomize(previous.patch,category,seed,locks,mutate)};apply(p);
}
void Processor::undo(){std::lock_guard guard(stateMutex);if(!undoAvailable)return;auto now=metadata;now.patch=snapshot();apply(previous);previous=now;}
bool Processor::canUndo() const {std::lock_guard guard(stateMutex);return undoAvailable;}
void Processor::compare(){std::lock_guard guard(stateMutex);ab[comparisonB?1:0]=metadata;ab[comparisonB?1:0].patch=snapshot();comparisonB=!comparisonB;apply(ab[comparisonB?1:0]);}
bool Processor::isB() const {std::lock_guard guard(stateMutex);return comparisonB;}
void Processor::setLocks(unsigned v){std::lock_guard guard(stateMutex);locks=v&15;}
unsigned Processor::getLocks() const {std::lock_guard guard(stateMutex);return locks;}
void Processor::getStateInformation(juce::MemoryBlock& dest) {
    std::lock_guard guard(stateMutex);auto p=metadata;p.patch=snapshot();auto state=presetJson(p);
    auto* obj=state.getDynamicObject();obj->setProperty("format","SIDLabHostState");obj->setProperty("visualStyle",visualStyle.load());obj->setProperty("locks",int(locks));
    obj->setProperty("abA",presetJson(ab[0]));obj->setProperty("abB",presetJson(ab[1]));obj->setProperty("comparisonB",comparisonB);
    obj->setProperty("undoAvailable",undoAvailable);obj->setProperty("previous",presetJson(previous));
    juce::Array<juce::var> bindings;for(const auto& binding:knobBindings)bindings.add(binding.load());obj->setProperty("midiKnobs",juce::var(bindings));
    const auto text=juce::JSON::toString(state,true);dest.replaceAll(text.toRawUTF8(),text.getNumBytesAsUTF8());
}
void Processor::setStateInformation(const void* data,int bytes) {
    if(bytes<=0||bytes>1024*1024)return;
    auto state=juce::JSON::parse(juce::String::fromUTF8(static_cast<const char*>(data),bytes));
    if(!state.isObject()||state["format"].toString()!="SIDLabHostState")return;
    state.getDynamicObject()->setProperty("format","SIDLabPreset");Preset p;juce::String error;
    if(!readPreset(state,p,error))return;
    cancelMidiLearn();
    std::array<int,8> bindings=defaultKnobCC;bool valid=true;
    const auto savedBindings=state["midiKnobs"];
    if(const auto* saved=savedBindings.getArray()) {
        valid=saved->size()==int(bindings.size());
        for(size_t i=0;valid&&i<bindings.size();++i){const auto& entry=saved->getReference(int(i));valid=entry.isInt()&&validMidiBinding(int(entry));bindings[i]=int(entry);for(size_t j=0;j<i;++j)valid=valid&&!overlappingMidiBindings(bindings[i],bindings[j]);}
    }
    if(!valid)bindings=defaultKnobCC;
    std::lock_guard guard(stateMutex);
    for(size_t i=0;i<bindings.size();++i)knobBindings[i].store(bindings[i],std::memory_order_release);
    apply(p);visualStyle.store(juce::jlimit(0,2,int(state["visualStyle"])));locks=unsigned(int(state["locks"]))&15;
    comparisonB=bool(state["comparisonB"]);undoAvailable=bool(state["undoAvailable"]);
    if(!readPreset(state["abA"],ab[0],error))ab[0]=p;
    if(!readPreset(state["abB"],ab[1],error))ab[1]=p;
    if(!readPreset(state["previous"],previous,error)){previous=p;undoAvailable=false;}
}
void Processor::enqueue(MidiEvent e) noexcept {
    const auto wr=midiWrite.load(std::memory_order_relaxed),rd=midiRead.load(std::memory_order_acquire);
    if(wr-rd>=guiMidi.size()){panicRequested.store(true);return;}
    guiMidi[wr%guiMidi.size()]=e;midiWrite.store(wr+1,std::memory_order_release);
}
void Processor::handleNoteOn(juce::MidiKeyboardState*,int ch,int note,float velocity){enqueue({uint8_t(0x90|((ch-1)&15)),uint8_t(note),uint8_t(juce::jlimit(1,127,int(velocity*127)))});}
void Processor::handleNoteOff(juce::MidiKeyboardState*,int ch,int note,float){enqueue({uint8_t(0x80|((ch-1)&15)),uint8_t(note),0});}
void Processor::timerCallback() {
    finishMidiLearn();
    if(const int program=requestedProgram.exchange(-1);program>=0)loadFactory(program);
    for(size_t i=0;i<pendingCC.size();++i) {
        float value=pendingCC[i].load(std::memory_order_acquire);
        if(value>=0){setValue(int(i),value);pendingCC[i].compare_exchange_strong(value,-1);}
    }
}
int Processor::midiKnobBinding(int knob) const noexcept {return knob>=0&&knob<8?knobBindings[size_t(knob)].load():-1;}
void Processor::armMidiLearn() {learnRequest.store((learnGeneration<<8)|unsigned(learningKnob+1),std::memory_order_release);}
void Processor::beginMidiLearn(int knob) {
    cancelMidiLearn();learningAll=knob<0;learningKnob=juce::jlimit(0,7,knob);learningDraft.fill(-1);
    learningActive.store(true,std::memory_order_release);armMidiLearn();
}
void Processor::cancelMidiLearn() {
    learningActive.store(false,std::memory_order_release);++learnGeneration;if(learnGeneration>=0x1000000)learnGeneration=1;
    learnRequest.store(0,std::memory_order_release);learnResult.store(0,std::memory_order_release);
}
void Processor::resetMidiKnobs() {
    cancelMidiLearn();{std::lock_guard guard(stateMutex);for(size_t i=0;i<knobBindings.size();++i)knobBindings[i].store(defaultKnobCC[i],std::memory_order_release);}
    updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
}
bool Processor::captureMidiLearn(int binding) noexcept {
    if(!learningActive.load(std::memory_order_acquire)||!learnableCC(binding%128))return false;
    auto request=learnRequest.load(std::memory_order_acquire);
    if(request!=0) {
        for(const auto& ignored:learnIgnore){const auto old=ignored.load(std::memory_order_acquire);if(unsigned(old>>16)==request>>8&&int(old&0xffff)==binding)return true;}
        if(learnRequest.compare_exchange_strong(request,0,std::memory_order_acq_rel))learnResult.store((uint64_t(request)<<16)|unsigned(binding),std::memory_order_release);
    }
    return true;
}
void Processor::finishMidiLearn() {
    const auto result=learnResult.exchange(0,std::memory_order_acq_rel);const auto request=unsigned(result>>16);
    if(result==0||!learningActive.load()||(request>>8)!=learnGeneration)return;
    const int binding=int(result&0xffff);learningDraft[size_t(learningKnob)]=binding;
    learnIgnore[size_t(learningKnob)].store((uint64_t(learnGeneration)<<16)|unsigned(binding),std::memory_order_release);
    if(learningAll&&learningKnob<7){++learningKnob;armMidiLearn();return;}
    {
        std::lock_guard guard(stateMutex);
        if(!learningActive.load()||(request>>8)!=learnGeneration)return;
        if(learningAll)for(size_t i=0;i<knobBindings.size();++i)knobBindings[i].store(learningDraft[i],std::memory_order_release);
        else {
            const int previous=knobBindings[size_t(learningKnob)].load();
            for(size_t i=0;i<knobBindings.size();++i)if(int(i)!=learningKnob&&overlappingMidiBindings(knobBindings[i].load(),binding))knobBindings[i].store(previous,std::memory_order_release);
            knobBindings[size_t(learningKnob)].store(binding,std::memory_order_release);
        }
    }
    cancelMidiLearn();updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
}
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
} // namespace sidlab
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new sidlab::Processor();}
