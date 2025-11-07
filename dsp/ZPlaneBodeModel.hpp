#pragma once
#include <array>
#include <complex>
#include <atomic>
#include <juce_dsp/juce_dsp.h>

/**
 * ZPlaneBodeModel - Frequency response computation for UI plotting
 * Thread-safe: audio thread writes snapshots, UI thread computes response
 */
class ZPlaneBodeModel
{
public:
    static constexpr size_t kPoints = 512;
    static constexpr size_t kMaxSections = 6;
    using MagnitudeArray = std::array<float, kPoints>;
    using FrequencyArray = std::array<float, kPoints>;

    struct SosSnapshot { float b0, b1, b2, a1, a2; };

    ZPlaneBodeModel();

    void setFrequencies(float minHz, float maxHz);
    const FrequencyArray& getFrequencies() const { return freqs_; }

    /** Copy SOS snapshot (fast, audio-thread safe) */
    void updateFromSnapshot(const SosSnapshot* sos, size_t count, double sampleRate);

    /** Compute response from snapshot (expensive, call from UI thread) */
    void computeResponseIfNeeded();

    const MagnitudeArray& getMagnitudes() const { return current_; }
    const MagnitudeArray& getShapeA() const { return shapeA_; }
    const MagnitudeArray& getShapeB() const { return shapeB_; }

private:
    void computeResponse(const SosSnapshot* sos, size_t count, double sr, MagnitudeArray& out);

    FrequencyArray freqs_;
    MagnitudeArray current_;
    MagnitudeArray shapeA_;
    MagnitudeArray shapeB_;
    double sampleRate_ { 48000.0 };

    // Lock-free snapshot buffer for audio->UI communication
    std::array<SosSnapshot, kMaxSections> sosSnapshot_;
    std::atomic<size_t> sosCount_ { 0 };
    std::atomic<double> snapshotSampleRate_ { 48000.0 };
    std::atomic<bool> snapshotReady_ { false };
};
