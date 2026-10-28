#pragma once
#include "PluginProcessor.h"

class EpinizerLook final : public juce::LookAndFeel_V4 {
public:
    EpinizerLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
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
    EpinizerLook look;
    juce::Image castle;
    std::array<juce::Slider,5> knobs;
    std::array<juce::TextButton,4> characters;
    juce::ComboBox presets;
    juce::ToggleButton bypass{"Bypass"},echo{"Echo"},hold{"Hold"};
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,5> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment,echoAttachment,holdAttachment;
    juce::TooltipWindow tooltip{this,500};
    int selected=-1;
    float level=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EpinizerEditor)
};
