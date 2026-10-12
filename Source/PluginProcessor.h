#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "EpicDSP.h"
#include <limits>
class EpinizerProcessor final : public juce::AudioProcessor {
public:
    EpinizerProcessor();
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    void reset() override { engine.reset(); }
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    const juce::String getName() const override {return "Epinizer";}
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    double getTailLengthSeconds() const override {return state.getRawParameterValue("hold")->load()>.5f?std::numeric_limits<double>::infinity():12.;}
    int getNumPrograms() override {return 8;}
    int getCurrentProgram() override {return program.load();}
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override {return state.getParameter("bypass");}
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> meter{0};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    epinizer::Settings settings() const;
    void render(juce::AudioBuffer<float>&,bool);
    epinizer::Engine engine;
    std::array<std::atomic<float>*,8> values{};
    std::atomic<int> program{0};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EpinizerProcessor)
};
