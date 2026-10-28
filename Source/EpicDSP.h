#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// Epinizer 3 sound (Claude, 2026-10-28). Roman: the effect must change the atmosphere of the sound itself with chains
// of effects - not a room reverb, not sub, octaves or doubled notes. Each character is its own chain:
//   Warm     - dynamic saturation, peak softening, level-dependent tone, tape drift, a tape memory that fades into texture
//   Horroble - blurred attacks with reversed swells, an atonal frequency shift, moving notches, a spiralling asymmetric
//              diffusion (the shift continues inside it)
//   Glass    - odd harmonic shaping of the highs, frequency-dependent dispersion, bright bands that open after the
//              attack, a moving bright haze
//   Massive  - band saturation of the low middle, a longer body (sustain shaper), a dense wall of reflections that
//              swells after accents, a deep short diffusion; the low end stays dry and centred
// Macros: density = the character's main transformation (Glow / Dread / Prism / Weight), space = time and size (Trail /
// Abyss / Bloom / Scale), motion = movement (Drift / Swarm / Flow / Surge). Echo makes the repeats distinct; it never
// carries the character. Hold freezes the character's own feedback texture (normalised, still moving) while new input
// is still processed on top. Storage is allocated in prepare(); every sample takes the same path for any block size.
namespace epinizer {
struct Settings {
    float space=.48f, density=.58f, motion=.4f, mix=.55f;
    int character=0;
    bool bypass=false, echo=true, hold=false;
    float width=1.f;
};

namespace dsp {
constexpr float pi=3.14159265358979323846f;
inline float finite(float v){return std::isfinite(v)?v:0.f;}
inline float unit(float v){return std::clamp(finite(v),0.f,1.f);}
inline float poleHz(float hz,double sr){hz=std::min(hz,float(sr*.45));return float(1.-std::exp(-2.*double(pi)*hz/sr));}
inline float poleTime(float seconds,double sr){return float(1.-std::exp(-1./(double(std::max(seconds,1.e-4f))*sr)));}
inline float flush(float v){return std::abs(v)<1.e-20f?0.f:v;}

struct Line {
    std::vector<float> d; size_t w=0;
    void prepare(size_t n){d.assign(std::max<size_t>(n,8),0.f);w=0;}
    void clear(){std::fill(d.begin(),d.end(),0.f);w=0;}
    void push(float v){d[w]=flush(v);if(++w==d.size())w=0;}
    float read(float delay) const {          // samples back from the newest one (1 = newest)
        delay=std::clamp(delay,1.f,float(d.size()-3));
        float p=float(w)-delay; if(p<0)p+=float(d.size());
        size_t a=size_t(p); if(a>=d.size())a=0;   // float rounding can land exactly on the end
        const float f=p-std::floor(p); const size_t b=a+1==d.size()?0:a+1;
        return d[a]+(d[b]-d[a])*f;
    }
};
// Schroeder all-pass with a (modulated) delay: smears without colouring the level.
struct Diffuser {
    Line line;
    float tick(float x,float delay,float g){const float d=line.read(delay);const float v=x+g*d;line.push(v);return d-g*v;}
};
// First-order all-pass: a cascade gives frequency-dependent delay (dispersion).
struct AllPass1 {
    float x1=0,y1=0;
    float tick(float x,float a){const float y=a*x+x1-a*y1;x1=x;y1=flush(y);return y;}
};
// Topology-preserving state-variable filter.
struct Svf {
    float ic1=0,ic2=0,a1=1,a2=0,a3=0,k=1;
    void set(float hz,float q,double sr){const float g=std::tan(pi*std::clamp(hz,10.f,float(sr*.45))/float(sr));k=1.f/q;a1=1.f/(1.f+g*(g+k));a2=g*a1;a3=g*a2;}
    void reset(){ic1=ic2=0;}
    struct Out{float lp,bp,hp;};
    Out tick(float x){const float v3=x-ic2,v1=a1*ic1+a2*v3,v2=ic2+a2*ic1+a3*v3;ic1=flush(2*v1-ic1);ic2=flush(2*v2-ic2);return {v2,v1,x-k*v1-v2};}
};
// Deterministic smooth random movement in -1..1 (not a periodic LFO).
struct Wander {
    uint32_t s=1;float a=0,b=0,target=0;int left=0;
    void seed(uint32_t v){s=v?v:1;a=b=target=0;left=0;}
    float next(float hz,double sr){
        if(--left<=0){s=s*1664525u+1013904223u;target=float(s>>8)/8388608.f-1.f;left=std::max(1,int(sr/double(std::max(hz,.01f))));}
        const float c=poleHz(hz,sr);a+=c*(target-a);b+=c*(a-b);return b;
    }
};
// 90-degree phase pair (Olli Niemitalo's all-pass design) for single-sideband frequency shifting.
struct Hilbert {
    std::array<float,4> ax1{},ax2{},ay1{},ay2{},bx1{},bx2{},by1{},by2{};float delayed=0;
    void reset(){ax1.fill(0);ax2.fill(0);ay1.fill(0);ay2.fill(0);bx1.fill(0);bx2.fill(0);by1.fill(0);by2.fill(0);delayed=0;}
    void tick(float x,float& re,float& im){
        static constexpr float ca[4]{.6923878f,.9360654322959f,.9882295226860f,.9987488452737f};
        static constexpr float cb[4]{.4021921162426f,.8561710882420f,.9722909545651f,.9952884791278f};
        float a=x,b=x;
        for(size_t i=0;i<4;++i){const float y=ca[i]*ca[i]*(a+ay2[i])-ax2[i];ax2[i]=ax1[i];ax1[i]=a;ay2[i]=ay1[i];ay1[i]=flush(y);a=y;}
        for(size_t i=0;i<4;++i){const float y=cb[i]*cb[i]*(b+by2[i])-bx2[i];bx2[i]=bx1[i];bx1[i]=b;by2[i]=by1[i];by1[i]=flush(y);b=y;}
        re=a;im=delayed;delayed=b;
    }
};
// Frequency shifter: every partial moves by the same number of Hz (inharmonic), it is not a second note.
struct Shifter {
    Hilbert h;double phase=0;
    void reset(){h.reset();phase=0;}
    float tick(float x,float hz,double sr){
        float re,im;h.tick(x,re,im);
        phase+=double(hz)/sr;phase-=std::floor(phase);
        const float p=float(phase)*2.f*pi;
        return re*std::cos(p)-im*std::sin(p);
    }
};

// A character's own feedback texture: four lines, orthogonal mixing, per-line colour, smear and movement. In Hold the
// input is closed and the energy captured at that moment is kept (the loop gain follows it), while the lines keep
// moving, so the texture lives instead of repeating.
struct Texture {
    struct Shape {
        std::array<float,4> seconds{};   // line lengths at size 1
        float size=1,feedback=.5f,lowpassHz=8000,highpassHz=60,moveMs=1,moveHz=.3f,shiftHz=0,saturate=0,smear=.5f;
        int disperse=0;
    };
    std::array<Line,4> line;std::array<Diffuser,4> smear;std::array<Wander,4> move;std::array<Shifter,4> shift;
    std::array<std::array<AllPass1,2>,4> disp;
    std::array<float,4> lp{},hp{};
    float energy=0,captured=0,loopGain=0;bool holding=false;double sr=48000;
    void prepare(double rate,float maxSeconds,uint32_t seed){
        sr=rate;
        for(size_t i=0;i<4;++i){line[i].prepare(size_t(rate*maxSeconds)+64);smear[i].line.prepare(size_t(rate*.06)+8);move[i].seed(seed+uint32_t(i)*7919u);}
        reset();
    }
    void reset(){
        for(size_t i=0;i<4;++i){line[i].clear();smear[i].line.clear();shift[i].reset();for(auto& a:disp[i])a=AllPass1{};}
        lp.fill(0);hp.fill(0);energy=captured=loopGain=0;holding=false;
        for(size_t i=0;i<4;++i)move[i].seed(uint32_t(i+1)*2654435761u);
    }
    void process(float inL,float inR,float open,bool hold,const Shape& s,float& outL,float& outR){
        std::array<float,4> r{};
        const float lpc=poleHz(hold?std::max(s.lowpassHz,12000.f):s.lowpassHz,sr),hpc=poleHz(hold?20.f:s.highpassHz,sr);
        static constexpr float smearMs[4]{7.3f,11.9f,17.1f,23.9f};
        for(size_t i=0;i<4;++i){
            const float m=move[i].next(s.moveHz*(1.f+.31f*float(i)),sr)*s.moveMs*.001f*float(sr);
            float v=line[i].read(s.seconds[i]*s.size*float(sr)+m+float(sr)*.004f);
            lp[i]+=lpc*(v-lp[i]);v=lp[i];hp[i]+=hpc*(v-hp[i]);v-=hp[i];
            v=smear[i].tick(v,smearMs[i]*.001f*float(sr)*(.6f+.4f*s.size),s.smear);
            for(int k=0;k<s.disperse&&k<2;++k)v=disp[i][size_t(k)].tick(v,-.62f);
            if(s.shiftHz!=0.f)v=shift[i].tick(v,s.shiftHz*(i%2?1.f:1.13f),sr);
            if(s.saturate>0.f)v=std::tanh(v*(1.f+s.saturate))/(1.f+s.saturate);   // never above unity for quiet sound
            r[i]=v;
        }
        const float e=(r[0]*r[0]+r[1]*r[1]+r[2]*r[2]+r[3]*r[3])*.25f;
        energy+=poleTime(.3f,sr)*(e-energy);
        if(hold&&!holding)captured=energy;   // what Hold keeps
        holding=hold;
        float g=s.feedback;
        if(hold&&captured>1.e-12f)
            g=std::clamp(std::pow(captured/std::max(energy,1.e-12f),.5f),.85f,1.15f);
        loopGain+=poleTime(.05f,sr)*(g-loopGain);
        const float h0=(r[0]+r[1]+r[2]+r[3])*.5f,h1=(r[0]-r[1]+r[2]-r[3])*.5f,h2=(r[0]+r[1]-r[2]-r[3])*.5f,h3=(r[0]-r[1]-r[2]+r[3])*.5f;
        const float in[4]{inL*open,inR*open,inL*open*.7f,-inR*open*.7f};
        const float h[4]{h0,h1,h2,h3};
        for(size_t i=0;i<4;++i){float v=in[i]+loopGain*h[i];if(hold)v=std::tanh(v*.8f)/.8f;line[i].push(v);}
        outL=r[0]+.6f*r[2];outR=r[1]+.6f*r[3];
    }
};

struct Context { float m1,m2,m3,echo,open,env,slow,onset;bool hold; };

//==============================================================================
struct Warm {
    std::array<Svf,2> body,lowCut;std::array<float,2> top{},top2{},pre{},dcIn{},dcOut{},tapeTone{};std::array<Line,2> drift,tape;
    std::array<Wander,2> wow;Texture memory;
    float flutter=0;double sr=48000;int counter=0;
    void prepare(double rate){sr=rate;for(auto& l:drift)l.prepare(size_t(rate*.04));for(auto& l:tape)l.prepare(size_t(rate*1.2));
        memory.prepare(rate,.9f,909);reset();}
    void reset(){for(auto& b:body)b.reset();for(auto& f:lowCut)f.reset();top.fill(0);top2.fill(0);pre.fill(0);dcIn.fill(0);dcOut.fill(0);tapeTone.fill(0);for(auto& l:drift)l.clear();for(auto& l:tape)l.clear();
        memory.reset();wow[0].seed(11);wow[1].seed(29);flutter=0;counter=0;}
    void process(const float x[2],float y[2],const Context& c){
        const float glow=c.m1,trail=c.m2,drf=c.m3;
        if((counter++&31)==0)for(auto& b:body)b.set(320.f,.7f,sr);
        // instant soft saturation with a little even colour softens the peaks; the top darkens with level (tape)
        const float drive=1.f+5.f*glow,norm=std::pow(drive,.55f);
        flutter+=float(6.3/sr);flutter-=std::floor(flutter);
        const float topHz=7000.f*std::pow(.1f,glow)*(1.f-.35f*std::min(1.f,c.env*2.f))+900.f;
        if((counter&31)==1)for(auto& f:lowCut)f.set(150.f,.707f,sr);
        float near[2];
        for(size_t ch=0;ch<2;++ch){
            const auto parts=lowCut[ch].tick(x[ch]);          // lp = the low end below 150 Hz (kept nearly linear)
            pre[ch]+=poleHz(topHz*1.6f,sr)*(x[ch]-parts.lp-pre[ch]);
            const float lows=parts.lp,rest=pre[ch];
            float v=lows*(1.f+.15f*glow)+std::tanh(drive*rest)/norm;
            v+=body[ch].tick(v).bp*.9f*glow;                       // fuller low middle, not sub
            top[ch]+=poleHz(topHz,sr)*(v-top[ch]);top2[ch]+=poleHz(topHz,sr)*(top[ch]-top2[ch]);v=top2[ch]; // softer top, darker when loud
            // tape drift of the processed sound itself (slow wow + a little flutter)
            const float depth=float(sr)*(.00008f+.0021f*drf);
            const float m=depth*(wow[ch].next(.6f+.9f*drf,sr)+.18f*std::sin(2*pi*flutter+float(ch)));
            drift[ch].push(v);near[ch]=drift[ch].read(float(sr)*.006f+m);
        }
        // memory: the trail keeps drifting, darkens and saturates on every pass and dissolves into texture
        Texture::Shape s;
        s.seconds={.11f,.17f,.23f,.31f};s.size=.5f+1.5f*trail;s.feedback=.25f+.5f*trail;s.lowpassHz=3000.f*std::pow(.55f,trail)+700.f;
        s.highpassHz=80.f;s.moveMs=.6f+3.f*drf;s.moveHz=.4f+.6f*drf;s.saturate=.4f+.8f*glow;s.smear=.62f;
        float ml,mr;memory.process(near[0],near[1],c.open,c.hold,s,ml,mr);
        // Echo: distinct tape repeats (two heads), darker each time
        for(size_t ch=0;ch<2;++ch){
            const float t=(ch?.29f:.21f)*(.6f+.9f*trail)*float(sr)+float(sr)*.0008f*drf*wow[ch].b;
            const float e=tape[ch].read(t);
            tapeTone[ch]+=poleHz(2400.f,sr)*(e-tapeTone[ch]);
            tape[ch].push(near[ch]*.6f+std::tanh(tapeTone[ch]*1.4f)/1.4f*.35f);
            y[ch]=near[ch]+(ch?mr:ml)*(.3f+.6f*trail)+tapeTone[ch]*.5f*c.echo;
        }
    }
};

//==============================================================================
struct Horroble {
    Line history;std::array<Shifter,2> front;std::array<Svf,2> dip,glow;std::array<Wander,3> swarm;Texture abyss;
    std::array<Line,2> echo;std::array<Shifter,2> echoShift;std::array<float,2> dcIn{},dcOut{},echoTone{};
    struct Grain{float age=-1,length=0,delay=0;};std::array<Grain,3> grains{};
    float lastOnset=0,refractory=0,blur=0;double sr=48000;int counter=0;
    void prepare(double rate){sr=rate;history.prepare(size_t(rate*1.7));abyss.prepare(rate,1.3f,101);for(auto& l:echo)l.prepare(size_t(rate*.8));reset();}
    void reset(){history.clear();for(auto& s:front)s.reset();for(auto& f:dip)f.reset();for(auto& f:glow)f.reset();swarm[0].seed(5);swarm[1].seed(17);swarm[2].seed(43);
        abyss.reset();for(auto& l:echo)l.clear();for(auto& s:echoShift)s.reset();dcIn.fill(0);dcOut.fill(0);echoTone.fill(0);grains.fill(Grain{});lastOnset=refractory=blur=0;counter=0;}
    void process(const float x[2],float y[2],const Context& c){
        const float dread=c.m1,depth=c.m2,sw=c.m3;
        const float wanderA=swarm[0].next(.08f+1.2f*sw,sr),wanderB=swarm[1].next(.11f+1.6f*sw,sr),wanderS=swarm[2].next(.05f+.4f*sw,sr);
        if((counter++&31)==0){
            const float fa=std::exp(std::log(250.f)+(std::log(2800.f)-std::log(250.f))*(.5f+.5f*wanderA));
            const float fb=std::exp(std::log(400.f)+(std::log(3600.f)-std::log(400.f))*(.5f+.5f*wanderB));
            for(auto& f:dip)f.set(fa,2.5f,sr);for(auto& f:glow)f.set(fb,5.f,sr);
        }
        // attacks: blurred in the near path, and each one comes back reversed a little later (a swell into it)
        const float mono=(x[0]+x[1])*.5f;history.push(mono);
        refractory=std::max(0.f,refractory-1.f);
        if(!c.hold&&c.onset>.5f&&lastOnset<=.5f&&refractory<=0.f){
            for(auto& g:grains)if(g.length<=0){g.length=float(sr)*(.14f+.4f*depth);g.age=-g.length;g.delay=0;break;}
            refractory=float(sr)*.12f;
        }
        lastOnset=c.onset;
        float rev=0;
        for(auto& g:grains){
            if(g.length<=0)continue;
            g.age+=1.f;
            if(g.age<0)continue;                                // waits until the segment is recorded
            if(g.age>=g.length){g=Grain{};continue;}
            // rises to the reversed attack and closes quickly after it (a swell into the note, then gone)
            const float u=g.age/g.length,w=u<.92f?std::sqrt(u/.92f):(1.f-u)/.08f;
            rev+=w*history.read(2.f*g.age+float(sr)*.004f);
        }
        blur+=(c.onset>blur?poleTime(.003f,sr):poleTime(.15f,sr))*(c.onset-blur);
        const float shiftHz=1.5f+30.f*dread+3.f*sw*wanderS;
        float near[2];
        for(size_t ch=0;ch<2;++ch){
            float v=x[ch]*(1.f-.55f*dread*blur);
            v+=.22f*dread*v*v;                                      // asymmetric
            const float dc=v-dcIn[ch]+.9995f*dcOut[ch];dcIn[ch]=v;dcOut[ch]=flush(dc);v=dc;
            const float shifted=front[ch].tick(v,ch?shiftHz:-shiftHz*.83f,sr);
            v=v*(1.f-.8f*dread)+shifted*(.25f+.75f*dread);
            v-=dip[ch].tick(v).bp*(.7f+.25f*sw);                     // moving dips
            v+=glow[ch].tick(v).bp*(.35f*sw+.15f*dread);             // a moving resonance
            near[ch]=v+rev*(.35f+.9f*dread)*(ch?.8f:1.f);
        }
        Texture::Shape s;
        s.seconds={.137f,.211f,.293f,.367f};s.size=.4f+1.3f*depth;s.feedback=.3f+.42f*depth;
        s.lowpassHz=6500.f*std::pow(.25f,depth)+900.f;s.highpassHz=90.f;s.moveMs=1.f+5.f*sw;s.moveHz=.15f+1.2f*sw;
        s.shiftHz=(2.f+8.f*dread)*(c.hold?1.f:.7f);s.smear=.62f;
        float tl,tr;abyss.process(near[0],near[1],c.open,c.hold,s,tl,tr);
        // Echo: distinct repeats that keep shifting (a spiral), never the character itself
        const float delays[2]{.31f,.47f};
        for(size_t ch=0;ch<2;++ch){
            const float e=echo[ch].read(delays[ch]*float(sr));
            echoTone[ch]+=poleHz(2600.f,sr)*(e-echoTone[ch]);
            echo[ch].push(near[ch]*.6f+.42f*echoShift[ch].tick(echoTone[1-ch],ch?7.f:-7.f,sr));
            y[ch]=near[ch]+(ch?tr:tl)*(.3f+.6f*depth)+echoTone[ch]*.5f*c.echo;
        }
    }
};

//==============================================================================
struct Glass {
    std::array<Svf,2> split,split2;std::array<std::array<AllPass1,24>,2> disperse;std::array<std::array<Svf,3>,2> bands;
    std::array<std::array<float,3>,3> bandEnv{};std::array<float,3> bandFast{};Texture haze;std::array<Line,2> echo;
    std::array<float,2> echoHp{},hazeIn{};double sr=48000;int counter=0;
    void prepare(double rate){sr=rate;haze.prepare(rate,.12f,303);for(auto& l:echo)l.prepare(size_t(rate*.45));reset();}
    void reset(){for(auto& f:split)f.reset();for(auto& f:split2)f.reset();for(auto& c:disperse)for(auto& a:c)a=AllPass1{};for(auto& c:bands)for(auto& b:c)b.reset();
        for(auto& e:bandEnv)e.fill(0);bandFast.fill(0);haze.reset();for(auto& l:echo)l.clear();echoHp.fill(0);hazeIn.fill(0);counter=0;}
    void process(const float x[2],float y[2],const Context& c){
        const float prism=c.m1,bloom=c.m2,flow=c.m3;
        if((counter++&31)==0){
            for(auto& f:split)f.set(1100.f,.6f,sr);for(auto& f:split2)f.set(1100.f,.6f,sr);
            static constexpr float hz[3]{2300.f,5000.f,9800.f};
            for(auto& ch:bands)for(size_t k=0;k<3;++k)ch[k].set(hz[k],1.3f,sr);
        }
        const float a=-(.6f+.35f*prism);
        const float k=1.f+7.f*prism;
        float bright[2],body[2],bandOut[2][3];
        for(size_t ch=0;ch<2;++ch){
            const auto s=split[ch].tick(x[ch]);
            const float h=s.hp+prism*.8f*(std::tanh(k*s.hp)/k*2.f-s.hp*.6f);   // odd overtones, no octave
            float v=s.lp*(.9f-.2f*prism)+h*(1.f+.35f*prism);
            for(auto& ap:disperse[ch])v=ap.tick(v,a);                       // the body arrives after the highs: a glassy chirp
            const auto s2=split2[ch].tick(v);
            body[ch]=s2.lp;bright[ch]=s2.hp;
            for(size_t b=0;b<3;++b)bandOut[ch][b]=bands[ch][b].tick(s2.hp).bp;
        }
        // the bright bands open after the attack, the higher ones later
        float open[3];
        for(size_t b=0;b<3;++b){
            const float lv=std::max(std::abs(bandOut[0][b]),std::abs(bandOut[1][b]));
            bandFast[b]+=(lv>bandFast[b]?poleTime(.001f,sr):poleTime(.03f,sr))*(lv-bandFast[b]);
            const float rise=(.025f+float(b)*.06f)*(.3f+2.2f*bloom);
            bandEnv[b][0]+=poleTime(rise,sr)*(bandFast[b]-bandEnv[b][0]);
            bandEnv[b][1]+=poleTime(rise*.7f,sr)*(bandEnv[b][0]-bandEnv[b][1]);
            open[b]=std::clamp(bandEnv[b][1]/(bandFast[b]+1.e-4f),0.f,3.f);
        }
        float bloomed[2];
        for(size_t ch=0;ch<2;++ch){
            float v=0;for(size_t b=0;b<3;++b)v+=bandOut[ch][b]*open[b]*(.6f+1.8f*bloom);
            bloomed[ch]=v;
            hazeIn[ch]=body[ch]*.45f+bright[ch]*.6f+v*.5f;
        }
        Texture::Shape s;
        s.seconds={.019f,.027f,.035f,.046f};s.size=.5f+1.5f*bloom;s.feedback=.3f+.55f*bloom;s.lowpassHz=14000.f;s.highpassHz=380.f;
        s.moveMs=.3f+2.4f*flow;s.moveHz=.2f+2.8f*flow;s.smear=.55f;s.disperse=2;
        float hl,hr;haze.process(hazeIn[0],hazeIn[1],c.open,c.hold,s,hl,hr);
        const float delays[2]{.19f,.285f};
        for(size_t ch=0;ch<2;++ch){
            const float e=echo[ch].read(delays[ch]*float(sr));
            echoHp[ch]+=poleHz(900.f,sr)*(e-echoHp[ch]);
            echo[ch].push(bright[ch]*.5f+bloomed[ch]*.3f+(e-echoHp[ch])*.35f);
            y[ch]=body[ch]+bright[ch]+bloomed[ch]+(ch?hr:hl)*(.45f+.6f*bloom)+(e-echoHp[ch])*.6f*c.echo;
        }
    }
};

//==============================================================================
struct Massive {
    std::array<Svf,2> lowSplit,highSplit,dense;Line taps;std::array<std::array<Diffuser,4>,2> wall;Texture deep;std::array<Line,2> echo;
    std::array<float,2> echoTone{},deepHp{};float fast=0,slowBody=0,pressure=0;double sr=48000;int counter=0;
    void prepare(double rate){sr=rate;taps.prepare(size_t(rate*.45));for(auto& w:wall)for(auto& d:w)d.line.prepare(size_t(rate*.06));
        deep.prepare(rate,.32f,707);for(auto& l:echo)l.prepare(size_t(rate*.5));reset();}
    void reset(){for(auto& f:lowSplit)f.reset();for(auto& f:highSplit)f.reset();for(auto& f:dense)f.reset();taps.clear();for(auto& w:wall)for(auto& d:w)d.line.clear();
        deep.reset();for(auto& l:echo)l.clear();echoTone.fill(0);deepHp.fill(0);fast=slowBody=pressure=0;counter=0;}
    void process(const float x[2],float y[2],const Context& c){
        const float weight=c.m1,scale=c.m2,surge=c.m3;
        if((counter++&31)==0){for(auto& f:lowSplit)f.set(110.f,.7f,sr);for(auto& f:highSplit)f.set(2400.f,.7f,sr);for(auto& f:dense)f.set(250.f,.8f,sr);}
        // a longer body: the gain rises while a note decays (slow vs fast envelope), never on silence
        const float lv=c.env;
        fast+=(lv>fast?poleTime(.001f,sr):poleTime(.06f,sr))*(lv-fast);
        slowBody+=(fast>slowBody?poleTime(.02f,sr):poleTime(.25f+.6f*surge,sr))*(fast-slowBody);
        const float now=std::max(std::abs(x[0]),std::abs(x[1]));
        const float ratio=slowBody/(std::max(fast,now)+1.e-4f);
        const float sustain=std::clamp(std::pow(std::max(ratio,1.f),.35f+.9f*weight),1.f,4.f);
        pressure+=(lv>pressure?poleTime(.12f,sr):poleTime(.5f+1.5f*scale,sr))*(lv-pressure);
        const float swell=std::clamp(pressure/(fast+1.e-3f),0.f,2.5f);
        const float dMid=1.f+6.f*weight,dLow=1.f+1.4f*weight;
        float mass[2],low[2];
        for(size_t ch=0;ch<2;++ch){
            const auto l=lowSplit[ch].tick(x[ch]);const auto h=highSplit[ch].tick(l.hp);
            low[ch]=std::tanh(l.lp*dLow)/std::pow(dLow,.5f);
            const float mid=std::tanh(h.lp*dMid)/std::pow(dMid,.6f);
            const float m2=mid+dense[ch].tick(mid).bp*1.2f*weight;                 // a denser low middle
            mass[ch]=(low[ch]+m2*(1.f+.5f*weight)+h.hp*(1.f+.3f*weight))*sustain;
        }
        // a dense wall of reflections (not a room): 12 close taps, then a short diffusion
        const float midSide=(mass[0]+mass[1])*.5f-(low[0]+low[1])*.5f*sustain;
        taps.push(midSide);
        float wl=0,wr=0,g=.8f;
        for(int i=0;i<12;++i){const float t=(.006f+float(i)*(.0035f+.007f*scale))*float(sr);const float v=taps.read(t)*g;(i%2?wr:wl)+=(i%3?v:-v);g*=.86f;}
        static constexpr float ms[4]{3.1f,7.3f,11.9f,17.7f};
        float wallOut[2]{wl,wr};
        for(size_t ch=0;ch<2;++ch)for(size_t k=0;k<4;++k)wallOut[ch]=wall[ch][k].tick(wallOut[ch],ms[k]*.001f*float(sr)*(.6f+.8f*scale)*(ch?1.09f:1.f),.62f);
        const float wallGain=(.35f+.5f*scale)*((1.f-surge)+surge*swell);   // the mass swells after accents
        Texture::Shape s;
        s.seconds={.047f,.061f,.079f,.097f};s.size=.5f+1.6f*scale;s.feedback=.35f+.45f*scale;s.lowpassHz=3200.f;s.highpassHz=160.f;
        s.moveMs=.4f+1.2f*surge;s.moveHz=.2f;s.smear=.6f;s.saturate=.6f*weight;
        float dl,dr;deep.process(wallOut[0],wallOut[1],c.open,c.hold,s,dl,dr);
        const float delays[2]{.24f,.36f};
        for(size_t ch=0;ch<2;++ch){
            const float e=echo[ch].read(delays[ch]*float(sr));
            echoTone[ch]+=poleHz(1800.f,sr)*(e-echoTone[ch]);
            echo[ch].push(mass[ch]*.55f+echoTone[ch]*.3f);
            y[ch]=mass[ch]+wallOut[ch]*wallGain+(ch?dr:dl)*(.25f+.45f*scale)+echoTone[ch]*.5f*c.echo;
        }
    }
};
} // namespace dsp

//==============================================================================
class Engine {
    dsp::Glass glass;dsp::Massive massive;dsp::Horroble horroble;dsp::Warm warm;
    std::array<float,4> macro{.48f,.58f,.4f,.55f},chain{1,0,0,0};std::array<bool,4> running{};
    std::array<float,2> dcIn{},dcOut{};
    double sr=48000;
    float smooth=0,chainSmooth=0,attack=0,release=0,slow=0,envelope=0,slowEnvelope=0;
    float bypassGain=1,echoGain=1,openGain=1,widthGain=1,outputGain=1;
    float dryPower=0,wetPower=0,level=1,levelSmooth=0,powerSmooth=0,dcCoefficient=0;bool armed=true;int armCount=0;
    static float ceiling(float x){const float a=std::abs(x);return a<=.9f?x:std::copysign(.9f+.09f*std::tanh((a-.9f)/.09f),x);}
    void startChain(size_t k){
        if(running[k])return;
        switch(k){case 0:glass.reset();break;case 1:massive.reset();break;case 2:horroble.reset();break;default:warm.reset();break;}
        running[k]=true;
    }
public:
    void prepare(double sampleRate){
        sr=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.,384000.):48000.;
        smooth=dsp::poleTime(.04f,sr);chainSmooth=dsp::poleTime(.12f,sr);
        attack=dsp::poleTime(.003f,sr);release=dsp::poleTime(.09f,sr);slow=dsp::poleTime(.18f,sr);
        powerSmooth=dsp::poleTime(.4f,sr);levelSmooth=dsp::poleTime(.6f,sr);dcCoefficient=float(std::exp(-2.*3.14159265358979*20./sr));
        glass.prepare(sr);massive.prepare(sr);horroble.prepare(sr);warm.prepare(sr);
        reset();
    }
    void reset(){
        glass.reset();massive.reset();horroble.reset();warm.reset();
        dcIn.fill(0);dcOut.fill(0);envelope=slowEnvelope=0;outputGain=1;dryPower=wetPower=0;level=1;armed=true;armCount=0;openGain=1;
        for(size_t k=0;k<4;++k)running[k]=chain[k]>0.f;
    }
    void snap(const Settings& s){
        macro={dsp::unit(s.space),dsp::unit(s.density),dsp::unit(s.motion),dsp::unit(s.mix)};
        chain.fill(0);chain[size_t(std::clamp(s.character,0,3))]=1;
        for(size_t k=0;k<4;++k)running[k]=chain[k]>0.f;
        bypassGain=s.bypass?0.f:1.f;echoGain=s.echo?1.f:0.f;widthGain=dsp::unit(s.width);openGain=s.hold?0.f:1.f;
    }
    void process(float* left,float* right,int count,const Settings& s){
        const std::array<float,4> targets{dsp::unit(s.space),dsp::unit(s.density),dsp::unit(s.motion),dsp::unit(s.mix)};
        const size_t character=size_t(std::clamp(s.character,0,3));
        for(int n=0;n<count;++n){
            for(size_t k=0;k<4;++k){
                macro[k]+=smooth*(targets[k]-macro[k]);
                const float t=k==character?1.f:0.f;
                if(t>0.f)startChain(k);
                chain[k]+=chainSmooth*(t-chain[k]);
                if(t==0.f&&chain[k]<1.e-5f){chain[k]=0;running[k]=false;}
            }
            bypassGain+=smooth*((s.bypass?0.f:1.f)-bypassGain);
            echoGain+=smooth*((s.echo?1.f:0.f)-echoGain);
            widthGain+=smooth*(dsp::unit(s.width)-widthGain);
            openGain+=smooth*((s.hold?0.f:1.f)-openGain);
            const float dry[2]{dsp::finite(left[n]),dsp::finite(right?right[n]:left[n])};
            float x[2];
            for(size_t c=0;c<2;++c){const float in=std::clamp(dry[c],-16.f,16.f);const float v=in-dcIn[c]+dcCoefficient*dcOut[c];dcIn[c]=in;dcOut[c]=dsp::flush(v);x[c]=v;}
            const float inputLevel=std::min(8.f,std::max(std::abs(x[0]),std::abs(x[1])));
            envelope+=(inputLevel>envelope?attack:release)*(inputLevel-envelope);
            slowEnvelope+=slow*(envelope-slowEnvelope);
            const float onset=std::clamp((envelope-slowEnvelope)/(slowEnvelope+.025f),0.f,1.f);
            float wet[2]{0,0};
            for(size_t k=0;k<4;++k){
                if(!running[k])continue;
                // macros: m1 = density, m2 = space, m3 = motion
                const dsp::Context ctx{macro[1],macro[0],macro[2],echoGain,openGain,envelope,slowEnvelope,onset,s.hold};
                float y[2];
                switch(k){case 0:glass.process(x,y,ctx);break;case 1:massive.process(x,y,ctx);break;case 2:horroble.process(x,y,ctx);break;default:warm.process(x,y,ctx);break;}
                wet[0]+=chain[k]*y[0];wet[1]+=chain[k]*y[1];
            }
            // wet as loud as dry (slow, frozen on silence): A/B and Mix compare the character, not the level
            dryPower+=powerSmooth*((x[0]*x[0]+x[1]*x[1])*.5f-dryPower);
            wetPower+=powerSmooth*((wet[0]*wet[0]+wet[1]*wet[1])*.5f-wetPower);
            // follows only while sound comes in (not on tails, not in Hold); takes the level at once when sound starts
            if(envelope<1.e-5f){armed=true;armCount=0;}
            if(!s.hold&&envelope>3.e-4f&&dryPower>1.e-8f&&wetPower>1.e-10f){
                const float target=std::clamp(std::sqrt(dryPower/wetPower),.25f,4.f);
                if(armed){if(++armCount>=int(.05*sr)){level=target;armed=false;}}   // after 50 ms of sound (the wet lags)
                else level+=levelSmooth*(target-level);
            }
            wet[0]=dsp::flush(wet[0]*level);wet[1]=dsp::flush(wet[1]*level);
            if(right&&widthGain!=1.f){const float m=(wet[0]+wet[1])*.5f;wet[0]=m+widthGain*(wet[0]-m);wet[1]=m+widthGain*(wet[1]-m);}
            if(!right){wet[0]=(wet[0]+wet[1])*.5f;}
            const float mixed[2]{dry[0]+macro[3]*(wet[0]-dry[0]),dry[1]+macro[3]*(wet[1]-dry[1])};
            const float peak=std::max(std::abs(mixed[0]),right?std::abs(mixed[1]):0.f);
            const float desired=peak>.94f?.94f/peak:1.f;
            outputGain=desired<outputGain?desired:outputGain+release*(desired-outputGain);
            for(size_t c=0;c<2;++c){
                const float protectedOut=macro[3]==0.f?dry[c]:ceiling(mixed[c]*outputGain);
                const float out=bypassGain==0.f?dry[c]:dry[c]+bypassGain*(protectedOut-dry[c]);
                if(c==0)left[n]=out;else if(right)right[n]=out;
            }
        }
    }
};
}
