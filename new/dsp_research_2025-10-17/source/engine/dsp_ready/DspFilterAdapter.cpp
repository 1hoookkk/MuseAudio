#include "DspFilterAdapter.h"

#if FIELD_USE_DSP_PLUGIN_READY

#include <algorithm>

namespace DspReady
{

DspFilterAdapter::~DspFilterAdapter()
{
    if (filter_ != nullptr)
    {
        dsp_filter_destroy(filter_);
        filter_ = nullptr;
    }
}

void DspFilterAdapter::prepare(double sampleRate, int samplesPerBlock)
{
    // Destroy old filter if it exists
    if (filter_ != nullptr)
    {
        dsp_filter_destroy(filter_);
        filter_ = nullptr;
    }

    // Create new filter at the given sample rate
    sampleRate_ = static_cast<float>(sampleRate);
    filter_ = dsp_filter_create(sampleRate_);

    // Set a reasonable smoothing rate (1ms)
    if (filter_ != nullptr)
        dsp_filter_set_smoothing(filter_, 0.001f);
}

void DspFilterAdapter::reset()
{
    if (filter_ != nullptr)
        dsp_filter_reset(filter_);
}

float DspFilterAdapter::processSample(float sample)
{
    if (filter_ == nullptr)
        return sample;

    // Use single-sample buffer for processing
    sampleBuffer_[0] = sample;
    dsp_filter_process(filter_, sampleBuffer_, 1);
    return sampleBuffer_[0];
}

void DspFilterAdapter::setFrequency(float hz)
{
    if (filter_ != nullptr)
    {
        // Clamp to valid range (20Hz - 20kHz)
        float clamped = std::clamp(hz, 20.0f, 20000.0f);
        dsp_filter_set_frequency(filter_, clamped);
    }
}

void DspFilterAdapter::setResonance(float r)
{
    if (filter_ != nullptr)
    {
        // Clamp to valid range (0.0 - 1.0)
        float clamped = std::clamp(r, 0.0f, 1.0f);
        dsp_filter_set_resonance(filter_, clamped);
    }
}

} // namespace DspReady

#endif // FIELD_USE_DSP_PLUGIN_READY
