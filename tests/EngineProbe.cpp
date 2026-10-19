#include "SidEngine.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
using namespace sidlab;
int main(int argc,char** argv) {
    const std::string path=argc>1?argv[1]:"sid-engine.wav";
    SidEngine engine;
    auto p=defaultPatch(); p[PlayMode]=0; p[FilterMode]=0;p[vp(0,Route)]=0;
    p[vp(0,Wave)]=1;p[vp(0,Sustain)]=15;
    engine.setPatch(p);engine.prepare(48000);
    std::vector<float> l(144000),r(l.size());
    engine.midi({0x90,57,110});engine.render(l.data(),r.data(),96000);
    engine.midi({0x80,57,0});engine.render(l.data()+96000,r.data()+96000,48000);
    double energy=0,peak=0,tail=0;
    for(size_t i=0;i<l.size();++i) { if(!std::isfinite(l[i]))return 2;energy+=l[i]*l[i];peak=std::max(peak,double(std::abs(l[i])));if(i>132000) tail+=l[i]*l[i]; }
    const double rms=std::sqrt(energy/l.size());
    std::ofstream out(path,std::ios::binary); const uint32_t data=uint32_t(l.size()*4),size=data+36;
    auto u32=[&](uint32_t x){out.write(reinterpret_cast<char*>(&x),4);};
    auto u16=[&](uint16_t x){out.write(reinterpret_cast<char*>(&x),2);};
    out.write("RIFF",4);u32(size);out.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(48000);u32(192000);u16(4);u16(16);out.write("data",4);u32(data);
    for(size_t i=0;i<l.size();++i) { u16(static_cast<uint16_t>(int16_t(l[i]*32767)));u16(static_cast<uint16_t>(int16_t(r[i]*32767))); }
    std::cout<<"real libresidfp render: RMS="<<rms<<" peak="<<peak<<" release-tail RMS="<<std::sqrt(tail/12000)<<" WAV="<<path<<"\n";
    return rms>0.001&&peak<1&&std::sqrt(tail/12000)<0.001?0:1;
}
