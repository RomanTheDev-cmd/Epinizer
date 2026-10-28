#include "PluginProcessor.h"
#include <iostream>
int main(int argc,char** argv){
    juce::ScopedJuceInitialiser_GUI init;
    EpinizerProcessor p;p.setCurrentProgram(5);juce::MemoryBlock saved;p.getStateInformation(saved);
    auto* width=p.state.getParameter("width");
    if(width==nullptr){std::cerr<<"FAIL: five-dial editor requires a real Width parameter\n";return 12;}
    width->setValueNotifyingHost(.37f);p.getStateInformation(saved);
    EpinizerProcessor restored;restored.setStateInformation(saved.getData(),int(saved.getSize()));
    for(const auto* id:{"space","density","motion","mix","character","bypass","echo","hold","width"})if(std::abs(p.state.getRawParameterValue(id)->load()-restored.state.getRawParameterValue(id)->load())>1.e-6f)return 1;
    if(restored.getCurrentProgram()!=5)return 2;
    restored.prepareToPlay(48000,512);juce::AudioBuffer<float> audio(2,512);audio.clear();audio.setSample(0,0,.7f);juce::MidiBuffer midi;restored.processBlock(audio,midi);
    if(!std::isfinite(audio.getMagnitude(0,512)))return 3;
    auto legacy=p.state.copyState();legacy.removeChild(legacy.getChildWithProperty("id","echo"),nullptr);legacy.removeChild(legacy.getChildWithProperty("id","hold"),nullptr);legacy.removeChild(legacy.getChildWithProperty("id","width"),nullptr);legacy.setProperty("version",1,nullptr);
    juce::MemoryBlock oldState;juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(),oldState);restored.setStateInformation(oldState.getData(),int(oldState.getSize()));
    if(restored.state.getRawParameterValue("echo")->load()>.5f||restored.state.getRawParameterValue("hold")->load()>.5f)return 5;
    if(restored.state.getRawParameterValue("width")->load()!=1.f)return 13;
    auto previous=p.state.copyState();previous.removeChild(previous.getChildWithProperty("id","width"),nullptr);previous.setProperty("version",3,nullptr);
    juce::MemoryBlock v3State;juce::AudioProcessor::copyXmlToBinary(*previous.createXml(),v3State);
    restored.state.getParameter("width")->setValueNotifyingHost(0.f);restored.setStateInformation(v3State.getData(),int(v3State.getSize()));
    if(restored.state.getRawParameterValue("width")->load()!=1.f)return 14;
    if(argc>1&&std::string(argv[1])=="--no-editor"){std::cout<<"PASS: presets, state, legacy Echo migration, processor render; editor skipped\n";return 0;}
    p.setCurrentProgram(0);std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    int sliders=0,characters=0;bool echoFound=false,holdFound=false,presetsFound=false;
    for(auto* child:editor->getChildren()) {
        if(child->isVisible()&&(!editor->getLocalBounds().contains(child->getBounds())||child->getWidth()<=0||child->getHeight()<=0)){std::cerr<<"Bounds: "<<child->getName()<<" "<<child->getBounds().toString()<<'\n';return 15;}
        if(auto* slider=dynamic_cast<juce::Slider*>(child)) {
            const auto id=slider->getName().toLowerCase();slider->setValue(.73,juce::sendNotificationSync);
            if(std::abs(p.state.getRawParameterValue(id)->load()-.73f)>.002f)return 6;
            ++sliders;
        }
        if(auto* button=dynamic_cast<juce::TextButton*>(child)) {button->onClick();++characters;}
        if(auto* toggle=dynamic_cast<juce::ToggleButton*>(child)) if(toggle->getButtonText()=="Echo") {
            toggle->setToggleState(false,juce::sendNotificationSync);
            if(p.state.getRawParameterValue("echo")->load()>.5f)return 7;
            echoFound=true;
        }
        if(auto* toggle=dynamic_cast<juce::ToggleButton*>(child)) if(toggle->getButtonText()=="Hold") {
            toggle->setToggleState(true,juce::sendNotificationSync);
            if(p.state.getRawParameterValue("hold")->load()<.5f)return 10;
            juce::MemoryBlock heldState;p.getStateInformation(heldState);restored.setStateInformation(heldState.getData(),int(heldState.getSize()));
            if(restored.state.getRawParameterValue("hold")->load()<.5f)return 11;
            holdFound=true;
        }
        if(auto* box=dynamic_cast<juce::ComboBox*>(child)) {
            box->setSelectedId(4,juce::sendNotificationSync);
            if(p.getCurrentProgram()!=3)return 8;
            presetsFound=true;
        }
    }
    if(sliders!=5||characters!=4||!echoFound||!holdFound||!presetsFound)return 9;
    p.setCurrentProgram(0);p.state.getParameter("echo")->setValueNotifyingHost(1.f);
    if(argc>1){editor.reset(p.createEditor());auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,2.f,juce::SoftwareImageType{});juce::FileOutputStream stream{juce::File(juce::String::fromUTF8(argv[1]))};stream.setPosition(0);stream.truncate();juce::PNGImageFormat png;if(!png.writeImageToStream(image,stream))return 4;}
    std::cout<<"PASS: preset/state round trip, processor render, UI controls/attachments, editor creation and snapshot\n";
}
