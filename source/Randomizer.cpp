// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Randomizer.h"
#include "DiscoPatches.h"
#include "SynthpopPatches.h"
#include <algorithm>
#include <cmath>
namespace sidlab {
namespace {
struct Random {
    uint64_t state;
    uint32_t next() noexcept { state^=state>>12;state^=state<<25;state^=state>>27;return uint32_t((state*2685821657736338717ULL)>>32); }
    float unit() noexcept { return (next()>>8)*(1.0f/16777216.0f); }
    float range(float a,float b) noexcept { return a+(b-a)*unit(); }
    int integer(int a,int b) noexcept { return a+int(next()%uint32_t(b-a+1)); }
};
}
unsigned sectionFor(int i) noexcept {
    if(i==PitchSweep||i==SweepTime||i==ArpGate||i==FilterEnvAmount||i==FilterEnvDecay)return LockModulation;
    if(i>=GlobalCount) {
        const int p=(i-GlobalCount)%VoiceCount;
        return p>=VAttack&&p<=VRelease?LockEnvelopes:LockOscillators;
    }
    if(i==Attack||i==Release) return LockEnvelopes;
    if(i==Model||i==Clock||i==Cutoff||i==Resonance||i==Grit||i==ChipVolume||i==FilterMode) return LockFilter;
    if(i==Movement||i==LfoRate||i==PwmDepth||i==FilterDepth||i==Arp||i==ArpRate||i==Stereo) return LockModulation;
    return 0;
}
Patch randomize(const Patch& base,int cat,uint64_t seed,unsigned locks,bool mutate) noexcept {
    const int requested=std::clamp(cat,0,int(categories.size())-1);cat=requested==6?2:(requested==7?4:(requested==8?1:requested)); Random r{seed?seed:0x5349444c4142ULL};
    Patch p=base;
    p[Model]=float(r.integer(0,1)); p[Clock]=0;
    p[Cutoff]=cat==0?r.range(300,1150):(cat==3?r.range(600,1500):r.range(600,2000));
    p[Resonance]=r.range(1,cat==5?13.0f:9.0f);p[Grit]=r.range(0.05f,cat==3?0.4f:0.85f);
    p[ChipVolume]=15;p[FilterMode]=cat>=4?float(1<<r.integer(0,2)):1.0f;
    p[Movement]=r.range(cat==3?0.3f:0.05f,cat>=4?0.95f:0.6f);
    p[LfoRate]=cat==3?r.range(0.1f,0.8f):r.range(0.5f,6.0f);
    p[PwmDepth]=r.range(0.1f,0.7f); p[FilterDepth]=r.range(0.02f,cat>=4?0.7f:0.25f);
    p[Attack]=0;p[Release]=0;p[Arp]=cat==2?float(r.integer(1,3)):0;
    p[ArpRate]=float(r.integer(0,2));p[Stereo]=cat==3?0.7f:0.2f;
    constexpr int musicalIntervals[] {0,0,0,7,12,-12};
    for(int v=0;v<3;++v) {
        p[vp(v,Enabled)]=v==0?1.0f:(r.unit()<(cat==3?0.95f:0.6f)?1.0f:0.0f);
        p[vp(v,Wave)]=cat==4?float(v==0?3:r.integer(0,3)):float(r.integer(0,cat==5?7:2));
        p[vp(v,Tune)]=float(v==0?0:musicalIntervals[r.integer(0,5)]);
        p[vp(v,Fine)]=v==0?0:r.range(-9,9);p[vp(v,Ratio)]=1;
        p[vp(v,Pulse)]=r.range(0.15f,0.75f);
        p[vp(v,VAttack)]=cat==3?float(r.integer(6,10)):float(r.integer(0,2));
        p[vp(v,Decay)]=float(cat==0?r.integer(4,8):r.integer(3,9));
        p[vp(v,Sustain)]=float(cat==4?r.integer(0,2):r.integer(cat==3?10:6,14));
        p[vp(v,VRelease)]=float(cat==3?r.integer(8,11):(cat==4?r.integer(5,9):r.integer(2,7)));
        p[vp(v,Ring)]=(cat>=4&&v==0)?1.0f:0;
        p[vp(v,Sync)]=(cat==5&&r.unit()<0.5f)?1.0f:0;
        p[vp(v,Route)]=1;
    }
    if(cat==4||cat==5) {p[vp(2,Enabled)]=1; p[vp(2,Tune)]=float(r.integer(-24,24));p[vp(2,Wave)]=0;}
    p[PitchSweep]=0;p[SweepTime]=80;p[ArpGate]=1;p[FilterEnvAmount]=0;p[FilterEnvDecay]=180;
    if(requested==6||requested==8){
        p=requested==6?discoPatch(r.integer(0,15)):synthpopPatch(r.integer(0,synthpopPatchCount-1));
        p[Cutoff]+=r.range(-130,130);p[Resonance]+=r.range(-1,1);
        p[FilterEnvDecay]*=r.range(0.75f,1.25f);p[vp(0,Pulse)]+=r.range(-0.035f,0.035f);
    }
    const auto& specs=parameterSpecs();
    for(int i=0;i<parameterCount;++i) {
        const unsigned section=sectionFor(i);
        if(!section||(section&locks)) p[i]=base[i];
        else if(mutate) {
            if(specs[i].step>0) {
                if(r.unit()>0.22f) p[i]=base[i];
                else if(specs[i].max-specs[i].min>15) p[i]=base[i]+float(r.integer(-1,1));
            } else p[i]=base[i]+(p[i]-base[i])*0.15f;
        }
    }
    if(!mutate){
        if(requested==7)p[PlayMode]=float(2+int(seed%3));
        else if(base[PlayMode]>=2)p[PlayMode]=1;
    }
    return sanitize(p);
}
} // namespace sidlab
