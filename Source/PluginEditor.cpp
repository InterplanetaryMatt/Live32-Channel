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
    : juce::AudioProcessorEditor(&p), processor(p), eqCurve(p.parameters), dynamicsCurve(p.parameters)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(980, 600, 1600, 1000);

    addKnob("trim", "GAIN", " dB", 1);
    addKnob("hpfHz", "FREQUENCY", " Hz", 0);
    addToggle("phase", "Ø");
    addToggle("hpfOn", "LOW CUT");

    addKnob("gateThreshold", "THRESHOLD", " dB", 1);
    addKnob("gateRange", "RANGE", " dB", 0);
    addKnob("gateAttack", "ATTACK", " ms", 1);
    addKnob("gateHold", "HOLD", " ms", 0);
    addKnob("gateRelease", "RELEASE", " ms", 0);
    addToggle("gateOn", "GATE");

    addKnob("compThreshold", "THRESHOLD", " dB", 1);
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
    addKnob("lowHz", "FREQUENCY", " Hz", 0);
    addKnob("lowGain", "GAIN", " dB", 1);
    addKnob("lowQ", "WIDTH", {}, 2);
    addChoice("lowMode", "MODE", { "LCUT", "LSHV", "PEQ" });

    addKnob("lowMidHz", "FREQUENCY", " Hz", 0);
    addKnob("lowMidGain", "GAIN", " dB", 1);
    addKnob("lowMidQ", "WIDTH", {}, 2);
    addChoice("lowMidMode", "MODE", { "VEQ", "PEQ" });

    addKnob("highMidHz", "FREQUENCY", " Hz", 0);
    addKnob("highMidGain", "GAIN", " dB", 1);
    addKnob("highMidQ", "WIDTH", {}, 2);
    addChoice("highMidMode", "MODE", { "VEQ", "PEQ" });

    addKnob("highHz", "FREQUENCY", " Hz", 0);
    addKnob("highGain", "GAIN", " dB", 1);
    addKnob("highQ", "WIDTH", {}, 2);
    addChoice("highMode", "MODE", { "PEQ", "HSHV", "HCUT" });

    static constexpr const char* bandNames[] = { "LOW", "LO MID", "HI MID", "HIGH" };
    for (int i = 0; i < 4; ++i)
    {
        eqBandButtons[static_cast<std::size_t>(i)] = std::make_unique<juce::ToggleButton>(bandNames[i]);
        auto& b = *eqBandButtons[static_cast<std::size_t>(i)];
        b.setClickingTogglesState(true);
        b.setRadioGroupId(3201, juce::dontSendNotification);
        b.onClick = [this, i] { selectEqBand(i); };
        addAndMakeVisible(b);
    }

    addAndMakeVisible(eqCurve);
    addAndMakeVisible(dynamicsCurve);
    addAndMakeVisible(meters);

    selectEqBand(0);

    // Size only after every dynamic control exists. setSize() triggers resized().
    setSize(1280, 720);
    resized();
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
    c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 18);
    c->slider.setNumDecimalPlacesToDisplay(decimals);
    c->slider.textFromValueFunction = [suffix, decimals](double value)
    {
        return juce::String(value, decimals) + suffix;
    };
    c->slider.valueFromTextFunction = [](const juce::String& textValue)
    {
        return textValue.getDoubleValue();
    };
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

    // Desk-like physical geography: PREAMP over GATE, EQ over DYNAMICS.
    drawSection(g, scaled(10, 60, 390, 285), "CONFIG / PREAMP");
    drawSection(g, scaled(408, 60, 862, 360), "EQUALISER");
    drawSection(g, scaled(10, 353, 390, 357), "GATE");
    drawSection(g, scaled(408, 428, 862, 282), "DYNAMICS");
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

void Live32ChannelAudioProcessorEditor::selectEqBand(int band)
{
    selectedEqBand = juce::jlimit(0, 3, band);
    for (int i = 0; i < 4; ++i)
        eqBandButtons[static_cast<std::size_t>(i)]->setToggleState(i == selectedEqBand, juce::dontSendNotification);
    updateEqBandVisibility();
    resized();
    repaint();
}

