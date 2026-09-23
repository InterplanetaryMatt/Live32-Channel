#include "PluginEditor.h"

namespace
{
const juce::Colour panelDark { 0xff171a1d };
const juce::Colour panelMid { 0xff22262a };
const juce::Colour line { 0xff4a5055 };
const juce::Colour amber { 0xffffa800 };

void drawSection(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour(panelDark);
    g.fillRoundedRectangle(r.toFloat(), 5.0f);
    g.setColour(line);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 5.0f, 1.0f);
    auto header = r.removeFromTop(30);
    g.setColour(panelMid);
    g.fillRoundedRectangle(header.toFloat(), 5.0f);
    g.fillRect(header.withTrimmedTop(10));
    g.setColour(amber);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText(title, header.reduced(10, 0), juce::Justification::centredLeft);
}

float toDb(float linear)
{
    return juce::Decibels::gainToDecibels(linear, -60.0f);
}
}

Live32ChannelAudioProcessorEditor::Live32ChannelAudioProcessorEditor(Live32ChannelAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p), eqCurve(p.parameters)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(980, 600, 1600, 1000);
    setSize(1280, 720);

    addKnob("trim", "GAIN", " dB", 1);
    addKnob("hpfHz", "LOW CUT", " Hz", 0);
    addToggle("phase", "Ø");
    addToggle("hpfOn", "LOW CUT");

    addKnob("gateThreshold", "THRESH", " dB", 1);
    addKnob("gateRange", "RANGE", " dB", 0);
    addKnob("gateAttack", "ATTACK", " ms", 1);
    addKnob("gateHold", "HOLD", " ms", 0);
    addKnob("gateRelease", "RELEASE", " ms", 0);
    addToggle("gateOn", "GATE");

    addKnob("compThreshold", "THRESH", " dB", 1);
    addKnob("compRatio", "RATIO", ":1", 1);
    addKnob("compAttack", "ATTACK", " ms", 1);
    addKnob("compRelease", "RELEASE", " ms", 0);
    addKnob("compMakeup", "MAKEUP", " dB", 1);
    addKnob("compKnee", "KNEE", {}, 0);
    addKnob("keyFilterHz", "KEY FREQ", " Hz", 0);
    addToggle("compOn", "COMP");
    addToggle("keyFilterOn", "KEY FILTER");
    addToggle("externalKey", "EXT KEY");

    addToggle("eqOn", "EQ");
    addKnob("lowHz", "FREQ", " Hz", 0);
    addKnob("lowGain", "GAIN", " dB", 1);
    addKnob("lowQ", "WIDTH", {}, 2);
    addChoice("lowMode", "LOW", { "LCUT", "LSHV", "PEQ" });

    addKnob("lowMidHz", "FREQ", " Hz", 0);
    addKnob("lowMidGain", "GAIN", " dB", 1);
    addKnob("lowMidQ", "WIDTH", {}, 2);
    addChoice("lowMidMode", "LO MID", { "VEQ", "PEQ" });

    addKnob("highMidHz", "FREQ", " Hz", 0);
    addKnob("highMidGain", "GAIN", " dB", 1);
    addKnob("highMidQ", "WIDTH", {}, 2);
    addChoice("highMidMode", "HI MID", { "VEQ", "PEQ" });

    addKnob("highHz", "FREQ", " Hz", 0);
    addKnob("highGain", "GAIN", " dB", 1);
    addKnob("highQ", "WIDTH", {}, 2);
    addChoice("highMode", "HIGH", { "PEQ", "HSHV", "HCUT" });

    addAndMakeVisible(eqCurve);
    addAndMakeVisible(meters);
    startTimerHz(30);
}

Live32ChannelAudioProcessorEditor::~Live32ChannelAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

Live32ChannelAudioProcessorEditor::KnobControl&
Live32ChannelAudioProcessorEditor::addKnob(const juce::String& id, const juce::String& text,
                                            const juce::String& suffix, int decimals)
{
    auto c = std::make_unique<KnobControl>();
    c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    c->slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.22f,
                                  juce::MathConstants<float>::pi * 2.78f, true);
    c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 18);
    c->slider.setTextValueSuffix(suffix);
    c->slider.setNumDecimalPlacesToDisplay(decimals);
    c->label.setText(text, juce::dontSendNotification);
    c->label.setJustificationType(juce::Justification::centred);
    c->label.setColour(juce::Label::textColourId, juce::Colour(0xffd9dddf));
    c->label.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, id, c->slider);
    addAndMakeVisible(c->slider);
    addAndMakeVisible(c->label);
    auto& ref = *c;
    knobs[id] = std::move(c);
    return ref;
}

