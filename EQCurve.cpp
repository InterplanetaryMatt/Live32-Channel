#include "EQCurve.h"

float EQCurve::raw(const char* id) const noexcept
{
    if (auto* p = parameters.getRawParameterValue(id)) return p->load();
    return 0.0f;
}

double EQCurve::bandMagnitude(int band, double f, double sr) const noexcept
{
    using live32::FilterMath;
    live32::BiquadCoefficients c;
    if (band == 0)
    {
        const int mode = static_cast<int>(raw("lowMode") + 0.5f);
        const float hz = raw("lowHz"), gain = raw("lowGain"), q = raw("lowQ");
        c = mode == 0 ? FilterMath::highPass(sr, hz, 0.707f)
            : mode == 1 ? FilterMath::lowShelf(sr, hz, gain, juce::jlimit(0.2f, 2.0f, q))
                        : FilterMath::peak(sr, hz, q, gain);
    }
    else if (band == 1)
    {
        const float q = raw("lowMidQ");
        c = FilterMath::peak(sr, raw("lowMidHz"), raw("lowMidMode") < 0.5f ? juce::jmax(0.45f, q * 0.55f) : q, raw("lowMidGain"));
    }
    else if (band == 2)
    {
        const float q = raw("highMidQ");
        c = FilterMath::peak(sr, raw("highMidHz"), raw("highMidMode") < 0.5f ? juce::jmax(0.45f, q * 0.55f) : q, raw("highMidGain"));
    }
    else
    {
        const int mode = static_cast<int>(raw("highMode") + 0.5f);
        const float hz = raw("highHz"), gain = raw("highGain"), q = raw("highQ");
        c = mode == 0 ? FilterMath::peak(sr, hz, q, gain)
            : mode == 1 ? FilterMath::highShelf(sr, hz, gain, juce::jlimit(0.2f, 2.0f, q))
                        : FilterMath::lowPass(sr, hz, 0.707f);
    }
    return FilterMath::magnitude(c, sr, f);
}

void EQCurve::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff080b0d));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(juce::Colour(0xff3b4348));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);

    auto graph = r.reduced(10.0f, 18.0f);
    const auto xForHz = [graph](double hz)
    {
        const double t = std::log10(hz / 20.0) / std::log10(20000.0 / 20.0);
        return graph.getX() + static_cast<float>(t) * graph.getWidth();
    };
    const auto yForDb = [graph](double db)
    {
        return graph.getCentreY() - static_cast<float>(db / 18.0) * graph.getHeight() * 0.5f;
    };

    for (double hz : { 20.0, 100.0, 1000.0, 10000.0, 20000.0 })
    {
        const float x = xForHz(hz);
        g.setColour(juce::Colour(0xff263038));
        g.drawVerticalLine(static_cast<int>(x), graph.getY(), graph.getBottom());
    }
    for (double db : { -12.0, -6.0, 0.0, 6.0, 12.0 })
    {
        const float y = yForDb(db);
        g.setColour(db == 0.0 ? juce::Colour(0xff65717a) : juce::Colour(0xff263038));
        g.drawHorizontalLine(static_cast<int>(y), graph.getX(), graph.getRight());
    }

    const bool eqEnabled = raw("eqOn") >= 0.5f;
    const bool hpfEnabled = raw("hpfOn") >= 0.5f;
    const double sr = 48000.0; // display reference; DSP itself follows host sample rate
    const float hpfHz = raw("hpfHz");

    juce::Path path;
    for (int px = 0; px < static_cast<int>(graph.getWidth()); ++px)
    {
        const double norm = static_cast<double>(px) / std::max(1.0f, graph.getWidth() - 1.0f);
        const double hz = 20.0 * std::pow(1000.0, norm);
        double mag = 1.0;

        // Dedicated Live32/M32-style LOW CUT is a real 4th-order Butterworth HPF:
        // two cascaded second-order stages at the same cutoff.
        if (hpfEnabled)
        {
            const auto hp1 = live32::FilterMath::highPass(sr, hpfHz, 0.5411961f);
            const auto hp2 = live32::FilterMath::highPass(sr, hpfHz, 1.30656296f);
            mag *= live32::FilterMath::magnitude(hp1, sr, hz);
            mag *= live32::FilterMath::magnitude(hp2, sr, hz);
        }

        if (eqEnabled)
            for (int band = 0; band < 4; ++band) mag *= bandMagnitude(band, hz, sr);

        const double db = juce::jlimit(-18.0, 18.0, 20.0 * std::log10(std::max(mag, 1.0e-9)));
        const float x = graph.getX() + static_cast<float>(px);
        const float y = yForDb(db);
        if (px == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }

    juce::Path fill = path;
    fill.lineTo(graph.getRight(), graph.getBottom());
    fill.lineTo(graph.getX(), graph.getBottom());
    fill.closeSubPath();
    g.setColour(juce::Colour(0x36ffa800));
    g.fillPath(fill);
    g.setColour((eqEnabled || hpfEnabled) ? juce::Colour(0xffffb21a) : juce::Colour(0xff70777c));
    g.strokePath(path, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    if (hpfEnabled)
    {
        g.setColour(juce::Colour(0xffffb21a));
        g.drawText("LOW CUT " + juce::String(hpfHz, 0) + " Hz", 10, 3, 140, 14, juce::Justification::centredLeft);
    }
    if (!eqEnabled)
    {
        g.setColour(juce::Colour(0xff858b90));
        g.drawText("EQ OFF", getWidth() - 70, 3, 60, 14, juce::Justification::centredRight);
    }
}
