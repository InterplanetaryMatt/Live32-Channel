#include "Live32LookAndFeel.h"

Live32LookAndFeel::Live32LookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe8e8e8));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff17191b));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3b3f43));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6e6e6));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff17191b));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff4a4e52));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1c1f22));
    setColour(juce::PopupMenu::textColourId, juce::Colours::white);
}

void Live32LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float start, float end, juce::Slider&)
{
    const auto size = static_cast<float>(juce::jmin(width, height));
    const auto radius = size * 0.34f;
    const auto cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const auto cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float angle = start + sliderPos * (end - start);

    constexpr int segments = 17;
    const auto amber = juce::Colour(0xffffa800);
    const auto darkAmber = juce::Colour(0xff4a3510);

    for (int i = 0; i < segments; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(segments - 1);
        const float a = start + t * (end - start);
        const bool lit = t <= sliderPos + 0.001f;
        const float inner = radius * 1.26f;
        const float outer = radius * 1.53f;
        juce::Path p;
        p.startNewSubPath(cx + std::sin(a) * inner, cy - std::cos(a) * inner);
        p.lineTo(cx + std::sin(a) * outer, cy - std::cos(a) * outer);
        g.setColour(lit ? amber : darkAmber);
        g.strokePath(p, juce::PathStrokeType(lit ? 2.7f : 1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour(juce::Colour(0xff6f7376));
    g.fillEllipse(cx - radius - 2.5f, cy - radius - 2.5f, (radius + 2.5f) * 2.0f, (radius + 2.5f) * 2.0f);
    g.setColour(juce::Colour(0xffb7b9ba));
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(juce::Colour(0xff202224));
    g.fillEllipse(cx - radius * 0.72f, cy - radius * 0.72f, radius * 1.44f, radius * 1.44f);
    g.setColour(juce::Colour(0xff090a0b));
    g.fillEllipse(cx - radius * 0.52f, cy - radius * 0.52f, radius * 1.04f, radius * 1.04f);

    juce::Path pointer;
    const float p0 = radius * 0.16f;
    const float p1 = radius * 0.78f;
    pointer.startNewSubPath(cx + std::sin(angle) * p0, cy - std::cos(angle) * p0);
    pointer.lineTo(cx + std::sin(angle) * p1, cy - std::cos(angle) * p1);
    g.setColour(juce::Colour(0xffeeeeee));
    g.strokePath(pointer, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void Live32LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hover, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = b.getToggleState();
    auto bg = on ? juce::Colour(0xffffa800) : juce::Colour(0xff24272a);
    if (hover) bg = bg.brighter(0.08f);
    if (down) bg = bg.darker(0.08f);

    g.setColour(bg);
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(on ? juce::Colour(0xff2b210c) : juce::Colour(0xff666a6e));
    g.drawRoundedRectangle(r, 3.0f, 1.0f);
    g.setColour(on ? juce::Colour(0xff111111) : juce::Colour(0xffdddddd));
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(4, 1), juce::Justification::centred, 1);
}

void Live32LookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                      int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(r, 3.0f, 1.0f);

    juce::Path arrow;
    const float cx = width - 12.0f;
    const float cy = height * 0.5f;
    arrow.addTriangle(cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour(juce::Colour(0xffffa800));
    g.fillPath(arrow);
}

void Live32LookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(5, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont(juce::FontOptions(11.0f, juce::Font::bold));
}
