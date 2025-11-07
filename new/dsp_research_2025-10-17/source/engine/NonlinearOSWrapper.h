#pragma once
#include <juce_dsp/juce_dsp.h>
#include <functional>

namespace EngineDSP
{
// Minimal, self-contained oversampling wrapper for nonlinear stage only.
// Zero-latency IIR halfband OS; filter (IIR cascade) remains at host SR.
class NonlinearOSWrapper
{
public:
    NonlinearOSWrapper() = default;

    void prepare(double sampleRate, int channels, int maxBlockSamples = 8192)
    {
        jassert(channels > 0);
        numChannels = channels;
        maxBlock = maxBlockSamples;
        overs = std::make_unique<juce::dsp::Oversampling<float>>(
            (size_t)channels,
            /*numStages=*/1,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            /*isMaxQuality=*/true);
        overs->initProcessing((size_t)maxBlock);
        fsOS = sampleRate * (1 << overs->getOversamplingFactor());
    }

    void reset()
    {
        if (overs) overs->reset();
    }

    void setEnabled(bool e) noexcept { enabled = e; }
    bool isEnabled() const noexcept { return enabled; }
    double getOversampledRate() const noexcept { return fsOS; }

    // Drive and saturation are provided as function objects operating in-place on a channel buffer.
    // applyDrive(ptr, numSamples, driveLinear)
    // applySaturation(ptr, numSamples, satAmount)
    void process(juce::AudioBuffer<float>& buffer,
                 float driveLinear,
                 float satAmount,
                 const std::function<void(float*, int, float)>& applyDrive,
                 const std::function<void(float*, int, float)>& applySaturation)
    {
        if (!enabled) return;
        if (satAmount <= 1.0e-6f && std::abs(driveLinear - 1.0f) <= 1.0e-6f) return;
        jassert(overs != nullptr);

        juce::dsp::AudioBlock<float> block(buffer);
        auto upBlock = overs->processSamplesUp(block);

        const int n = (int)upBlock.getNumSamples();
        const size_t chN = upBlock.getNumChannels();
        for (size_t ch = 0; ch < chN; ++ch)
        {
            float* x = upBlock.getChannelPointer(ch);
            if (applyDrive)      applyDrive(x, n, driveLinear);
            if (applySaturation) applySaturation(x, n, satAmount);
        }

        overs->processSamplesDown(block);
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> overs;
    int numChannels = 0;
    int maxBlock = 0;
    double fsOS = 0.0;
    bool enabled = true;
};
} // namespace EngineDSP