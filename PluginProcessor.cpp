#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
using APVTS = juce::AudioProcessorValueTreeState;

juce::NormalisableRange<float> logRange(float min, float max, float centre)
{
    juce::NormalisableRange<float> r(min, max);
    r.setSkewForCentre(centre);
    return r;
}

float raw(const APVTS& state, const char* id) noexcept
{
    if (auto* p = state.getRawParameterValue(id))
        return p->load();
    return 0.0f;
}
}

Live32ChannelAudioProcessor::Live32ChannelAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "LIVE32_CHANNEL", createParameterLayout())
{
}

void Live32ChannelAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate.store(sampleRate);
    dsp.prepare(sampleRate);
    dsp.setParameters(readDSPParameters());
}

bool Live32ChannelAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.getChannelSet(true, 1);
        if (!sidechain.isDisabled() && sidechain != juce::AudioChannelSet::mono() && sidechain != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

live32::Parameters Live32ChannelAudioProcessor::readDSPParameters() const noexcept
{
    live32::Parameters p;
    p.trimDb = raw(parameters, "trim");
    p.phaseInvert = raw(parameters, "phase") >= 0.5f;
    p.hpfEnabled = raw(parameters, "hpfOn") >= 0.5f;
    p.hpfHz = raw(parameters, "hpfHz");

    p.gateEnabled = raw(parameters, "gateOn") >= 0.5f;
    p.gateThresholdDb = raw(parameters, "gateThreshold");
    p.gateRangeDb = raw(parameters, "gateRange");
    p.gateAttackMs = raw(parameters, "gateAttack");
    p.gateHoldMs = raw(parameters, "gateHold");
    p.gateReleaseMs = raw(parameters, "gateRelease");

    p.eqEnabled = raw(parameters, "eqOn") >= 0.5f;
    p.lowHz = raw(parameters, "lowHz");
    p.lowGainDb = raw(parameters, "lowGain");
    p.lowQ = raw(parameters, "lowQ");
    p.lowMode = static_cast<int>(raw(parameters, "lowMode") + 0.5f);
    p.lowMidHz = raw(parameters, "lowMidHz");
    p.lowMidGainDb = raw(parameters, "lowMidGain");
    p.lowMidQ = raw(parameters, "lowMidQ");
    p.lowMidMode = static_cast<int>(raw(parameters, "lowMidMode") + 0.5f);
    p.highMidHz = raw(parameters, "highMidHz");
    p.highMidGainDb = raw(parameters, "highMidGain");
    p.highMidQ = raw(parameters, "highMidQ");
    p.highMidMode = static_cast<int>(raw(parameters, "highMidMode") + 0.5f);
    p.highHz = raw(parameters, "highHz");
    p.highGainDb = raw(parameters, "highGain");
    p.highQ = raw(parameters, "highQ");
    p.highMode = static_cast<int>(raw(parameters, "highMode") + 0.5f);

    p.compEnabled = raw(parameters, "compOn") >= 0.5f;
    p.compThresholdDb = raw(parameters, "compThreshold");
    p.compRatio = raw(parameters, "compRatio");
    p.compAttackMs = raw(parameters, "compAttack");
    p.compReleaseMs = raw(parameters, "compRelease");
    p.compMakeupDb = raw(parameters, "compMakeup");
    p.compKneeControl = raw(parameters, "compKnee");
    p.compKeyFilterEnabled = raw(parameters, "keyFilterOn") >= 0.5f;
    p.compKeyFilterHz = raw(parameters, "keyFilterHz");
    p.useExternalSidechain = raw(parameters, "externalKey") >= 0.5f;
    return p;
}

void Live32ChannelAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    dsp.setParameters(readDSPParameters());
    dsp.beginMeterBlock();

    auto main = getBusBuffer(buffer, false, 0);
    const int channels = main.getNumChannels();
    const int samples = main.getNumSamples();

    const bool sidechainAvailable = getBusCount(true) > 1 && !getChannelLayoutOfBus(true, 1).isDisabled();
    const float* sidechainLeft = nullptr;
    const float* sidechainRight = nullptr;
    if (sidechainAvailable)
    {
        auto sidechain = getBusBuffer(buffer, true, 1);
        if (sidechain.getNumChannels() > 0)
        {
            sidechainLeft = sidechain.getReadPointer(0);
            sidechainRight = sidechain.getNumChannels() > 1 ? sidechain.getReadPointer(1) : sidechainLeft;
        }
    }

    float* left = main.getWritePointer(0);
    float* right = channels > 1 ? main.getWritePointer(1) : nullptr;

    for (int i = 0; i < samples; ++i)
    {
        float l = left[i];
        float r = right != nullptr ? right[i] : l;

        float keyL = l;
        float keyR = r;
        if (sidechainLeft != nullptr)
        {
            keyL = sidechainLeft[i];
            keyR = sidechainRight[i];
        }

        dsp.processFrame(l, r, keyL, keyR);
        left[i] = l;
        if (right != nullptr)
            right[i] = r;
    }

    const auto m = dsp.meters();
    inputMeter.store(m.inputPeak);
    outputMeter.store(m.outputPeak);
    gainReductionMeter.store(m.compressorGainReductionDb);
    gateClosedMeter.store(m.gateClosed);
}

