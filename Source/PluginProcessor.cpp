#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace {
struct Preset {const char* name; int character; float space,density,motion,mix;};
constexpr Preset presets[]{
    // Epinizer 3 (2026-10-28): space = Bloom / Scale / Abyss / Trail, density = Prism / Weight / Dread / Glow,
    // motion = Flow / Surge / Swarm / Drift. Starting points for listening, not final.
    {"Glass / Frost light",0,.35f,.55f,.35f,.60f}, {"Glass / Prism shimmer",0,.70f,.80f,.60f,.70f},
    {"Massive / Pressure",1,.30f,.60f,.50f,.60f}, {"Massive / Monolith",1,.70f,.85f,.70f,.75f},
    {"Horroble / Presence",2,.35f,.55f,.40f,.60f}, {"Horroble / Abyss",2,.80f,.80f,.70f,.75f},
    {"Warm / Faded tape",3,.35f,.55f,.40f,.60f}, {"Warm / Velvet memory",3,.65f,.75f,.55f,.70f}};
}
juce::AudioProcessorValueTreeState::ParameterLayout EpinizerProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    const char* ids[]{"space","density","motion","mix"};const char* names[]{"Space","Density","Motion","Mix"};
    const float defaults[]{.48f,.58f,.4f,.55f};
    for(int i=0;i<4;++i) p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ids[i],1},names[i],juce::NormalisableRange<float>{0,1,.001f},defaults[i],juce::AudioParameterFloatAttributes().withLabel("%") .withStringFromValueFunction([](float v,int){return juce::String(juce::roundToInt(v*100));}).withValueFromStringFunction([](const juce::String& v){return v.getFloatValue()*.01f;})));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"character",1},"Character",juce::StringArray{"Glass","Massive","Horroble","Warm"},0));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"echo",1},"Echo",true));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"hold",1},"Hold",false));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"width",1},"Width",juce::NormalisableRange<float>{0,1,.001f},1.f,
        juce::AudioParameterFloatAttributes().withLabel("%").withStringFromValueFunction([](float v,int){return juce::String(juce::roundToInt(v*100));}).withValueFromStringFunction([](const juce::String& v){return v.getFloatValue()*.01f;})));
    return p;
}
EpinizerProcessor::EpinizerProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"EpinizerState",layout()) {
    const char* ids[]{"space","density","motion","mix","character","bypass","echo","hold","width"};
    for(size_t i=0;i<values.size();++i) values[i]=state.getRawParameterValue(ids[i]);
}
epinizer::Settings EpinizerProcessor::settings() const {return {values[0]->load(),values[1]->load(),values[2]->load(),values[3]->load(),int(values[4]->load()),values[5]->load()>.5f,values[6]->load()>.5f,values[7]->load()>.5f,values[8]->load()};}
void EpinizerProcessor::prepareToPlay(double sr,int) {engine.prepare(sr);engine.snap(settings());meter.store(0);}
bool EpinizerProcessor::isBusesLayoutSupported(const BusesLayout& b) const {return b.getMainInputChannelSet()==b.getMainOutputChannelSet()&&(b.getMainOutputChannelSet()==juce::AudioChannelSet::mono()||b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo());}
void EpinizerProcessor::render(juce::AudioBuffer<float>& b,bool bypass) {
    juce::ScopedNoDenormals noDenormals;
    for(int c=getTotalNumInputChannels();c<b.getNumChannels();++c)b.clear(c,0,b.getNumSamples());
    if(b.getNumChannels()==0)return;
    auto s=settings();s.bypass=s.bypass||bypass;
    engine.process(b.getWritePointer(0),b.getNumChannels()>1?b.getWritePointer(1):nullptr,b.getNumSamples(),s);
    float peak=0;for(int c=0;c<b.getNumChannels();++c)peak=std::max(peak,b.getMagnitude(c,0,b.getNumSamples()));
    meter.store(peak,std::memory_order_relaxed);
}
void EpinizerProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) {render(b,false);}
void EpinizerProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&) {render(b,true);}
const juce::String EpinizerProcessor::getProgramName(int i) {return presets[juce::jlimit(0,7,i)].name;}
void EpinizerProcessor::setCurrentProgram(int i) {
    i=juce::jlimit(0,7,i);program.store(i);const auto& p=presets[i];
    const char* ids[]{"space","density","motion","mix","character","echo","hold"};const float v[]{p.space,p.density,p.motion,p.mix,float(p.character),1.f,0.f};
    for(int k=0;k<7;++k){auto* param=state.getParameter(ids[k]);param->beginChangeGesture();param->setValueNotifyingHost(param->convertTo0to1(v[k]));param->endChangeGesture();}
}
void EpinizerProcessor::getStateInformation(juce::MemoryBlock& b) {auto tree=state.copyState();tree.setProperty("program",program.load(),nullptr);tree.setProperty("version",5,nullptr);/* 5: Epinizer 3 sound; old projects open with it */if(auto xml=tree.createXml())copyXmlToBinary(*xml,b);}
void EpinizerProcessor::setStateInformation(const void* d,int size) {
    if(auto xml=getXmlFromBinary(d,size))if(xml->hasTagName(state.state.getType())) {
        auto tree=juce::ValueTree::fromXml(*xml);
        if(int(tree.getProperty("version",1))<2&&!tree.getChildWithProperty("id","echo").isValid()) {
            juce::ValueTree legacyEcho("PARAM");legacyEcho.setProperty("id","echo",nullptr);legacyEcho.setProperty("value",0.f,nullptr);tree.addChild(legacyEcho,-1,nullptr);
        }
        if(!tree.getChildWithProperty("id","hold").isValid()){juce::ValueTree oldHold("PARAM");oldHold.setProperty("id","hold",nullptr);oldHold.setProperty("value",0.f,nullptr);tree.addChild(oldHold,-1,nullptr);}
        if(!tree.getChildWithProperty("id","width").isValid()){juce::ValueTree oldWidth("PARAM");oldWidth.setProperty("id","width",nullptr);oldWidth.setProperty("value",1.f,nullptr);tree.addChild(oldWidth,-1,nullptr);}
        // Validate saved parameter values before allowing them onto the audio thread.
        for(auto child:tree)if(child.hasProperty("value")) {
            const auto id=child["id"].toString();auto* param=state.getParameter(id);
            if(param==nullptr)return;
            const float v=float(child["value"]);if(!std::isfinite(v))return;
            child.setProperty("value",param->convertFrom0to1(param->convertTo0to1(v)),nullptr);
        }
        program.store(juce::jlimit(0,7,int(tree.getProperty("program",0))));state.replaceState(tree);
    }
}
juce::AudioProcessorEditor* EpinizerProcessor::createEditor(){return new EpinizerEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new EpinizerProcessor();}
