// SPDX-License-Identifier: AGPL-3.0-or-later
#include "AudioDisplay.h"
#include <algorithm>
#include <cmath>
namespace sidlab {
namespace {
const auto cyan=juce::Colour(0xff72ded2),purple=juce::Colour(0xffb8ace0),amber=juce::Colour(0xffefbc77);
constexpr float pi=juce::MathConstants<float>::pi;
}
AudioDisplay::AudioDisplay(SidEngine& e):engine(e){
    for(size_t i=0;i<window.size();++i)window[i]=0.5f-0.5f*std::cos(2*pi*float(i)/float(window.size()-1));
    update();
}
void AudioDisplay::update(){
    rate=engine.visualSampleRate.load(std::memory_order_relaxed);
    const unsigned head=engine.scopeHead.load(std::memory_order_acquire);
    orbitCentre=0;
    for(unsigned i=0;i<wave.size();++i){wave[i]=engine.scope[(head+i)%wave.size()].load(std::memory_order_relaxed);orbitCentre+=wave[i]/float(wave.size());}
    signalPeak=0;for(float x:wave)signalPeak=std::max(signalPeak,std::abs(x-orbitCentre));
    const unsigned ah=engine.analysisHead.load(std::memory_order_acquire);
    fftData.fill(0);
    float dc=0;for(unsigned i=0;i<1024;++i){fftData[i]=engine.analysis[(ah+i)%1024].load(std::memory_order_relaxed);dc+=fftData[i]/1024.0f;}
    for(unsigned i=0;i<1024;++i)fftData[i]=(fftData[i]-dc)*window[i];
    fft.performFrequencyOnlyForwardTransform(fftData.data(),true);
    const double maxHz=std::min(16000.0,rate*0.48);
    for(int band=0;band<32;++band){
        const double lo=35.0*std::pow(maxHz/35.0,band/32.0),hi=35.0*std::pow(maxHz/35.0,(band+1)/32.0);
        const int first=std::clamp(int(lo*1024/rate),1,510),last=std::clamp(int(std::ceil(hi*1024/rate)),first+1,512);
        float magnitude=0;for(int bin=first;bin<last;++bin)magnitude=std::max(magnitude,fftData[size_t(bin)]/256.0f);
        const float db=20*std::log10(std::max(magnitude,0.00001f));
        const float level=std::clamp((db+84.0f)/78.0f,0.0f,1.0f);
        bands[size_t(band)]+=(level-bands[size_t(band)])*(level>bands[size_t(band)]?0.65f:0.19f);
        peaks[size_t(band)]=std::max(bands[size_t(band)],peaks[size_t(band)]-0.018f);
    }
    if(signalPeak>0.00002f)orbitAngle=std::fmod(orbitAngle+0.018f+std::min(signalPeak,0.1f)*0.12f,2*pi);
}
void AudioDisplay::paint(juce::Graphics& g,juce::Rectangle<float> b,int style,bool large) const {
    juce::Graphics::ScopedSaveState saved(g);g.reduceClipRegion(b.toNearestInt());
    const float mid=b.getCentreY();
    g.setColour(cyan.withAlpha(0.055f));
    for(int i=0;i<=12;++i)g.drawVerticalLine(int(b.getX()+i*b.getWidth()/12),b.getY(),b.getBottom());
    for(int i=0;i<=4;++i)g.drawHorizontalLine(int(b.getY()+i*b.getHeight()/4),b.getX(),b.getRight());
    if(style==1){
        const float step=b.getWidth()/32;
        juce::Path ridge;
        for(int i=0;i<32;++i){
            const float height=bands[size_t(i)]*(b.getHeight()-4),x=b.getX()+i*step+1;
            const auto colour=cyan.interpolatedWith(purple,float(i)/31);
            g.setGradientFill(juce::ColourGradient(colour.withAlpha(0.9f),x,b.getBottom()-height,
                colour.withAlpha(0.16f),x,b.getBottom(),false));
            g.fillRoundedRectangle(x,b.getBottom()-height,std::max(1.0f,step-2),std::max(0.5f,height),1);
            g.setColour(amber.withAlpha(0.8f));
            g.drawHorizontalLine(int(b.getBottom()-peaks[size_t(i)]*(b.getHeight()-4)),x,x+std::max(1.0f,step-2));
            if(i==0)ridge.startNewSubPath(x,b.getBottom()-height);else ridge.lineTo(x,b.getBottom()-height);
        }
        g.setColour(cyan.withAlpha(0.25f));g.strokePath(ridge,juce::PathStrokeType(1));
    } else if(style==2){
        g.setColour(purple.withAlpha(0.09f));
        g.drawEllipse(b.getCentreX()-b.getWidth()*0.36f,mid-b.getHeight()*0.36f,b.getWidth()*0.72f,b.getHeight()*0.72f,1);
        g.drawHorizontalLine(int(mid),b.getX(),b.getRight());
        const float gain=1.0f/std::max(signalPeak,0.0005f),cs=std::cos(orbitAngle),sn=std::sin(orbitAngle);
        for(int layer=0;layer<3;++layer){
            juce::Path orbit;
            for(unsigned i=0;i<256;++i){
                const unsigned t=(i+unsigned(layer*17))%256;
                const float x=(wave[t]-orbitCentre)*gain,y=(wave[(t+7+unsigned(layer*5))%256]-orbitCentre)*gain,z=(wave[(t+19+unsigned(layer*11))%256]-orbitCentre)*gain;
                const float px=b.getCentreX()+(cs*x+sn*z)*b.getWidth()*0.30f;
                const float py=mid+(y*0.8f+(sn*x-cs*z)*0.27f)*b.getHeight()*0.36f;
                if(i==0)orbit.startNewSubPath(px,py);else orbit.lineTo(px,py);
            }
            const auto colour=layer==0?cyan:(layer==1?purple:amber);
            g.setColour(colour.withAlpha(0.045f));g.strokePath(orbit,juce::PathStrokeType(large?9.0f:5.0f));
            g.setColour(colour.withAlpha(0.16f));g.strokePath(orbit,juce::PathStrokeType(3));
            g.setColour(colour.withAlpha(layer==0?0.9f:0.4f));g.strokePath(orbit,juce::PathStrokeType(large?1.2f:0.8f));
        }
    } else {
        g.setColour(cyan.withAlpha(0.13f));g.drawHorizontalLine(int(mid),b.getX(),b.getRight());
        juce::Path trace;
        for(unsigned i=0;i<256;++i){const float x=b.getX()+i*b.getWidth()/255.0f,y=mid-std::clamp(wave[i]*8,-1.0f,1.0f)*b.getHeight()*0.44f;if(i==0)trace.startNewSubPath(x,y);else trace.lineTo(x,y);}
        g.setColour(cyan.withAlpha(0.08f));g.strokePath(trace,juce::PathStrokeType(7));
        g.setColour(cyan.withAlpha(0.18f));g.strokePath(trace,juce::PathStrokeType(3));
        g.setColour(cyan);g.strokePath(trace,juce::PathStrokeType(1.2f));
    }
}
} // namespace sidlab
