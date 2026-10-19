// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DiscoPatches.h"
#include <algorithm>
namespace sidlab {
Patch discoPatch(int index) noexcept {
    auto p=defaultPatch();
    p[PlayMode]=0;p[Movement]=0;p[Arp]=4;p[ArpRate]=1;p[ArpGate]=0.55f;
    p[Grit]=0.2f;p[Master]=0.7f;p[Resonance]=3;p[Cutoff]=1100;
    // Each recipe explicitly chooses its sources and routing; no inherited layers.
    auto voice=[&](int v,int wave,int tune,float fine,float pw,int a,int d,int s,int r,bool routed=true){
        p[vp(v,Enabled)]=1;p[vp(v,Wave)]=float(wave);p[vp(v,Tune)]=float(tune);
        p[vp(v,Fine)]=fine;p[vp(v,Pulse)]=pw;p[vp(v,VAttack)]=float(a);
        p[vp(v,Decay)]=float(d);p[vp(v,Sustain)]=float(s);p[vp(v,VRelease)]=float(r);
        p[vp(v,Route)]=routed?1.0f:0.0f;
    };
    switch(std::clamp(index,0,15)){
        case 0: // Crystal Cartridge II: clear upper-register glass over a quiet sub.
            p[Model]=1;p[Cutoff]=1750;p[Resonance]=1;p[Grit]=0.08f;p[Arp]=9;p[ArpGate]=0.42f;
            voice(0,0,12,0,.5f,0,5,0,4,false);voice(1,2,0,-3,.28f,0,4,2,3);
            break;
        case 1: // Munich Midnight: rounded, single-source octave bass with filter bite.
            p[Model]=0;p[Cutoff]=260;p[Resonance]=5;p[FilterEnvAmount]=1150;p[FilterEnvDecay]=65;
            p[Arp]=11;p[ArpGate]=0.72f;p[Grit]=.38f;
            voice(0,1,-12,0,.5f,0,7,9,3);
            break;
        case 2: // Neon Autobahn: dry, hollow pulse engine with an unfiltered triangle floor.
            p[Model]=1;p[Cutoff]=1150;p[Resonance]=2;p[Arp]=4;p[ArpGate]=.28f;
            p[FilterEnvAmount]=650;p[FilterEnvDecay]=22;
            voice(0,2,0,0,.5f,0,4,0,2);voice(1,0,-12,0,.5f,0,4,0,2,false);
            break;
        case 3: // Chrome Circuit: hard-sync sweep, bright and deliberately aggressive.
            p[Model]=1;p[Cutoff]=1900;p[Resonance]=1;p[Grit]=.55f;p[Arp]=7;p[ArpGate]=.66f;
            p[PitchSweep]=19;p[SweepTime]=180;
            voice(0,1,0,0,.5f,0,6,10,3,false);p[vp(0,Sync)]=1;
            // Oscillator 3 is a silent, running sync source for oscillator 1.
            p[vp(2,Tune)]=-12;p[vp(2,Ratio)]=1.5f;
            break;
        case 4: // Transistor Nights: nasal band-pass double pulse, almost reed-like.
            p[Model]=1;p[FilterMode]=2;p[Cutoff]=900;p[Resonance]=8;p[Arp]=5;
            p[FilterEnvAmount]=550;p[FilterEnvDecay]=110;p[ArpGate]=.78f;
            voice(0,2,0,-4,.14f,1,7,8,5);voice(1,2,0,4,.19f,1,6,8,4);
            break;
        case 5: // Velvet Motorik: soft triangle/sub wood, no bright saw layer.
            p[Model]=1;p[Cutoff]=680;p[Resonance]=0;p[Grit]=0;p[Arp]=4;p[ArpGate]=.9f;
            p[FilterEnvAmount]=320;p[FilterEnvDecay]=210;p[Master]=.9f;
            voice(0,0,0,0,.5f,1,7,7,6);voice(1,0,-12,0,.5f,0,8,9,5);
            break;
        case 6: // Disco Databus: percussive saw brass with a high transient layer.
            p[Model]=0;p[Cutoff]=500;p[Resonance]=4;p[FilterEnvAmount]=1400;p[FilterEnvDecay]=105;
            p[Arp]=6;p[ArpGate]=.62f;p[Grit]=.42f;
            voice(0,1,0,-6,.5f,0,6,3,4);voice(1,1,0,6,.5f,0,6,3,4);
            voice(2,1,12,0,.5f,0,3,0,1);
            break;
        case 7: // Midnight Expressway: deep square/sub bass, slower envelope roll.
            p[Model]=0;p[Cutoff]=390;p[Resonance]=2;p[FilterEnvAmount]=500;p[FilterEnvDecay]=330;
            p[Arp]=11;p[ArpGate]=.86f;p[Grit]=.22f;
            voice(0,2,-12,0,.5f,0,8,11,5);voice(1,0,-24,0,.5f,0,7,10,4,false);
            break;
        case 8: // Glass Transmission: dry, metallic ring-modulated triangle mallet.
            p[Model]=1;p[Cutoff]=1800;p[Resonance]=0;p[Grit]=0;p[Arp]=10;p[ArpGate]=.5f;
            voice(0,0,0,0,.5f,0,5,0,5,false);p[vp(0,Ring)]=1;
            p[vp(2,Tune)]=19;p[vp(2,Ratio)]=1.125f;
            break;
        case 9: // Amber Sequencer: short, highly resonant octave pluck.
            p[Model]=0;p[Cutoff]=210;p[Resonance]=11;p[FilterEnvAmount]=1550;p[FilterEnvDecay]=32;
            p[Arp]=8;p[ArpGate]=.46f;p[Grit]=.12f;
            voice(0,2,0,0,.32f,0,5,5,2);
            break;
        case 10: // Silver Boulevard: wide high-pass string pulses with slow PWM.
            p[Model]=1;p[PlayMode]=1;p[FilterMode]=4;p[Cutoff]=620;p[Resonance]=1;
            p[Movement]=.75f;p[PwmDepth]=.55f;p[FilterDepth]=.03f;p[LfoRate]=.17f;
            p[Arp]=6;p[ArpGate]=1;p[Stereo]=.8f;p[Master]=.95f;
            voice(0,2,0,-9,.34f,5,8,12,8);voice(1,2,0,9,.61f,5,8,12,8);
            voice(2,1,12,0,.5f,6,7,6,7);
            break;
        case 11: // Pulse Pursuit: wiry upper-register PWM lead, one clear oscillator.
            p[Model]=1;p[Cutoff]=1850;p[Resonance]=2;p[Arp]=7;p[ArpRate]=0;p[ArpGate]=.8f;
            p[Movement]=.85f;p[PwmDepth]=.7f;p[FilterDepth]=0;p[LfoRate]=2.4f;
            voice(0,2,12,0,.12f,0,7,11,4,false);
            break;
        case 12: // Neon Discotheque: three-source analog-style poly brass stabs.
            p[Model]=1;p[PlayMode]=1;p[Cutoff]=650;p[Resonance]=3;p[FilterEnvAmount]=1200;
            p[FilterEnvDecay]=160;p[Arp]=6;p[ArpGate]=.48f;p[Stereo]=.65f;p[Master]=.9f;
            voice(0,1,0,-12,.5f,1,6,4,6);voice(1,2,0,12,.44f,1,7,6,6);
            voice(2,1,-12,0,.5f,0,7,5,5);
            break;
        case 13: // Afterhours Circuit: dark, gently swelling detuned pad sequencer.
            p[Model]=1;p[PlayMode]=1;p[Cutoff]=480;p[Resonance]=1;p[Arp]=5;p[ArpRate]=0;
            p[ArpGate]=1;p[Movement]=.3f;p[LfoRate]=.11f;p[PwmDepth]=.3f;
            p[FilterEnvAmount]=-300;p[FilterEnvDecay]=480;p[Stereo]=.9f;p[Master]=1;
            voice(0,0,0,-7,.5f,8,8,12,9);voice(1,2,0,7,.45f,7,8,10,9);
            voice(2,0,12,0,.5f,9,8,8,8,false);
            break;
        case 14: // Quartz Runner: high bell-like ratios, clean and chiming.
            p[Model]=1;p[Cutoff]=1950;p[Resonance]=0;p[Grit]=0;p[Arp]=9;p[ArpGate]=.6f;
            voice(0,0,12,0,.5f,0,6,0,6,false);voice(1,0,19,0,.5f,0,5,0,5,false);
            voice(2,0,24,0,.5f,0,4,0,4,false);p[vp(1,Ratio)]=1.003f;
            break;
        case 15: // Electric Horizon: open, rising mixed-wave fifths, cinematic sustain.
            p[Model]=0;p[Cutoff]=1300;p[Resonance]=3;p[Arp]=7;p[ArpGate]=1;p[ArpRate]=0;
            p[FilterEnvAmount]=-1000;p[FilterEnvDecay]=700;p[Grit]=.3f;
            voice(0,4,0,-4,.5f,4,8,11,8);voice(1,1,7,4,.5f,5,8,8,7);
            voice(2,0,-12,0,.5f,3,8,9,6,false);
            break;
    }
    return sanitize(p);
}
}
