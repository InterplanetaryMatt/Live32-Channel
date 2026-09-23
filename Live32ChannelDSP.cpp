#include "Live32ChannelDSP.h"

namespace live32
{
namespace
{
inline double clampFrequency(double sampleRate, double frequency) noexcept
{
    return std::min(frequency, sampleRate * 0.45);
}
}

BiquadCoefficients FilterMath::peak(double sampleRate, float freq, float q, float gainDb) noexcept
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w = 2.0 * kPi * clampFrequency(sampleRate, freq) / sampleRate;
    const double alpha = std::sin(w) / (2.0 * std::max<double>(q, 0.05));
    const double cw = std::cos(w);
    const double a0 = 1.0 + alpha / A;
    return {
        (1.0 + alpha * A) / a0,
        (-2.0 * cw) / a0,
        (1.0 - alpha * A) / a0,
        (-2.0 * cw) / a0,
        (1.0 - alpha / A) / a0
    };
}

BiquadCoefficients FilterMath::lowPass(double sampleRate, float freq, float q) noexcept
{
    const double w = 2.0 * kPi * clampFrequency(sampleRate, freq) / sampleRate;
    const double alpha = std::sin(w) / (2.0 * std::max<double>(q, 0.05));
    const double cw = std::cos(w);
    const double a0 = 1.0 + alpha;
    return {
        ((1.0 - cw) / 2.0) / a0,
        (1.0 - cw) / a0,
        ((1.0 - cw) / 2.0) / a0,
        (-2.0 * cw) / a0,
        (1.0 - alpha) / a0
    };
}

BiquadCoefficients FilterMath::highPass(double sampleRate, float freq, float q) noexcept
{
    const double w = 2.0 * kPi * clampFrequency(sampleRate, freq) / sampleRate;
    const double alpha = std::sin(w) / (2.0 * std::max<double>(q, 0.05));
    const double cw = std::cos(w);
    const double a0 = 1.0 + alpha;
    return {
        ((1.0 + cw) / 2.0) / a0,
        (-(1.0 + cw)) / a0,
        ((1.0 + cw) / 2.0) / a0,
        (-2.0 * cw) / a0,
        (1.0 - alpha) / a0
    };
}

BiquadCoefficients FilterMath::lowShelf(double sampleRate, float freq, float gainDb, float slope) noexcept
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w = 2.0 * kPi * clampFrequency(sampleRate, freq) / sampleRate;
    const double cw = std::cos(w);
    const double sw = std::sin(w);
    const double S = std::clamp<double>(slope, 0.1, 2.0);
    const double alpha = sw / 2.0 * std::sqrt(std::max((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0, 0.0));
    const double beta = 2.0 * std::sqrt(A) * alpha;
    const double a0 = (A + 1.0) + (A - 1.0) * cw + beta;
    return {
        A * ((A + 1.0) - (A - 1.0) * cw + beta) / a0,
        2.0 * A * ((A - 1.0) - (A + 1.0) * cw) / a0,
        A * ((A + 1.0) - (A - 1.0) * cw - beta) / a0,
        -2.0 * ((A - 1.0) + (A + 1.0) * cw) / a0,
        ((A + 1.0) + (A - 1.0) * cw - beta) / a0
    };
}

BiquadCoefficients FilterMath::highShelf(double sampleRate, float freq, float gainDb, float slope) noexcept
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w = 2.0 * kPi * clampFrequency(sampleRate, freq) / sampleRate;
    const double cw = std::cos(w);
    const double sw = std::sin(w);
    const double S = std::clamp<double>(slope, 0.1, 2.0);
    const double alpha = sw / 2.0 * std::sqrt(std::max((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0, 0.0));
    const double beta = 2.0 * std::sqrt(A) * alpha;
    const double a0 = (A + 1.0) - (A - 1.0) * cw + beta;
    return {
        A * ((A + 1.0) + (A - 1.0) * cw + beta) / a0,
        -2.0 * A * ((A - 1.0) + (A + 1.0) * cw) / a0,
        A * ((A + 1.0) + (A - 1.0) * cw - beta) / a0,
        2.0 * ((A - 1.0) - (A + 1.0) * cw) / a0,
        ((A + 1.0) - (A - 1.0) * cw - beta) / a0
    };
}

double FilterMath::magnitude(const BiquadCoefficients& c, double sampleRate, double frequency) noexcept
{
    const double w = 2.0 * kPi * frequency / sampleRate;
    const std::complex<double> z1 = std::exp(std::complex<double>(0.0, -w));
    const std::complex<double> z2 = z1 * z1;
    const std::complex<double> numerator = c.b0 + c.b1 * z1 + c.b2 * z2;
    const std::complex<double> denominator = 1.0 + c.a1 * z1 + c.a2 * z2;
    return std::abs(numerator / denominator);
}

