#pragma once
#include <JuceHeader.h>
#include "../DSP/Live32ChannelDSP.h"

class EQCurve final : public juce::Component
{
public:
    explicit EQCurve(juce::AudioProcessorValueTreeState& state) : parameters(state) {}
    void paint(juce::Graphics&) override;

private:
    float raw(const char* id) const noexcept;
    double bandMagnitude(int band, double frequency, double sampleRate) const noexcept;
    juce::AudioProcessorValueTreeState& parameters;
};
