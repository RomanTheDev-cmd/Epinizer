#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
int runChecks(int argc,char** argv) {
    const bool skipEditor=argc==3&&std::string(argv[2])=="--no-editor";
    if(argc!=2&&!skipEditor){std::cerr<<"Usage: epinizer_vst3_host_tests.exe path/to/Epinizer.vst3\n";return 1;}
    std::cerr<<"Stage: discover VST3\n";
    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile(descriptions,juce::File(juce::String::fromUTF8(argv[1])).getFullPathName());
    if(descriptions.size()!=1){std::cerr<<"FAIL: VST3 factory discovery\n";return 2;}
    juce::String error;
    std::cerr<<"Stage: instantiate\n";
    auto instance=format.createInstanceFromDescription(*descriptions[0],48000,512,error);
    if(!instance){std::cerr<<"FAIL: VST3 instantiate: "<<error<<'\n';return 3;}
    if(instance->getTotalNumInputChannels()!=2||instance->getTotalNumOutputChannels()!=2)return 4;
    std::cerr<<"Stage: render\n";
    instance->setRateAndBufferSizeDetails(48000,512);instance->prepareToPlay(48000,512);
    juce::AudioBuffer<float> buffer(2,512);juce::MidiBuffer midi;
    double tailEnergy=0;float peak=0;
    for(int block=0;block<200;++block){buffer.clear();if(block==0){buffer.setSample(0,0,.7f);buffer.setSample(1,0,.7f);}instance->processBlock(buffer,midi);
        for(int c=0;c<2;++c)for(int n=0;n<512;++n){const float v=buffer.getSample(c,n);if(!std::isfinite(v))return 5;peak=std::max(peak,std::abs(v));if(block>2)tailEnergy+=double(v)*v;}}
    if(peak>1||tailEnergy<1.e-8){std::cerr<<"FAIL: VST3 impulse/tail\n";return 6;}
    std::cerr<<"Stage: state\n";
    auto& parameters=instance->getParameters();if(parameters.size()<6)return 7;
    auto* parameter=parameters[0];parameter->setValueNotifyingHost(.31f);
    juce::MemoryBlock state;instance->getStateInformation(state);parameter->setValueNotifyingHost(.76f);
    instance->setStateInformation(state.getData(),int(state.getSize()));
    if(std::abs(parameter->getValue()-.31f)>.005f){std::cerr<<"FAIL: VST3 state round trip\n";return 8;}
    std::cerr<<"Stage: editor\n";
    if(!instance->hasEditor())return 9;
    std::cerr<<"Stage: editor capability confirmed\n";
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    if(!skipEditor) editor.reset(instance->createEditorIfNeeded());
    std::cerr<<"Stage: host editor construction returned, size "<<(editor?editor->getWidth():0)<<"x"<<(editor?editor->getHeight():0)<<"\n";
    if(!skipEditor&&(!editor||editor->getWidth()<400||editor->getHeight()<300))return 10;
    if(!skipEditor) {
        // Exercise the same asynchronous attach/detach lifecycle as a DAW window.
        struct WindowLife { std::unique_ptr<juce::AudioPluginInstance> plugin; std::unique_ptr<juce::AudioProcessorEditor> editor; };
        auto life=std::make_shared<WindowLife>();life->plugin=std::move(instance);life->editor=std::move(editor);
        life->editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);
        life->editor->setVisible(true);
        juce::Timer::callAfterDelay(250,[life]{
            life->editor->setVisible(false);life->editor->removeFromDesktop();life->editor.reset();
            std::cerr<<"Stage: editor detached and destroyed\n";
            life->plugin->releaseResources();life->plugin.reset();
            std::cout<<"PASS: actual VST3 load, render, state, editor attach/detach lifecycle\n";
            juce::MessageManager::getInstance()->stopDispatchLoop();
        });
        return 100;
    }
    instance->releaseResources();instance.reset();
    std::cout<<"PASS: actual VST3 discovery, load, stereo render, finite output, spatial tail, state round trip; editor "<<(skipEditor?"skipped":"created")<<"\n";
    return 0;
}

int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    int result=1;
    auto* messages=juce::MessageManager::getInstance();
    juce::MessageManager::callAsync([&]{ result=runChecks(argc,argv);if(result==100)result=0;else messages->stopDispatchLoop(); });
    messages->runDispatchLoop();
    return result;
}
