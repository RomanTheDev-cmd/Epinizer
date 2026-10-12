#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace legacy_epinizer {
struct Settings { float space=.6f, density=.35f, motion=.3f, mix=.45f; int character=0; bool bypass=false; bool echo=true; };
class Engine {
    struct Delay {
        std::vector<float> data; size_t head=0;
        void prepare(size_t n) { data.assign(n,0.f); head=0; }
        void clear() { std::fill(data.begin(),data.end(),0.f); head=0; }
        void push(float x) { data[head]=x; head=(head+1)%data.size(); }
        float read(float samples) const {
            float p=float(head)-samples; if(p<0) p+=float(data.size());
            if(p>=float(data.size())) p=0; // Floating-point wrap may round exactly to size.
            const auto a=size_t(p); const auto b=(a+1)%data.size();
            return data[a]+(data[b]-data[a])*(p-float(a));
        }
    };
    std::array<Delay,8> tank;
    std::array<Delay,2> shift,echoLine;
    std::array<float,8> lengths{}, damp{};
    std::array<float,4> macro{.6f,.35f,.3f,.45f};
    std::array<float,4> colour{1,0,0,0};
    std::array<float,2> dcIn{},dcOut{},tone{},echoTone{};
    std::array<double,2> phase{0,.37};
    double sr=48000; float smooth=.001f,colourSmooth=.001f,bypassGain=1,dcCoefficient=.995f,echoGain=1;
    static float safe(float x) { return std::isfinite(x) ? std::clamp(x,-16.f,16.f) : 0.f; }
    static float limit(float x) {
        const float a=std::abs(x);
        return a<=.85f ? x : std::copysign(.85f+.14f*std::tanh((a-.85f)/.14f),x);
    }
public:
    void prepare(double sampleRate) {
        sr=std::clamp(sampleRate,8000.,384000.);
        dcCoefficient=float(std::exp(-6.283185307*22./sr));
        smooth=float(1.-std::exp(-1./(.04*sr))); colourSmooth=float(1.-std::exp(-1./(.12*sr)));
        constexpr std::array<float,8> ms{29.7f,37.1f,41.1f,43.7f,53.1f,61.7f,71.3f,79.9f};
        for(size_t i=0;i<8;++i) { lengths[i]=float(sr)*ms[i]*.001f; tank[i].prepare(size_t(lengths[i])+4); }
        for(auto& d:shift) d.prepare(size_t(sr*.09)+8);
        for(auto& d:echoLine) d.prepare(size_t(sr*.25)+8);
        reset();
    }
    void reset() {
        for(auto& d:tank) d.clear(); for(auto& d:shift) d.clear(); for(auto& d:echoLine) d.clear();
        damp.fill(0); dcIn.fill(0);dcOut.fill(0);tone.fill(0);echoTone.fill(0);phase={0,.37};
    }
    void snap(const Settings& s) {
        macro={s.space,s.density,s.motion,s.mix}; colour.fill(0);colour[size_t(std::clamp(s.character,0,3))]=1;
        bypassGain=s.bypass?0.f:1.f;echoGain=s.echo?1.f:0.f;
    }
    void process(float* left,float* right,int count,const Settings& s) {
        const std::array<float,4> target{std::clamp(s.space,0.f,1.f),std::clamp(s.density,0.f,1.f),std::clamp(s.motion,0.f,1.f),std::clamp(s.mix,0.f,1.f)};
        for(int n=0;n<count;++n) {
            for(size_t k=0;k<4;++k) {macro[k]+=smooth*(target[k]-macro[k]); colour[k]+=colourSmooth*((int(k)==s.character?1.f:0.f)-colour[k]);}
            bypassGain+=smooth*((s.bypass?0.f:1.f)-bypassGain);
            echoGain+=smooth*((s.echo?1.f:0.f)-echoGain);
            const float space=macro[0], density=macro[1], motion=macro[2];
            const float glass=colour[0], massive=colour[1], horror=colour[2],warm=colour[3];
            const float cutoff=6500*glass+4300*massive+2600*horror+2100*warm;
            const float lp=float(1.-std::exp(-6.283185307*cutoff/sr));
            const float feedback=.45f+space*.43f+massive*.025f+horror*.015f;
            const float drive=1.f+density*(1.5f*glass+3.f*massive+5.f*horror+2.5f*warm);
            const float dry[2]{safe(left[n]),safe(right?right[n]:left[n])};
            float excited[2], detuned[2];
            for(size_t c=0;c<2;++c) {
                const float dc=dry[c]-dcIn[c]+dcCoefficient*dcOut[c]; dcIn[c]=dry[c]; dcOut[c]=dc;
                const float sat=std::tanh(dc*drive)/std::sqrt(drive);
                tone[c]+=lp*(sat-tone[c]);
                excited[c]=(sat*(.8f-.35f*warm)+tone[c]*(.2f+.35f*warm));
                shift[c].push(excited[c]);
                const double cents=(2.+motion*(10.+horror*12.))*(c==0?1.:-1.);
                const double ratio=std::pow(2.,cents/1200.);
                phase[c]+=(1.-ratio)/(sr*.04); phase[c]-=std::floor(phase[c]);
                const float p=float(phase[c]),q=std::fmod(p+.5f,1.f);
                const float w=.5f-.5f*std::cos(6.283185307f*p);
                detuned[c]=w*shift[c].read(float(sr)*(.008f+.04f*p))+(1-w)*shift[c].read(float(sr)*(.008f+.04f*q));
            }
            // Quiet, filtered stereo repeats preserve the attack and widen the tail.
            const float echoTap[2]{echoLine[0].read(float(sr)*.135f),echoLine[1].read(float(sr)*.2025f)};
            for(size_t c=0;c<2;++c){
                echoTone[c]+=(lp*.65f)*(echoTap[c]-echoTone[c]);
                const float v=excited[c]*.8f+detuned[c]*motion*.2f+echoTap[1-c]*(.18f+.12f*space);
                echoLine[c].push(std::abs(v)<1.e-20f?0.f:v);
            }
            std::array<float,8> taps{}; float sum=0;
            for(size_t k=0;k<8;++k) { float v=tank[k].read(lengths[k]); damp[k]+=lp*(v-damp[k]); taps[k]=damp[k]; sum+=taps[k]; }
            for(size_t k=0;k<8;++k) {
                float input=(excited[k%2]*.65f+detuned[k%2]*motion*.55f)*.22f;
                if(k&2) input=-input;
                float v=input+feedback*(taps[k]-sum*.25f);
                tank[k].push(std::abs(v)<1.e-20f?0.f:v);
            }
            const float reverb[2]{(taps[0]+taps[2]-taps[4]+taps[6])*.55f,(taps[1]+taps[3]-taps[5]+taps[7])*.55f};
            for(size_t c=0;c<2;++c) {
                float wet=excited[c]*.48f+detuned[c]*motion*.65f+reverb[c]*(.45f+space*.85f)+echoTone[c]*echoGain*(.12f+.16f*motion);
                float mixed=dry[c]+macro[3]*(wet-dry[c]);
                // Dry and bypass are bit transparent at their endpoints; processed signal has a soft safety ceiling.
                float processed=mixed+macro[3]*(limit(mixed)-mixed);
                const float out=dry[c]+bypassGain*(processed-dry[c]);
                if(c==0) left[n]=out; else if(right) right[n]=out;
            }
        }
    }
};
}
