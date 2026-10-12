#include "PluginEditor.h"
namespace {
const juce::Colour ink{0xffedf3ff},muted{0xff98a8bf};
const std::array<juce::Colour,4> accents{juce::Colour{0xff89dcff},juce::Colour{0xffb6a0ff},juce::Colour{0xffff8aa9},juce::Colour{0xffffce95}};
void font(juce::Graphics& g,float size,bool bold=false){g.setFont(juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain)));}
void panel(juce::Graphics& g,juce::Rectangle<float> r,float radius) {
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(.075f),r.getX(),r.getY(),juce::Colours::white.withAlpha(.018f),r.getRight(),r.getBottom(),false));g.fillRoundedRectangle(r,radius);
    g.setColour(juce::Colours::white.withAlpha(.13f));g.drawRoundedRectangle(r.reduced(.5f),radius,1);
    g.setColour(juce::Colours::white.withAlpha(.12f));g.drawLine(r.getX()+radius,r.getY()+1,r.getRight()-radius,r.getY()+1);
}
}
GlassLook::GlassLook() {
    setColour(juce::Slider::textBoxTextColourId,ink);setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff20293d));setColour(juce::ComboBox::textColourId,ink);
    setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff455069));
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff172035));setColour(juce::PopupMenu::textColourId,ink);
    setColour(juce::TextButton::buttonColourId,juce::Colour(0xff202b40));setColour(juce::TextButton::textColourOffId,muted);
    setColour(juce::TextButton::textColourOnId,juce::Colour(0xff101929));
    setColour(juce::ToggleButton::textColourId,muted);
}
void GlassLook::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider& slider) {
    auto r=juce::Rectangle<float>(float(x),float(y),float(width),float(height)).withSizeKeepingCentre(112,112).reduced(7);
    const auto c=r.getCentre();const auto accent=slider.findColour(juce::Slider::rotarySliderFillColourId);
    juce::Path track;track.addCentredArc(c.x,c.y,r.getWidth()*.5f,r.getHeight()*.5f,0,start,end,true);
    g.setColour(juce::Colours::white.withAlpha(.08f));g.strokePath(track,juce::PathStrokeType(3));
    juce::Path arc;arc.addCentredArc(c.x,c.y,r.getWidth()*.5f,r.getHeight()*.5f,0,start,start+position*(end-start),true);
    g.setColour(accent.withAlpha(.12f));g.strokePath(arc,juce::PathStrokeType(10));g.setColour(accent);g.strokePath(arc,juce::PathStrokeType(2.5f));
    auto inner=r.reduced(9);g.setColour(juce::Colours::black.withAlpha(.28f));g.fillEllipse(inner.translated(0,5));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff526079),inner.getX(),inner.getY(),juce::Colour(0xff111b2c),inner.getRight(),inner.getBottom(),false));g.fillEllipse(inner);
    g.setColour(juce::Colours::white.withAlpha(.24f));g.drawEllipse(inner,1);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(.13f),c.x,inner.getY(),juce::Colours::transparentWhite,c.x,c.y,false));g.fillEllipse(inner.reduced(4));
    const float angle=start+position*(end-start);const juce::Point<float> a{c.x+std::sin(angle)*24,c.y-std::cos(angle)*24},b{c.x+std::sin(angle)*32,c.y-std::cos(angle)*32};
    g.setColour(accent);g.drawLine({a,b},3);
}
EpinizerEditor::EpinizerEditor(EpinizerProcessor& p):AudioProcessorEditor(p),owner(p) {
    setLookAndFeel(&look);setSize(800,520);setResizable(false,false);
    const char* ids[]{"space","density","motion","mix"};const char* labels[]{"Space","Density","Motion","Mix"};
    const char* tips[]{"Space — atmosphere and dynamic bloom. The space opens between notes.","Density — ensemble voices, body and character layers.","Motion — six independent detune voices, tape drift and reverse movement.","Mix — dry / processed balance. 0% is transparent. Double-click a dial to reset."};
    const float defaults[]{.48f,.58f,.4f,.55f};
    for(size_t i=0;i<4;++i){auto& k=knobs[i];k.setName(labels[i]);k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,80,26);k.setColour(juce::Slider::rotarySliderFillColourId,accents[0]);k.setTooltip(tips[i]);addAndMakeVisible(k);attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],k);k.setDoubleClickReturnValue(true,defaults[i]);k.textFromValueFunction=[](double v){return juce::String(juce::roundToInt(v*100))+"%";};k.valueFromTextFunction=[](const juce::String& v){return v.getDoubleValue()*.01;};k.updateText();k.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);}
    const char* names[]{"Glass","Massive","Horroble","Warm"};
    for(size_t i=0;i<4;++i){auto& b=characters[i];b.setButtonText(names[i]);b.setColour(juce::TextButton::buttonOnColourId,accents[i]);addAndMakeVisible(b);b.onClick=[this,i]{auto* param=owner.state.getParameter("character");param->beginChangeGesture();param->setValueNotifyingHost(float(i)/3);param->endChangeGesture();};}
    for(int i=0;i<p.getNumPrograms();++i)presets.addItem(p.getProgramName(i),i+1);
    presets.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);presets.setTooltip("Factory starting points. Your edits are saved with the DAW session.");presets.onChange=[this]{owner.setCurrentProgram(presets.getSelectedId()-1);};addAndMakeVisible(presets);
    addAndMakeVisible(bypass);bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);
    addAndMakeVisible(echo);echo.setTooltip("Character echo: short reflections, or darker tape repeats in Warm.");echoAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"echo",echo);
    addAndMakeVisible(hold);hold.setTooltip("Capture and sustain the last atmosphere. Release to return to live processing. The captured audio is not saved in the DAW state.");holdAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"hold",hold);
    timerCallback();startTimerHz(24);
}
EpinizerEditor::~EpinizerEditor(){stopTimer();setLookAndFeel(nullptr);}
void EpinizerEditor::timerCallback(){
    const int current=int(owner.state.getRawParameterValue("character")->load());
    if(current!=selected){selected=juce::jlimit(0,3,current);for(size_t i=0;i<4;++i){characters[i].setToggleState(int(i)==selected,juce::dontSendNotification);knobs[i].setColour(juce::Slider::rotarySliderFillColourId,accents[size_t(selected)]);}repaint();}
    presets.setSelectedId(owner.getCurrentProgram()+1,juce::dontSendNotification);
    level=std::max(owner.meter.load(),level*.86f);repaint(30,460,740,50);
}
void EpinizerEditor::resized(){
    presets.setBounds(496,35,270,34);
    for(int i=0;i<4;++i){characters[size_t(i)].setBounds(48+i*178,164,166,38);knobs[size_t(i)].setBounds(48+i*178,269,166,150);}
    hold.setBounds(508,471,76,26);echo.setBounds(586,471,76,26);bypass.setBounds(670,471,96,26);
}
void EpinizerEditor::paint(juce::Graphics& g){
    g.fillAll(juce::Colour(0xff090f1d));const auto accent=accents[size_t(std::max(0,selected))];
    g.setGradientFill(juce::ColourGradient(accent.withAlpha(.17f),140,80,juce::Colours::transparentBlack,630,470,true));g.fillAll();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff8267ba).withAlpha(.10f),770,390,juce::Colours::transparentBlack,290,70,true));g.fillAll();
    g.setColour(accent);font(g,12,true);g.drawText("E P I N I Z E R   /   A U D I O",34,28,400,20,juce::Justification::left);
    g.setColour(ink);font(g,38,true);g.drawText("Make it monumental.",32,64,620,52,juce::Justification::left);
    g.setColour(muted);font(g,14);g.drawText("Six voices. Four worlds. Your sound, expanded.",35,120,500,22,juce::Justification::left);
    panel(g,{32,153,736,62},18);
    panel(g,{32,231,736,217},24);
    const char* names[]{"SPACE","DENSITY","MOTION","MIX"};const char* descriptions[]{"Bloom & scale","Voices & body","Drift & texture","Dry / epic"};
    for(int i=0;i<4;++i){const int x=48+i*178;g.setColour(ink);font(g,12,true);g.drawText(names[i],x,251,166,20,juce::Justification::centred);g.setColour(muted);font(g,12);g.drawText(descriptions[i],x,418,166,18,juce::Justification::centred);if(i<3){g.setColour(juce::Colours::white.withAlpha(.055f));g.drawVerticalLine(x+172,258,422);}}
    g.setColour(accent);g.fillEllipse(35,480,6,6);g.setColour(muted);font(g,12);const char* moods[]{"GLASS  /  Prism voices + octave light","MASSIVE  /  Ensemble + lower octave","HORROBLE  /  Reverse + spectral unrest","WARM  /  Tape echoes + velvet voices"};g.drawText(moods[std::max(0,selected)],49,471,370,26,juce::Justification::left);
    g.setColour(muted);font(g,10);g.drawText("OUT",388,473,32,22,juce::Justification::left);
    for(int i=0;i<12;++i){g.setColour(float(i)/12.f<level?accent.withAlpha(.85f):juce::Colours::white.withAlpha(.07f));g.fillRoundedRectangle(float(424+i*6),480,3,8,1);}
}
