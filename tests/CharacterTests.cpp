#include "EpicDSP.h"
#include <cstdlib>
#include <iostream>
#include <complex>
void require(bool b,const char* label){if(!b){std::cerr<<"FAIL: "<<label<<'\n';std::exit(1);}}
double band(const std::vector<float>& signal,double frequency,int begin=48000) {
    std::complex<double> result{};const int end=int(signal.size());
    for(int n=begin;n<end;++n){const double phase=-6.283185307179586*frequency*n/48000.;result+=double(signal[size_t(n)])*std::complex<double>(std::cos(phase),std::sin(phase));}
    return std::abs(result)/double(end-begin);
}
int main(){
    std::array<std::vector<float>,4> outputs;
    for(int c=0;c<4;++c){epinizer::Engine engine;engine.prepare(48000);epinizer::Settings s{.55f,.75f,.50f,1.f,c,false,false,false};engine.snap(s);
        auto& l=outputs[size_t(c)];l.resize(96000);auto r=l;
        for(size_t n=0;n<l.size();++n)l[n]=r[n]=.16f*std::sin(float(n)*float(6.283185307179586*500./48000.));
        engine.process(l.data(),r.data(),int(l.size()),s);
        double side=0;for(size_t n=10000;n<l.size();++n)side+=double(l[n]-r[n])*(l[n]-r[n]);
        require(side/l.size()>1.e-5,"stereo ensemble produces width");
    }
    auto peakBand=[&](int c,int hz){double level=0;for(int f=hz-24;f<=hz+24;f+=2)level=std::max(level,band(outputs[size_t(c)],f));return level;};
    const double glassOctave=peakBand(0,1000),warmUp=peakBand(3,1000);
    const double massiveSub=peakBand(1,250),warmDown=peakBand(3,250);
    std::cout<<"Octave magnitudes: Glass 1000Hz="<<glassOctave<<" Warm="<<warmUp<<"; Massive 250Hz="<<massiveSub<<" Warm="<<warmDown<<'\n';
    require(glassOctave>std::max(.001,warmUp*5),"Glass adds an upper octave");
    require(massiveSub>std::max(.001,warmDown*5),"Massive adds a lower octave");
    for(int a=0;a<4;++a)for(int b=a+1;b<4;++b){double difference=0;for(size_t n=48000;n<outputs[0].size();++n){double v=outputs[size_t(a)][n]-outputs[size_t(b)][n];difference+=v*v;}require(difference/48000>.0001,"characters have distinct signal paths");}
    epinizer::Engine holdEngine;holdEngine.prepare(48000);epinizer::Settings h{.5f,.6f,.4f,1.f,0,false,true,false};holdEngine.snap(h);
    std::vector<float> l(48000),r(48000);for(int i=0;i<48000;++i)l[size_t(i)]=r[size_t(i)]=.18f*std::sin(float(i)*.061f);
    holdEngine.process(l.data(),r.data(),48000,h);h.hold=true;l.assign(48000*8,0);r=l;holdEngine.process(l.data(),r.data(),int(l.size()),h);
    double first=0,last=0;for(int i=48000;i<96000;++i)first+=double(l[size_t(i)])*l[size_t(i)];for(int i=48000*7;i<48000*8;++i)last+=double(l[size_t(i)])*l[size_t(i)];
    require(last>first*.6&&last<first*1.5,"Hold sustains without runaway gain");
    float seamStep=0;for(size_t n=96001;n<l.size();++n)seamStep=std::max(seamStep,std::abs(l[n]-l[n-1]));require(seamStep<.10f,"Hold loop has a smooth splice");
    h.hold=false;l.assign(48000*15,0);r=l;holdEngine.process(l.data(),r.data(),int(l.size()),h);double ending=0;for(size_t n=l.size()-48000;n<l.size();++n)ending+=double(l[n])*l[n];require(ending<1.e-8,"released Hold decays");
    // Reset cannot replay a previous captured atmosphere after a seek/transport reset.
    holdEngine.reset();h.hold=true;l.assign(48000,0);r=l;holdEngine.process(l.data(),r.data(),48000,h);require(band(l,500,0)==0,"reset clears captured Hold audio");
    std::cout<<"PASS: octave signatures, six-voice stereo width, four distinct characters, bounded Hold/splices/release/reset\n";
}
