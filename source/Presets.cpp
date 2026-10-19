// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Presets.h"
#include "DiscoPatches.h"
#include "SynthpopPatches.h"
#include <cmath>
#include <charconv>
namespace sidlab {
const std::vector<Preset>& factoryPresets() {
    static const auto presets=[] {
        constexpr const char* names[] {
            "Dirty C64 Bass","Warm Triangle Bass","6581 Filter Growl","Dark Trip-Hop Bass",
            "Arcade Lead","Chiptune Pluck","Laser Pulse Lead","Silver Saw Solo",
            "Classic SID Arpeggio","Minor Neon Steps","Octave Runner","Crystal Cartridge",
            "Space Atmosphere","PWM Dream Pad","Dusk on the Databus","Glass Satellites",
            "Retro Game Explosion","Noisy Percussion","Ring Modulator FX","Robot Transmission",
            "Broken Raster","Metallic Labyrinth","Voltage Ghost","Unstable Orbit"
        };
        std::vector<Preset> result;result.reserve(104);
        for(int i=0;i<24;++i) {
            const uint64_t seed=0x5349441000ULL+uint64_t(i)*7919;
            auto p=randomize(defaultPatch(),i/4,seed);
            p[PlayMode]=(i/4==0||i/4==1||i/4==2)?0.0f:1.0f;
            if(i==0) {p[Model]=0;p[vp(0,Wave)]=2;p[vp(1,Enabled)]=1;p[vp(1,Tune)]=-12;p[Grit]=0.9f;p[Cutoff]=850;}
            if(i==1) {p[vp(0,Wave)]=0;p[vp(1,Enabled)]=0;p[vp(2,Enabled)]=0;p[Cutoff]=1600;p[Model]=1;p[Grit]=0.15f;}
            if(i==2) {p[Model]=0;p[Resonance]=11;p[Cutoff]=900;p[Movement]=0.8f;p[FilterDepth]=0.45f;}
            if(i==3) {p[vp(0,Wave)]=0;p[vp(1,Enabled)]=1;p[vp(1,Tune)]=-12;p[vp(1,Wave)]=2;p[Cutoff]=650;p[Resonance]=8;}
            if(i==5) for(int v=0;v<3;++v) {p[vp(v,VAttack)]=0;p[vp(v,Decay)]=6;p[vp(v,Sustain)]=0;p[vp(v,VRelease)]=4;}
            if(i==7) {p[Model]=1;p[vp(0,Wave)]=1;p[Cutoff]=1900;p[vp(1,Enabled)]=1;p[vp(1,Fine)]=7;}
            if(i==8) {p[Arp]=1;p[ArpRate]=1;p[Movement]=0.1f;p[vp(0,Wave)]=2;}
            if(i==9) p[Arp]=2;
            if(i==10) {p[Arp]=3;p[ArpRate]=2;}
            if(i==13) for(int v=0;v<3;++v) {p[vp(v,Wave)]=2;p[vp(v,Enabled)]=1;p[Movement]=0.8f;p[PwmDepth]=0.7f;p[LfoRate]=0.24f;}
            if(i==16) {p[vp(0,Wave)]=3;p[vp(0,Decay)]=9;p[vp(0,Sustain)]=0;p[vp(0,VRelease)]=8;p[Cutoff]=1600;}
            if(i==17) {p[vp(0,Wave)]=3;p[vp(0,Decay)]=5;p[vp(0,Sustain)]=0;p[vp(0,VRelease)]=3;p[vp(2,Enabled)]=0;}
            if(i==18) {p[vp(0,Wave)]=0;p[vp(0,Ring)]=1;p[vp(2,Enabled)]=1;p[vp(2,Tune)]=19;p[vp(2,Wave)]=0;}
            p[ArpGate]=1;result.push_back({names[i],i/4,seed,sanitize(p)});
        }
        // Original sound-design families. The first 24 indices remain stable.
        const auto crystal=result[11].patch;
        constexpr const char* sequencers[]{"Crystal Cartridge II","Munich Midnight","Neon Autobahn","Chrome Circuit",
            "Transistor Nights","Velvet Motorik","Disco Databus","Midnight Expressway",
            "Glass Transmission","Amber Sequencer","Silver Boulevard","Pulse Pursuit",
            "Neon Discotheque","Afterhours Circuit","Quartz Runner","Electric Horizon"};
        constexpr const char* arpeggios[]{"Quartz Cartridge","Prism Sequencer","Glass Ladder","Moonbase Marbles",
            "Silver Bubbles","Crystal Rain","Pixel Glassworks","Amber Arpeggiator"};
        constexpr const char* basses[]{"Midnight Octave Bass","Rubber Subway","Disco Sub Circuit","Autobahn Rubber Bass",
            "Pulse Engine Bass","Amber Voltage Bass","Velvet Mono Bass","Chrome Sequencer Bass"};
        constexpr const char* leads[]{"Laser Boulevard","Neon Silk Lead","Chrome Ribbon Solo","Velvet Arcade Lead",
            "Glass Needle","Transistor Hero","Pulse Cathedral","Tape Skyline Lead"};
        constexpr const char* pads[]{"Afterglow Strings","Violet Transit Pad","Neon Cinema","Cassette Aurora",
            "Floating Cassette","Lunar Tape Choir","Velvet Satellites","Amber Horizons"};
        constexpr const char* kits[]{"BitBox 64 Kit","Disco Circuit Kit","Chrome Arcade Kit","Soft Tape Kit",
            "Industrial Raster Kit","Neon Club Kit","Tiny Cartridge Kit","Darkroom Kit"};
        auto add=[&](const char* name,int category,Patch p){
            result.push_back({name,category,0x5349442000ULL+uint64_t(result.size())*104729,sanitize(p)});
        };
        for(int i=0;i<16;++i)add(sequencers[i],6,discoPatch(i));
        for(int i=0;i<8;++i){
            auto p=crystal;p[Cutoff]=650+125.0f*i;p[Resonance]=float(3+i%6);p[Model]=1;
            p[vp(0,Wave)]=i%3==2?2:0;p[vp(1,Wave)]=float(i%3==1?2:1);p[vp(1,Tune)]=i%2?-12:0;
            p[vp(1,Fine)]=-3.0f-i;p[vp(2,Enabled)]=float(i%2);p[vp(2,Tune)]=12;
            p[Arp]=float(i<3?i+1:9+(i%3));p[ArpGate]=0.5f+0.05f*i;
            p[Movement]=0.15f+0.05f*i;p[LfoRate]=0.4f+0.23f*i;p[PwmDepth]=0.3f+0.04f*i;
            for(int v=0;v<3;++v){p[vp(v,VAttack)]=0;p[vp(v,Decay)]=float(4+i%4);p[vp(v,Sustain)]=float(6+i%6);p[vp(v,VRelease)]=float(3+i%4);}
            add(arpeggios[i],2,p);
        }
        for(int i=0;i<8;++i){
            auto p=defaultPatch();p[PlayMode]=0;p[Model]=float(i%2);p[Cutoff]=550+145.0f*i;
            p[Resonance]=float(2+i%8);p[Grit]=0.2f+0.09f*i;p[Movement]=0.08f+0.04f*i;
            p[vp(0,Wave)]=float(i%3);p[vp(0,Decay)]=float(4+i%4);p[vp(0,Sustain)]=float(8+i%7);
            p[vp(0,VRelease)]=3;p[vp(0,Pulse)]=0.18f+0.075f*i;
            p[vp(1,Enabled)]=1;p[vp(1,Wave)]=0;p[vp(1,Tune)]=-12;p[vp(1,Sustain)]=11;p[vp(1,VRelease)]=2;
            p[Arp]=(i==3||i==7)?4.0f:0.0f;p[ArpGate]=0.55f;p[FilterDepth]=0.07f;
            if(i==5){p[PitchSweep]=12;p[SweepTime]=32;}
            add(basses[i],0,p);
        }
        for(int i=0;i<8;++i){
            auto p=defaultPatch();p[PlayMode]=0;p[Model]=float((i+1)%2);p[Cutoff]=1050+125.0f*i;
            p[Resonance]=float(2+i%8);p[vp(0,Wave)]=i%2?2:1;p[vp(0,Pulse)]=0.18f+0.06f*i;
            p[vp(0,VAttack)]=float(i%3);p[vp(0,Sustain)]=float(10+i%6);p[vp(0,VRelease)]=float(3+i%5);
            p[vp(1,Enabled)]=1;p[vp(1,Wave)]=float(i%3);p[vp(1,Fine)]=4+float(i);p[vp(1,Tune)]=i==5?12:0;
            p[Movement]=0.15f+0.08f*i;p[LfoRate]=0.7f+0.4f*i;p[PwmDepth]=0.3f;
            if(i==6){p[vp(0,Wave)]=0;p[vp(0,Ring)]=1;p[vp(2,Enabled)]=1;p[vp(2,Tune)]=7;}
            add(leads[i],1,p);
        }
        for(int i=0;i<8;++i){
            auto p=defaultPatch();p[Model]=float(i%2);p[Cutoff]=750+130.0f*i;p[Resonance]=float(2+i%6);
            p[Movement]=0.4f+0.055f*i;p[LfoRate]=0.12f+0.07f*i;p[PwmDepth]=0.35f+0.05f*i;
            p[FilterDepth]=0.1f+0.02f*i;p[Stereo]=0.55f+0.04f*i;
            for(int v=0;v<3;++v){p[vp(v,Enabled)]=1;p[vp(v,Wave)]=(i+v)%3==0?0:2;p[vp(v,Tune)]=v==2?12:0;p[vp(v,Fine)]=float((v-1)*(3+i));p[vp(v,VAttack)]=float(6+i%4);p[vp(v,Decay)]=8;p[vp(v,Sustain)]=12;p[vp(v,VRelease)]=float(8+i%4);p[vp(v,Pulse)]=0.25f+0.15f*v;}
            add(pads[i],3,p);
        }
        for(int i=0;i<8;++i){
            auto p=defaultPatch();p[PlayMode]=float(2+i%3);p[Model]=float(i%2);
            p[Cutoff]=900+100.0f*i;p[Resonance]=float(2+i%6);p[Grit]=0.18f+0.1f*i;
            p[Master]=0.85f;p[Movement]=0;p[Arp]=0;p[Release]=i==3?1.0f:0.0f;p[Stereo]=0.16f+0.04f*i;
            add(kits[i],7,p);
        }
        constexpr const char* synthpopNames[]{"Japan Fifth Bass","Tokyo Neon Pluck","Forever Brass","Melody String Machine",
            "Berlin Glass Bells","Summer Voltage Lead","Night Drive Octaves","Diamond Pad",
            "Jet Set Sync Lead","Harbour Electric Keys","Young Horizon Choir","Japan Brass Stabs",
            "Midnight Marimba","Arcade Romance Lead","1984 Spark Arp","Forever Fade Strings",
            "Melody Hook Strings","Melody Octave Drive","Melody Bright Lead","Melody Staccato Strings",
            "Melody Glass Pulse","Melody Finale Ensemble","Melody Bandpass Keys","Melody Warm Backdrop"};
        static_assert(std::size(synthpopNames)==synthpopPatchCount);
        for(int i=0;i<synthpopPatchCount;++i)add(synthpopNames[i],8,synthpopPatch(i));
        return result;
    }();return presets;
}
juce::var presetJson(const Preset& p) {
    auto* root=new juce::DynamicObject;juce::var v(root);
    root->setProperty("format","SIDLabPreset");root->setProperty("schema",3);
    root->setProperty("name",p.name);root->setProperty("category",p.category);
    root->setProperty("seed",juce::String(p.seed));
    auto* parameters=new juce::DynamicObject;root->setProperty("parameters",juce::var(parameters));
    const auto patch=sanitize(p.patch);const auto& specs=parameterSpecs();
    for(int i=0;i<parameterCount;++i) parameters->setProperty(specs[i].id.c_str(),patch[i]);
    return v;
}
bool readPreset(const juce::var& v,Preset& p,juce::String& error) {
    const int schema=int(v["schema"]);
    if(!v.isObject()||v["format"].toString()!="SIDLabPreset"||(schema<1||schema>3)) {error="Unsupported SID Lab preset (schema 1, 2 or 3 required).";return false;}
    const auto values=v["parameters"];
    if(!values.isObject()) {error="Missing parameter object.";return false;}
    Patch patch=defaultPatch();const auto& specs=parameterSpecs();
    for(int i=0;i<parameterCount;++i) {
        const auto val=values.getProperty(specs[i].id.c_str(),juce::var());
        const int required=schema==1?legacyParameterCount:(schema==2?schema2ParameterCount:parameterCount);
        if(i>=required&&val.isVoid())continue;
        if(!(val.isInt()||val.isInt64()||val.isDouble())||!std::isfinite(double(val))) {error="Missing or invalid parameter: "+specs[i].id;return false;}
        patch[i]=float(double(val));
    }
    p.patch=sanitize(patch);p.name=v["name"].toString().substring(0,128);
    if(p.name.isEmpty()) p.name="Untitled";
    p.category=juce::jlimit(0,int(categories.size())-1,int(v["category"]));
    const auto seedText=v["seed"].toString().toStdString();
    const auto parsedSeed=std::from_chars(seedText.data(),seedText.data()+seedText.size(),p.seed);
    if(parsedSeed.ec!=std::errc()||parsedSeed.ptr!=seedText.data()+seedText.size()) {error="Invalid unsigned 64-bit seed.";return false;}
    error.clear();return true;
}
juce::Result savePresetFile(const juce::File& file,const Preset& p) {
    juce::TemporaryFile temp(file);
    if(!temp.getFile().replaceWithText(juce::JSON::toString(presetJson(p),false))) return juce::Result::fail("Cannot write temporary preset: "+file.getFullPathName());
    return temp.overwriteTargetFileWithTemporary()?juce::Result::ok():juce::Result::fail("Cannot replace preset: "+file.getFullPathName());
}
juce::Result loadPresetFile(const juce::File& file,Preset& p) {
    if(!file.existsAsFile()||file.getSize()>1024*1024) return juce::Result::fail("Preset is missing or exceeds 1 MB.");
    juce::String error;juce::var v;
    const auto parsed=juce::JSON::parse(file.loadFileAsString(),v);
    if(parsed.failed()) return parsed;
    return readPreset(v,p,error)?juce::Result::ok():juce::Result::fail(error);
}
} // namespace sidlab