void Live32ChannelAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void Live32ChannelAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout Live32ChannelAudioProcessor::createParameterLayout()
{
    using ID = juce::ParameterID;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"trim",1}, "Trim", juce::NormalisableRange<float>(-18.f,18.f,0.1f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(ID{"phase",1}, "Phase Invert", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>(ID{"hpfOn",1}, "High Pass", true));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"hpfHz",1}, "HPF Frequency", logRange(20.f,400.f,80.f), 80.f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(ID{"gateOn",1}, "Gate", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"gateThreshold",1}, "Gate Threshold", juce::NormalisableRange<float>(-80.f,0.f,0.5f), -45.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"gateRange",1}, "Gate Range", juce::NormalisableRange<float>(0.f,80.f,1.f), 50.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"gateAttack",1}, "Gate Attack", logRange(0.1f,100.f,5.f), 5.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"gateHold",1}, "Gate Hold", juce::NormalisableRange<float>(0.f,500.f,1.f), 80.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"gateRelease",1}, "Gate Release", logRange(5.f,2000.f,180.f), 180.f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(ID{"eqOn",1}, "EQ", true));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"lowHz",1}, "Low Frequency", logRange(20.f,400.f,100.f), 100.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"lowGain",1}, "Low Gain", juce::NormalisableRange<float>(-15.f,15.f,0.1f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"lowQ",1}, "Low Q", logRange(0.3f,10.f,0.8f), 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ID{"lowMode",1}, "Low Mode", juce::StringArray{"LCUT","LSHV","PEQ"}, 2));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"lowMidHz",1}, "Low Mid Frequency", logRange(80.f,2500.f,500.f), 500.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"lowMidGain",1}, "Low Mid Gain", juce::NormalisableRange<float>(-15.f,15.f,0.1f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"lowMidQ",1}, "Low Mid Q", logRange(0.3f,10.f,1.2f), 1.2f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ID{"lowMidMode",1}, "Low Mid Mode", juce::StringArray{"VEQ","PEQ"}, 1));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"highMidHz",1}, "High Mid Frequency", logRange(300.f,10000.f,2500.f), 2500.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"highMidGain",1}, "High Mid Gain", juce::NormalisableRange<float>(-15.f,15.f,0.1f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"highMidQ",1}, "High Mid Q", logRange(0.3f,10.f,1.2f), 1.2f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ID{"highMidMode",1}, "High Mid Mode", juce::StringArray{"VEQ","PEQ"}, 1));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"highHz",1}, "High Frequency", logRange(1000.f,20000.f,8000.f), 8000.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"highGain",1}, "High Gain", juce::NormalisableRange<float>(-15.f,15.f,0.1f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"highQ",1}, "High Q", logRange(0.3f,10.f,0.8f), 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ID{"highMode",1}, "High Mode", juce::StringArray{"PEQ","HSHV","HCUT"}, 0));

    p.push_back(std::make_unique<juce::AudioParameterBool>(ID{"compOn",1}, "Compressor", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"compThreshold",1}, "Comp Threshold", juce::NormalisableRange<float>(-60.f,0.f,0.5f), -18.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"compRatio",1}, "Comp Ratio", juce::NormalisableRange<float>(1.f,20.f,0.1f), 4.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"compAttack",1}, "Comp Attack", logRange(0.1f,200.f,10.f), 10.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"compRelease",1}, "Comp Release", logRange(5.f,2000.f,120.f), 120.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"compMakeup",1}, "Comp Makeup", juce::NormalisableRange<float>(-12.f,18.f,0.1f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"compKnee",1}, "Comp Knee", juce::NormalisableRange<float>(0.f,5.f,1.f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(ID{"keyFilterOn",1}, "Key Filter", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{"keyFilterHz",1}, "Key Filter Frequency", logRange(20.f,20000.f,120.f), 120.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(ID{"externalKey",1}, "External Sidechain", false));

    return { p.begin(), p.end() };
}

juce::AudioProcessorEditor* Live32ChannelAudioProcessor::createEditor()
{
    return new Live32ChannelAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Live32ChannelAudioProcessor();
}
