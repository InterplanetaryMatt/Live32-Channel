#pragma once
#include <JuceHeader.h>

class Live32LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    Live32LookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool,
                      int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
};
