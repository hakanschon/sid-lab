// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Parameters.h"
#include <algorithm>
#include <cmath>
namespace sidlab {
const std::array<ParamSpec, parameterCount>& parameterSpecs() {
    static const auto specs = [] {
        std::array<ParamSpec, parameterCount> a {{
            {"model","Chip model","",0,1,1,0}, {"clock","Clock","",0,1,1,0},
            {"mode","Playing mode","",0,4,1,1}, {"polyphony","Polyphony","chips",1,4,1,4},
            {"cutoff","Brightness","",0,2047,1,1250}, {"resonance","Resonance","",0,15,1,4},
            {"grit","Grit","%",0,1,0,0.25f}, {"movement","Movement","%",0,1,0,0.25f},
            {"attack","Attack offset","steps",0,15,1,0}, {"release","Release offset","steps",0,15,1,0},
            {"master","Master","%",0,1,0,0.7f}, {"chipVolume","SID volume","",0,15,1,15},
            {"filterMode","Shared filter","",0,7,1,1}, {"lfoRate","LFO rate","Hz",0.05f,12,0,2.0f},
            {"pwmDepth","PWM depth","%",0,1,0,0.4f}, {"filterDepth","Filter motion","%",0,1,0,0.2f},
            {"arp","Arpeggio","",0,11,1,0}, {"arpRate","Arp division","",0,3,1,1},
            {"stereo","Poly spread","%",0,1,0,0.25f}, {"bendRange","Pitch bend","st",1,24,1,2}
        }};
        for (int v=0; v<3; ++v) {
            const auto prefix = "v" + std::to_string(v+1) + ".";
            const std::array<ParamSpec, VoiceCount> voice {{
                {"enabled","Enable","",0,1,1,v==0 ? 1.0f:0.0f},
                {"wave","Waveform","",0,7,1,v==0 ? 1.0f:2.0f},
                {"tune","Transpose","st",-36,36,1,0}, {"fine","Fine tune","ct",-100,100,0,0},
                {"ratio","Frequency ratio","x",0.25f,8,0,1}, {"pulse","Pulse width","%",0.02f,0.98f,0,0.5f},
                {"attack","Attack","step",0,15,1,0}, {"decay","Decay","step",0,15,1,5},
                {"sustain","Sustain","step",0,15,1,12}, {"release","Release","step",0,15,1,5},
                {"ring","Ring modulation","",0,1,1,0}, {"sync","Oscillator sync","",0,1,1,0},
                {"route","Route to filter","",0,1,1,1}
            }};
            for(int i=0;i<VoiceCount;++i) {
                a[GlobalCount+v*VoiceCount+i]=voice[i];
                a[GlobalCount+v*VoiceCount+i].id=prefix+voice[i].id;
                a[GlobalCount+v*VoiceCount+i].name="Osc " + std::to_string(v+1) + " " + voice[i].name;
            }
        }
        a[PitchSweep]={"pitchSweep","Pitch sweep","st",-48,48,0,0};
        a[SweepTime]={"sweepTime","Sweep time","ms",5,600,0,80};
        a[ArpGate]={"arpGate","Step gate","%",0.05f,1,0,1};
        a[FilterEnvAmount]={"filterEnvAmount","Filter pluck","steps",-1800,1800,0,0};
        a[FilterEnvDecay]={"filterEnvDecay","Filter pluck decay","ms",5,1500,0,180};
        return a;
    }();
    return specs;
}
Patch defaultPatch() { Patch p{}; const auto& s=parameterSpecs(); for(int i=0;i<parameterCount;++i) p[i]=s[i].initial; return p; }
Patch sanitize(Patch p) noexcept {
    const auto& s=parameterSpecs();
    for(int i=0;i<parameterCount;++i) {
        if(!std::isfinite(p[i])) p[i]=s[i].initial;
        p[i]=std::clamp(p[i],s[i].min,s[i].max);
        if(s[i].step>0) p[i]=s[i].min+std::round((p[i]-s[i].min)/s[i].step)*s[i].step;
    }
    return p;
}
const ArpPattern& arpeggioPattern(int index) noexcept {
    static constexpr std::array<ArpPattern,12> patterns {{
        {4,{0,0,0,0}}, {4,{0,4,7,12}}, {4,{0,3,7,12}}, {4,{0,12,7,12}},
        {8,{0,0,12,0,7,0,12,7}}, {8,{0,7,12,15,12,7,3,7}},
        {8,{0,7,12,16,12,7,4,7}}, {8,{0,12,7,19,0,12,10,7}},
        {16,{0,0,12,0,7,7,12,7,3,3,15,3,7,7,12,7}},
        {8,{0,12,7,19,4,16,7,12}}, {8,{12,7,3,0,15,10,7,3}},
        {8,{0,12,0,12,7,19,7,19}}
    }};
    return patterns[std::clamp(index,0,11)];
}
uint16_t frequencyRegister(double note,double clock,double ratio) noexcept {
    if(!std::isfinite(note)||!std::isfinite(ratio)||clock<=0||!std::isfinite(clock)) return 0;
    const double hz=440.0*std::exp2((std::clamp(note,-100.0,200.0)-69.0)/12.0)*std::clamp(ratio,0.0,16.0);
    return static_cast<uint16_t>(std::clamp(std::round(hz*16777216.0/clock),0.0,65535.0));
}
uint8_t waveformBits(int w) noexcept {
    constexpr std::array<uint8_t,8> bits {0x10,0x20,0x40,0x80,0x30,0x50,0x60,0x70};
    return bits[std::clamp(w,0,7)];
}
std::array<uint8_t,3> filterRegisters(const Patch& p,float cut,float res) noexcept {
    int c=std::clamp(static_cast<int>(std::lround(cut)),0,2047);
    int r=std::clamp(static_cast<int>(std::lround(res)),0,15), route=0;
    for(int v=0;v<3;++v) if(p[vp(v,Route)]>0.5f) route|=(1<<v);
    return {static_cast<uint8_t>(c&7),static_cast<uint8_t>(c>>3),static_cast<uint8_t>((r<<4)|route)};
}
} // namespace sidlab