Live32ChannelAudioProcessorEditor::ToggleControl&
Live32ChannelAudioProcessorEditor::addToggle(const juce::String& id, const juce::String& text)
{
    auto c = std::make_unique<ToggleControl>();
    c->button.setButtonText(text);
    c->button.setClickingTogglesState(true);
    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters, id, c->button);
    addAndMakeVisible(c->button);
    auto& ref = *c;
    toggles[id] = std::move(c);
    return ref;
}

Live32ChannelAudioProcessorEditor::ChoiceControl&
Live32ChannelAudioProcessorEditor::addChoice(const juce::String& id, const juce::String& text,
                                              std::initializer_list<const char*> items)
{
    auto c = std::make_unique<ChoiceControl>();
    int n = 1;
    for (auto* item : items) c->combo.addItem(item, n++);
    c->label.setText(text, juce::dontSendNotification);
    c->label.setJustificationType(juce::Justification::centred);
    c->label.setColour(juce::Label::textColourId, amber);
    c->label.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters, id, c->combo);
    addAndMakeVisible(c->combo);
    addAndMakeVisible(c->label);
    auto& ref = *c;
    choices[id] = std::move(c);
    return ref;
}

void Live32ChannelAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0f11));

    auto header = getLocalBounds().removeFromTop(52);
    g.setColour(juce::Colour(0xff111417));
    g.fillRect(header);
    g.setColour(amber);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("LIVE32 CHANNEL", 18, 8, 290, 30, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffaeb4b8));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("DIGITAL LIVE CONSOLE CHANNEL STRIP", 310, 11, 280, 24, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff676d72));
    g.drawText("M32-inspired workflow • independent project", 18, 34, 420, 14, juce::Justification::centredLeft);

    const float sx = getWidth() / 1280.0f;
    const float sy = (getHeight() - 52.0f) / 668.0f;
    auto scaled = [sx, sy](int x, int y, int w, int h)
    {
        return juce::Rectangle<int>(juce::roundToInt(x * sx), juce::roundToInt(52 + (y - 52) * sy),
                                    juce::roundToInt(w * sx), juce::roundToInt(h * sy));
    };

    drawSection(g, scaled(10, 60, 185, 650), "PREAMP");
    drawSection(g, scaled(202, 60, 255, 650), "GATE");
    drawSection(g, scaled(464, 60, 318, 650), "DYNAMICS");
    drawSection(g, scaled(789, 60, 481, 650), "EQUALISER");
}

void Live32ChannelAudioProcessorEditor::layoutKnob(const char* id, juce::Rectangle<int> r)
{
    auto it = knobs.find(id); if (it == knobs.end()) return;
    auto& c = *it->second;
    c.label.setBounds(r.removeFromTop(16));
    c.slider.setBounds(r);
}

void Live32ChannelAudioProcessorEditor::layoutToggle(const char* id, juce::Rectangle<int> r)
{
    auto it = toggles.find(id); if (it != toggles.end()) it->second->button.setBounds(r);
}

void Live32ChannelAudioProcessorEditor::layoutChoice(const char* id, juce::Rectangle<int> r)
{
    auto it = choices.find(id); if (it == choices.end()) return;
    auto& c = *it->second;
    c.label.setBounds(r.removeFromTop(15));
    c.combo.setBounds(r);
}

