#pragma once
#include "ZPlaneFilter.h"
#include "ZPlaneShapes.hpp"
#include <juce_audio_processors/juce_audio_processors.h>

// Forward declaration to avoid circular dependency
class ZPlaneBodeModel;

/**
 * ZPlaneEngineV2 - NEW wrapper using parallel architecture
 * Coexists with old ZPlaneEngine.h via different name
 */
class ZPlaneEngineV2
{
public:
    struct SosSnapshot { float b0, b1, b2, a1, a2; };

    ZPlaneEngineV2();

    void prepare(double sampleRate, int blockSize);
    void reset();

    void setShapePair(int pairIndex, const ZPlaneShapes& shapes);
    void setMorph(float value);                // 0..1
    void setIntensity(float value);            // 0..1
    void setDriveDb(float dB);                 // -12..+12
    void setSaturation(float value);           // 0..1
    void setLfo(float rateHz, float depth);    // 0.02..8, 0..1
    void setAutoMakeup(bool shouldApply);

    void process(float* left, float* right, int numSamples);

    void getSnapshot(SosSnapshot* dest, size_t count) const;
    float getEffectiveMorph() const { return effectiveMorph_; }

private:
    void updateCoefficients();
    float computeRMS(const float* buffer, int numSamples);
    void computeAutoGain(float inputRMS, float outputRMS);

    emu::BiquadCascade<6> cascadeL_, cascadeR_;
    std::array<emu::PolePair, 6> polesA_, polesB_;
    std::array<emu::PolePair, 6> currentPoles_;

    // Per-sample coefficient interpolation (prevents zipper noise)
    struct CoeffSet { float b0, b1, b2, a1, a2; };
    std::array<CoeffSet, 6> coeffsStart_, coeffsEnd_, coeffsDelta_;
    int samplesUntilUpdate_ { 0 };

    double sampleRate_ { 48000.0 };
    int currentPair_ { -1 };

    // Parameter smoothing (20ms ramps, prevents zipper noise)
    juce::LinearSmoothedValue<float> morphSmooth_, intensitySmooth_, driveSmooth_, saturationSmooth_;
    float lfoRate_ { 0.5f };
    float lfoDepth_ { 0.0f };
    bool autoMakeup_ { true };

    float lfoPhase_ { 0.0f };
    float effectiveMorph_ { 0.5f };
    float makeupGain_ { 1.0f };

    // RMS buffers for auto makeup gain
    std::vector<float> inputBuffer_, outputBuffer_;
    int rmsBufferPos_ { 0 };
};
