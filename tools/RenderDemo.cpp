#include "EpicDSP.h"
#include "LegacyDSP.h"
#include <fstream>
#include <string>
#include <cstdint>
#include <iostream>
#include <iterator>
void wav(const std::string& name,const std::vector<float>& l,const std::vector<float>& r){
    std::ofstream out(name,std::ios::binary);auto u16=[&](uint16_t v){out.put(char(v));out.put(char(v>>8));};auto u32=[&](uint32_t v){u16(uint16_t(v));u16(uint16_t(v>>16));};
    out.write("RIFF",4);u32(uint32_t(l.size()*4+36));out.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(48000);u32(192000);u16(4);u16(16);out.write("data",4);u32(uint32_t(l.size()*4));
    for(size_t i=0;i<l.size();++i){u16(uint16_t(int16_t(std::clamp(l[i],-1.f,1.f)*32767)));u16(uint16_t(int16_t(std::clamp(r[i],-1.f,1.f)*32767)));}
}
double rms(const std::vector<float>& l,const std::vector<float>& r,size_t frames){double sum=0;for(size_t i=0;i<frames;++i)sum+=double(l[i])*l[i]+double(r[i])*r[i];return std::sqrt(sum/(2*double(frames)));}
void match(std::vector<float>& l,std::vector<float>& r,double target,size_t active){
    const double gain=target/std::max(1.e-9,rms(l,r,active));float peak=0;for(size_t i=0;i<l.size();++i)peak=std::max({peak,std::abs(l[i]),std::abs(r[i])});
    const float safeGain=float(std::min(gain,.95/std::max(1.e-9,double(peak))));for(size_t i=0;i<l.size();++i){l[i]*=safeGain;r[i]*=safeGain;}
}
void render(const std::string& dir,const std::string& source,std::vector<float> dry,size_t active){
    dry.resize(active+48000*4,0);const double reference=rms(dry,dry,active);wav(dir+"/"+source+"-00-Dry.wav",dry,dry);
    const char* names[]{"Glass","Massive","Horroble","Warm"};
    for(int c=0;c<4;++c){
        auto l=dry,r=dry;epinizer::Engine engine;engine.prepare(48000);epinizer::Settings s{.55f,.72f,.55f,.65f,c,false,true,false};engine.snap(s);engine.process(l.data(),r.data(),int(l.size()),s);match(l,r,reference,active);
        wav(dir+"/"+source+"-0"+std::to_string(c+1)+"-"+names[c]+"-v2.wav",l,r);
        auto oldL=dry,oldR=dry;legacy_epinizer::Engine old;old.prepare(48000);legacy_epinizer::Settings legacy{s.space,s.density,s.motion,s.mix,c,false,true};old.snap(legacy);old.process(oldL.data(),oldR.data(),int(oldL.size()),legacy);match(oldL,oldR,reference,active);
        wav(dir+"/"+source+"-0"+std::to_string(c+1)+"-"+names[c]+"-v1.wav",oldL,oldR);
        std::vector<float> abL,abR;for(const auto* channel:{&oldL,&l}){abL.insert(abL.end(),channel->begin(),channel->end());abL.insert(abL.end(),24000,0);}for(const auto* channel:{&oldR,&r}){abR.insert(abR.end(),channel->begin(),channel->end());abR.insert(abR.end(),24000,0);}
        wav(dir+"/"+source+"-0"+std::to_string(c+1)+"-"+names[c]+"-AB-v1-then-v2.wav",abL,abR);
        double delta=0;for(size_t n=0;n<active;++n){double d=l[n]-oldL[n];delta+=d*d;}
        std::cout<<source<<" / "<<names[c]<<": normalized difference RMS "<<std::sqrt(delta/active)<<" (reference "<<reference<<")\n";
    }
}
int main(int argc,char** argv){
    if(argc<2)return 1;const std::string dir=argv[1];constexpr size_t active=48000*5;
    std::vector<float> synth(active),drums(active);
    for(size_t i=0;i<active;++i){const double t=double(i)/48000.,beat=std::fmod(t,.75);
        for(double hz:{130.8128,164.8138,195.9977})synth[i]+=float(.085*(std::sin(6.283185307*hz*t)+.25*std::sin(12.566370614*hz*t))*std::exp(-beat*5));
        const double noise=std::sin(double(i)*12.9898)*43758.5453;
        drums[i]=float(.40*std::sin(6.283185307*(60*beat+9*(1-std::exp(-beat*35))))*std::exp(-beat*27)+.14*(2*(noise-std::floor(noise))-1)*std::exp(-beat*70));
    }
    render(dir,"Synth",synth,active);render(dir,"Drums",drums,active);
    if(argc>2){std::ifstream file(argv[2],std::ios::binary);file.seekg(0,std::ios::end);const auto bytes=file.tellg();file.seekg(0);std::vector<float> voice(size_t(bytes)/4);file.read(reinterpret_cast<char*>(voice.data()),bytes);const auto frames=voice.size();if(frames)render(dir,"Voice",voice,frames);}
    // Freeze demo: play two seconds, hold for three, release into two seconds of silence.
    epinizer::Engine engine;engine.prepare(48000);epinizer::Settings settings;settings.mix=.7f;engine.snap(settings);std::vector<float> l(48000*7,0),r=l;
    std::copy_n(synth.begin(),48000*2,l.begin());r=l;engine.process(l.data(),r.data(),48000*2,settings);settings.hold=true;engine.process(l.data()+96000,r.data()+96000,144000,settings);settings.hold=false;engine.process(l.data()+240000,r.data()+240000,96000,settings);wav(dir+"/Hold-capture-sustain-release.wav",l,r);
}
