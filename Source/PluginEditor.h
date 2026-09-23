#pragma once

#include <JuceHeader.h>
#include <map>
#include <memory>
#include "PluginProcessor.h"
#include "UI/Live32LookAndFeel.h"
#include "UI/EQCurve.h"

class Live32ChannelAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    explicit Live32ChannelAudioProcessorEditor(Live32ChannelAudioProcessor&);
    ~Live32ChannelAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct KnobControl
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct ToggleControl
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    struct ChoiceControl
    {
        juce::ComboBox combo;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    class MeterPanel final : public juce::Component
    {
    public:
        void setValues(float in, float out, float gr, float gate)
        {
            input = in; output = out; gainReduction = gr; gateClosed = gate; repaint();
        }
        void paint(juce::Graphics&) override;
    private:
        float input { 0.0f }, output { 0.0f }, gainReduction { 0.0f }, gateClosed { 0.0f };
    };

    KnobControl& addKnob(const juce::String& id, const juce::String& label,
                         const juce::String& suffix = {}, int decimals = 1);
    ToggleControl& addToggle(const juce::String& id, const juce::String& text);
    ChoiceControl& addChoice(const juce::String& id, const juce::String& label,
                             std::initializer_list<const char*> items);

    void layoutKnob(const char* id, juce::Rectangle<int> bounds);
    void layoutToggle(const char* id, juce::Rectangle<int> bounds);
    void layoutChoice(const char* id, juce::Rectangle<int> bounds);
    void timerCallback() override;

    Live32ChannelAudioProcessor& processor;
    Live32LookAndFeel lookAndFeel;
    EQCurve eqCurve;
    MeterPanel meters;

    std::map<juce::String, std::unique_ptr<KnobControl>> knobs;
    std::map<juce::String, std::unique_ptr<ToggleControl>> toggles;
    std::map<juce::String, std::unique_ptr<ChoiceControl>> choices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Live32ChannelAudioProcessorEditor)
};