void ChannelDSP::prepare(double newSampleRate) noexcept
{
    sampleRate = std::max(newSampleRate, 8000.0);
    reset();
    updateCoefficients();
}

void ChannelDSP::reset() noexcept
{
    for (auto& b : hpfStage1) b.reset();
    for (auto& b : hpfStage2) b.reset();
    for (auto& channel : eq)
        for (auto& b : channel)
            b.reset();

    gateGain = 1.0f;
    compGain = 1.0f;
    gateEnvelope = compEnvelope = 0.0f;
    gateHoldCounter = 0.0f;
    keyHpfPrevX[0] = keyHpfPrevX[1] = 0.0f;
    keyHpfPrevY[0] = keyHpfPrevY[1] = 0.0f;
    meterState = {};
}

void ChannelDSP::setParameters(const Parameters& newParams) noexcept
{
    params = newParams;
    updateCoefficients();
}

void ChannelDSP::updateCoefficients() noexcept
{
    trimLinear = dbToLin(params.trimDb);
    phaseMultiplier = params.phaseInvert ? -1.0f : 1.0f;

    const auto hp1 = FilterMath::highPass(sampleRate, params.hpfHz, 0.5411961001f);
    const auto hp2 = FilterMath::highPass(sampleRate, params.hpfHz, 1.3065629649f);
    for (int ch = 0; ch < 2; ++ch)
    {
        hpfStage1[ch].setCoefficients(hp1);
        hpfStage2[ch].setCoefficients(hp2);
    }

    gateThresholdLinear = dbToLin(params.gateThresholdDb);
    gateFloor = dbToLin(-params.gateRangeDb);
    gateAttackCoeff = std::exp(-1.0f / (std::max(params.gateAttackMs, 0.1f) * 0.001f * static_cast<float>(sampleRate)));
    gateReleaseCoeff = std::exp(-1.0f / (std::max(params.gateReleaseMs, 1.0f) * 0.001f * static_cast<float>(sampleRate)));
    gateHoldSamples = params.gateHoldMs * 0.001f * static_cast<float>(sampleRate);

    BiquadCoefficients low;
    if (params.lowMode == 0) low = FilterMath::highPass(sampleRate, params.lowHz, 0.707f);
    else if (params.lowMode == 1) low = FilterMath::lowShelf(sampleRate, params.lowHz, params.lowGainDb, std::clamp(params.lowQ, 0.2f, 2.0f));
    else low = FilterMath::peak(sampleRate, params.lowHz, params.lowQ, params.lowGainDb);

    const auto lowMid = FilterMath::peak(sampleRate, params.lowMidHz,
        params.lowMidMode == 0 ? std::max(0.45f, params.lowMidQ * 0.55f) : params.lowMidQ,
        params.lowMidGainDb);

    const auto highMid = FilterMath::peak(sampleRate, params.highMidHz,
        params.highMidMode == 0 ? std::max(0.45f, params.highMidQ * 0.55f) : params.highMidQ,
        params.highMidGainDb);

    BiquadCoefficients high;
    if (params.highMode == 0) high = FilterMath::peak(sampleRate, params.highHz, params.highQ, params.highGainDb);
    else if (params.highMode == 1) high = FilterMath::highShelf(sampleRate, params.highHz, params.highGainDb, std::clamp(params.highQ, 0.2f, 2.0f));
    else high = FilterMath::lowPass(sampleRate, params.highHz, 0.707f);

    for (int ch = 0; ch < 2; ++ch)
    {
        eq[ch][0].setCoefficients(low);
        eq[ch][1].setCoefficients(lowMid);
        eq[ch][2].setCoefficients(highMid);
        eq[ch][3].setCoefficients(high);
    }

    compAttackCoeff = std::exp(-1.0f / (std::max(params.compAttackMs, 0.1f) * 0.001f * static_cast<float>(sampleRate)));
    compReleaseCoeff = std::exp(-1.0f / (std::max(params.compReleaseMs, 1.0f) * 0.001f * static_cast<float>(sampleRate)));
    compKneeDb = params.compKneeControl * 3.0f;

    const float rc = 1.0f / (2.0f * static_cast<float>(kPi) * std::max(params.compKeyFilterHz, 20.0f));
    const float dt = 1.0f / static_cast<float>(sampleRate);
    keyHpfA = rc / (rc + dt);
}

void ChannelDSP::beginMeterBlock() noexcept
{
    meterState.inputPeak = 0.0f;
    meterState.outputPeak = 0.0f;
    meterState.compressorGainReductionDb = 0.0f;
    meterState.gateClosed = 0.0f;
}

