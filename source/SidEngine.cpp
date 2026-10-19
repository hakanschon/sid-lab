// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SidEngine.h"
#include "SID.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace sidlab {
namespace {
constexpr double pi=3.14159265358979323846;
constexpr std::array<double,16> releaseSeconds {0.006,0.024,0.048,0.072,0.114,0.168,0.204,0.240,0.300,0.750,1.5,2.4,3,9,15,24};
}
SidEngine::SidEngine() : patch(defaultPatch()) { for(auto& x:scope) x.store(0);for(auto& x:analysis)x.store(0); }
SidEngine::~SidEngine() = default;
void SidEngine::prepare(double sr) {
    if(switching) patch=pendingPatch;
    if(!std::isfinite(sr)||sr<=0) sr=48000;
    sampleRate=std::clamp(sr,11025.0,192000.0);visualSampleRate.store(sampleRate);
    for(int b=0;b<4;++b) for(auto& s:banks[b]) {
        s.chip=std::make_unique<reSIDfp::SID>();
        s.chip->setChipModel((b/2)==0 ? reSIDfp::MOS6581:reSIDfp::CSG8580);
        s.chip->setSamplingParameters((b%2)==0?palClock:ntscClock,reSIDfp::RESAMPLE,sampleRate);
        // Fixed physical characteristics; never rebuild these tables in render().
        s.chip->setFilter6581Curve(0.5);
        s.chip->setFilter8580Curve(0.5);
        s.chip->reset();
        s.regs.fill(0xff); s.active=false; s.note=-1; s.gate=false; s.gain=0; s.tail=0; s.last=0;
    }
    keys={}; pedal={}; bend={};transitionMidiCount=0;
    bank=int(patch[Model])*2+int(patch[Clock]); mode=int(patch[PlayMode]); capacity=int(patch[Polyphony]);
    pendingPatch=patch;switching=false; transitionGain=1; master=0; cutoff=patch[Cutoff]; prepared=true;
}
void SidEngine::setPatch(const Patch& p) noexcept {
    const auto clean=sanitize(p);
    pendingBank=int(clean[Model])*2+int(clean[Clock]);
    pendingMode=int(clean[PlayMode]); pendingCapacity=int(clean[Polyphony]);
    bool discrete=clean[Model]!=patch[Model]||clean[Clock]!=patch[Clock]||clean[PlayMode]!=patch[PlayMode]||clean[Polyphony]!=patch[Polyphony];
    for(int v=0;v<3;++v) for(auto k:{Enabled,Wave,Tune,Ratio,Ring,Sync,Route,VAttack,Decay,Sustain,VRelease})
        discrete=discrete||clean[vp(v,k)]!=patch[vp(v,k)];
    pendingPatch=clean;
    if(prepared&&(discrete||switching)) switching=true;
    else patch=clean;
}
void SidEngine::setTempo(double t) noexcept { if(std::isfinite(t)&&t>=20&&t<=400) tempo=t; }
void SidEngine::write(Slot& s,int reg,uint8_t value) noexcept {
    if(s.regs[reg]!=value) { s.chip->write(reg,value); s.regs[reg]=value; }
}
void SidEngine::start(Slot& s,int channel,int note,int velocity) noexcept {
    // A short residual fade preserves the previous output at a steal/retrigger.
    const float residual=s.active?s.last:0;
    s.chip->reset(); s.regs.fill(0xff);
    s.note=note; s.channel=channel; s.age=++age; s.velocity=velocity/127.0f;
    s.gain=0; s.tail=residual; s.gate=true; s.active=true; s.killing=false;
    s.released=0; s.phase=0; s.arpPhase=0;s.elapsed=0;s.oneShot=mode>=2;s.rootNote=float(note);
    if(s.oneShot){const auto d=makeDrum(patch,mode-1,note);s.drumPatch=d.patch;s.drumType=d.type;s.rootNote=d.rootNote;s.autoGate=d.gateSeconds;}
    update(s,0);
}
void SidEngine::release(Slot& s) noexcept {
    if(!s.gate) return;
    s.gate=false; s.released=0;
    const auto& sound=s.oneShot?s.drumPatch:patch;
    s.releaseLimit=0.02;
    for(int v=0;v<3;++v) {
        const int r=std::clamp(int(sound[vp(v,VRelease)]+sound[Release]),0,15);
        if(sound[vp(v,Enabled)]>0.5f) s.releaseLimit=std::max(s.releaseLimit,releaseSeconds[r]*1.5+0.05);
        write(s,v*7+4,static_cast<uint8_t>(s.regs[v*7+4]&0xfe));
    }
}
void SidEngine::noteOn(int ch,int note,int vel) noexcept {
    if(mode>=2&&drumForNote(note)==DrumType::ClosedHat)
        for(auto& s:banks[bank])if(s.active&&s.oneShot&&s.drumType==DrumType::OpenHat){release(s);s.killing=true;}
    auto& k=keys[ch][note]; if(mode<2&&k.count<255) ++k.count;
    k.sustained=false; k.velocity=static_cast<uint8_t>(vel); k.age=++age;
    if(mode==0) { start(banks[bank][0],ch,note,vel); return; }
    Slot* chosen=nullptr;
    for(int i=0;i<capacity;++i) {
        auto& s=banks[bank][i];
        if(s.active&&s.note==note&&s.channel==ch) { chosen=&s; break; }
        if(!s.active) chosen=&s;
    }
    if(chosen==nullptr) {
        for(int i=0;i<capacity;++i) {
            auto& s=banks[bank][i];
            if(chosen==nullptr||(!s.gate&&chosen->gate)||(s.gate==chosen->gate&&s.age<chosen->age)) chosen=&s;
        }
    }
    start(*chosen,ch,note,vel);
}
void SidEngine::reconcileMono() noexcept {
    uint64_t latest=0; int n=-1,c=0;
    for(int ch=0;ch<16;++ch) for(int note=0;note<128;++note) {
        const auto& k=keys[ch][note];
        if((k.count>0||k.sustained)&&k.age>latest) { latest=k.age;n=note;c=ch; }
    }
    auto& s=banks[bank][0];
    if(n<0) release(s);
    else if(!s.gate||s.note!=n||s.channel!=c) start(s,c,n,keys[c][n].velocity);
}
void SidEngine::noteOff(int ch,int note) noexcept {
    if(mode>=2)return;
    auto& k=keys[ch][note];
    if(k.count==0) return;
    if(k.count>0) --k.count;
    if(k.count>0) return;
    k.sustained=pedal[ch];
    if(k.sustained) return;
    if(mode==0) { reconcileMono(); return; }
    for(auto& s:banks[bank]) if(s.note==note&&s.channel==ch) release(s);
}
void SidEngine::clearChannel(int ch,bool hard) noexcept {
    keys[ch]={}; pedal[ch]=false;
    unsigned kept=0;for(unsigned i=0;i<transitionMidiCount;++i)if((transitionMidi[i].status&15)!=ch)transitionMidi[kept++]=transitionMidi[i];transitionMidiCount=kept;
    for(auto& s:banks[bank]) if(s.channel==ch) { release(s); if(hard) s.killing=true; }
    if(mode==0) reconcileMono();
}
void SidEngine::midi(MidiEvent e) noexcept {
    if(!prepared) return;
    const int ch=e.status&15, command=e.status&0xf0;
    const int d1=e.data1&127, d2=e.data2&127;
    if(switching&&(mode>=2||pendingMode>=2)&&(command==0x90||command==0x80)){
        if(transitionMidiCount<transitionMidi.size())transitionMidi[transitionMidiCount++]=e;
        else panic(); // A bounded queue must never leave an unmatched key behind.
        return;
    }
    if(command==0x90&&d2>0) { noteOn(ch,d1,d2); activity.fetch_add(1,std::memory_order_relaxed); }
    else if(command==0x80||(command==0x90&&d2==0)) noteOff(ch,d1);
    else if(command==0xe0) bend[ch]=std::clamp(((d1|(d2<<7))-8192)/8192.0f,-1.0f,1.0f);
    else if(command==0xb0) {
        if(d1==64&&mode<2) {
            const bool was=pedal[ch]; pedal[ch]=d2>=64;
            if(was&&!pedal[ch]) {
                for(int note=0;note<128;++note) if(keys[ch][note].count==0&&keys[ch][note].sustained) {
                    keys[ch][note].sustained=false;
                    for(auto& s:banks[bank]) if(s.note==note&&s.channel==ch) release(s);
                }
                if(mode==0) reconcileMono();
            }
        } else if(d1==120) clearChannel(ch,true);
        else if(d1==123) clearChannel(ch,false);
        else if(d1==121) { bend[ch]=0; pedal[ch]=false; for(int n=0;n<128;++n) if(keys[ch][n].sustained&&keys[ch][n].count==0) { keys[ch][n].sustained=false; for(auto& s:banks[bank]) if(s.note==n&&s.channel==ch) release(s); } if(mode==0) reconcileMono(); }
    }
}
void SidEngine::panic() noexcept { for(int ch=0;ch<16;++ch) clearChannel(ch,true); }
void SidEngine::switchBank() noexcept {
    for(auto& s:banks[bank]) { release(s); s.active=false; s.note=-1; }
    patch=pendingPatch;bank=pendingBank; mode=pendingMode; capacity=pendingCapacity;
    for(auto& s:banks[bank]) { s.active=false; s.note=-1; s.last=0; }
    // Replay the newest held notes without modifying key reference counts.
    if(mode>=2){keys={};pedal={};}
    else if(mode==0) reconcileMono();
    else {
        uint64_t ceiling=std::numeric_limits<uint64_t>::max();
        for(int slot=0;slot<capacity;++slot) {
            uint64_t latest=0; int n=-1,c=0;
            for(int ch=0;ch<16;++ch) for(int note=0;note<128;++note) {
                const auto& k=keys[ch][note];
                if((k.count||k.sustained)&&k.age<ceiling&&k.age>latest) {latest=k.age;n=note;c=ch;}
            }
            if(n<0) break;
            ceiling=latest; start(banks[bank][slot],c,n,keys[c][n].velocity);
        }
    }
    switching=false;
    const unsigned count=transitionMidiCount;transitionMidiCount=0;
    for(unsigned i=0;i<count;++i)midi(transitionMidi[i]);
}
void SidEngine::update(Slot& s,int count) noexcept {
    const auto& sound=s.oneShot?s.drumPatch:patch;
    const double lfo=std::sin(2*pi*s.phase);
    s.phase=std::fmod(s.phase+sound[LfoRate]*count/sampleRate,1.0);
    constexpr std::array<double,4> divisions {2,4,8,12};
    const int arp=int(sound[Arp]);
    const auto& pattern=arpeggioPattern(arp);
    const int arpNote=pattern.notes[static_cast<int>(s.arpPhase)%pattern.length];
    const bool stepGate=arp==0||sound[ArpGate]>=0.999f||std::fmod(s.arpPhase,1.0)<sound[ArpGate];
    const double sweep=sound[PitchSweep]*std::exp(-s.elapsed/(sound[SweepTime]*0.001));
    const double stepHz=tempo/60.0*divisions[int(sound[ArpRate])];
    const double envelopeTime=arp==0?s.elapsed:std::fmod(s.arpPhase,1.0)/stepHz;
    const double filterEnvelope=sound[FilterEnvAmount]==0?0:sound[FilterEnvAmount]*std::exp(-envelopeTime/(sound[FilterEnvDecay]*0.001));
    s.arpPhase=std::fmod(s.arpPhase+stepHz*count/sampleRate,double(pattern.length));
    const double chipClock=(bank%2)==0?palClock:ntscClock;
    for(int v=0;v<3;++v) {
        const int base=v*7;
        const double note=s.rootNote+sweep+sound[vp(v,Tune)]+sound[vp(v,Fine)]/100.0+bend[s.channel]*sound[BendRange]+arpNote;
        const auto freq=frequencyRegister(note,chipClock,sound[vp(v,Ratio)]);
        write(s,base,static_cast<uint8_t>(freq&255)); write(s,base+1,static_cast<uint8_t>(freq>>8));
        const double pw=std::clamp(sound[vp(v,Pulse)]+lfo*sound[Movement]*sound[PwmDepth]*0.45,0.01,0.99);
        const int width=int(std::round(pw*4095));
        write(s,base+2,static_cast<uint8_t>(width&255)); write(s,base+3,static_cast<uint8_t>((width>>8)&15));
        const int attack=std::clamp(int(sound[vp(v,VAttack)]+sound[Attack]),0,15);
        const int rel=std::clamp(int(sound[vp(v,VRelease)]+sound[Release]),0,15);
        write(s,base+5,static_cast<uint8_t>((attack<<4)|int(sound[vp(v,Decay)])));
        write(s,base+6,static_cast<uint8_t>((int(sound[vp(v,Sustain)])<<4)|rel));
        uint8_t ctrl=waveformBits(int(sound[vp(v,Wave)]));
        if(sound[vp(v,Ring)]>0.5f) ctrl|=4;
        if(sound[vp(v,Sync)]>0.5f) ctrl|=2;
        if(sound[vp(v,Enabled)]<0.5f) ctrl=0;
        else if(s.gate&&stepGate) {
            bool burst=true;
            if(s.oneShot&&s.drumType==DrumType::Clap&&s.elapsed<0.035)
                burst=std::fmod(s.elapsed,0.012)<0.006;
            if(burst)ctrl|=1;
        }
        write(s,base+4,ctrl);
    }
    const float modCut=(s.oneShot?sound[Cutoff]:cutoff)+static_cast<float>(lfo)*sound[Movement]*sound[FilterDepth]*1400;
    const auto f=filterRegisters(sound,modCut+float(filterEnvelope),sound[Resonance]+sound[Grit]*4);
    write(s,21,f[0]); write(s,22,f[1]); write(s,23,f[2]);
    // Grit raises the actual SID mixer volume and resonance, not cosmetic EQ.
    const int volume=std::clamp(int(std::round(sound[ChipVolume]*(0.65f+0.35f*sound[Grit]))),0,15);
    write(s,24,static_cast<uint8_t>((int(sound[FilterMode])<<4)|volume));
}
void SidEngine::render(float* l,float* r,int samples) noexcept {
    if(!prepared) { std::fill_n(l,samples,0); std::fill_n(r,samples,0); return; }
    int offset=0;
    const float gainCoeff=1.0f-std::exp(-1.0f/(0.003f*float(sampleRate)));
    const float tailCoeff=std::exp(-1.0f/(0.003f*float(sampleRate)));
    while(offset<samples) {
        const int n=std::min(32,samples-offset);
        cutoff+=(patch[Cutoff]-cutoff)*(1.0f-std::exp(-float(n)/(0.008f*float(sampleRate))));
        for(auto& s:banks[bank]) if(s.active) { if(s.oneShot&&s.gate&&s.elapsed>=s.autoGate)release(s);update(s,n); s.chip->clock(s.scratch.data(),n); }
        for(int i=0;i<n;++i) {
            float left=0,right=0;
            for(int j=0;j<4;++j) {
                auto& s=banks[bank][j]; if(!s.active) continue;
                s.gain+=((s.killing?0.0f:s.velocity)-s.gain)*gainCoeff;
                const float x=s.scratch[i]/32768.0f*s.gain+s.tail;
                s.tail*=tailCoeff; s.last=x;
                const float pan=(mode==0?0.0f:((j/3.0f)-0.5f)*patch[Stereo]);
                left+=x*(1-pan);right+=x*(1+pan);
                s.elapsed+=1.0/sampleRate;
                if(!s.gate) { s.released+=1.0/sampleRate; if(s.released>s.releaseLimit||(s.killing&&s.gain<0.0001f)) { s.active=false;s.note=-1;s.last=0; } }
            }
            master+=(patch[Master]-master)*gainCoeff;
            if(switching) transitionGain=std::max(0.0f,transitionGain-1.0f/(0.004f*float(sampleRate)));
            else transitionGain=std::min(1.0f,transitionGain+1.0f/(0.004f*float(sampleRate)));
            // Fixed headroom supports four full chips without a clipping limiter.
            const float headroom=mode==0?0.65f:0.22f;
            l[offset+i]=left*master*headroom*transitionGain;
            r[offset+i]=right*master*headroom*transitionGain;
            const unsigned ah=analysisHead.load(std::memory_order_relaxed);
            analysis[ah%analysis.size()].store((l[offset+i]+r[offset+i])*0.5f,std::memory_order_relaxed);
            analysisHead.store(ah+1,std::memory_order_release);
            if(++scopeDecimation==8) {
                scopeDecimation=0;
                const unsigned h=scopeHead.load(std::memory_order_relaxed);
                scope[h%scope.size()].store((l[offset+i]+r[offset+i])*0.5f,std::memory_order_relaxed);
                scopeHead.store(h+1,std::memory_order_release);
            }
        }
        offset+=n;
        if(switching&&transitionGain<=0) switchBank();
    }
}
int SidEngine::soundingVoices() const noexcept { int n=0; for(const auto& s:banks[bank]) n+=s.active?1:0; return n; }
int SidEngine::heldNotes() const noexcept { int n=0; for(const auto& ch:keys) for(const auto& k:ch) n+=(k.count||k.sustained)?1:0; return n; }
uint8_t SidEngine::debugRegister(int slot,int address) const noexcept { return banks[bank][std::clamp(slot,0,3)].regs[std::clamp(address,0,24)]; }
} // namespace sidlab
