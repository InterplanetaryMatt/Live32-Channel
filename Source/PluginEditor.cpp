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

    // IMPORTANT: do not enable host resizing until all dynamically-created
    // controls exist. Some hosts (including REAPER) may trigger resized()
    // immediately from setResizable()/setResizeLimits().
    addKnob("trim", "GAIN", " dB", 1);
    addKnob("hpfHz", "FREQUENCY", " Hz", 0);
    addToggle("phase", "Φ");
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

    // Establish the initial EQ-band state without forcing an early resized().
    selectedEqBand = 0;
    for (int i = 0; i < 4; ++i)
        if (auto* b = eqBandButtons[static_cast<std::size_t>(i)].get())
            b->setToggleState(i == selectedEqBand, juce::dontSendNotification);
    updateEqBandVisibility();

    // Size and enable host resizing only after every dynamic control exists.
    // setSize() triggers the first safe resized() call.
    setSize(1280, 720);
    setResizable(true, true);
    setResizeLimits(980, 600, 1600, 1000);
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

    // Do not use JUCE's Slider text box here. SliderAttachment can inherit the
    // parameter's raw floating-point text for logarithmic NormalisableRanges,
    // which is why values such as 4.9999995 appeared in the previous build.
    // A dedicated readout label gives Live32 deterministic desk-style text.
    c->slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    c->label.setText(text, juce::dontSendNotification);
    c->label.setJustificationType(juce::Justification::centred);
    c->label.setColour(juce::Label::textColourId, juce::Colour(0xffd9dddf));
    c->label.setFont(juce::FontOptions(10.5f, juce::Font::bold));

    c->valueLabel.setJustificationType(juce::Justification::centred);
    c->valueLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe8e8e8));
    c->valueLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff17191b));
    c->valueLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff596066));
    c->valueLabel.setFont(juce::FontOptions(10.5f));
    c->valueLabel.setEditable(false, false, false);

    auto* slider = &c->slider;
    auto* valueLabel = &c->valueLabel;
    const auto updateReadout = [slider, valueLabel, suffix, decimals]
    {
        const auto value = slider->getValue();
        juce::String shown;

        if (suffix == " Hz" && value >= 1000.0)
        {
            const int khzDecimals = value < 10000.0 ? 2 : 1;
            shown = juce::String(value / 1000.0, khzDecimals) + " kHz";
        }
        else
        {
            shown = juce::String(value, decimals) + suffix;
        }

        valueLabel->setText(shown, juce::dontSendNotification);
    };

    c->slider.onValueChange = updateReadout;
    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, id, c->slider);
    updateReadout();

    addAndMakeVisible(c->slider);
    addAndMakeVisible(c->label);
    addAndMakeVisible(c->valueLabel);
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

    // Physical-console geography: hardware controls occupy the left/middle,
    // while the visual feedback lives together in an LCD-style window at right.
    drawSection(g, scaled(10, 60, 300, 285), "CONFIG / PREAMP");
    drawSection(g, scaled(318, 60, 480, 285), "EQUALISER");
    drawSection(g, scaled(10, 353, 300, 357), "GATE");
    drawSection(g, scaled(318, 353, 480, 357), "DYNAMICS");
    drawSection(g, scaled(806, 60, 464, 650), "CHANNEL DISPLAY");

    // LCD divider and small page labels.
    auto lcd = scaled(820, 96, 436, 595);
    g.setColour(juce::Colour(0xff101417));
    g.fillRoundedRectangle(lcd.toFloat(), 4.0f);
    g.setColour(juce::Colour(0xff3b4348));
    g.drawRoundedRectangle(lcd.toFloat().reduced(0.5f), 4.0f, 1.0f);

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(amber);
    g.drawText("EQUALISER", scaled(832, 102, 110, 16), juce::Justification::centredLeft);
    g.drawText("DYNAMICS", scaled(832, 386, 110, 16), juce::Justification::centredLeft);
}