void Live32ChannelAudioProcessorEditor::updateEqBandVisibility()
{
    static constexpr const char* freq[] = { "lowHz", "lowMidHz", "highMidHz", "highHz" };
    static constexpr const char* gain[] = { "lowGain", "lowMidGain", "highMidGain", "highGain" };
    static constexpr const char* width[] = { "lowQ", "lowMidQ", "highMidQ", "highQ" };
    static constexpr const char* mode[] = { "lowMode", "lowMidMode", "highMidMode", "highMode" };

    for (int i = 0; i < 4; ++i)
    {
        const bool visible = i == selectedEqBand;
        for (auto* id : { freq[i], gain[i], width[i] })
        {
            if (auto it = knobs.find(id); it != knobs.end())
            {
                it->second->slider.setVisible(visible);
                it->second->label.setVisible(visible);
            }
        }
        if (auto it = choices.find(mode[i]); it != choices.end())
        {
            it->second->combo.setVisible(visible);
            it->second->label.setVisible(visible);
        }
    }
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

    // PREAMP: two large hardware-style encoders and the desk switches.
    layoutKnob("trim", R(28, 108, 130, 128));
    layoutKnob("hpfHz", R(166, 108, 130, 128));
    layoutToggle("phase", R(48, 270, 72, 30));
    layoutToggle("hpfOn", R(176, 270, 92, 30));
    meters.setBounds(R(305, 104, 78, 205));

    // GATE: threshold is the prominent surface control; detailed timing remains close by.
    layoutKnob("gateThreshold", R(28, 405, 130, 128));
    layoutToggle("gateOn", R(51, 548, 84, 30));
    layoutKnob("gateRange", R(165, 395, 96, 112));
    layoutKnob("gateAttack", R(276, 395, 96, 112));
    layoutKnob("gateHold", R(165, 535, 96, 112));
    layoutKnob("gateRelease", R(276, 535, 96, 112));

    // EQ: graph on the left, then the same WIDTH/FREQUENCY/GAIN control stack
    // and four band-select keys used on the physical desk.
    eqCurve.setBounds(R(430, 102, 500, 260));
    layoutToggle("eqOn", R(1140, 354, 78, 30));

    static constexpr const char* freq[] = { "lowHz", "lowMidHz", "highMidHz", "highHz" };
    static constexpr const char* gain[] = { "lowGain", "lowMidGain", "highMidGain", "highGain" };
    static constexpr const char* width[] = { "lowQ", "lowMidQ", "highMidQ", "highQ" };
    static constexpr const char* mode[] = { "lowMode", "lowMidMode", "highMidMode", "highMode" };

    layoutKnob(width[selectedEqBand], R(944, 92, 118, 100));
    layoutKnob(freq[selectedEqBand], R(944, 190, 118, 100));
    layoutKnob(gain[selectedEqBand], R(944, 288, 118, 100));
    layoutChoice(mode[selectedEqBand], R(1074, 314, 94, 54));

    for (int i = 0; i < 4; ++i)
        eqBandButtons[static_cast<std::size_t>(i)]->setBounds(R(1168, 105 + i * 54, 82, 34));

    // DYNAMICS: M32-style prominent threshold/COMP controls plus an LCD-like
    // transfer graph and the detailed compressor controls alongside.
    dynamicsCurve.setBounds(R(430, 468, 312, 205));
    layoutKnob("compThreshold", R(758, 472, 112, 112));
    layoutToggle("compOn", R(774, 592, 80, 30));
    layoutKnob("compRatio", R(875, 472, 92, 102));
    layoutKnob("compKnee", R(973, 472, 92, 102));
    layoutKnob("keyFilterHz", R(1071, 472, 92, 102));
    layoutKnob("compAttack", R(875, 585, 92, 102));
    layoutKnob("compRelease", R(973, 585, 92, 102));
    layoutKnob("compMakeup", R(1071, 585, 92, 102));
    layoutToggle("keyFilterOn", R(1172, 506, 82, 28));
    layoutToggle("externalKey", R(1172, 544, 82, 28));
}

void Live32ChannelAudioProcessorEditor::timerCallback()
{
    const auto gr = processor.getGainReductionMeter();
    meters.setValues(processor.getInputMeter(), processor.getOutputMeter(), gr, processor.getGateClosedMeter());
    dynamicsCurve.setGainReduction(gr);
    eqCurve.repaint();
}

void Live32ChannelAudioProcessorEditor::MeterPanel::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff111417));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(juce::Colour(0xff3e4448));
    g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    const float top = 25.0f;
    const float bottomMargin = 34.0f;
    const float h = juce::jmax(30.0f, r.getHeight() - top - bottomMargin);
    const float meterW = juce::jlimit(7.0f, 14.0f, r.getWidth() * 0.14f);
    const float centres[] = { r.getWidth() * 0.22f, r.getWidth() * 0.50f, r.getWidth() * 0.78f };

    const auto drawLevel = [&](int index, float level, const char* name)
    {
        const float db = toDb(level);
        const float norm = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
        juce::Rectangle<float> track(centres[index] - meterW * 0.5f, top, meterW, h);
        g.setColour(juce::Colour(0xff080a0b));
        g.fillRect(track);
        g.setColour(db > -3.0f ? juce::Colour(0xffff6734) : juce::Colour(0xffffa800));
        g.fillRect(track.withTrimmedTop(h * (1.0f - norm)));
        g.setColour(juce::Colour(0xffcdd1d4));
        g.drawText(name, juce::Rectangle<int>(juce::roundToInt(centres[index] - 14.0f), 5, 28, 14), juce::Justification::centred);
    };

    drawLevel(0, input, "IN");
    drawLevel(1, output, "OUT");

    g.setColour(juce::Colour(0xffcdd1d4));
    g.drawText("GR", juce::Rectangle<int>(juce::roundToInt(centres[2] - 14.0f), 5, 28, 14), juce::Justification::centred);
    const float grNorm = juce::jlimit(0.0f, 1.0f, gainReduction / 24.0f);
    juce::Rectangle<float> gr(centres[2] - meterW * 0.5f, top, meterW, h);
    g.setColour(juce::Colour(0xff080a0b));
    g.fillRect(gr);
    g.setColour(juce::Colour(0xffffa800));
    g.fillRect(gr.withHeight(gr.getHeight() * grNorm));

    g.setColour(gateClosed > 0.5f ? juce::Colour(0xffffa800) : juce::Colour(0xff353a3e));
    g.fillEllipse(r.getWidth() * 0.5f - 5.0f, r.getHeight() - 24.0f, 10.0f, 10.0f);
    g.setColour(juce::Colour(0xffaeb4b8));
    g.drawText("GATE", 0, static_cast<int>(r.getHeight() - 17.0f), static_cast<int>(r.getWidth()), 14, juce::Justification::centred);
}

