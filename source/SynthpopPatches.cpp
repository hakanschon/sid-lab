// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SynthpopPatches.h"
#include <algorithm>
namespace sidlab {
Patch synthpopPatch(int index) noexcept {
    auto p=defaultPatch();
    p[Model]=1;p[PlayMode]=0;p[Movement]=0;p[Arp]=0;p[Grit]=.1f;
    p[Resonance]=2;p[Cutoff]=1450;p[Master]=.75f;
    auto voice=[&](int v,int wave,int tune,float fine,float pw,int a,int d,int s,int r,bool routed=true){
        p[vp(v,Enabled)]=1;p[vp(v,Wave)]=float(wave);p[vp(v,Tune)]=float(tune);
        p[vp(v,Fine)]=fine;p[vp(v,Pulse)]=pw;p[vp(v,VAttack)]=float(a);
        p[vp(v,Decay)]=float(d);p[vp(v,Sustain)]=float(s);p[vp(v,VRelease)]=float(r);
        p[vp(v,Route)]=routed?1.0f:0.0f;
    };
    switch(std::clamp(index,0,synthpopPatchCount-1)){
        case 0: // Japan Fifth Bass: a played two-oscillator fifth, not an arp melody.
            p[Model]=0;p[Cutoff]=360;p[Resonance]=3;p[Grit]=.3f;
            p[FilterEnvAmount]=1000;p[FilterEnvDecay]=80;
            voice(0,2,-12,-2,.44f,0,6,6,3);voice(1,1,-5,2,.5f,0,5,3,2);
            break;
        case 1: // Tokyo Neon Pluck: crisp pulse transient and a clean upper triangle.
            p[Cutoff]=1050;p[FilterEnvAmount]=950;p[FilterEnvDecay]=48;p[Resonance]=4;
            voice(0,2,0,0,.23f,0,5,0,4);voice(1,0,12,0,.5f,0,4,0,3,false);
            break;
        case 2: // Forever Brass: expressive sustained poly brass, gently opening onset.
            p[PlayMode]=1;p[Cutoff]=620;p[FilterEnvAmount]=1050;p[FilterEnvDecay]=270;
            p[Master]=.95f;p[Stereo]=.5f;
            voice(0,1,0,-7,.5f,2,8,11,7);voice(1,2,0,7,.48f,3,8,10,7);
            voice(2,0,-12,0,.5f,1,7,8,6);
            break;
        case 3: // Melody String Machine: detuned pulse ensemble and octave sheen.
            p[PlayMode]=1;p[FilterMode]=4;p[Cutoff]=260;p[Resonance]=0;p[Grit]=0;
            p[Movement]=.8f;p[PwmDepth]=.52f;p[FilterDepth]=.015f;p[LfoRate]=.21f;
            p[Master]=1;p[Stereo]=.85f;
            voice(0,2,0,-11,.32f,6,8,13,9);voice(1,2,0,11,.67f,6,8,13,9);
            voice(2,1,12,0,.5f,7,7,6,8);
            break;
        case 4: // Berlin Glass Bells: pure partials with different decay times.
            p[PlayMode]=1;p[Cutoff]=1950;p[Resonance]=0;p[Grit]=0;p[Master]=.9f;
            voice(0,0,0,0,.5f,0,8,0,7,false);voice(1,0,12,4,.5f,0,6,0,5,false);
            voice(2,0,19,-3,.5f,0,4,0,4,false);p[vp(2,Ratio)]=1.17f;
            break;
        case 5: // Summer Voltage Lead: singing, close-detuned saws with a dark body.
            p[Model]=0;p[Cutoff]=860;p[Resonance]=4;p[FilterEnvAmount]=650;p[FilterEnvDecay]=310;
            p[Grit]=.25f;
            voice(0,1,0,-5,.5f,1,8,12,5);voice(1,1,0,5,.5f,1,8,12,5);
            break;
        case 6: // Night Drive Octaves: the bank's low pulse sequencer.
            p[Model]=0;p[Cutoff]=470;p[FilterEnvAmount]=780;p[FilterEnvDecay]=55;
            p[Arp]=11;p[ArpRate]=1;p[ArpGate]=.55f;
            voice(0,2,-12,0,.35f,0,6,7,3);voice(1,0,-24,0,.5f,0,5,4,2,false);
            break;
        case 7: // Diamond Pad: slowly breathing poly layer with a clear upper edge.
            p[PlayMode]=1;p[Cutoff]=1020;p[Resonance]=1;p[Master]=1;p[Stereo]=.8f;
            p[Movement]=.45f;p[PwmDepth]=.45f;p[FilterDepth]=.06f;p[LfoRate]=.13f;
            p[FilterEnvAmount]=-650;p[FilterEnvDecay]=900;
            voice(0,2,0,-6,.42f,8,8,12,10);voice(1,0,0,6,.5f,9,8,12,10);
            voice(2,1,12,0,.5f,9,7,4,9);
            break;
        case 8: // Jet Set Sync Lead: biting synced saw, short pitched attack.
            p[Cutoff]=1850;p[Resonance]=1;p[Grit]=.4f;p[PitchSweep]=7;p[SweepTime]=38;
            voice(0,1,0,0,.5f,0,6,11,4);p[vp(0,Sync)]=1;
            p[vp(2,Tune)]=-12;p[vp(2,Ratio)]=1.25f;
            break;
        case 9: // Harbour Electric Keys: warm low-pass triangle/pulse keyboard.
            p[PlayMode]=1;p[Cutoff]=720;p[Resonance]=1;p[Master]=1;
            p[FilterEnvAmount]=650;p[FilterEnvDecay]=125;p[Stereo]=.45f;
            voice(0,0,0,0,.5f,0,8,1,7);voice(1,2,12,0,.5f,0,5,0,4);
            break;
        case 10: // Young Horizon Choir: soft band-pass texture and dry low body.
            p[PlayMode]=1;p[Model]=0;p[FilterMode]=2;p[Cutoff]=550;p[Resonance]=5;
            p[Master]=1;p[Stereo]=.65f;p[Movement]=.3f;p[PwmDepth]=.25f;
            p[FilterDepth]=.08f;p[LfoRate]=.19f;
            voice(0,2,0,-4,.42f,7,8,11,9);voice(1,4,7,4,.5f,8,8,8,9);
            voice(2,0,-12,0,.5f,7,8,3,8,false);
            break;
        case 11: // Japan Brass Stabs: sharp three-source chord hits with filter snap.
            p[PlayMode]=1;p[Cutoff]=420;p[Resonance]=4;p[Master]=1;p[Stereo]=.55f;
            p[FilterEnvAmount]=1500;p[FilterEnvDecay]=65;
            voice(0,1,0,-12,.5f,0,6,3,4);voice(1,1,0,12,.5f,0,6,3,4);
            voice(2,2,-12,0,.49f,0,5,2,3);
            break;
        case 12: // Midnight Marimba: hollow pulse/triangle mallet, no sustained layer.
            p[Cutoff]=850;p[Resonance]=0;p[Grit]=0;p[FilterEnvAmount]=450;p[FilterEnvDecay]=26;
            voice(0,0,0,0,.5f,0,5,0,4,false);voice(1,2,12,0,.5f,0,3,0,2);
            break;
        case 13: // Arcade Romance Lead: rounded PWM lead with a short sub transient.
            p[Cutoff]=1350;p[Resonance]=2;p[Movement]=.65f;p[PwmDepth]=.36f;
            p[FilterDepth]=.02f;p[LfoRate]=.8f;
            voice(0,2,0,0,.3f,2,7,13,6);voice(1,0,-12,0,.5f,0,4,0,2,false);
            break;
        case 14: // 1984 Spark Arp: light high-pass pulse figures with ring sparkle.
            p[FilterMode]=4;p[Cutoff]=420;p[Resonance]=2;p[Arp]=9;p[ArpRate]=1;p[ArpGate]=.62f;
            p[Movement]=.25f;p[PwmDepth]=.2f;p[FilterDepth]=0;p[LfoRate]=.31f;
            voice(0,2,0,-3,.22f,0,5,5,3);voice(1,0,12,3,.5f,0,4,0,3);
            p[vp(1,Ring)]=1;
            break;
        case 15: // Forever Fade Strings: warm low-pass saws with an opening swell.
            p[PlayMode]=1;p[Model]=0;p[Cutoff]=980;p[Resonance]=1;p[Master]=1;p[Stereo]=.9f;
            p[FilterEnvAmount]=-600;p[FilterEnvDecay]=1100;
            voice(0,1,0,-13,.5f,7,8,12,10);voice(1,1,0,13,.5f,8,8,12,10);
            voice(2,0,12,0,.5f,9,8,8,9,false);
            break;
        case 16: // Melody Hook Strings: fast bowed attack, clear upper string edge.
            p[PlayMode]=1;p[Cutoff]=1540;p[Resonance]=1;p[Grit]=0;p[Master]=1;p[Stereo]=.72f;
            p[FilterEnvAmount]=450;p[FilterEnvDecay]=160;p[Movement]=.22f;p[PwmDepth]=.25f;
            p[FilterDepth]=0;p[LfoRate]=.28f;
            voice(0,1,0,-8,.5f,2,7,13,5);voice(1,2,0,8,.42f,3,7,12,5);
            voice(2,1,12,0,.5f,1,6,5,4);
            break;
        case 17: // Melody Octave Drive: tight pulse bass with a dry sub and step snap.
            p[Model]=0;p[Cutoff]=400;p[Resonance]=4;p[Grit]=.25f;
            p[FilterEnvAmount]=950;p[FilterEnvDecay]=70;p[Arp]=11;p[ArpRate]=1;p[ArpGate]=.52f;
            voice(0,2,-12,0,.24f,0,6,6,3);voice(1,0,-24,0,.5f,0,6,4,2,false);
            break;
        case 18: // Melody Bright Lead: focused upper pulse and a softer triangle body.
            p[Cutoff]=1850;p[Resonance]=1;p[Movement]=.45f;p[PwmDepth]=.3f;
            p[FilterDepth]=0;p[LfoRate]=.45f;
            voice(0,2,0,-4,.16f,0,7,12,4,false);voice(1,0,0,4,.5f,1,7,9,5);
            break;
        case 19: // Melody Staccato Strings: short bow-like chord hits, no sustained bed.
            p[PlayMode]=1;p[Cutoff]=1000;p[Resonance]=2;p[Master]=1;p[Stereo]=.58f;
            p[FilterEnvAmount]=800;p[FilterEnvDecay]=90;
            voice(0,1,0,-10,.5f,1,5,0,3);voice(1,1,0,10,.5f,2,5,0,3);
            voice(2,2,12,0,.36f,1,4,0,2);
            break;
        case 20: // Melody Glass Pulse: upper triangle ring chime and a pulse transient.
            p[PlayMode]=1;p[Cutoff]=1900;p[Resonance]=0;p[Grit]=0;p[Master]=.95f;
            voice(0,0,12,0,.5f,0,7,0,6,false);p[vp(0,Ring)]=1;
            voice(1,2,0,0,.18f,0,3,0,2);p[vp(2,Tune)]=7;p[vp(2,Ratio)]=1.06f;
            break;
        case 21: // Melody Finale Ensemble: bright sustained octave strings for chords.
            p[PlayMode]=1;p[Cutoff]=1850;p[Resonance]=0;p[Grit]=0;p[Master]=1;p[Stereo]=1;
            p[Movement]=.55f;p[PwmDepth]=.42f;p[FilterDepth]=.015f;p[LfoRate]=.16f;
            voice(0,1,0,-14,.5f,5,8,13,9);voice(1,2,0,14,.62f,6,8,13,9);
            voice(2,1,12,0,.5f,7,8,8,8);
            break;
        case 22: // Melody Bandpass Keys: thin, short midrange pulse accents.
            p[FilterMode]=2;p[Cutoff]=650;p[Resonance]=6;
            p[FilterEnvAmount]=180;p[FilterEnvDecay]=50;
            voice(0,2,0,0,.09f,0,6,0,3);voice(1,0,12,0,.5f,0,4,0,2);
            break;
        case 23: // Melody Warm Backdrop: dark, broad pad under a brighter played part.
            p[PlayMode]=1;p[Model]=0;p[Cutoff]=750;p[Resonance]=1;p[Master]=1;p[Stereo]=.82f;
            p[FilterEnvAmount]=-500;p[FilterEnvDecay]=700;
            p[Movement]=.25f;p[PwmDepth]=.6f;p[FilterDepth]=.1f;p[LfoRate]=.16f;
            voice(0,0,0,-17,.5f,9,8,10,10);voice(1,2,0,17,.56f,8,8,10,10);
            voice(2,2,-12,0,.33f,9,7,6,9);
            break;
    }
    return sanitize(p);
}
}