void Live32ChannelAudioProcessorEditor::layoutKnob(const char* id, juce::Rectangle<int> r)
{
    auto it = knobs.find(id); if (it == knobs.end()) return;
    auto& c = *it->second;
    c.label.setBounds(r.removeFromTop(15));
    c.valueLabel.setBounds(r.removeFromBottom(18).reduced(2, 0));
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
        if (auto* b = eqBandButtons[static_cast<std::size_t>(i)].get())
            b->setToggleState(i == selectedEqBand, juce::dontSendNotification);

    updateEqBandVisibility();

    // During construction a host may request layout before the editor has a
    // useful size. Avoid forcing layout until the component is established.
    if (getWidth() > 0 && getHeight() > 52)
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
                it->second->valueLabel.setVisible(visible);
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
    // Hosts are allowed to call resized() during editor construction.
    if (getWidth() <= 0 || getHeight() <= 52)
        return;

    const float sx = getWidth() / 1280.0f;
    const float sy = (getHeight() - 52.0f) / 668.0f;
    auto R = [sx, sy](int x, int y, int w, int h)
    {
        return juce::Rectangle<int>(juce::roundToInt(x * sx), juce::roundToInt(52 + (y - 52) * sy),
                                    juce::roundToInt(w * sx), juce::roundToInt(h * sy));
    };

    // CONFIG / PREAMP — close to the physical surface: gain, low-cut frequency,
    // phase and LOW CUT switch, with compact metering alongside.
    layoutKnob("trim",  R(26, 110, 112, 122));
    layoutKnob("hpfHz", R(150, 110, 112, 122));
    layoutToggle("phase", R(40, 268, 74, 30));
    layoutToggle("hpfOn", R(151, 268, 92, 30));
    meters.setBounds(R(250, 105, 48, 202));

    // GATE — threshold remains the large surface control; detailed controls
    // are arranged around it rather than taking over the panel.
    layoutKnob("gateThreshold", R(24, 400, 116, 124));
    layoutToggle("gateOn", R(42, 545, 82, 30));
    layoutKnob("gateRange",   R(150, 390, 70, 106));
    layoutKnob("gateAttack",  R(226, 390, 70, 106));
    layoutKnob("gateHold",    R(150, 520, 70, 106));
    layoutKnob("gateRelease", R(226, 520, 70, 106));

    // EQUALISER — emulate the M32 surface relationship: MODE at the left,
    // WIDTH / FREQUENCY / GAIN down the centre and four band keys at right.
    static constexpr const char* freq[]  = { "lowHz", "lowMidHz", "highMidHz", "highHz" };
    static constexpr const char* gain[]  = { "lowGain", "lowMidGain", "highMidGain", "highGain" };
    static constexpr const char* width[] = { "lowQ", "lowMidQ", "highMidQ", "highQ" };
    static constexpr const char* mode[]  = { "lowMode", "lowMidMode", "highMidMode", "highMode" };

    // MODE and EQ on/off sit to the left of the encoder stack, matching the
    // desk's physical hierarchy and keeping them clear of the value readouts.
    layoutChoice(mode[selectedEqBand], R(338, 174, 96, 56));
    layoutToggle("eqOn", R(350, 286, 74, 30));

    // Give each selected-band encoder its own vertical lane so labels and
    // readouts never collide.
    layoutKnob(width[selectedEqBand],  R(472, 72, 108, 82));
    layoutKnob(freq[selectedEqBand],   R(472, 160, 108, 82));
    layoutKnob(gain[selectedEqBand],   R(472, 248, 108, 82));

    for (int i = 0; i < 4; ++i)
        if (auto* b = eqBandButtons[static_cast<std::size_t>(i)].get())
            b->setBounds(R(686, 82 + i * 51, 90, 34));

    // DYNAMICS — again arranged as hardware first. The LCD graph is no longer
    // embedded among the controls.
    layoutKnob("compThreshold", R(338, 400, 116, 124));
    layoutToggle("compOn", R(356, 548, 82, 30));
    layoutKnob("compRatio",   R(466, 390, 78, 106));
    layoutKnob("compKnee",    R(552, 390, 78, 106));
    layoutKnob("keyFilterHz", R(638, 390, 78, 106));
    layoutKnob("compAttack",  R(466, 520, 78, 106));
    layoutKnob("compRelease", R(552, 520, 78, 106));
    layoutKnob("compMakeup",  R(638, 520, 78, 106));
    layoutToggle("keyFilterOn", R(714, 454, 72, 28));
    layoutToggle("externalKey", R(714, 492, 72, 28));

    // Shared desk-style LCD area on the right.
    eqCurve.setBounds(R(832, 124, 412, 235));
    dynamicsCurve.setBounds(R(832, 408, 412, 245));
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

    // The transfer display intentionally excludes makeup gain so the curve
    // directly communicates threshold, ratio and knee.
    return inputDb + reductionDb;
}

