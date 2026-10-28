#include "PluginEditor.h"
#include <BinaryData.h>

namespace {
const juce::Colour paper{0xfff0f2ec},secondary{0xffb5bfb3};
const juce::Colour green{0xffa8e878},forest{0xff397f50},ground{0xff101510};

juce::Font uiFont(float size,bool bold=false) {
    return juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain));
}
void text(juce::Graphics& g,const juce::String& value,juce::Rectangle<int> area,
          float size,juce::Colour colour,bool bold=false,int align=juce::Justification::left) {
    g.setFont(uiFont(size,bold));g.setColour(colour);
    g.drawText(value,area,align,false);
}
void focus(juce::Graphics& g,juce::Component& c,juce::Rectangle<float> bounds) {
    if(c.hasKeyboardFocus(true)) {
        g.setColour(green);g.drawRoundedRectangle(bounds.reduced(1.5f),5.f,1.5f);
    }
}
}

EpinizerLook::EpinizerLook() {
    setColour(juce::Slider::textBoxTextColourId,paper);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff141b15));
    setColour(juce::TextEditor::backgroundColourId,ground);
    setColour(juce::TextEditor::textColourId,paper);
    setColour(juce::TextEditor::highlightColourId,forest);
    setColour(juce::TextEditor::focusedOutlineColourId,green);
    setColour(juce::ComboBox::textColourId,paper);
    setColour(juce::ComboBox::backgroundColourId,ground);
    setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff44503f));
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff171e17));
    setColour(juce::PopupMenu::textColourId,paper);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,forest);
    setColour(juce::PopupMenu::highlightedTextColourId,paper);
    setColour(juce::TooltipWindow::backgroundColourId,ground);
    setColour(juce::TooltipWindow::textColourId,paper);
    setColour(juce::TooltipWindow::outlineColourId,forest);
}

void EpinizerLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,
                                   float start,float end,juce::Slider& slider) {
    const auto r=juce::Rectangle<float>(float(x),float(y),float(w),float(h))
                     .withSizeKeepingCentre(126.f,126.f).reduced(7.f);
    const auto centre=r.getCentre();
    const float radius=r.getWidth()*.5f,angle=start+pos*(end-start);
    const bool isMix=slider.getName()=="Mix";
    juce::Path track,active;
    track.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);
    active.addCentredArc(centre.x,centre.y,radius,radius,0,start,angle,true);
    const juce::PathStrokeType stroke(2.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded);
    g.setColour(juce::Colour(0xff394434));g.strokePath(track,juce::PathStrokeType(1.5f));
    g.setGradientFill(juce::ColourGradient(isMix?juce::Colour(0xff8d9686):forest,
        r.getX(),r.getBottom(),isMix?paper:green,r.getRight(),r.getY(),false));
    g.strokePath(active,stroke);
    const auto face=r.reduced(10.f);
    juce::Path shadow;shadow.addEllipse(face);
    juce::DropShadow(juce::Colours::black.withAlpha(.40f),8,{0,4}).drawForPath(g,shadow);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff222c22),centre.x,face.getY(),
                                         juce::Colour(0xff151c16),centre.x,face.getBottom(),false));
    g.fillEllipse(face);
    const auto a=centre.getPointOnCircumference(22.f,angle);
    const auto b=centre.getPointOnCircumference(34.f,angle);
    g.setColour(isMix?paper:green);g.drawLine({a,b},2.5f);
    if(slider.hasKeyboardFocus(true)) {
        g.setColour(green);g.drawEllipse(r.expanded(15.f),1.5f);
    }
}

