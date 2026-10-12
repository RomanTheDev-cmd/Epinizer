#include "EpicDSP.h"
#include <iostream>
#include <random>
#include <chrono>
#include <cstdlib>
void check(bool b,const char* message){if(!b){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main(){
    const auto start=std::chrono::steady_clock::now();
    for(double sr:{8000.,44100.,48000.,96000.,192000.})for(int character=0;character<4;++character){
        epinizer::Engine e;e.prepare(sr);epinizer::Settings s{1,1,1,1,character,false};e.snap(s);
        const int length=int(sr*15);std::vector<float> l(size_t(length),0),r(size_t(length),0);l[0]=r[0]=.9f;
        e.process(l.data(),r.data(),length,s);double energy=0,late=0;float peak=0;
        for(int i=0;i<length;++i){check(std::isfinite(l[size_t(i)])&&std::isfinite(r[size_t(i)]),"finite impulse response");peak=std::max(peak,std::abs(l[size_t(i)]));if(i>sr*.1)energy+=l[size_t(i)]*l[size_t(i)];if(i>sr*14)late+=l[size_t(i)]*l[size_t(i)];}
        check(peak<=1,"impulse bounded");check(energy>1.e-7,"audible spatial tail");check(late<1.e-8,"tail decays");
    }
    epinizer::Engine a,b;a.prepare(48000);b.prepare(48000);epinizer::Settings s{.9f,.8f,.9f,.7f,2,false};a.snap(s);b.snap(s);
    std::vector<float> l(48000),r(48000);std::mt19937 rng(42);std::uniform_real_distribution<float> noise(-.6f,.6f);for(auto& v:l)v=noise(rng);r=l;auto l2=l,r2=r;
    a.process(l.data(),r.data(),48000,s);for(int n=0;n<48000;){const int count=std::min(127,48000-n);b.process(l2.data()+n,r2.data()+n,count,s);n+=count;}check(l==l2&&r==r2,"block size invariant");
    for(bool bypass:{false,true}){a.prepare(48000);s.mix=bypass?.8f:0;s.bypass=bypass;a.snap(s);auto original=l;a.process(l.data(),nullptr,int(l.size()),s);check(l==original,"dry / bypass transparent");}
    a.prepare(48000);s={1,1,1,1,3,false};a.snap(s);std::fill(l.begin(),l.end(),8);std::fill(r.begin(),r.end(),-8);a.process(l.data(),r.data(),48000,s);for(float v:l)check(std::isfinite(v)&&std::abs(v)<1,"hot input limited");
    for(int block=0;block<1000;++block){s.character=block%4;s.space=float(block%2);s.density=1-s.space;s.motion=s.space;s.mix=.7f;s.bypass=block%11==0;for(int i=0;i<127;++i){l[size_t(i)]=noise(rng);r[size_t(i)]=noise(rng);}a.process(l.data(),r.data(),127,s);for(int i=0;i<127;++i)check(std::isfinite(l[size_t(i)])&&std::abs(l[size_t(i)])<2,"automation stress");}
    a.prepare(48000);s={.8f,.7f,.9f,.7f,0,false};a.snap(s);float previous=0,maxStep=0;
    for(int n=0;n<48000;++n){if(n%2400==0){s.character=(n/2400)%4;s.mix=1-s.mix;s.bypass=!s.bypass;s.echo=!s.echo;}float v=.3f*std::sin(float(n)*.057595865f);a.process(&v,nullptr,1,s);maxStep=std::max(maxStep,std::abs(v-previous));previous=v;}
    check(maxStep<.1f,"smooth automation transitions");
    a.prepare(48000);b.prepare(48000);s={.7f,.3f,.3f,1.f,0,false,true};a.snap(s);auto noEcho=s;noEcho.echo=false;b.snap(noEcho);
    l.assign(24000,0);r=l;l[0]=r[0]=.5f;l2=l;r2=r;
    a.process(l.data(),r.data(),24000,s);b.process(l2.data(),r2.data(),24000,noEcho);
    double echoDifference=0;for(int i=0;i<24000;++i){const float delta=l[size_t(i)]-l2[size_t(i)];if(i<6480)check(std::abs(delta)<1.e-7f,"echo preserves initial attack");else echoDifference+=double(delta)*delta;}
    check(echoDifference>1.e-6,"echo adds delayed energy");
    std::cout<<"PASS: 20 rate/character impulse tails, finite/bounded output, dry/bypass, mono, block invariance, hot input, automation, gentle echo. "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<" seconds\n";
}