void Live32ChannelAudioProcessorEditor::DynamicsCurve::paint(juce::Graphics& g)
{
    auto outer = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff080b0d));
    g.fillRoundedRectangle(outer, 4.0f);
    g.setColour(juce::Colour(0xff3b4348));
    g.drawRoundedRectangle(outer, 4.0f, 1.0f);

    const bool enabled = raw("compOn") >= 0.5f;
    const double threshold = raw("compThreshold");
    const double ratio = juce::jmax(1.0, static_cast<double>(raw("compRatio")));
    const double kneeControl = static_cast<double>(raw("compKnee"));
    const double kneeDb = kneeControl * 3.0;
    const double makeup = raw("compMakeup");

    // Leave space at the bottom for M32-style parameter readouts and at the
    // right for a dedicated gain-reduction meter.
    auto graph = outer.reduced(12.0f, 20.0f);
    graph.removeFromBottom(42.0f);
    auto grArea = graph.removeFromRight(30.0f);
    graph.removeFromRight(8.0f);

    constexpr double minDb = -60.0;
    constexpr double maxDb = 0.0;

    const auto xForDb = [graph, minDb, maxDb](double db)
    {
        return graph.getX() + static_cast<float>((db - minDb) / (maxDb - minDb)) * graph.getWidth();
    };
    const auto yForDb = [graph, minDb, maxDb](double db)
    {
        return graph.getBottom() - static_cast<float>((db - minDb) / (maxDb - minDb)) * graph.getHeight();
    };

    // Console-like square grid.
    for (double db : { -60.0, -48.0, -36.0, -24.0, -12.0, 0.0 })
    {
        g.setColour(db == 0.0 ? juce::Colour(0xff566169) : juce::Colour(0xff283239));
        g.drawVerticalLine(static_cast<int>(xForDb(db)), graph.getY(), graph.getBottom());
        g.drawHorizontalLine(static_cast<int>(yForDb(db)), graph.getX(), graph.getRight());
    }

    // Unity reference.
    juce::Path unity;
    unity.startNewSubPath(xForDb(minDb), yForDb(minDb));
    unity.lineTo(xForDb(maxDb), yForDb(maxDb));
    g.setColour(juce::Colour(0xff657078));
    g.strokePath(unity, juce::PathStrokeType(1.0f));

    // Highlight the soft-knee region so the user can see what KNEE is doing.
    if (enabled && kneeDb > 0.0)
    {
        const auto left = xForDb(juce::jlimit(minDb, maxDb, threshold - kneeDb * 0.5));
        const auto right = xForDb(juce::jlimit(minDb, maxDb, threshold + kneeDb * 0.5));
        g.setColour(juce::Colour(0x18ffa800));
        g.fillRect(juce::Rectangle<float>(left, graph.getY(), juce::jmax(1.0f, right - left), graph.getHeight()));
    }

    juce::Path curve;
    constexpr int points = 200;
    for (int i = 0; i < points; ++i)
    {
        const double inDb = minDb + (maxDb - minDb) * static_cast<double>(i) / static_cast<double>(points - 1);
        // Always show the configured transfer characteristic. When COMP is
        // bypassed the curve is dimmed, but the user can still see the ratio
        // and knee they are about to engage.
        const double outDb = outputForInput(inDb);
        const float x = xForDb(inDb);
        const float y = yForDb(juce::jlimit(minDb, maxDb, outDb));
        if (i == 0) curve.startNewSubPath(x, y);
        else curve.lineTo(x, y);
    }

    // Threshold line and threshold point, similar to the desk dynamics page.
    const float threshX = xForDb(threshold);
    const float threshY = yForDb(outputForInput(threshold));
    g.setColour(juce::Colour(0x99ffa800));
    g.drawVerticalLine(static_cast<int>(threshX), graph.getY(), graph.getBottom());

    g.setColour(enabled ? juce::Colour(0xffffc13b) : juce::Colour(0xff9a8251));
    g.strokePath(curve, juce::PathStrokeType(2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // M32-style threshold point: the bend in the transfer line should be
    // immediately visible, even before the compressor is engaged.
    g.setColour(enabled ? juce::Colour(0xffffa800) : juce::Colour(0xff9a8251));
    g.fillRect(threshX - 4.0f, threshY - 4.0f, 8.0f, 8.0f);

    // Mark the knee boundaries. With a non-zero knee the curve transitions
    // smoothly between these two points rather than making a hard corner.
    if (kneeDb > 0.0)
    {
        const auto kneeIn = juce::jlimit(minDb, maxDb, threshold - kneeDb * 0.5);
        const auto kneeOut = juce::jlimit(minDb, maxDb, threshold + kneeDb * 0.5);
        g.setColour(juce::Colour(0x88ffa800));
        g.drawVerticalLine(static_cast<int>(xForDb(kneeIn)), graph.getY(), graph.getBottom());
        g.drawVerticalLine(static_cast<int>(xForDb(kneeOut)), graph.getY(), graph.getBottom());
    }

    // Gain-reduction meter on the right.
    g.setColour(juce::Colour(0xff14191c));
    g.fillRect(grArea);
    const float grNorm = juce::jlimit(0.0f, 1.0f, gainReduction / 24.0f);
    auto grFill = grArea.reduced(7.0f, 2.0f);
    grFill = grFill.withHeight(grFill.getHeight() * grNorm);
    grFill.setY(grArea.getY() + 2.0f);
    g.setColour(juce::Colour(0xffd84b35));
    g.fillRect(grFill);
    g.setColour(juce::Colour(0xffaeb4b8));
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.drawText("GR", grArea.toNearestInt().withHeight(14), juce::Justification::centred);

    // Header status.
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(enabled ? juce::Colour(0xffffb21a) : juce::Colour(0xff858b90));
    g.drawText(enabled ? "COMP" : "COMP OFF", 10, 4, 90, 14, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffd9dddf));
    g.drawText("GR " + juce::String(gainReduction, 1) + " dB",
               getWidth() - 98, 4, 88, 14, juce::Justification::centredRight);

    // M32-style numerical readouts under the graph.
    auto info = outer.toNearestInt().reduced(12, 6);
    info = info.removeFromBottom(34);
    const int cellW = info.getWidth() / 4;

    const auto drawCell = [&](int idx, const juce::String& title, const juce::String& value)
    {
        auto c = info.withX(info.getX() + idx * cellW).withWidth(cellW - 4);
        auto top = c.removeFromTop(12);
        g.setColour(juce::Colour(0xff9ea6ab));
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.drawText(title, top, juce::Justification::centred);
        g.setColour(juce::Colour(0xffe2e5e7));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(value, c, juce::Justification::centred);
    };

    drawCell(0, "THRESH", juce::String(threshold, 1) + " dB");
    drawCell(1, "RATIO",  juce::String(ratio, 1) + ":1");
    drawCell(2, "KNEE",   juce::String(kneeControl, 0));
    drawCell(3, "MAKEUP", juce::String(makeup, 1) + " dB");
}
