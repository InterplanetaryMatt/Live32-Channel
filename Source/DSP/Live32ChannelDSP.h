#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <complex>

namespace live32
{
constexpr double kPi = 3.1415926535897932384626433832795;

inline float dbToLin(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

inline float linToDb(float x) noexcept
{
    return 20.0f * std::log10(std::max(x, 1.0e-12f));
}

struct BiquadCoefficients
{
    double b0 { 1.0 }, b1 { 0.0 }, b2 { 0.0 }, a1 { 0.0 }, a2 { 0.0 };
};

class Biquad
{
public:
    void setCoefficients(const BiquadCoefficients& newCoefficients) noexcept { c = newCoefficients; }
    void reset() noexcept { z1 = z2 = 0.0; }

    float process(float x) noexcept
    {
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return static_cast<float>(y);
    }

    const BiquadCoefficients& coefficients() const noexcept { return c; }

private:
    BiquadCoefficients c {};
    double z1 { 0.0 }, z2 { 0.0 };
};

struct FilterMath
{
    static BiquadCoefficients peak(double sampleRate, float freq, float q, float gainDb) noexcept;
    static BiquadCoefficients lowPass(double sampleRate, float freq, float q) noexcept;
    static BiquadCoefficients highPass(double sampleRate, float freq, float q) noexcept;
    static BiquadCoefficients lowShelf(double sampleRate, float freq, float gainDb, float slope) noexcept;
    static BiquadCoefficients highShelf(double sampleRate, float freq, float gainDb, float slope) noexcept;
    static double magnitude(const BiquadCoefficients& c, double sampleRate, double frequency) noexcept;
};

struct Parameters
{
    float trimDb { 0.0f };
    bool phaseInvert { false };
    bool hpfEnabled { true };
    float hpfHz { 80.0f };

    bool gateEnabled { false };
    float gateThresholdDb { -45.0f };
    float gateRangeDb { 50.0f };
    float gateAttackMs { 5.0f };
    float gateHoldMs { 80.0f };
    float gateReleaseMs { 180.0f };

    bool eqEnabled { true };
    float lowHz { 100.0f }, lowGainDb { 0.0f }, lowQ { 0.8f };
    float lowMidHz { 500.0f }, lowMidGainDb { 0.0f }, lowMidQ { 1.2f };
    float highMidHz { 2500.0f }, highMidGainDb { 0.0f }, highMidQ { 1.2f };
    float highHz { 8000.0f }, highGainDb { 0.0f }, highQ { 0.8f };
    int lowMode { 2 };      // 0 LCUT, 1 LSHV, 2 PEQ
    int lowMidMode { 1 };   // 0 VEQ, 1 PEQ
    int highMidMode { 1 };  // 0 VEQ, 1 PEQ
    int highMode { 0 };     // 0 PEQ, 1 HSHV, 2 HCUT

    bool compEnabled { false };
    float compThresholdDb { -18.0f };
    float compRatio { 4.0f };
    float compAttackMs { 10.0f };
    float compReleaseMs { 120.0f };
    float compMakeupDb { 0.0f };
    float compKneeControl { 0.0f }; // Live32 JSFX 0..5; multiplied by 3 dB
    bool compKeyFilterEnabled { false };
    float compKeyFilterHz { 120.0f };
    bool useExternalSidechain { false };
};

struct MeterState
{
    float inputPeak { 0.0f };
    float outputPeak { 0.0f };
    float compressorGainReductionDb { 0.0f };
    float gateClosed { 0.0f };
};

class ChannelDSP
{
public:
    void prepare(double newSampleRate) noexcept;
    void reset() noexcept;
    void setParameters(const Parameters& newParams) noexcept;

    void processFrame(float& left, float& right, float keyLeft, float keyRight) noexcept;

    MeterState meters() const noexcept { return meterState; }
    void beginMeterBlock() noexcept;

private:
    void updateCoefficients() noexcept;

    double sampleRate { 48000.0 };
    Parameters params {};

    float trimLinear { 1.0f };
    float phaseMultiplier { 1.0f };
    float gateThresholdLinear { dbToLin(-45.0f) };
    float gateFloor { dbToLin(-50.0f) };
    float gateAttackCoeff { 0.0f };
    float gateReleaseCoeff { 0.0f };
    float gateHoldSamples { 0.0f };
    float compAttackCoeff { 0.0f };
    float compReleaseCoeff { 0.0f };
    float compKneeDb { 0.0f };
    float keyHpfA { 0.0f };

    std::array<Biquad, 2> hpfStage1 {};
    std::array<Biquad, 2> hpfStage2 {};
    std::array<std::array<Biquad, 4>, 2> eq {};

    float gateGain { 1.0f };
    float compGain { 1.0f };
    float gateEnvelope { 0.0f };
    float compEnvelope { 0.0f };
    float gateHoldCounter { 0.0f };

    float keyHpfPrevX[2] { 0.0f, 0.0f };
    float keyHpfPrevY[2] { 0.0f, 0.0f };

    MeterState meterState {};
};
} // namespace live32
