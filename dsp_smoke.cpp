#include "../Source/DSP/Live32ChannelDSP.h"
#include <cmath>
#include <iostream>

int main()
{
    live32::ChannelDSP dsp;
    live32::Parameters p;
    p.trimDb = 3.0f;
    p.hpfEnabled = true;
    p.hpfHz = 80.0f;
    p.gateEnabled = true;
    p.gateThresholdDb = -50.0f;
    p.eqEnabled = true;
    p.lowMidGainDb = 3.0f;
    p.compEnabled = true;
    p.compThresholdDb = -18.0f;
    p.compRatio = 4.0f;
    dsp.prepare(48000.0);
    dsp.setParameters(p);

    double sum = 0.0;
    for (int i = 0; i < 48000; ++i)
    {
        float l = 0.25f * std::sin(2.0 * live32::kPi * 1000.0 * i / 48000.0);
        float r = l;
        dsp.processFrame(l, r, l, r);
        if (!std::isfinite(l) || !std::isfinite(r))
            return 2;
        sum += std::abs(l) + std::abs(r);
    }

    const auto m = dsp.meters();
    std::cout << "sum=" << sum << " in=" << m.inputPeak << " out=" << m.outputPeak
              << " gr=" << m.compressorGainReductionDb << " gate=" << m.gateClosed << "\n";

    const auto q1 = live32::FilterMath::highPass(48000.0, 80.0f, 0.5411961001f);
    const auto q2 = live32::FilterMath::highPass(48000.0, 80.0f, 1.3065629649f);
    for (double f : {20.0, 40.0, 80.0, 160.0, 1000.0})
    {
        const double mag = live32::FilterMath::magnitude(q1, 48000.0, f) *
                           live32::FilterMath::magnitude(q2, 48000.0, f);
        const double db = 20.0 * std::log10(std::max(mag, 1.0e-12));
        std::cout << "HPF " << f << " Hz: " << db << " dB\n";
    }
    return sum > 1.0 ? 0 : 3;
}