void Live32ChannelAudioProcessorEditor::resized()
{
    const float sx = getWidth() / 1280.0f;
    const float sy = (getHeight() - 52.0f) / 668.0f;
    auto R = [sx, sy](int x, int y, int w, int h)
    {
        return juce::Rectangle<int>(juce::roundToInt(x * sx), juce::roundToInt(52 + (y - 52) * sy),
                                    juce::roundToInt(w * sx), juce::roundToInt(h * sy));
    };

    // PREAMP
    layoutKnob("trim", R(34, 112, 136, 130));
    layoutKnob("hpfHz", R(34, 275, 136, 130));
    layoutToggle("phase", R(38, 440, 55, 28));
    layoutToggle("hpfOn", R(102, 440, 65, 28));
    meters.setBounds(R(31, 500, 142, 178));

    // GATE
    layoutToggle("gateOn", R(222, 101, 75, 28));
    layoutKnob("gateThreshold", R(220, 145, 104, 122));
    layoutKnob("gateRange", R(330, 145, 104, 122));
    layoutKnob("gateAttack", R(220, 303, 104, 122));
    layoutKnob("gateHold", R(330, 303, 104, 122));
    layoutKnob("gateRelease", R(273, 461, 104, 122));

    // DYNAMICS
    layoutToggle("compOn", R(485, 101, 76, 28));
    layoutKnob("compThreshold", R(484, 145, 90, 118));
    layoutKnob("compRatio", R(578, 145, 90, 118));
    layoutKnob("compKnee", R(672, 145, 90, 118));
    layoutKnob("compAttack", R(484, 298, 90, 118));
    layoutKnob("compRelease", R(578, 298, 90, 118));
    layoutKnob("compMakeup", R(672, 298, 90, 118));
    layoutToggle("keyFilterOn", R(485, 468, 92, 27));
    layoutToggle("externalKey", R(584, 468, 83, 27));
    layoutKnob("keyFilterHz", R(665, 448, 96, 122));

    // EQ
    layoutToggle("eqOn", R(808, 100, 62, 28));
    eqCurve.setBounds(R(873, 98, 376, 178));

    const int xs[] = { 808, 920, 1032, 1144 };
    const char* freq[] = { "lowHz", "lowMidHz", "highMidHz", "highHz" };
    const char* gain[] = { "lowGain", "lowMidGain", "highMidGain", "highGain" };
    const char* q[] = { "lowQ", "lowMidQ", "highMidQ", "highQ" };
    const char* mode[] = { "lowMode", "lowMidMode", "highMidMode", "highMode" };
    for (int i = 0; i < 4; ++i)
    {
        layoutChoice(mode[i], R(xs[i], 292, 92, 48));
        layoutKnob(freq[i], R(xs[i], 354, 92, 105));
        layoutKnob(gain[i], R(xs[i], 470, 92, 105));
        layoutKnob(q[i], R(xs[i], 586, 92, 105));
    }
}

void Live32ChannelAudioProcessorEditor::timerCallback()
{
    meters.setValues(processor.getInputMeter(), processor.getOutputMeter(),
                     processor.getGainReductionMeter(), processor.getGateClosedMeter());
    eqCurve.repaint();
}

void Live32ChannelAudioProcessorEditor::MeterPanel::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff111417));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(juce::Colour(0xff3e4448));
    g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    const auto drawMeter = [&](float x, float level, const char* name)
    {
        const float top = 24.0f, h = r.getHeight() - 48.0f, w = 19.0f;
        const float db = toDb(level);
        const float norm = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
        juce::Rectangle<float> track(x, top, w, h);
        g.setColour(juce::Colour(0xff080a0b)); g.fillRect(track);
        auto fill = track.withTrimmedTop(h * (1.0f - norm));
        g.setColour(db > -3.0f ? juce::Colour(0xffff6734) : juce::Colour(0xffffa800)); g.fillRect(fill);
        g.setColour(juce::Colour(0xffcdd1d4)); g.drawText(name, static_cast<int>(x - 4), 5, 28, 15, juce::Justification::centred);
    };

    drawMeter(17.0f, input, "IN");
    drawMeter(48.0f, output, "OUT");

    g.setColour(juce::Colour(0xffcdd1d4));
    g.drawText("GR", 80, 5, 28, 15, juce::Justification::centred);
    const float grNorm = juce::jlimit(0.0f, 1.0f, gainReduction / 24.0f);
    juce::Rectangle<float> gr(86.0f, 24.0f, 16.0f, r.getHeight() - 48.0f);
    g.setColour(juce::Colour(0xff080a0b)); g.fillRect(gr);
    g.setColour(juce::Colour(0xffffa800)); g.fillRect(gr.withHeight(gr.getHeight() * grNorm));

    g.setColour(gateClosed > 0.5f ? juce::Colour(0xffffa800) : juce::Colour(0xff353a3e));
    g.fillEllipse(r.getWidth() - 27.0f, 8.0f, 10.0f, 10.0f);
    g.setColour(juce::Colour(0xffaeb4b8));
    g.drawText("GATE", 76, static_cast<int>(r.getHeight() - 22), 50, 14, juce::Justification::centred);
}
