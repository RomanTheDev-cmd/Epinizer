#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace epinizer {
struct Settings {
    float space=.48f, density=.58f, motion=.4f, mix=.55f;
    int character=0;
    bool bypass=false, echo=true, hold=false;
};

// All storage is allocated in prepare(). Each sample follows the same path,
// irrespective of the host's block size. No random generator or UI work here.
class Engine {
    static constexpr float pi=3.14159265358979323846f;
    struct Delay {
        std::vector<float> data;
        size_t head=0;
        void prepare(size_t n) { data.assign(n,0.f); head=0; }
        void clear() { std::fill(data.begin(),data.end(),0.f); head=0; }
        void push(float v) { data[head]=v; if(++head==data.size()) head=0; }
        float read(float samples) const {
            float p=float(head)-samples;
            if(p<0) p+=float(data.size());
            if(p>=float(data.size())) p=0;
            const auto a=size_t(p), b=(a+1)%data.size();
            return data[a]+(data[b]-data[a])*(p-float(a));
        }
        float at(size_t start,size_t offset) const { return data[(start+offset)%data.size()]; }
    };
    struct Oscillator {
        double sine=0,cosine=1,stepSin=0,stepCos=1;
        void prepare(double hz,double rate,double phase) {
            sine=std::sin(phase);cosine=std::cos(phase);
            stepSin=std::sin(2*pi*hz/rate);stepCos=std::cos(2*pi*hz/rate);
        }
        float next() {
            const double s=sine*stepCos+cosine*stepSin;
            cosine=cosine*stepCos-sine*stepSin;sine=s;
            return float(sine);
        }
    };
    std::array<Delay,8> tank;
    Delay ensembleLine,pitchLine,reverseLine;
    std::array<Delay,2> echoLine,holdLine;
    std::array<Oscillator,6> drift;
    std::array<double,6> voicePhase{};
    std::array<double,2> octavePhase{};
    std::array<float,8> lengths{},tankDamp{};
    std::array<float,6> voiceTone{};
    std::array<float,4> macro{.48f,.58f,.4f,.55f},colour{1,0,0,0};
    std::array<float,2> dcIn{},dcOut{},low{},echoTone{},reverseTone{},frozenTone{};
    double sr=48000;
    float smooth=0,colourSmooth=0,dcCoefficient=0,lowCoefficient=0,ensembleHighpass=0;
    float attackCoefficient=0,releaseCoefficient=0,slowCoefficient=0;
    float envelope=0,slowEnvelope=0,duck=1,bypassGain=1,echoGain=1,holdGain=0;
    float densityDrive=1,damping=0,feedback=0,detuneDepth=0;
    float outputGain=1;
    size_t controlCounter=0,freezeStart=0,freezeRead=0,freezeSpan=0,freezeFade=0;
    bool wasHolding=false;
    double reversePhase=0,ringPhase=0;
    float dcMonoIn=0,dcMonoOut=0;
    static float finite(float v) { return std::isfinite(v)?v:0.f; }
    static float unit(float v) { return std::clamp(finite(v),0.f,1.f); }
    static float ceiling(float x) {
        const float a=std::abs(x);
        return a<=.9f?x:std::copysign(.9f+.09f*std::tanh((a-.9f)/.09f),x);
    }
    void updateControls() {
        const float glass=colour[0],massive=colour[1],horror=colour[2],warm=colour[3];
        const float cutoff=7200*glass+3900*massive+2500*horror+2800*warm;
        damping=float(1.-std::exp(-2*pi*cutoff/sr));
        feedback=.40f+macro[0]*.44f+massive*.035f+horror*.025f;
        densityDrive=1.f+macro[1]*(1.2f*glass+3.4f*massive+2.5f*horror+2.f*warm);
        detuneDepth=3.f+macro[2]*(12.f+9.f*horror);
    }
    // Two complementary Hann windows remove discontinuities at a pitch-shift splice.
    float shifted(const Delay& line,double& phase,double ratio,float window,float base) {
        phase+=(1.-ratio)/double(window);phase-=std::floor(phase);
        const float p=float(phase),q=p>=.5f?p-.5f:p+.5f;
        const float w=.5f-.5f*std::cos(2*pi*p);
        return w*line.read(base+window*p)+(1-w)*line.read(base+window*q);
    }
    float reverseGrain(float offset,float duration) const {
        float p=float(reversePhase)+offset;p-=std::floor(p);
        const float q=p>=.5f?p-.5f:p+.5f;
        const float w=.5f-.5f*std::cos(2*pi*p);
        return w*reverseLine.read(float(sr)*.025f+2*duration*p)
             +(1-w)*reverseLine.read(float(sr)*.025f+2*duration*q);
    }
    float frozen(size_t channel) const {
        if(freezeRead<freezeSpan) return holdLine[channel].at(freezeStart,freezeRead);
        const auto overlap=freezeRead-freezeSpan;
        const float w=.5f-.5f*std::cos(pi*float(overlap)/float(freezeFade));
        return (1-w)*holdLine[channel].at(freezeStart,freezeRead)
             +w*holdLine[channel].at(freezeStart,overlap);
    }
public:
    void prepare(double sampleRate) {
        sr=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.,384000.):48000.;
        smooth=float(1.-std::exp(-1./(.04*sr)));
        colourSmooth=float(1.-std::exp(-1./(.12*sr)));
        dcCoefficient=float(std::exp(-2*pi*22./sr));
        lowCoefficient=float(1.-std::exp(-2*pi*170./sr));
        ensembleHighpass=float(std::exp(-2*pi*180./sr));
        attackCoefficient=float(1.-std::exp(-1./(.003*sr)));
        releaseCoefficient=float(1.-std::exp(-1./(.09*sr)));
        slowCoefficient=float(1.-std::exp(-1./(.18*sr)));
        constexpr std::array<float,8> ms{29.7f,37.1f,41.1f,43.7f,53.1f,61.7f,71.3f,79.9f};
        for(size_t i=0;i<8;++i) {
            lengths[i]=std::round(float(sr)*ms[i]*.001f);
            tank[i].prepare(size_t(lengths[i])+4);
        }
        ensembleLine.prepare(size_t(sr*.12)+8);
        pitchLine.prepare(size_t(sr*.14)+8);
        reverseLine.prepare(size_t(sr*.60)+8);
        for(auto& d:echoLine) d.prepare(size_t(sr*.36)+8);
        // A finite loop, not unity feedback: Hold cannot self-amplify.
        freezeSpan=size_t(sr*.28);freezeFade=size_t(sr*.035);
        for(auto& d:holdLine) d.prepare(freezeSpan+freezeFade);
        reset();
    }
    void reset() {
        for(auto& d:tank)d.clear();ensembleLine.clear();pitchLine.clear();reverseLine.clear();
        for(auto& d:echoLine)d.clear();for(auto& d:holdLine)d.clear();
        tankDamp.fill(0);voiceTone.fill(0);dcIn.fill(0);dcOut.fill(0);low.fill(0);
        echoTone.fill(0);reverseTone.fill(0);frozenTone.fill(0);
        constexpr std::array<double,6> rates{.19,.27,.37,.43,.13,.31};
        for(size_t i=0;i<6;++i) {
            voicePhase[i]=double(i)*.137;drift[i].prepare(rates[i],sr,double(i)*1.047);
        }
        octavePhase={.13,.61};reversePhase=0;ringPhase=0;
        envelope=slowEnvelope=0;duck=1;outputGain=1;controlCounter=0;
        freezeRead=freezeStart=0;wasHolding=false;holdGain=0;dcMonoIn=dcMonoOut=0;
        updateControls();
    }
    void snap(const Settings& s) {
        macro={unit(s.space),unit(s.density),unit(s.motion),unit(s.mix)};
        colour.fill(0);colour[size_t(std::clamp(s.character,0,3))]=1;
        bypassGain=s.bypass?0.f:1.f;echoGain=s.echo?1.f:0.f;
        // A restored Hold starts from silence until audio has actually been captured.
        updateControls();
    }
    void process(float* left,float* right,int count,const Settings& s) {
        const std::array<float,4> targets{unit(s.space),unit(s.density),unit(s.motion),unit(s.mix)};
        const int character=std::clamp(s.character,0,3);
        for(int n=0;n<count;++n) {
            for(size_t k=0;k<4;++k){macro[k]+=smooth*(targets[k]-macro[k]);colour[k]+=colourSmooth*((int(k)==character?1.f:0.f)-colour[k]);}
            bypassGain+=smooth*((s.bypass?0.f:1.f)-bypassGain);
            echoGain+=smooth*((s.echo?1.f:0.f)-echoGain);
            if((controlCounter++&31u)==0)updateControls();
            const float space=macro[0],density=macro[1],motion=macro[2];
            const float glass=colour[0],massive=colour[1],horror=colour[2],warm=colour[3];
            const float dry[2]{finite(left[n]),finite(right?right[n]:left[n])};
            float body[2],excited[2];
            const float inputLevel=std::min(8.f,std::max(std::abs(dry[0]),std::abs(dry[1])));
            envelope+=(inputLevel>envelope?attackCoefficient:releaseCoefficient)*(inputLevel-envelope);
            slowEnvelope+=slowCoefficient*(envelope-slowEnvelope);
            const float onset=std::clamp((envelope-slowEnvelope)/(slowEnvelope+.025f),0.f,1.f);
            const float duckTarget=1.f/(1.f+envelope*(5.f+space*7.f));
            duck+=(duckTarget<duck?attackCoefficient:slowCoefficient)*(duckTarget-duck);
            for(size_t c=0;c<2;++c) {
                const float input=std::clamp(dry[c],-16.f,16.f);
                const float dc=input-dcIn[c]+dcCoefficient*dcOut[c];dcIn[c]=input;dcOut[c]=dc;
                low[c]+=lowCoefficient*(dc-low[c]);body[c]=low[c];
                const float high=dc-low[c];
                const float saturated=std::tanh(high*densityDrive)/std::sqrt(densityDrive);
                excited[c]=saturated*(1.f+massive*density*onset*.28f)+low[c]*(1.f+massive*density*.22f);
            }
            const float mid=(excited[0]+excited[1])*.5f;
            const float monoHP=mid-dcMonoIn+ensembleHighpass*dcMonoOut;
            dcMonoIn=mid;dcMonoOut=monoHP;
            // Low body remains centred; six independently moving voices occupy the sides.
            ensembleLine.push(monoHP);pitchLine.push(monoHP);reverseLine.push(mid);
            float ensemble[2]{0,0};
            constexpr std::array<float,6> offsets{-1.f,1.f,-.55f,.55f,-.24f,.24f};
            constexpr std::array<float,6> panL{.96f,.04f,.80f,.20f,.64f,.36f};
            for(size_t v=0;v<6;++v) {
                const float wobble=drift[v].next();
                const float cents=offsets[v]*detuneDepth+wobble*(.7f+motion*(2.f+warm*2.f));
                const double ratio=1.+double(cents)*.0005776226504666;
                float voice=shifted(ensembleLine,voicePhase[v],ratio,float(sr)*(.027f+float(v)*.0023f),float(sr)*(.006f+float(v)*.0017f));
                voiceTone[v]+=damping*(voice-voiceTone[v]);
                const float gain=(v<2?1.f:std::clamp(density*(v<4?2.f:1.5f),0.f,1.f))*.32f;
                ensemble[0]+=voiceTone[v]*gain*panL[v];ensemble[1]+=voiceTone[v]*gain*(1-panL[v]);
            }
            const float up=shifted(pitchLine,octavePhase[0],2.,float(sr)*.065f,float(sr)*.008f);
            const float down=shifted(pitchLine,octavePhase[1],.5,float(sr)*.085f,float(sr)*.009f);
            // Horroble re-reads slices backwards and introduces frequency, not pitch, movement.
            const float slice=float(sr)*(.12f+.04f*space);
            reversePhase+=1./double(slice);reversePhase-=std::floor(reversePhase);
            ringPhase+=(21.+motion*47.)/sr;ringPhase-=std::floor(ringPhase);
            const float ring=std::sin(2*pi*float(ringPhase));
            float characterLayer[2];
            for(size_t c=0;c<2;++c) {
                reverseTone[c]+=(damping*.55f)*(reverseGrain(c==0?0.f:.29f,slice)-reverseTone[c]);
                characterLayer[c]=glass*up*(.10f+.33f*density)
                    +massive*down*(.08f+.30f*density)
                    +horror*(reverseTone[c]*(.30f+.40f*motion)+monoHP*ring*motion*.18f);
            }
            const float early[2]{ensemble[0]*.8f+characterLayer[0],ensemble[1]*.8f+characterLayer[1]};
            const float echoDelay[2]{float(sr)*(.135f+warm*(.055f+float(drift[0].sine)*(.0006f+.002f*motion))),float(sr)*(.2025f+warm*(.0825f+float(drift[3].sine)*(.0006f+.002f*motion)))};
            float echo[2];
            for(size_t c=0;c<2;++c)echo[c]=echoLine[c].read(echoDelay[c]);
            for(size_t c=0;c<2;++c) {
                echoTone[c]+=(damping*(.65f-.30f*warm))*(echo[c]-echoTone[c]);
                const float v=excited[c]*.7f+early[c]*.24f+echoTone[1-c]*(.18f+.18f*warm+.10f*space);
                echoLine[c].push(std::abs(v)<1.e-20f?0.f:std::tanh(v));
            }
            std::array<float,8> taps{};float sum=0;
            for(size_t k=0;k<8;++k){const float v=tank[k].read(lengths[k]);tankDamp[k]+=damping*(v-tankDamp[k]);taps[k]=tankDamp[k];sum+=taps[k];}
            for(size_t k=0;k<8;++k) {
                const size_t c=k%2;
                float input=(excited[c]*.35f+early[c]*.70f+echoTone[c]*echoGain*.14f)*.22f;
                if(k&2u)input=-input;
                const float v=input+feedback*(taps[k]-sum*.25f);
                tank[k].push(std::abs(v)<1.e-20f?0.f:v);
            }
            const float reverb[2]{(taps[0]+taps[2]-taps[4]+taps[6])*.52f,(taps[1]+taps[3]-taps[5]+taps[7])*.52f};
            float wet[2];
            const float bloom=.32f+.95f*duck;
            const float centredLow=(body[0]+body[1])*.5f;
            for(size_t c=0;c<2;++c)wet[c]=excited[c]*(.62f+.10f*warm)
                +ensemble[c]*(.55f+.50f*density)*(.55f+.45f*motion)
                +characterLayer[c]*(.75f+.25f*duck)
                +reverb[c]*space*bloom*(.40f+.35f*space)
                +echoTone[c]*echoGain*(.10f+.20f*motion+.14f*warm)*(.55f+.45f*duck)
                +centredLow*massive*density*.12f;
            if(s.hold&&!wasHolding){freezeStart=holdLine[0].head;freezeRead=0;}
            wasHolding=s.hold;
            holdGain+=smooth*((s.hold?1.f:0.f)-holdGain);
            for(size_t c=0;c<2;++c) {
                const float held=frozen(c);
                frozenTone[c]+=damping*(held-frozenTone[c]);
                const float cloud=(frozenTone[c]+reverb[c]*.15f)*.9f;
                if(!s.hold&&holdGain<.001f)holdLine[c].push(wet[c]);
                wet[c]+=holdGain*(cloud-wet[c]);
            }
            if(++freezeRead==freezeSpan+freezeFade)freezeRead=freezeFade;
            float mixed[2]{dry[0]+macro[3]*(wet[0]-dry[0]),dry[1]+macro[3]*(wet[1]-dry[1])};
            const float peak=std::max(std::abs(mixed[0]),std::abs(mixed[1]));
            const float desiredGain=peak>.94f?.94f/peak:1.f;
            outputGain=desiredGain<outputGain?desiredGain:outputGain+releaseCoefficient*(desiredGain-outputGain);
            for(size_t c=0;c<2;++c) {
                // Transparent endpoints; effected output has linked stereo level protection.
                const float protectedOut=macro[3]==0.f?dry[c]:ceiling(mixed[c]*outputGain);
                const float out=bypassGain==0.f?dry[c]:dry[c]+bypassGain*(protectedOut-dry[c]);
                if(c==0)left[n]=out;else if(right)right[n]=out;
            }
        }
    }
};
}