float Live32ChannelAudioProcessorEditor::DynamicsCurve::raw(const char* id) const noexcept
{
    if (auto* p = parameters.getRawParameterValue(id)) return p->load();
    return 0.0f;
}

double Live32ChannelAudioProcessorEditor::DynamicsCurve::outputForInput(double inputDb) const noexcept
{
    const double threshold = raw("compThreshold");
    const double ratio = juce::jmax(1.0, static_cast<double>(raw("compRatio")));
    const double knee = static_cast<double>(raw("compKnee")) * 3.0;
    const double makeup = raw("compMakeup");
    const double xDb = inputDb - threshold;
    double reductionDb = 0.0;

    if (knee <= 0.0)
    {
        if (xDb > 0.0)
            reductionDb = xDb * (1.0 / ratio - 1.0);
    }
    else if (xDb <= -knee * 0.5)
    {
        reductionDb = 0.0;
    }
    else if (xDb >= knee * 0.5)
    {
        reductionDb = xDb * (1.0 / ratio - 1.0);
    }
    else
    {
        const double t = xDb + knee * 0.5;
        reductionDb = (1.0 / ratio - 1.0) * t * t / (2.0 * knee);
    }

    return inputDb + reductionDb + makeup;
}

void Live32ChannelAudioProcessorEditor::DynamicsCurve::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff080b0d));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(juce::Colour(0xff3b4348));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);

    auto graph = r.reduced(12.0f, 18.0f);
    constexpr double xMin = -60.0, xMax = 6.0;
    constexpr double yMin = -60.0, yMax = 12.0;
    const auto xForDb = [graph, xMin, xMax](double db)
    {
        return graph.getX() + static_cast<float>((db - xMin) / (xMax - xMin)) * graph.getWidth();
    };
    const auto yForDb = [graph, yMin, yMax](double db)
    {
        return graph.getBottom() - static_cast<float>((db - yMin) / (yMax - yMin)) * graph.getHeight();
    };

    for (double db : { -48.0, -36.0, -24.0, -12.0, 0.0 })
    {
        g.setColour(juce::Colour(0xff263038));
        g.drawVerticalLine(static_cast<int>(xForDb(db)), graph.getY(), graph.getBottom());
        g.drawHorizontalLine(static_cast<int>(yForDb(db)), graph.getX(), graph.getRight());
    }

    juce::Path unity;
    unity.startNewSubPath(xForDb(xMin), yForDb(xMin));
    unity.lineTo(xForDb(xMax), yForDb(xMax));
    g.setColour(juce::Colour(0xff586168));
    g.strokePath(unity, juce::PathStrokeType(1.0f));

    const bool enabled = raw("compOn") >= 0.5f;
    juce::Path curve;
    constexpr int points = 160;
    for (int i = 0; i < points; ++i)
    {
        const double inDb = xMin + (xMax - xMin) * static_cast<double>(i) / static_cast<double>(points - 1);
        const double outDb = enabled ? outputForInput(inDb) : inDb + raw("compMakeup");
        const float x = xForDb(inDb);
        const float y = yForDb(juce::jlimit(yMin, yMax, outDb));
        if (i == 0) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
    }

    const double threshold = raw("compThreshold");
    g.setColour(juce::Colour(0x88ffa800));
    g.drawVerticalLine(static_cast<int>(xForDb(threshold)), graph.getY(), graph.getBottom());

    g.setColour(enabled ? juce::Colour(0xffffb21a) : juce::Colour(0xff70777c));
    g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(enabled ? juce::Colour(0xffffb21a) : juce::Colour(0xff858b90));
    g.drawText(enabled ? "COMP TRANSFER" : "COMP OFF", 10, 4, 130, 15, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffd9dddf));
    g.drawText("GR " + juce::String(gainReduction, 1) + " dB", getWidth() - 90, 4, 80, 15, juce::Justification::centredRight);
}
