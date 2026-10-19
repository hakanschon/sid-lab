// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "AllocationGuard.h"
#include <juce_dsp/juce_dsp.h>
#include <CoreFoundation/CoreFoundation.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <thread>
#include <set>
using namespace sidlab;
namespace {
int checks=0,failures=0;
void check(bool condition,std::string_view message){++checks;if(!condition){++failures;std::cerr<<"FAIL: "<<message<<"\n";}}
void section(const char* name){std::cout<<"TEST "<<name<<"\n";}
struct Audio {std::vector<float> l,r;double rms=0,peak=0;};
Audio render(SidEngine& e,int n){Audio a;a.l.resize(size_t(n));a.r.resize(size_t(n));e.render(a.l.data(),a.r.data(),n);double sum=0;for(int i=0;i<n;++i){check(std::isfinite(a.l[size_t(i)])&&std::isfinite(a.r[size_t(i)]),"finite sample");sum+=a.l[size_t(i)]*a.l[size_t(i)];a.peak=std::max(a.peak,double(std::abs(a.l[size_t(i)])));}a.rms=std::sqrt(sum/n);return a;}
bool nearPatch(const Patch& a,const Patch& b){const auto& specs=parameterSpecs();for(int i=0;i<parameterCount;++i)if(std::abs(a[i]-b[i])>1e-5f*std::max(1.0f,specs[i].max-specs[i].min))return false;return true;}
void checkPatch(const Patch& p){const auto& s=parameterSpecs();for(int i=0;i<parameterCount;++i)check(std::isfinite(p[i])&&p[i]>=s[i].min&&p[i]<=s[i].max,"parameter bounds "+s[i].id);}
void reset(SidEngine& e,Patch p,double rate=48000){e.setPatch(p);e.prepare(rate);}
void wav(const juce::File& file,const juce::AudioBuffer<float>& audio,double sr){
    juce::WavAudioFormat format;auto fileStream=file.createOutputStream();check(bool(fileStream),"WAV output stream");
    if(!fileStream)return;fileStream->setPosition(0);fileStream->truncate();
    std::unique_ptr<juce::OutputStream> stream=std::move(fileStream);
    juce::AudioFormatWriterOptions options;options=options.withSampleRate(sr).withNumChannels(2).withBitsPerSample(24);
    auto writer=format.createWriterFor(stream,options);check(bool(writer),"WAV writer");if(writer)check(writer->writeFromAudioSampleBuffer(audio,0,audio.getNumSamples()),"WAV data");
}
void voicingTests(SidEngine& engine,const juce::File& evidence,int firstPreset,const char* bank,const char* filename,int count=16){
    section("bank timbre comparison with identical note, no arp and normalized spectral energy");std::cout<<"BANK "<<bank<<"\n";
    juce::dsp::FFT fft(11);std::array<float,4096> data{};
    std::vector<std::array<double,11>> profiles(size_t(count),std::array<double,11>{});
    std::vector<double> centroids(size_t(count),0),tailRatios(size_t(count),0);
    juce::AudioBuffer<float> comparison(2,count*120000);comparison.clear();
    for(int patch=0;patch<count;++patch){
        auto p=factoryPresets()[size_t(firstPreset+patch)].patch;
        // Remove pattern, division, gate and loudness differences from the check.
        p[Arp]=0;p[ArpRate]=1;p[ArpGate]=1;p[Master]=.7f;
        reset(engine,p);engine.midi({0x90,60,110});auto tone=render(engine,48000);
        std::array<double,8> energy{};double weighted=0,total=0;
        constexpr double edges[]{150,350,800,1800,4000,9000,16000};
        for(int frame=0;frame+2048<=48000;frame+=1024){
            data.fill(0);float mean=0;for(int i=0;i<2048;++i)mean+=tone.l[size_t(frame+i)]/2048.0f;
            for(int i=0;i<2048;++i)data[size_t(i)]=(tone.l[size_t(frame+i)]-mean)*(.5f-.5f*std::cos(2*juce::MathConstants<float>::pi*i/2047));
            fft.performFrequencyOnlyForwardTransform(data.data(),true);
            for(int bin=2;bin<1024;++bin){const double hz=bin*48000.0/2048,e=double(data[size_t(bin)])*data[size_t(bin)];int band=0;while(band<7&&hz>edges[band])++band;energy[size_t(band)]+=e;weighted+=e*hz;total+=e;}
        }
        check(total>1e-8,"each bank recipe has actual AC signal under the same held note");
        for(int band=0;band<8;++band)profiles[size_t(patch)][size_t(band)]=std::sqrt(energy[size_t(band)]/std::max(total,1e-20));
        centroids[size_t(patch)]=weighted/std::max(total,1e-20);
        double acTotal=0,late=0,first=0,middle=0,dcEstimate=0;
        for(size_t i=0;i<tone.l.size();++i){dcEstimate+=(tone.l[i]-dcEstimate)*.0026;const double x=tone.l[i]-dcEstimate,e=x*x;acTotal+=e;if(i<4800)first+=e;else if(i<19200)middle+=e;else late+=e;}
        profiles[size_t(patch)][8]=std::sqrt(first/std::max(acTotal,1e-20));profiles[size_t(patch)][9]=std::sqrt(middle/std::max(acTotal,1e-20));profiles[size_t(patch)][10]=std::sqrt(late/std::max(acTotal,1e-20));tailRatios[size_t(patch)]=late/std::max(acTotal,1e-20);
        std::cout<<"TIMBRE "<<factoryPresets()[size_t(firstPreset+patch)].name<<" centroid_hz="<<centroids[size_t(patch)]<<" late_energy_fraction="<<tailRatios[size_t(patch)]<<"\n";
        // A second render lets the user compare the actual sequencer recipes.
        reset(engine,factoryPresets()[size_t(firstPreset+patch)].patch);engine.midi({0x90,60,110});auto seq=render(engine,96000);
        float dc=0;for(float x:seq.l)dc+=x/96000.0f;double sum=0,peak=0;for(float x:seq.l){sum+=(x-dc)*(x-dc);peak=std::max(peak,double(std::abs(x-dc)));}
        const double gain=std::min(.09/std::max(std::sqrt(sum/96000),1e-8),.7/std::max(peak,1e-8));
        for(int i=0;i<96000;++i){const float fade=std::min({1.0f,i/240.0f,(95999-i)/240.0f});for(int ch=0;ch<2;++ch)comparison.setSample(ch,patch*120000+i,(seq.l[size_t(i)]-dc)*float(gain)*fade);}
    }
    double closest=100;
    for(size_t a=0;a<profiles.size();++a)for(size_t b=a+1;b<profiles.size();++b){double distance=0;for(size_t k=0;k<11;++k){const double d=profiles[a][k]-profiles[b][k];distance+=d*d;}distance=std::sqrt(distance);closest=std::min(closest,distance);if(distance<.1)std::cout<<"CLOSE_TIMBRES "<<a<<" "<<b<<" distance="<<distance<<"\n";check(distance>=.1,"bank recipes differ in normalized spectrum/envelope with pattern disabled");}
    const auto [low,high]=std::minmax_element(centroids.begin(),centroids.end());const auto [shortest,longest]=std::minmax_element(tailRatios.begin(),tailRatios.end());
    check(*high>*low*4,"bank spans dark and bright spectra independently of gain");check(*longest-*shortest>.3,"bank spans percussive and sustained envelopes");
    std::cout<<"METRIC "<<bank<<" normalized nearest_distance="<<closest<<" centroid_range="<<*low<<".."<<*high<<" tail_fraction_range="<<*shortest<<".."<<*longest<<"\n";
    wav(evidence.getChildFile(filename),comparison,48000);
}
// Native run-loop calls may return before their requested timeout. Wait for
// the observable message-thread result instead of assuming one pump was enough.
template <typename Predicate> bool waitForMessages(Predicate predicate){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while(!predicate()&&std::chrono::steady_clock::now()<deadline){
        CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.005,false);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return predicate();
}
void midiKnobTests(const juce::File& evidence){
    section("1.5 eight rotary controls, MIDI Learn and project recall");
    Processor p;p.prepareToPlay(48000,256);juce::AudioBuffer<float> block(2,256);juce::MidiBuffer midi;midi.ensureSize(2048);
    auto cc=[&](int channel,int number,int value){midi.addEvent(juce::MidiMessage::controllerEvent(channel,number,value),73);startAllocationGuard();p.processBlock(block,midi);const auto allocations=stopAllocationGuard();check(allocations==0,"knob and learn callbacks allocate no heap memory");};
    // Pump the native message loop so the same timer that updates a real editor
    // publishes MIDI values and advances the learning wizard.
    auto timers=[] {
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(80);
        while(std::chrono::steady_clock::now()<deadline){
            CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.005,false);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };
    for(size_t knob=0;knob<midiKnobParams.size();++knob)for(int value:{0,63,127}){
        const auto before=p.snapshot();cc(16,defaultKnobCC[knob],value);auto expected=before;
        const auto& s=parameterSpecs()[midiKnobParams[knob]];expected[size_t(midiKnobParams[knob])]=s.min+(s.max-s.min)*value/127.0f;
        check(nearPatch(p.snapshot(),sanitize(expected)),"all eight default controllers change exactly their intended parameter at both endpoints and midpoint");
    }
    waitForMessages([&]{for(int parameter:midiKnobParams)if(std::abs(p.parameters.getRawParameterValue(parameterSpecs()[parameter].id)->load()-p.snapshot()[parameter])>=.001f)return false;return true;});
    for(int parameter:midiKnobParams)check(std::abs(p.parameters.getRawParameterValue(parameterSpecs()[parameter].id)->load()-p.snapshot()[parameter])<.001f,"MIDI changes publish to visible/host parameters");
    for(int value=0;value<128;++value)midi.addEvent(juce::MidiMessage::controllerEvent(1,74,value),value);
    startAllocationGuard();p.processBlock(block,midi);check(stopAllocationGuard()==0&&p.snapshot()[Cutoff]==2047,"dense rotary sweeps retain the last value without callback allocation");
    cc(1,1,37);check(std::abs(p.snapshot()[Movement]-37.0f/127)<.0001f,"modulation wheel retains its legacy movement mapping");cc(1,7,83);check(std::abs(p.snapshot()[Master]-83.0f/127)<.0001f,"volume CC retains its legacy master mapping");
    const auto oldPatch=p.snapshot();p.beginMidiLearn();
    for(int reserved:{0,6,32,38,64,96,97,98,99,100,101,120,121,123})cc(1,reserved,0);
    timers();check(p.midiLearnKnob()==0,"pedals, banks, data entry and note-control commands do not steal a knob assignment");
    // Non-default controller/channel combinations model a custom controller preset.
    for(int knob=0;knob<8;++knob){
        cc(3,20+knob,50);check(waitForMessages([&]{return p.midiLearnKnob()==(knob==7?-1:knob+1);}),"learn wizard advances once for each distinct rotary control");
        if(knob<7){cc(3,20+knob,51);timers();check(p.midiLearnKnob()==knob+1,"continued movement of the previous knob cannot capture the next slot");check(p.midiKnobBinding(knob)==defaultKnobCC[size_t(knob)],"partial learning does not replace the active map");}
    }
    check(nearPatch(p.snapshot(),oldPatch),"learning eight controls does not alter the sound");
    for(int knob=0;knob<8;++knob){
        check(p.midiKnobBinding(knob)==midiBinding(3,20+knob),"wizard commits the complete learned channel/CC map");
        const auto before=p.snapshot();cc(2,20+knob,127);check(nearPatch(p.snapshot(),before),"learned controls ignore the same CC on a different channel");
        cc(3,20+knob,29+knob*9);auto expected=before;const int parameter=midiKnobParams[size_t(knob)];const auto& s=parameterSpecs()[parameter];expected[parameter]=s.min+(s.max-s.min)*(29+knob*9)/127.0f;
        check(nearPatch(p.currentPreset().patch,sanitize(expected)),"learned knobs affect saved audible values before GUI synchronization");
    }
    const auto learnedPatch=p.snapshot();juce::MemoryBlock state;p.getStateInformation(state);Processor restored;restored.setStateInformation(state.getData(),int(state.getSize()));
    check(nearPatch(restored.snapshot(),learnedPatch),"project recall preserves the complete edited sound");for(int knob=0;knob<8;++knob)check(restored.midiKnobBinding(knob)==midiBinding(3,20+knob),"project recall preserves every rotary assignment");
    p.loadFactory(96);p.generate(8,1234,false);p.undo();p.compare();for(int knob=0;knob<8;++knob)check(p.midiKnobBinding(knob)==midiBinding(3,20+knob),"preset/discovery/A-B actions retain controller setup");
    p.beginMidiLearn();cc(5,40,80);timers();p.cancelMidiLearn();for(int knob=0;knob<8;++knob)check(p.midiKnobBinding(knob)==midiBinding(3,20+knob),"cancel keeps all previous assignments");
    p.beginMidiLearn(2);cc(4,44,80);p.cancelMidiLearn();timers();check(p.midiKnobBinding(2)==midiBinding(3,22),"cancel rejects an already queued learning result");
    p.beginMidiLearn(0);cc(3,21,80);check(waitForMessages([&]{return p.midiLearnKnob()==-1&&p.midiKnobBinding(0)==midiBinding(3,21)&&p.midiKnobBinding(1)==midiBinding(3,20);}),"reusing an assigned control swaps its old binding instead of creating duplicates");
    p.resetMidiKnobs();for(int knob=0;knob<8;++knob)check(p.midiKnobBinding(knob)==defaultKnobCC[size_t(knob)],"standard assignments can be restored");
    auto saved=juce::JSON::parse(juce::String::fromUTF8(static_cast<const char*>(state.getData()),int(state.getSize())));
    auto restoreJson=[&]{const auto text=juce::JSON::toString(saved);restored.setStateInformation(text.toRawUTF8(),text.getNumBytesAsUTF8());};
    saved.getDynamicObject()->removeProperty("midiKnobs");restoreJson();check(restored.midiKnobBinding(0)==74&&restored.midiKnobBinding(1)==71,"older projects receive compatible standard mappings");
    saved.getDynamicObject()->setProperty("midiKnobs",juce::var(juce::Array<juce::var>{74,74,70,76,73,72,77,7}));restoreJson();check(restored.midiKnobBinding(1)==71,"duplicate stored controller bindings fall back to defaults");
    saved.getDynamicObject()->setProperty("midiKnobs",juce::var(juce::Array<juce::var>{64,71,70,76,73,72,77,7}));restoreJson();check(restored.midiKnobBinding(0)==74,"reserved stored controller bindings fall back to defaults");
    saved.getDynamicObject()->setProperty("midiKnobs",juce::var(juce::Array<juce::var>{74,71}));restoreJson();check(restored.midiKnobBinding(0)==74,"incomplete maps cannot create partially assigned projects");
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());bool visible=false;
    for(auto* child:editor->getChildren())if(child->getComponentID()=="midiKnobs")visible=child->isVisible()&&!dynamic_cast<juce::TextButton*>(child)->getTooltip().isEmpty();
    check(visible,"eight-knob setup is visible with instructions in the actual editor");
    auto capture=[&](const char* name){auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);juce::PNGImageFormat png;auto stream=evidence.getChildFile(name).createOutputStream();if(stream){stream->setPosition(0);stream->truncate();check(png.writeImageToStream(image,*stream),"eight-knob editor capture writes");}};
    timers();capture("midi-knob-mode.png");p.beginMidiLearn();
    const bool prompted=waitForMessages([&]{for(auto* child:editor->getChildren())if(child->getComponentID()=="midiKnobs")return dynamic_cast<juce::TextButton*>(child)->getButtonText()=="TURN KNOB 1";return false;});
    check(prompted,"real editor shows the physical knob requested by the learning wizard");capture("midi-learn-mode.png");p.cancelMidiLearn();
}
void presetWorkflowTests(const juce::File& evidence){
    section("1.4 randomized and edited user-preset export/import");
    Processor source;source.loadFactory(96);const auto beforeDiscovery=source.snapshot();source.generate(8,93473,false);
    source.setValue(FilterEnvAmount,735);source.setValue(FilterEnvDecay,245);source.setValue(vp(0,Fine),7.25f);source.setLocks(9);
    source.prepareToPlay(48000,256);juce::AudioBuffer<float> b(2,256);juce::MidiBuffer midi;midi.ensureSize(1024);
    midi.addEvent(juce::MidiMessage::controllerEvent(1,74,82),0);midi.addEvent(juce::MidiMessage::controllerEvent(1,71,45),0);midi.addEvent(juce::MidiMessage::controllerEvent(1,1,63),0);midi.addEvent(juce::MidiMessage::controllerEvent(1,7,99),0);source.processBlock(b,midi);
    auto captured=source.currentPreset();captured.name="Randomized and Edited Keeper";
    auto expected=captured.patch;expected[Cutoff]=2047.0f*82/127;expected[Resonance]=15.0f*45/127;expected[Movement]=63.0f/127;expected[Master]=99.0f/127;expected=sanitize(expected);
    check(captured.patch==expected,"export snapshot includes audible MIDI CCs before GUI timer synchronization");
    // Host normalization uses float values; file recall below still checks the captured values exactly.
    check(std::abs(captured.patch[FilterEnvAmount]-735)<.001f&&std::abs(captured.patch[FilterEnvDecay]-245)<.001f&&std::abs(captured.patch[vp(0,Fine)]-7.25f)<.0001f,"manual edits are captured with randomized values");
    // A later controller event represents continued playing while the native save dialog is open.
    midi.addEvent(juce::MidiMessage::controllerEvent(1,74,24),0);source.processBlock(b,midi);const auto duringDialog=source.snapshot();
    const auto file=evidence.getChildFile("user-preset-roundtrip.sidlab.json");check(savePresetFile(file,captured).wasOk(),"edited discovery exports to a portable preset file");
    check(source.snapshot()==duringDialog&&source.getLocks()==9&&source.canUndo(),"export does not change the live sound or discovery history");
    source.undo();check(nearPatch(source.snapshot(),beforeDiscovery),"discovery undo remains usable after export");
    Preset imported;check(loadPresetFile(file,imported).wasOk(),"exported discovery imports from disk");
    Processor fresh;fresh.load(imported);check(fresh.snapshot()==captured.patch,"fresh instance restores all sixty-four saved values");
    check(fresh.currentPreset().name==captured.name&&fresh.currentPreset().seed==93473&&fresh.currentPreset().category==8,"import restores user name, seed and category");
    check(imported.patch[Cutoff]!=duringDialog[Cutoff],"export preserves captured sound despite later controller movement");
    auto updated=imported;updated.patch[FilterEnvAmount]=-320;updated.patch[ArpGate]=.37f;check(savePresetFile(file,updated).wasOk(),"user preset can be overwritten with a new edited version");
    check(loadPresetFile(file,imported).wasOk()&&imported.patch==updated.patch,"overwrite replaces the file with the complete updated sound");
    const auto invalid=evidence.getChildFile("invalid-user-preset.json");check(invalid.replaceWithText("{\"format\":\"not a SID Lab preset\"}"),"invalid-import fixture writes");const auto unchanged=fresh.snapshot();
    check(loadPresetFile(invalid,imported).failed()&&fresh.snapshot()==unchanged,"failed import preserves the active sound");invalid.deleteFile();
    // A real editor render checks the visible import/export affordances.
    fresh.loadFactory(96);fresh.setVisualStyle(2);fresh.prepareToPlay(48000,256);midi.addEvent(juce::MidiMessage::noteOn(1,67,(juce::uint8)110),0);for(int pass=0;pass<32;++pass)fresh.processBlock(b,midi);
    std::unique_ptr<juce::AudioProcessorEditor> editor(fresh.createEditor());bool save=false,open=false;
    for(auto* child:editor->getChildren())if(auto* button=dynamic_cast<juce::TextButton*>(child)){save|=button->getButtonText()=="SAVE / EXPORT"&&!button->getTooltip().isEmpty();open|=button->getButtonText()=="IMPORT PRESET"&&!button->getTooltip().isEmpty();}
    check(save&&open,"preset transfer actions have explicit visible labels and help");
    auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);juce::PNGImageFormat png;auto stream=evidence.getChildFile("melody-preset-mode.png").createOutputStream();if(stream){stream->setPosition(0);stream->truncate();check(png.writeImageToStream(image,*stream),"Melody/import-export editor capture writes");}
}
void synthpopIntegrationTests(const juce::File& evidence){
    section("1.3 synth-pop category, recall, polyphony and callback allocation");
    Processor pop,recall;pop.loadFactory(80);pop.setVisualStyle(1);pop.prepareToPlay(48000,256);
    check(categories.size()==9&&pop.currentPreset().category==8,"new category is appended after existing categories");
    juce::MemoryBlock state;pop.getStateInformation(state);recall.setStateInformation(state.getData(),int(state.getSize()));
    check(nearPatch(recall.snapshot(),pop.snapshot())&&recall.currentPreset().category==8&&recall.visualStyle.load()==1,"new category and complete sound restore through host state");
    juce::AudioBuffer<float> warm(2,256);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)110),0);for(int pass=0;pass<16;++pass)pop.processBlock(warm,midi);
    check(pop.engine.debugRegister(0,0)+(pop.engine.debugRegister(0,1)<<8)==frequencyRegister(48-.02,palClock),"Japan bass root uses its low oscillator");
    check(pop.engine.debugRegister(0,7)+(pop.engine.debugRegister(0,8)<<8)==frequencyRegister(55+.02,palClock),"Japan bass second oscillator is a fifth above the root");
    {std::unique_ptr<juce::AudioProcessorEditor> editor(pop.createEditor());juce::ComboBox* category=nullptr;juce::ComboBox* browser=nullptr;juce::TextButton* previous=nullptr;juce::TextButton* next=nullptr;
        for(auto* child:editor->getChildren()){
            if(child->getComponentID()=="categoryBrowser")category=dynamic_cast<juce::ComboBox*>(child);
            if(child->getComponentID()=="presetBrowser")browser=dynamic_cast<juce::ComboBox*>(child);
            if(auto* button=dynamic_cast<juce::TextButton*>(child)){if(button->getButtonText()=="<")previous=button;if(button->getButtonText()==">")next=button;}
        }
        check(category&&browser&&previous&&next,"category filter and preset arrows exist");
        if(category&&browser&&previous&&next){category->setSelectedId(10,juce::sendNotificationSync);check(browser->getNumItems()==24,"80s category exposes all twenty-four patches");
            auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);juce::PNGImageFormat png;auto stream=evidence.getChildFile("synthpop-mode.png").createOutputStream();if(stream){stream->setPosition(0);stream->truncate();check(png.writeImageToStream(image,*stream),"synth-pop category editor capture writes");}
            next->onClick();check(pop.getCurrentProgram()==81&&pop.currentPreset().name=="Tokyo Neon Pluck","next arrow loads the next new-category sound");previous->onClick();previous->onClick();check(pop.getCurrentProgram()==103,"previous arrow wraps within the new category");next->onClick();check(pop.getCurrentProgram()==80,"next arrow wraps within the new category");
        }
    }
    uint64_t allocations=0;
    for(int preset=80;preset<104;++preset){
        Preset parsed;juce::String error;check(readPreset(presetJson(factoryPresets()[size_t(preset)]),parsed,error)&&parsed.category==8&&nearPatch(parsed.patch,factoryPresets()[size_t(preset)].patch),"new-category preset JSON round trip");
        for(double rate:{44100.0,48000.0,96000.0})for(int blockSize:{64,256,1024}){
            pop.loadFactory(preset);pop.prepareToPlay(rate,blockSize);juce::AudioBuffer<float> block(2,blockSize);juce::MidiBuffer m;m.ensureSize(2048);
            for(int pass=0;pass<8;++pass){m.clear();if(pass==0)for(int note:{48,52,55,59})m.addEvent(juce::MidiMessage::noteOn(1,note,(juce::uint8)110),blockSize/4);if(pass==5)m.addEvent(juce::MidiMessage::allNotesOff(1),blockSize/2);
                startAllocationGuard();pop.processBlock(block,m);allocations+=stopAllocationGuard();check(block.getMagnitude(0,blockSize)<1,"new-category rate/buffer renders remain unclipped");if(pass==0)check(pop.engine.soundingVoices()==(pop.snapshot()[PlayMode]==0?1:4),"new voices honor mono/poly architecture");
            }
        }
    }
    check(allocations==0,"new-category callbacks allocate no heap memory");std::cout<<"METRIC synthpop callback heap calls="<<allocations<<"\n";
    // Original chord progression, rendered through the processor, demonstrates the string patch.
    pop.loadFactory(83);pop.prepareToPlay(48000,256);juce::AudioBuffer<float> demo(2,384000),block(2,256);juce::MidiBuffer m;m.ensureSize(4096);
    constexpr int chords[4][3]{{48,51,55},{53,56,60},{55,59,62},{56,60,63}};
    for(int offset=0;offset<384000;offset+=256){const int count=std::min(256,384000-offset);block.setSize(2,count,false,false,true);m.clear();
        for(int chord=0;chord<4;++chord)for(int note:chords[chord]){const int on=chord*96000+73,off=on+84000;if(on>=offset&&on<offset+count)m.addEvent(juce::MidiMessage::noteOn(1,note,(juce::uint8)105),on-offset);if(off>=offset&&off<offset+count)m.addEvent(juce::MidiMessage::noteOff(1,note),off-offset);}
        pop.processBlock(block,m);for(int channel=0;channel<2;++channel)demo.copyFrom(channel,offset,block,channel,0,count);
    }
    check(demo.getRMSLevel(0,0,384000)>.0005f&&demo.getMagnitude(0,384000)<1,"synth-pop string chords are audible and unclipped");
    const auto file=evidence.getChildFile("sid-lab-synthpop-chords.wav");wav(file,demo,48000);juce::WavAudioFormat format;std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(file.createInputStream().release(),true));check(reader&&reader->lengthInSamples==384000&&reader->sampleRate==48000,"synth-pop chord demo reopens with correct duration and rate");
}
}
int main(int argc,char** argv){
    juce::ScopedJuceInitialiser_GUI init;
    const auto evidence=argc>1?juce::File(argv[1]):juce::File::getCurrentWorkingDirectory().getChildFile("evidence");evidence.createDirectory();
    section("1 frequency conversion");
    check(frequencyRegister(69,palClock)==7493,"PAL A4 exact register");check(frequencyRegister(69,ntscClock)==7218,"NTSC A4 exact register");
    for(double clock:{palClock,ntscClock})for(int note=24;note<105;++note){const auto reg=frequencyRegister(note,clock);const double hz=reg*clock/16777216.0,expected=440*std::exp2((note-69)/12.0);check(std::abs(hz-expected)<=clock/33554432.0+1e-8,"frequency rounding tolerance");}
    section("2 gate ADSR and release");SidEngine engine;auto basic=defaultPatch();basic[PlayMode]=0;basic[Model]=1;basic[FilterMode]=0;basic[vp(0,Route)]=0;basic[Movement]=0;basic[vp(0,Sustain)]=15;basic[vp(0,VRelease)]=2;
    reset(engine,basic);engine.midi({0x90,69,110});check((engine.debugRegister(0,4)&1)==1,"gate on");check(engine.debugRegister(0,6)==0xf2,"sustain/release packing");const auto basicAudio=render(engine,24000);check(basicAudio.rms>0.001,"SID has meaningful RMS");engine.midi({0x80,69,0});check((engine.debugRegister(0,4)&1)==0,"gate off");auto released=render(engine,48000);check(engine.soundingVoices()==0,"release retires chip");double tail=0;for(size_t i=released.l.size()-4000;i<released.l.size();++i)tail+=released.l[i]*released.l[i];check(tail<1e-9,"release returns to silence");
    section("3 parameter bounds and invalid values");auto bad=basic;for(int i=0;i<parameterCount;++i)bad[i]=i%2?1e20f:-1e20f;checkPatch(sanitize(bad));bad.fill(std::numeric_limits<float>::quiet_NaN());checkPatch(sanitize(bad));
    section("4 shared filter register mapping");auto filter=basic;filter[vp(0,Route)]=1;filter[vp(1,Route)]=0;filter[vp(2,Route)]=1;const auto regs=filterRegisters(filter,0x5ab,9);check(regs[0]==3&&regs[1]==0xb5&&regs[2]==0x95,"11-bit cutoff, resonance and three routing bits");check(waveformBits(7)==0x70&&waveformBits(3)==0x80,"waveform bit mapping");
    section("5 real chip audio and 6 distinct models");
    auto modelPatch=basic;modelPatch[FilterMode]=1;modelPatch[Cutoff]=1000;modelPatch[Resonance]=8;modelPatch[vp(0,Route)]=1;modelPatch[Model]=0;
    reset(engine,modelPatch);engine.midi({0x90,57,110});auto a6581=render(engine,24000);modelPatch[Model]=1;reset(engine,modelPatch);engine.midi({0x90,57,110});auto a8580=render(engine,24000);double modelDifference=0;for(size_t i=4000;i<a6581.l.size();++i){const double d=a6581.l[i]-a8580.l[i];modelDifference+=d*d;}modelDifference=std::sqrt(modelDifference/20000);check(a6581.rms>0.0001&&a8580.rms>0.0001,"both SID models produce audio");check(modelDifference>0.0005,"chip models differ through emulation");std::cout<<"METRIC model RMS 6581="<<a6581.rms<<" 8580="<<a8580.rms<<" difference="<<modelDifference<<"\n";
    section("7 pulse width");auto pulse=basic;pulse[vp(0,Wave)]=2;pulse[vp(0,Pulse)]=0.2f;reset(engine,pulse);engine.midi({0x90,60,100});auto narrow=render(engine,12000);check((engine.debugRegister(0,3)<<8|engine.debugRegister(0,2))==819,"12-bit pulse width packing");pulse[vp(0,Pulse)]=0.5f;reset(engine,pulse);engine.midi({0x90,60,100});auto square=render(engine,12000);double pwDiff=0;for(size_t i=0;i<square.l.size();++i)pwDiff+=std::abs(square.l[i]-narrow.l[i]);check(pwDiff>1,"pulse width changes actual audio");
    section("8 randomizer validity reproducibility and 9 locks");
    for(int cat=0;cat<int(categories.size());++cat)for(uint64_t seed=1;seed<=40;++seed){const auto p=randomize(basic,cat,seed);checkPatch(p);check(p==randomize(basic,cat,seed),"deterministic seeded patch");check(p[vp(0,Enabled)]==1,"musical root oscillator enabled");for(unsigned lock:{1u,2u,4u,8u,15u}){const auto locked=randomize(basic,cat,seed,lock);for(int i=0;i<parameterCount;++i)if(sectionFor(i)&lock)check(locked[i]==basic[i],"section lock preserved");}const auto mutation=randomize(p,cat,seed,0,true);checkPatch(mutation);}
    section("10 preset files and 11 plugin host state");
    Processor plugin;const auto expected=factoryPresets()[13];plugin.load(expected);plugin.setLocks(5);plugin.generate(3,123456,true);const auto generated=plugin.currentPreset();plugin.undo();check(nearPatch(plugin.snapshot(),expected.patch),"randomization undo restores patch");plugin.undo();check(nearPatch(plugin.snapshot(),generated.patch),"undo toggles for rollback");
    plugin.compare();const auto bPatch=plugin.snapshot();plugin.setValue(Cutoff,777);const auto editedB=plugin.snapshot();plugin.compare();check(nearPatch(plugin.snapshot(),generated.patch),"A/B retains A");plugin.compare();check(nearPatch(plugin.snapshot(),editedB),"A/B retains edited B");
    const auto presetFile=evidence.getChildFile("preset-roundtrip.sidlab.json");check(savePresetFile(presetFile,generated).wasOk(),"preset save");Preset loaded;check(loadPresetFile(presetFile,loaded).wasOk()&&loaded.patch==generated.patch&&loaded.seed==generated.seed,"preset JSON round trip");
    auto highSeed=generated;highSeed.seed=std::numeric_limits<uint64_t>::max();Preset highRead;juce::String highError;check(readPreset(presetJson(highSeed),highRead,highError)&&highRead.seed==highSeed.seed,"unsigned 64-bit seed round trip");
    auto invalid=presetJson(generated);invalid.getDynamicObject()->setProperty("schema",999);juce::String error;check(!readPreset(invalid,loaded,error),"future schema rejected");
    juce::MemoryBlock state;plugin.getStateInformation(state);Processor restored;restored.setStateInformation(state.getData(),int(state.getSize()));check(nearPatch(restored.snapshot(),plugin.snapshot()),"host state restores every synthesis parameter");check(restored.getLocks()==5&&restored.isB()==plugin.isB(),"host state restores locks and A/B selection");restored.compare();plugin.compare();check(nearPatch(restored.snapshot(),plugin.snapshot()),"host state restores comparison sounds");
    section("1.1 compatibility, visual state, sequencer gates and pitch sweeps");
    Preset legacy;
    check(loadPresetFile(juce::File(SIDLAB_SOURCE_DIR).getChildFile("tests/fixtures/crystal-cartridge-v1.sidlab.json"),legacy).wasOk(),"schema 1 Crystal Cartridge still loads");
    check(nearPatch(legacy.patch,factoryPresets()[11].patch),"original Crystal Cartridge values and extension defaults preserved");
    check(legacy.patch[ArpGate]==1&&legacy.patch[PitchSweep]==0,"legacy presets use open gates and zero pitch sweep");
    auto oldState=juce::JSON::parse(juce::File(SIDLAB_SOURCE_DIR).getChildFile("tests/fixtures/crystal-cartridge-v1.sidlab.json").loadFileAsString());
    oldState.getDynamicObject()->setProperty("format","SIDLabHostState");const auto oldText=juce::JSON::toString(oldState);
    restored.setStateInformation(oldText.toRawUTF8(),oldText.getNumBytesAsUTF8());
    check(nearPatch(restored.snapshot(),legacy.patch)&&restored.visualStyle.load()==0,"legacy host state restores with Trace default");
    plugin.loadFactory(73);const auto kitPatch=plugin.snapshot();plugin.setVisualStyle(2);
    check(nearPatch(plugin.snapshot(),kitPatch),"visual style leaves synthesis parameters unchanged");
    plugin.getStateInformation(state);restored.setStateInformation(state.getData(),int(state.getSize()));
    check(nearPatch(restored.snapshot(),kitPatch)&&restored.visualStyle.load()==2,"kit and Orbit restore through host state");
    check(plugin.getNumPrograms()==104&&plugin.getProgramName(79)=="Darkroom Kit"&&plugin.getProgramName(95)=="Forever Fade Strings"&&plugin.getProgramName(103)=="Melody Warm Backdrop","host exposes expanded factory bank");
    auto incomplete=presetJson(factoryPresets()[24]);incomplete["parameters"].getDynamicObject()->removeProperty("arpGate");
    check(!readPreset(incomplete,loaded,error),"current schema requires all synthesis parameters");
    auto gated=basic;gated[Arp]=5;gated[ArpGate]=0.3f;gated[ArpRate]=1;reset(engine,gated);engine.setTempo(120);engine.midi({0x90,60,110});
    render(engine,3000);check((engine.debugRegister(0,4)&1)==0,"short step gate closes inside the first sixteenth");
    render(engine,3104);check((engine.debugRegister(0,4)&1)==1,"next sixteenth retriggers SID gate");
    check((engine.debugRegister(0,1)<<8|engine.debugRegister(0,0))==frequencyRegister(67,palClock),"disco pattern advances to fifth");
    gated[ArpGate]=1;reset(engine,gated);engine.midi({0x90,60,110});render(engine,3000);check((engine.debugRegister(0,4)&1)==1,"full step length preserves continuous legacy gate");
    auto swept=basic;swept[PitchSweep]=24;swept[SweepTime]=20;reset(engine,swept);engine.midi({0x90,48,110});
    const int initialFrequency=engine.debugRegister(0,1)<<8|engine.debugRegister(0,0);render(engine,12000);
    const int settledFrequency=engine.debugRegister(0,1)<<8|engine.debugRegister(0,0);
    check(initialFrequency>settledFrequency*3&&std::abs(settledFrequency-int(frequencyRegister(48,palClock)))<=1,"pitch sweep decays into tuned SID frequency");
    check(randomize(kitPatch,0,42)[PlayMode]==1&&randomize(basic,7,42)[PlayMode]>=2,"randomizer enters and leaves kit architecture with category");
    section("1.1 SID percussion recipes, one shots, choke and callback allocations");
    check(drumForNote(36)==DrumType::Kick&&drumForNote(38)==DrumType::Snare&&drumForNote(46)==DrumType::OpenHat,"GM percussion mapping");
    check(drumForNote(60)==DrumType::Kick&&drumForNote(62)==DrumType::Snare,"controller octave fallback mapping");
    for(int kit=1;kit<=3;++kit)for(int note=0;note<128;++note)checkPatch(makeDrum(defaultPatch(),kit,note).patch);
    std::set<long long> drumSignatures;uint64_t drumAllocations=0;
    for(int kit=1;kit<=3;++kit){auto p=defaultPatch();p[PlayMode]=float(kit+1);p[Model]=1;reset(engine,p);
        for(int note:{36,38,39,42,46,41,48,37,56,49,51}){
            reset(engine,p);engine.midi({0x99,uint8_t(note),110});engine.midi({0x89,uint8_t(note),0});
            check(engine.heldNotes()==0&&engine.soundingVoices()==1,"short MIDI notes leave one-shot percussion sounding without held keys");
            auto hit=render(engine,12000);check(hit.rms>0.00002&&hit.peak<1,"every kit instrument produces finite sound with headroom");drumSignatures.insert(std::llround(hit.rms*1e10));
            render(engine,144000);check(engine.soundingVoices()==0,"percussion tail retires automatically");
        }
        reset(engine,p);engine.midi({0x99,46,110});render(engine,512);engine.midi({0x99,42,110});render(engine,8000);check(engine.soundingVoices()==0,"closed hat chokes open hat and both retire");
        reset(engine,p);for(int note:{36,38,46,56,49})engine.midi({0x99,uint8_t(note),110});check(engine.soundingVoices()==4,"drum overlap respects four-chip voice stealing");engine.panic();render(engine,4000);check(engine.soundingVoices()==0,"panic clears kit voices");
        for(double rate:{44100.0,48000.0,96000.0})for(int size:{64,128,256,512,1024}){
            restored.load({"Drum allocation",7,1,p});restored.prepareToPlay(rate,size);juce::AudioBuffer<float> drumBlock(2,size);juce::MidiBuffer dm;dm.ensureSize(4096);
            for(int pass=0;pass<16;++pass){dm.clear();dm.addEvent(juce::MidiMessage::noteOn(10,drumPadNotes[size_t(pass)%8],(juce::uint8)110),size/3);dm.addEvent(juce::MidiMessage::noteOff(10,drumPadNotes[size_t(pass)%8]),size/2);
                startAllocationGuard();restored.processBlock(drumBlock,dm);drumAllocations+=stopAllocationGuard();check(drumBlock.getMagnitude(0,size)<1,"drum matrix output headroom");}
        }
    }
    check(drumSignatures.size()>=28,"kit instruments and recipe families have distinct audio signatures");
    auto transitionKit=defaultPatch();transitionKit[PlayMode]=3;reset(engine,basic);engine.setPatch(transitionKit);engine.midi({0x99,36,110});engine.midi({0x89,36,0});
    auto transitionHit=render(engine,12000);check(transitionHit.rms>0.0001&&engine.heldNotes()==0,"first short drum hit survives architecture-change fade");
    engine.setPatch(basic);engine.midi({0x90,60,110});render(engine,512);check(engine.heldNotes()==1,"first melodic note survives leaving kit architecture");engine.midi({0x80,60,0});render(engine,24000);check(engine.heldNotes()==0&&engine.soundingVoices()==0,"transition note-off leaves no stuck voice");
    engine.setPatch(transitionKit);engine.midi({0x99,46,110});engine.panic();render(engine,12000);check(engine.soundingVoices()==0,"panic cancels queued transition hits");
    check(drumAllocations==0,"drum triggering, noteoffs, sweeps and auto-gates make zero callback heap calls");std::cout<<"METRIC drum callback heap calls="<<drumAllocations<<" distinct_hits="<<drumSignatures.size()<<"\n";
    section("1.2 filter pluck registers and v2 compatibility");
    Preset version2;check(loadPresetFile(juce::File(SIDLAB_SOURCE_DIR).getChildFile("tests/fixtures/crystal-cartridge-ii-v2.sidlab.json"),version2).wasOk(),"original schema 2 preset loads");
    check(version2.patch[FilterEnvAmount]==0&&version2.patch[FilterEnvDecay]==180,"schema 2 preserves old tone with inactive filter pluck");
    auto v2State=juce::JSON::parse(juce::File(SIDLAB_SOURCE_DIR).getChildFile("tests/fixtures/crystal-cartridge-ii-v2.sidlab.json").loadFileAsString());v2State.getDynamicObject()->setProperty("format","SIDLabHostState");auto v2Text=juce::JSON::toString(v2State);restored.setStateInformation(v2Text.toRawUTF8(),v2Text.getNumBytesAsUTF8());check(nearPatch(restored.snapshot(),version2.patch),"schema 2 project retains saved sound across factory revoicing");
    auto pluck=basic;pluck[Cutoff]=250;pluck[FilterEnvAmount]=1200;pluck[FilterEnvDecay]=30;pluck[vp(0,Route)]=1;pluck[FilterMode]=1;
    reset(engine,pluck);engine.midi({0x90,60,110});auto readCutoff=[&]{return int(engine.debugRegister(0,21))+(int(engine.debugRegister(0,22))<<3);};
    check(readCutoff()==1450,"filter pluck opens the real 11-bit SID cutoff on note-on");render(engine,6000);check(readCutoff()<275,"filter pluck decays towards base brightness");
    pluck[Arp]=4;reset(engine,pluck);engine.setTempo(120);engine.midi({0x90,60,110});render(engine,5900);const int closedCutoff=readCutoff();render(engine,200);check(closedCutoff<280&&readCutoff()>1300,"filter envelope retriggers on sequencer step");
    pluck[Arp]=0;pluck[FilterEnvAmount]=-200;reset(engine,pluck);engine.midi({0x90,60,110});check(readCutoff()==50,"negative filter pluck starts below brightness");render(engine,6000);check(readCutoff()>240,"negative filter pluck rises towards brightness");
    auto without=pluck;without[FilterEnvAmount]=0;reset(engine,without);engine.midi({0x90,60,110});render(engine,6000);check(readCutoff()==250,"zero filter pluck preserves previous cutoff behavior");
    restored.loadFactory(25);restored.prepareToPlay(48000,512);juce::AudioBuffer<float> pluckBlock(2,512);juce::MidiBuffer pluckMidi;pluckMidi.ensureSize(1024);pluckMidi.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)110),0);startAllocationGuard();restored.processBlock(pluckBlock,pluckMidi);const auto pluckAllocs=stopAllocationGuard();check(pluckAllocs==0,"filter pluck callback makes zero heap calls");
    section("12 multi-note 13 stealing sustain all-notes-off 14 repeats");
    auto poly=basic;poly[PlayMode]=1;reset(engine,poly);for(int n:{48,52,55,60,64,67})engine.midi({0x90,uint8_t(n),100});check(engine.soundingVoices()==4,"four complete chips, oldest stolen");auto chord=render(engine,12000);check(chord.rms>0.003&&chord.peak<1,"poly audio with headroom");engine.midi({0xb0,64,127});engine.midi({0x80,67,0});check(engine.heldNotes()==6,"sustain holds key");engine.midi({0xb0,64,0});check(engine.heldNotes()==5,"pedal up releases sustained key");engine.midi({0xb0,123,0});render(engine,24000);check(engine.heldNotes()==0&&engine.soundingVoices()==0,"all notes off including stolen keys");
    reset(engine,basic);engine.midi({0x90,60,100});engine.midi({0x90,60,100});engine.midi({0x80,60,0});check(engine.heldNotes()==1,"repeated notes reference counted");engine.midi({0x80,60,0});check(engine.heldNotes()==0,"matching repeated noteoffs release");
    for(int n=0;n<128;++n){engine.midi({0x90,uint8_t(40+n%40),100});engine.midi({0x80,uint8_t(40+n%40),0});}render(engine,24000);check(engine.heldNotes()==0&&engine.soundingVoices()==0,"rapid repeated sequence has no stuck notes");
    reset(engine,basic);engine.midi({0x90,60,100});engine.midi({0x90,67,100});engine.midi({0x80,67,0});check((engine.debugRegister(0,1)<<8|engine.debugRegister(0,0))==frequencyRegister(60,palClock),"mono last-note priority returns to held key");engine.panic();render(engine,4000);check(engine.heldNotes()==0&&engine.soundingVoices()==0,"panic fades and clears");
    section("15 sample rates 16 buffers offsets 19 callback allocation 20 CPU");
    uint64_t allocs=0;double worstRatio=0,worstMs=0;
    startAllocationGuard();void* calibration=::operator new(37);::operator delete(calibration);const auto calibrationCount=stopAllocationGuard();check(calibrationCount>0,"allocation trap calibration");
    for(double sr:{44100.0,48000.0,96000.0}){
        restored.load(factoryPresets()[13]);restored.prepareToPlay(sr,1024);
        for(int block:{64,128,256,512,1024}){
            juce::AudioBuffer<float> buffer(2,block);juce::MidiBuffer midi;midi.ensureSize(2048);
            midi.addEvent(juce::MidiMessage::allSoundOff(1),0);restored.processBlock(buffer,midi);
            for(int flush=0;flush<int(sr*0.05/block)+1;++flush)restored.processBlock(buffer,midi);
            midi.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)110),block/2);
            startAllocationGuard();restored.processBlock(buffer,midi);allocs+=stopAllocationGuard();
            for(int i=0;i<block/2;++i)check(std::abs(buffer.getSample(0,i))<0.001f,"MIDI offset does not sound before note");
            for(int note:{48,52,55})midi.addEvent(juce::MidiMessage::noteOn(1,note,(juce::uint8)100),0);
            restored.processBlock(buffer,midi);
            const auto start=std::chrono::steady_clock::now();
            for(int pass=0;pass<20;++pass){
                if(pass==3)midi.addEvent(juce::MidiMessage::pitchWheel(1,12000),block/3);
                if(pass==4)midi.addEvent(juce::MidiMessage::controllerEvent(1,74,110),block/4);
                startAllocationGuard();restored.processBlock(buffer,midi);allocs+=stopAllocationGuard();
                for(int c=0;c<2;++c)for(int i=0;i<block;++i)check(std::isfinite(buffer.getSample(c,i))&&std::abs(buffer.getSample(c,i))<1,"finite unclipped matrix audio");
            }
            const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            const double ratio=elapsed/(20*block/sr);worstRatio=std::max(worstRatio,ratio);worstMs=std::max(worstMs,elapsed/20*1000);
            std::cout<<"METRIC sr="<<sr<<" block="<<block<<" 4-chip CPU/audio="<<ratio<<" mean_ms="<<elapsed/20*1000<<"\n";
            restored.panicRequested.store(true);restored.processBlock(buffer,midi);
        }
    }
    std::cout<<"METRIC callback heap calls="<<allocs<<" worst_cpu_ratio="<<worstRatio<<" worst_mean_ms="<<worstMs<<"\n";
    restored.prepareToPlay(48000,512);
    juce::AudioBuffer<float> coldBuffer(2,512);juce::MidiBuffer coldMidi;coldMidi.ensureSize(2048);coldMidi.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)110),0);
    uint64_t coldAllocations=0;
    std::thread coldThread([&]{startAllocationGuard();restored.processBlock(coldBuffer,coldMidi);coldAllocations=stopAllocationGuard();});coldThread.join();
    std::cout<<"METRIC cold-thread first callback heap calls="<<coldAllocations<<"\n";
    check(coldAllocations==0,"first callback on a fresh audio thread has no heap calls");
    check(allocs==0,"no malloc/calloc/realloc/free/new/delete during processBlock");check(worstRatio<0.9,"four chips remain below audio deadline on this machine");
    section("17 extreme parameters and bank switching");
    restored.prepareToPlay(48000,512);juce::AudioBuffer<float> ext(2,512);juce::MidiBuffer extMidi;extMidi.ensureSize(2048);extMidi.addEvent(juce::MidiMessage::noteOn(1,72,(juce::uint8)127),0);restored.processBlock(ext,extMidi);
    for(int extreme=0;extreme<6;++extreme){auto p=basic;const auto& specs=parameterSpecs();for(int i=0;i<parameterCount;++i)p[i]=extreme%2?specs[i].max:specs[i].min;p[Model]=float(extreme%2);p[Clock]=float((extreme/2)%2);p[PlayMode]=float(extreme%2);Preset extremePreset{"Extreme",5,1,sanitize(p)};restored.load(extremePreset);startAllocationGuard();restored.processBlock(ext,extMidi);const auto count=stopAllocationGuard();check(count==0,"bank/preset switch allocation free");for(int i=0;i<512;++i)check(std::isfinite(ext.getSample(0,i))&&std::abs(ext.getSample(0,i))<1,"extreme parameter output bounded");}
    section("18 lifecycle and editor snapshots");
    for(int iteration=0;iteration<4;++iteration){Processor life;if(iteration==0)life.loadFactory(24);life.prepareToPlay(iteration%2?44100:96000,128);juce::AudioBuffer<float> b(2,128);juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)100),0);for(int warm=0;warm<48;++warm)life.processBlock(b,m);std::unique_ptr<juce::AudioProcessorEditor> editor(life.createEditor());check(bool(editor)&&editor->getWidth()>=1000,"custom editor constructs");if(iteration==0){auto capture=[&](const char* name){auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.0f);juce::PNGImageFormat png;auto stream=evidence.getChildFile(name).createOutputStream();if(stream){stream->setPosition(0);stream->truncate();check(png.writeImageToStream(image,*stream),"editor snapshot writes");}};
        capture("play-mode.png");
        juce::TextButton* visual=nullptr;for(auto* child:editor->getChildren())if(child->getComponentID()=="visualStyle")visual=dynamic_cast<juce::TextButton*>(child);
        check(visual!=nullptr,"visual style switch is accessible");
        if(visual){const auto before=life.snapshot();visual->onClick();capture("spectrum-mode.png");check(life.visualStyle.load()==1&&nearPatch(life.snapshot(),before),"Spectrum switch preserves patch");visual->onClick();capture("orbit-mode.png");check(life.visualStyle.load()==2,"Orbit is selectable");visual->onClick();}
        for(auto* child:editor->getChildren())if(auto* button=dynamic_cast<juce::TextButton*>(child);button&&button->getButtonText()=="CHIP MODE")button->onClick();capture("chip-mode.png");editor->setSize(1000,714);capture("chip-mode-compact.png");}editor.reset();life.releaseResources();}
    {Processor kit;kit.loadFactory(73);kit.setVisualStyle(2);kit.prepareToPlay(48000,512);juce::AudioBuffer<float> b(2,512);juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(10,38,(juce::uint8)110),0);kit.processBlock(b,m);std::unique_ptr<juce::AudioProcessorEditor> editor(kit.createEditor());
        int visiblePads=0;for(auto* child:editor->getChildren())if(child->getComponentID().startsWith("drumPad")&&child->isVisible())++visiblePads;check(visiblePads==8,"kit exposes eight playable pads");
        auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.0f);juce::PNGImageFormat png;auto stream=evidence.getChildFile("drums-orbit-mode.png").createOutputStream();if(stream){stream->setPosition(0);stream->truncate();check(png.writeImageToStream(image,*stream),"drum editor snapshot writes");}
        for(auto* child:editor->getChildren())if(child->getComponentID()=="drumPad0")if(auto* button=dynamic_cast<juce::TextButton*>(child))button->onClick();kit.processBlock(b,m);check(kit.engine.activity.load()==2,"drum pad injects actual MIDI hit");
    }
    voicingTests(engine,evidence,24,"disco","disco-timbre-comparison.wav");
    voicingTests(engine,evidence,80,"synthpop","synthpop-timbre-comparison.wav");
    voicingTests(engine,evidence,96,"melody","melody-timbre-comparison.wav",8);
    presetWorkflowTests(evidence);
    midiKnobTests(evidence);
    synthpopIntegrationTests(evidence);
    section("factory sound coverage and deterministic MIDI WAV");
    std::set<long long> signatures;int nonSilent=0;
    for(size_t i=0;i<factoryPresets().size();++i){const auto& preset=factoryPresets()[i];checkPatch(preset.patch);reset(engine,preset.patch);engine.midi({0x90,uint8_t(preset.category==0?45:60),110});auto audio=render(engine,preset.category==3?72000:24000);if(audio.rms>0.00005)++nonSilent;signatures.insert(std::llround(audio.rms*1e9));std::cout<<"PRESET "<<i+1<<" "<<preset.name<<" RMS="<<audio.rms<<" peak="<<audio.peak<<"\n";check(audio.peak<1,"factory preset headroom");const auto file=juce::File(SIDLAB_SOURCE_DIR).getChildFile("presets/Factory").getChildFile(juce::String(int(i)+1).paddedLeft('0',2)+" "+juce::File::createLegalFileName(preset.name)+".sidlab.json");file.getParentDirectory().createDirectory();check(savePresetFile(file,preset).wasOk(),"factory preset export");}
    check(nonSilent==int(factoryPresets().size()),"every factory preset generates real sound");check(signatures.size()==factoryPresets().size(),"distinct factory preset audio RMS signatures");
    plugin.load(factoryPresets()[8]);plugin.prepareToPlay(48000,256);juce::AudioBuffer<float> demo(2,48000*8),block(2,256);juce::MidiBuffer midi;midi.ensureSize(4096);
    for(int offset=0;offset<demo.getNumSamples();offset+=256){const int n=std::min(256,demo.getNumSamples()-offset);block.setSize(2,n,false,false,true);midi.clear();for(int step=0;step<8;++step){const int on=step*36000+73,off=on+27000;const int notes[]{45,48,52,55,57,55,52,48};if(on>=offset&&on<offset+n)midi.addEvent(juce::MidiMessage::noteOn(1,notes[step],(juce::uint8)108),on-offset);if(off>=offset&&off<offset+n)midi.addEvent(juce::MidiMessage::noteOff(1,notes[step]),off-offset);}plugin.processBlock(block,midi);for(int c=0;c<2;++c)demo.copyFrom(c,offset,block,c,0,n);}
    wav(evidence.getChildFile("sid-lab-demo.wav"),demo,48000);check(demo.getRMSLevel(0,0,demo.getNumSamples())>0.0005,"plugin MIDI WAV nonzero");
    {juce::WavAudioFormat format;std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(evidence.getChildFile("sid-lab-demo.wav").createInputStream().release(),true));check(bool(reader),"rendered WAV reopens");if(reader){check(reader->sampleRate==48000&&reader->numChannels==2&&reader->lengthInSamples==384000,"on-disk WAV stereo rate and duration");juce::AudioBuffer<float> decoded(2,384000);check(reader->read(&decoded,0,384000,0,true,true),"on-disk WAV decodes");check(std::abs(decoded.getRMSLevel(0,0,384000)-demo.getRMSLevel(0,0,384000))<1e-6f,"on-disk WAV RMS matches processor output");check(evidence.getChildFile("sid-lab-demo.wav").getSize()<2310000,"render replaces existing file without appending");}}
    section("1.1 disco and percussion demo renders");
    for(bool drums:{false,true}){plugin.loadFactory(drums?73:24);plugin.prepareToPlay(48000,256);demo.clear();
        for(int offset=0;offset<demo.getNumSamples();offset+=256){const int n=std::min(256,demo.getNumSamples()-offset);block.setSize(2,n,false,false,true);midi.clear();
            auto addHit=[&](int when,int note,int velocity){if(when>=offset&&when<offset+n)midi.addEvent(juce::MidiMessage::noteOn(drums?10:1,note,(juce::uint8)velocity),when-offset);if(when+120>=offset&&when+120<offset+n&&drums)midi.addEvent(juce::MidiMessage::noteOff(10,note),when+120-offset);};
            if(drums){for(int step=0;step<64;++step){const int when=step*6000+73;addHit(when,step%2?46:42,step%2?72:88);if(step%4==0)addHit(when,36,116);if(step%8==4)addHit(when,38,108);if(step%16==12)addHit(when,39,96);if(step==0)addHit(when,49,90);}}
            else{for(int bar=0;bar<4;++bar){const int on=bar*96000+73,off=on+90000;const int notes[]{48,44,46,43};addHit(on,notes[bar],110);if(off>=offset&&off<offset+n)midi.addEvent(juce::MidiMessage::noteOff(1,notes[bar]),off-offset);}}
            plugin.processBlock(block,midi);for(int c=0;c<2;++c)demo.copyFrom(c,offset,block,c,0,n);
        }
        const auto file=evidence.getChildFile(drums?"sid-lab-drums-demo.wav":"sid-lab-disco-demo.wav");wav(file,demo,48000);check(demo.getRMSLevel(0,0,demo.getNumSamples())>0.0005&&demo.getMagnitude(0,demo.getNumSamples())<1,"expanded demo is audible and unclipped");
        juce::WavAudioFormat format;std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(file.createInputStream().release(),true));check(reader&&reader->lengthInSamples==384000&&reader->sampleRate==48000,"expanded demo WAV decodes with correct duration and rate");
    }
    std::cout<<"RESULT "<<(failures==0?"PASS":"FAIL")<<" checks="<<checks<<" failures="<<failures<<"\n";
    return failures==0?0:1;
}
