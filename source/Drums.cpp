// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Drums.h"
#include <algorithm>
namespace sidlab {
DrumType drumForNote(int note) noexcept {
    switch(note){
        case 35:case 36:return DrumType::Kick;
        case 37:return DrumType::Rim;
        case 38:case 40:return DrumType::Snare;
        case 39:return DrumType::Clap;
        case 42:case 44:return DrumType::ClosedHat;
        case 46:return DrumType::OpenHat;
        case 41:case 43:case 45:return DrumType::LowTom;
        case 47:case 48:case 50:return DrumType::HighTom;
        case 49:case 52:case 55:case 57:return DrumType::Crash;
        case 51:case 53:case 59:return DrumType::Ride;
        case 56:return DrumType::Cowbell;
        default:break;
    }
    // Every octave remains playable on a 49-key controller, with GM overrides above.
    constexpr std::array<DrumType,12> octave{DrumType::Kick,DrumType::Rim,DrumType::Snare,
        DrumType::Clap,DrumType::ClosedHat,DrumType::LowTom,DrumType::ClosedHat,
        DrumType::HighTom,DrumType::Cowbell,DrumType::Crash,DrumType::OpenHat,DrumType::Ride};
    return octave[std::clamp(note,0,127)%12];
}
DrumRecipe makeDrum(const Patch& base,int kit,int note) noexcept {
    DrumRecipe d;d.patch=base;d.type=drumForNote(note);auto& p=d.patch;
    p[Arp]=0;p[FilterEnvAmount]=0;p[PitchSweep]=0;p[SweepTime]=40;p[Movement]=base[Movement]*0.2f;
    p[FilterMode]=1;
    for(int v=0;v<3;++v){
        p[vp(v,Enabled)]=v==0?1.0f:0.0f;p[vp(v,Wave)]=0;p[vp(v,Tune)]=0;
        p[vp(v,Fine)]=0;p[vp(v,Ratio)]=1;p[vp(v,VAttack)]=0;
        p[vp(v,Decay)]=4;p[vp(v,Sustain)]=0;p[vp(v,VRelease)]=4;
        p[vp(v,Ring)]=0;p[vp(v,Sync)]=0;p[vp(v,Route)]=1;p[vp(v,Pulse)]=0.5f;
    }
    float cutoff=1400,resonance=1;
    switch(d.type){
        case DrumType::Kick:
            d.rootNote=kit==2?33.0f:36.0f;p[PitchSweep]=kit==3?40:30;p[SweepTime]=kit==2?38:24;
            p[vp(0,VRelease)]=kit==2?7:6;p[vp(0,Decay)]=7;cutoff=1000;
            if(kit==3){p[vp(1,Enabled)]=1;p[vp(1,Wave)]=2;p[vp(1,Tune)]=12;p[vp(1,Sustain)]=0;p[vp(1,VRelease)]=2;}
            break;
        case DrumType::Snare:
            d.rootNote=52;p[vp(0,Wave)]=3;p[vp(0,VRelease)]=5;
            p[vp(1,Enabled)]=1;p[vp(1,Wave)]=0;p[vp(1,Tune)]=-6;p[vp(1,Route)]=0;p[vp(1,VRelease)]=3;
            p[FilterMode]=4;cutoff=kit==2?750:1050;resonance=4;p[PitchSweep]=7;p[SweepTime]=12;
            break;
        case DrumType::Clap:
            d.rootNote=76;p[vp(0,Wave)]=3;p[vp(0,VRelease)]=6;p[FilterMode]=2;
            cutoff=1200;resonance=8;d.gateSeconds=0.045;break;
        case DrumType::ClosedHat:case DrumType::OpenHat:
            d.rootNote=98;p[vp(0,Wave)]=3;p[FilterMode]=4;cutoff=1450;resonance=3;
            p[vp(0,VRelease)]=d.type==DrumType::OpenHat?(kit==2?9:8):2;
            d.gateSeconds=0.004;
            if(kit==3){p[vp(1,Enabled)]=1;p[vp(1,Wave)]=0;p[vp(1,Ring)]=1;p[vp(1,VRelease)]=2;p[vp(2,Enabled)]=1;p[vp(2,Wave)]=2;p[vp(2,Tune)]=-7;p[vp(2,VRelease)]=1;}
            break;
        case DrumType::LowTom:case DrumType::HighTom:
            d.rootNote=d.type==DrumType::LowTom?43.0f:50.0f;p[PitchSweep]=14;p[SweepTime]=55;
            p[vp(0,VRelease)]=7;p[vp(1,Enabled)]=1;p[vp(1,Wave)]=0;p[vp(1,Tune)]=7;p[vp(1,VRelease)]=5;
            cutoff=1200;break;
        case DrumType::Rim:
            d.rootNote=84;p[vp(0,Wave)]=2;p[vp(0,Pulse)]=0.12f;p[vp(0,VRelease)]=1;
            p[vp(1,Enabled)]=1;p[vp(1,Wave)]=0;p[vp(1,Tune)]=19;p[vp(1,VRelease)]=0;
            p[FilterMode]=2;cutoff=1650;resonance=6;d.gateSeconds=0.003;break;
        case DrumType::Cowbell:
            d.rootNote=77;p[vp(0,Wave)]=2;p[vp(0,Pulse)]=0.45f;p[vp(0,VRelease)]=6;
            p[vp(1,Enabled)]=1;p[vp(1,Wave)]=2;p[vp(1,Tune)]=7;p[vp(1,Fine)]=-18;p[vp(1,VRelease)]=5;
            p[FilterMode]=2;cutoff=1500;resonance=3;break;
        case DrumType::Crash:case DrumType::Ride:
            d.rootNote=96;p[vp(0,Wave)]=3;p[vp(0,VRelease)]=d.type==DrumType::Crash?10:8;
            p[vp(1,Enabled)]=1;p[vp(1,Wave)]=2;p[vp(1,Tune)]=-7;p[vp(1,VRelease)]=4;
            p[FilterMode]=4;cutoff=1250;resonance=5;break;
    }
    p[Cutoff]=std::clamp(cutoff*base[Cutoff]/1200.0f,100.0f,2047.0f);
    p[Resonance]=std::clamp(resonance+base[Resonance]-4.0f,0.0f,15.0f);
    p[PitchSweep]+=base[PitchSweep];
    if(kit==3)p[Grit]=std::max(p[Grit],0.65f);
    d.patch=sanitize(p);return d;
}
} // namespace sidlab
