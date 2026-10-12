#pragma once
#include "PluginProcessor.h"
class GlassLook final : public juce::LookAndFeel_V4 {
public:
    GlassLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};
class EpinizerEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit EpinizerEditor(EpinizerProcessor&);
    ~EpinizerEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    EpinizerProcessor& owner;
    GlassLook look;
    std::array<juce::Slider,4> knobs;
    std::array<juce::TextButton,4> characters;
    juce::ComboBox presets;
    juce::ToggleButton bypass{"Bypass"},echo{"Echo"},hold{"Hold"};
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,4> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment,echoAttachment,holdAttachment;
    juce::TooltipWindow tooltip{this,500};
    int selected=-1;float level=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EpinizerEditor)
};
