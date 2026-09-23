#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "DSP/Live32ChannelDSP.h"

class Live32ChannelAudioProcessor final : public juce::AudioProcessor
{
public:
    Live32ChannelAudioProcessor();
    ~Live32ChannelAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;

    float getInputMeter() const noexcept { return inputMeter.load(); }
    float getOutputMeter() const noexcept { return outputMeter.load(); }
    float getGainReductionMeter() const noexcept { return gainReductionMeter.load(); }
    float getGateClosedMeter() const noexcept { return gateClosedMeter.load(); }
    double getCurrentSampleRateForUI() const noexcept { return currentSampleRate.load(); }

private:
    live32::Parameters readDSPParameters() const noexcept;

    live32::ChannelDSP dsp;
    std::atomic<float> inputMeter { 0.0f };
    std::atomic<float> outputMeter { 0.0f };
    std::atomic<float> gainReductionMeter { 0.0f };
    std::atomic<float> gateClosedMeter { 0.0f };
    std::atomic<double> currentSampleRate { 48000.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Live32ChannelAudioProcessor)
};