void EpinizerLook::drawButtonBackground(juce::Graphics& g,juce::Button& button,
                                       const juce::Colour&,bool over,bool down) {
    const auto r=button.getLocalBounds().toFloat().reduced(.5f);
    const bool active=button.getToggleState();
    g.setColour(active?juce::Colour(0xe6294028):juce::Colour(over?0xd9273325:0xb3131b13));
    g.fillRoundedRectangle(r,8.f);
    if(active) {g.setColour(green);g.fillRoundedRectangle(r.getCentreX()-18.f,r.getBottom()-4.f,36.f,2.f,1.f);}
    if(down) {g.setColour(juce::Colours::white.withAlpha(.04f));g.fillRoundedRectangle(r,8.f);}
    focus(g,button,r);
}
void EpinizerLook::drawButtonText(juce::Graphics& g,juce::TextButton& button,bool,bool) {
    text(g,button.getButtonText().toUpperCase(),button.getLocalBounds().reduced(5,0),
         13.f,button.getToggleState()?green:secondary,true,juce::Justification::centred);
}
void EpinizerLook::drawToggleButton(juce::Graphics& g,juce::ToggleButton& button,bool over,bool down) {
    const auto r=button.getLocalBounds().toFloat().reduced(.5f);
    const bool active=button.getToggleState();
    g.setColour(active?juce::Colour(0xff20341f):juce::Colour(over?0xff1b261a:0xff111910));g.fillRoundedRectangle(r,8.f);
    g.setColour(active?green:juce::Colour(0xff64715c));
    g.fillEllipse(13.f,15.f,6.f,6.f);
    text(g,button.getButtonText().toUpperCase(),button.getLocalBounds().withTrimmedLeft(22),
         11.5f,active||down?green:secondary,true,juce::Justification::centred);
    focus(g,button,r);
}
void EpinizerLook::drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox& box) {
    const auto r=juce::Rectangle<float>(0,0,float(w),float(h)).reduced(.5f);
    g.setColour(juce::Colour(0xe6111911));g.fillRoundedRectangle(r,7.f);
    g.setColour(juce::Colour(0xff4a5a40));g.drawHorizontalLine(h-1,10.f,float(w)-10.f);
    focus(g,box,r);
    juce::Path arrow;const float cx=float(w)-17,cy=float(h)*.5f;
    arrow.startNewSubPath(cx-4,cy-2);arrow.lineTo(cx,cy+2);arrow.lineTo(cx+4,cy-2);
    g.setColour(green);g.strokePath(arrow,juce::PathStrokeType(1.5f));
}
juce::Font EpinizerLook::getComboBoxFont(juce::ComboBox&) {return uiFont(13.f);}
juce::Font EpinizerLook::getLabelFont(juce::Label&) {return uiFont(19.f);}
juce::Label* EpinizerLook::createSliderTextBox(juce::Slider& slider) {
    auto* label=juce::LookAndFeel_V4::createSliderTextBox(slider);
    label->setColour(juce::Label::backgroundColourId,juce::Colours::transparentBlack);
    label->setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);
    label->setColour(juce::TextEditor::backgroundColourId,ground);
    label->setColour(juce::TextEditor::outlineColourId,juce::Colours::transparentBlack);
    label->setColour(juce::TextEditor::focusedOutlineColourId,green);
    return label;
}