void ChannelDSP::processFrame(float& left, float& right, float keyLeft, float keyRight) noexcept
{
    meterState.inputPeak = std::max(meterState.inputPeak, std::max(std::abs(left), std::abs(right)));

    left *= trimLinear * phaseMultiplier;
    right *= trimLinear * phaseMultiplier;

    if (params.hpfEnabled)
    {
        left = hpfStage1[0].process(left);
        right = hpfStage1[1].process(right);
        left = hpfStage2[0].process(left);
        right = hpfStage2[1].process(right);
    }

    if (params.gateEnabled)
    {
        const float detector = std::max(std::abs(left), std::abs(right));
        if (detector > gateEnvelope)
            gateEnvelope = gateAttackCoeff * gateEnvelope + (1.0f - gateAttackCoeff) * detector;
        else
            gateEnvelope = gateReleaseCoeff * gateEnvelope + (1.0f - gateReleaseCoeff) * detector;

        float target = gateFloor;
        if (gateEnvelope >= gateThresholdLinear)
        {
            gateHoldCounter = gateHoldSamples;
            target = 1.0f;
        }
        else if (gateHoldCounter > 0.0f)
        {
            gateHoldCounter -= 1.0f;
            target = 1.0f;
        }

        if (target > gateGain)
            gateGain = gateAttackCoeff * gateGain + (1.0f - gateAttackCoeff) * target;
        else
            gateGain = gateReleaseCoeff * gateGain + (1.0f - gateReleaseCoeff) * target;

        left *= gateGain;
        right *= gateGain;
    }
    else
    {
        gateGain = 1.0f;
    }

    const bool gateClosed = params.gateEnabled && gateEnvelope < gateThresholdLinear && gateHoldCounter <= 0.0f;
    meterState.gateClosed = std::max(meterState.gateClosed, gateClosed ? 1.0f : 0.0f);

    if (params.eqEnabled)
    {
        for (int band = 0; band < 4; ++band)
        {
            left = eq[0][band].process(left);
            right = eq[1][band].process(right);
        }
    }

    float reductionDb = 0.0f;
    if (params.compEnabled)
    {
        float detectorLeft = params.useExternalSidechain ? keyLeft : left;
        float detectorRight = params.useExternalSidechain ? keyRight : right;

        if (params.compKeyFilterEnabled)
        {
            const float yL = keyHpfA * (keyHpfPrevY[0] + detectorLeft - keyHpfPrevX[0]);
            const float yR = keyHpfA * (keyHpfPrevY[1] + detectorRight - keyHpfPrevX[1]);
            keyHpfPrevX[0] = detectorLeft; keyHpfPrevX[1] = detectorRight;
            keyHpfPrevY[0] = yL; keyHpfPrevY[1] = yR;
            detectorLeft = yL; detectorRight = yR;
        }

        const float detector = std::max(std::abs(detectorLeft), std::abs(detectorRight));
        if (detector > compEnvelope)
            compEnvelope = compAttackCoeff * compEnvelope + (1.0f - compAttackCoeff) * detector;
        else
            compEnvelope = compReleaseCoeff * compEnvelope + (1.0f - compReleaseCoeff) * detector;

        const float envDb = linToDb(compEnvelope);
        const float xDb = envDb - params.compThresholdDb;
        const float ratio = std::max(params.compRatio, 1.0f);
        const float knee = compKneeDb;
        float redDb = 0.0f;

        if (knee <= 0.0f)
        {
            if (xDb > 0.0f)
                redDb = xDb * (1.0f / ratio - 1.0f);
        }
        else if (xDb <= -knee / 2.0f)
        {
            redDb = 0.0f;
        }
        else if (xDb >= knee / 2.0f)
        {
            redDb = xDb * (1.0f / ratio - 1.0f);
        }
        else
        {
            const float t = xDb + knee / 2.0f;
            redDb = (1.0f / ratio - 1.0f) * t * t / (2.0f * knee);
        }

        const float target = dbToLin(redDb + params.compMakeupDb);
        if (target < compGain)
            compGain = compAttackCoeff * compGain + (1.0f - compAttackCoeff) * target;
        else
            compGain = compReleaseCoeff * compGain + (1.0f - compReleaseCoeff) * target;

        left *= compGain;
        right *= compGain;
        reductionDb = std::max(0.0f, -redDb);
    }
    else
    {
        // Matches the current Live32 JSFX exactly: makeup remains an output trim
        // even with the compressor switched off.
        compGain = dbToLin(params.compMakeupDb);
        left *= compGain;
        right *= compGain;
    }

    meterState.compressorGainReductionDb = std::max(meterState.compressorGainReductionDb, reductionDb);
    meterState.outputPeak = std::max(meterState.outputPeak, std::max(std::abs(left), std::abs(right)));
}
} // namespace live32
