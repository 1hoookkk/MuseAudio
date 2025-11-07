#include "ZPlaneBodeModel.hpp"
#include <cmath>

ZPlaneBodeModel::ZPlaneBodeModel()
{
    setFrequencies(20.0f, 20000.0f);
    current_.fill(0.0f);
    shapeA_.fill(0.0f);
    shapeB_.fill(0.0f);

    // Initialize snapshot buffer
    for (auto& s : sosSnapshot_)
        s = SosSnapshot{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
}

void ZPlaneBodeModel::setFrequencies(float minHz, float maxHz)
{
    const float logMin = std::log10(minHz);
    const float logMax = std::log10(maxHz);

    for (size_t i = 0; i < kPoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(kPoints - 1);
        const float logFreq = logMin + t * (logMax - logMin);
        freqs_[i] = std::pow(10.0f, logFreq);
    }
}

void ZPlaneBodeModel::updateFromSnapshot(const SosSnapshot* sos, size_t count, double sampleRate)
{
    // Fast copy to lock-free buffer (audio-thread safe)
    const size_t numSections = std::min(count, kMaxSections);
    for (size_t i = 0; i < numSections; ++i)
        sosSnapshot_[i] = sos[i];

    sosCount_.store(numSections, std::memory_order_release);
    snapshotSampleRate_.store(sampleRate, std::memory_order_release);
    snapshotReady_.store(true, std::memory_order_release);
}

void ZPlaneBodeModel::computeResponseIfNeeded()
{
    // Check if new snapshot is ready (UI thread only)
    if (!snapshotReady_.load(std::memory_order_acquire))
        return;

    // Read snapshot data
    const size_t count = sosCount_.load(std::memory_order_acquire);
    const double sr = snapshotSampleRate_.load(std::memory_order_acquire);

    // Compute expensive response
    computeResponse(sosSnapshot_.data(), count, sr, current_);

    // Clear flag
    snapshotReady_.store(false, std::memory_order_release);
}

void ZPlaneBodeModel::computeResponse(const SosSnapshot* sos, size_t count, double sr, MagnitudeArray& out)
{
    using cd = std::complex<double>;
    const double pi = juce::MathConstants<double>::pi;

    for (size_t i = 0; i < kPoints; ++i)
    {
        const double freq = freqs_[i];
        const double omega = 2.0 * pi * freq / sr;

        cd H(1.0, 0.0);

        // Cascade all biquad sections
        for (size_t s = 0; s < count; ++s)
        {
            const auto& section = sos[s];

            const cd ejw(std::cos(omega), std::sin(omega));
            const cd ejw2(std::cos(2.0 * omega), std::sin(2.0 * omega));

            const cd num = cd(section.b0) + cd(section.b1) / ejw + cd(section.b2) / ejw2;
            const cd den = cd(1.0) + cd(section.a1) / ejw + cd(section.a2) / ejw2;

            H *= (num / den);
        }

        const float mag = static_cast<float>(std::abs(H));
        out[i] = juce::Decibels::gainToDecibels(mag, -60.0f);
    }
}