EpinizerEditor::EpinizerEditor(EpinizerProcessor& p):AudioProcessorEditor(p),owner(p),
    castle(juce::ImageCache::getFromMemory(BinaryData::castleentrance_png,BinaryData::castleentrance_pngSize)) {
    setLookAndFeel(&look);setSize(1000,640);setResizable(false,false);
    const char* ids[]{"space","density","motion","width","mix"};
    const char* labels[]{"Space","Density","Motion","Width","Mix"};
    const char* tips[]{
        "Space: atmosphere size, decay and bloom between notes.",
        "Density: ensemble voices, body and character layers.",
        "Motion: detune, drift and character movement.",
        "Width: wet stereo width. 0% centres the effect; 100% keeps its full stereo image. Dry is unchanged.",
        "Mix: original / processed balance. 0% is transparent; 100% is processed. Double-click any dial to reset."};
    const float defaults[]{.48f,.58f,.4f,1.f,.55f};
    for(size_t i=0;i<knobs.size();++i) {
        auto& k=knobs[i];k.setLookAndFeel(&look);k.setName(labels[i]);k.setTitle(labels[i]);
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
        k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,28);
        k.setTooltip(tips[i]);k.setWantsKeyboardFocus(true);k.setExplicitFocusOrder(int(i)+6);
        addAndMakeVisible(k);
        attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],k);
        k.setDoubleClickReturnValue(true,defaults[i]);
        k.textFromValueFunction=[](double v){return juce::String(juce::roundToInt(v*100))+"%";};
        k.valueFromTextFunction=[](const juce::String& v){return v.getDoubleValue()*.01;};k.updateText();
    }
    const char* names[]{"Glass","Massive","Horroble","Warm"};
    for(size_t i=0;i<characters.size();++i) {
        auto& b=characters[i];b.setButtonText(names[i]);b.setTitle(juce::String(names[i])+" character");
        b.setWantsKeyboardFocus(true);b.setExplicitFocusOrder(int(i)+2);addAndMakeVisible(b);
        b.onClick=[this,i]{auto* param=owner.state.getParameter("character");param->beginChangeGesture();
            param->setValueNotifyingHost(float(i)/3);param->endChangeGesture();timerCallback();};
    }
    for(int i=0;i<p.getNumPrograms();++i)presets.addItem(p.getProgramName(i),i+1);
    presets.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);
    presets.setTitle("Factory preset");presets.setExplicitFocusOrder(1);
    presets.setTooltip("Factory starting points. Your edits are saved in the DAW session.");
    presets.onChange=[this]{owner.setCurrentProgram(presets.getSelectedId()-1);};addAndMakeVisible(presets);
    addAndMakeVisible(bypass);bypass.setExplicitFocusOrder(13);
    bypass.setTooltip("Bypass the effect and pass the original signal.");
    bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);
    addAndMakeVisible(echo);echo.setExplicitFocusOrder(12);
    echo.setTooltip("Character echo: short reflections, or darker repeats in Warm.");
    echoAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"echo",echo);
    addAndMakeVisible(hold);hold.setExplicitFocusOrder(11);
    hold.setTooltip("Hold the captured atmosphere. Captured audio is not saved in the DAW state.");
    holdAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"hold",hold);
    timerCallback();startTimerHz(24);
}
EpinizerEditor::~EpinizerEditor() {stopTimer();setLookAndFeel(nullptr);}
void EpinizerEditor::timerCallback() {
    const int current=juce::jlimit(0,3,int(owner.state.getRawParameterValue("character")->load()));
    if(current!=selected) {
        selected=current;
        for(size_t i=0;i<characters.size();++i)characters[i].setToggleState(int(i)==selected,juce::dontSendNotification);
        repaint();
    }
    presets.setSelectedId(owner.getCurrentProgram()+1,juce::dontSendNotification);
    const float next=std::max(owner.meter.load(),level*.86f);
    if(std::abs(next-level)>.001f) {level=next;repaint(32,553,148,42);}
}
void EpinizerEditor::resized() {
    presets.setBounds(654,58,314,32);
    for(int i=0;i<4;++i)characters[size_t(i)].setBounds(32+i*236,115,228,38);
    for(int i=0;i<5;++i)knobs[size_t(i)].setBounds(40+i*184,326,184,156);
    hold.setBounds(626,559,104,36);echo.setBounds(744,559,104,36);bypass.setBounds(862,559,106,36);
}
void EpinizerEditor::paint(juce::Graphics& g) {
    g.fillAll(ground);
    if(castle.isValid())g.drawImage(castle,getLocalBounds().toFloat(),juce::RectanglePlacement::fillDestination);
    g.setColour(juce::Colours::black.withAlpha(.15f));g.fillAll();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xf4101510),0,0,juce::Colour(0x90101510),0,106,false));
    g.fillRect(0,0,getWidth(),106);
    g.setGradientFill(juce::ColourGradient(forest.withAlpha(.24f),0,0,juce::Colours::transparentBlack,550,100,false));
    g.fillRect(0,0,getWidth(),106);
    text(g,"EPINIZER",{32,15,420,58},42.f,paper,true);
    text(g,"MyDarknessDays",{650,18,318,25},15.f,paper,true,juce::Justification::right);
    g.setColour(green.withAlpha(.16f));g.drawHorizontalLine(105,32.f,968.f);
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.70f),0,155,
                                         juce::Colours::transparentBlack,620,155,false));
    g.fillRect(0,164,560,108);
    const char* names[]{"GLASS","MASSIVE","HORROBLE","WARM"};
    const char* moods[]{"Prism voices. Octave light.","Ensemble weight. Lower octave.",
                         "Reverse fragments. Unstable overtones.","Tape echoes. Velvet voices."};
    const auto index=size_t(std::max(0,selected));
    text(g,names[index],{40,190,490,36},26.f,paper,true);
    text(g,moods[index],{42,232,480,24},14.f,secondary);
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack,0,266,
                                         juce::Colour(0xf50b100b),0,518,false));
    g.fillRect(0,266,getWidth(),374);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xa6172518),32,286,
                                         juce::Colour(0xeb0d120d),32,530,false));
    g.fillRoundedRectangle(32,286,936,244,9.f);
    const char* labels[]{"SPACE","DENSITY","MOTION","WIDTH","MIX"};
    const char* descriptions[]{"Bloom & scale","Voices & body","Drift & texture","Stereo field","Original / effect"};
    for(int i=0;i<5;++i) {
        const int x=40+i*184;
        text(g,labels[i],{x,302,184,23},13.f,i==4?paper:green,true,juce::Justification::centred);
        text(g,descriptions[i],{x,495,184,20},12.f,secondary,false,juce::Justification::centred);
    }
    g.setColour(paper.withAlpha(.10f));g.drawHorizontalLine(546,32.f,968.f);
    text(g,"OUT",{32,562,36,30},10.5f,secondary,true);
    for(int i=0;i<16;++i) {
        g.setColour(float(i)/16.f<level?green:juce::Colour(0xff35412e));
        g.fillRect(72+i*6,573,3,9);
    }
    text(g,"ATMOSPHERE / "+juce::String(names[index]),{208,563,400,27},11.5f,secondary);
    text(g,"2.1.0",{860,610,108,18},10.5f,secondary,false,juce::Justification::right);
}
