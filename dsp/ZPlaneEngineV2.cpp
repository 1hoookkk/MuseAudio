#include "ZPlaneEngineV2.hpp"
#include <cmath>

ZPlaneEngineV2::ZPlaneEngineV2()
{
    cascadeL_.reset();
    cascadeR_.reset();
}

void ZPlaneEngineV2::prepare(double sampleRate, int blockSize)
{
    sampleRate_ = sampleRate;
    cascadeL_.reset();
    cascadeR_.reset();
    lfoPhase_ = 0.0f;
    makeupGain_ = 1.0f;

    // Initialize parameter smoothing (20ms ramps)
    morphSmooth_.reset(sampleRate, 0.02);
    intensitySmooth_.reset(sampleRate, 0.02);
    driveSmooth_.reset(sampleRate, 0.02);
    saturationSmooth_.reset(sampleRate, 0.02);

    morphSmooth_.setCurrentAndTargetValue(0.5f);
    intensitySmooth_.setCurrentAndTargetValue(0.4f);
    driveSmooth_.setCurrentAndTargetValue(1.0f);
    saturationSmooth_.setCurrentAndTargetValue(0.2f);

    // Allocate RMS buffers for auto makeup (block size)
    inputBuffer_.resize(blockSize);
    outputBuffer_.resize(blockSize);
    rmsBufferPos_ = 0;
}

void ZPlaneEngineV2::reset()
{
    cascadeL_.reset();
    cascadeR_.reset();
    lfoPhase_ = 0.0f;
}

void ZPlaneEngineV2::setShapePair(int pairIndex, const ZPlaneShapes& shapes)
{
    currentPair_ = juce::jlimit(0, 3, pairIndex);
    const auto& pair = shapes.getPair(currentPair_);

    // Convert flat arrays to PolePair structs
    for (size_t i = 0; i < 6; ++i)
    {
        polesA_[i] = emu::PolePair{ pair.shapeA[i*2], pair.shapeA[i*2 + 1] };
        polesB_[i] = emu::PolePair{ pair.shapeB[i*2], pair.shapeB[i*2 + 1] };
    }

    updateCoefficients();
}

void ZPlaneEngineV2::setMorph(float value)
{
    morphSmooth_.setTargetValue(juce::jlimit(0.0f, 1.0f, value));
}

void ZPlaneEngineV2::setIntensity(float value)
{
    intensitySmooth_.setTargetValue(juce::jlimit(0.0f, 1.0f, value));
}

void ZPlaneEngineV2::setDriveDb(float dB)
{
    const float gain = juce::Decibels::decibelsToGain(juce::jlimit(-12.0f, 12.0f, dB));
    driveSmooth_.setTargetValue(gain);
}

void ZPlaneEngineV2::setSaturation(float value)
{
    saturationSmooth_.setTargetValue(juce::jlimit(0.0f, 1.0f, value));
}

void ZPlaneEngineV2::setLfo(float rateHz, float depth)
{
    lfoRate_ = juce::jlimit(0.02f, 8.0f, rateHz);
    lfoDepth_ = juce::jlimit(0.0f, 1.0f, depth);
}

void ZPlaneEngineV2::setAutoMakeup(bool shouldApply)
{
    autoMakeup_ = shouldApply;
}

void ZPlaneEngineV2::process(float* left, float* right, int numSamples)
{
    const float phaseInc = juce::MathConstants<float>::twoPi * lfoRate_ / static_cast<float>(sampleRate_);

    // Capture input RMS if auto makeup enabled
    if (autoMakeup_)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            inputBuffer_[i] = (left[i] + right[i]) * 0.5f;
        }
    }

    // Setup coefficient interpolation ONCE per block
    if (samplesUntilUpdate_ <= 0)
    {
        // Capture current coefficients as START
        for (size_t i = 0; i < 6; ++i)
        {
            const auto& s = cascadeL_.sections[i];
            coeffsStart_[i] = CoeffSet{ s.b0, s.b1, s.b2, s.a1, s.a2 };
        }

        // Advance smoothers by entire block (not per-sample!)
        morphSmooth_.skip(numSamples);
        intensitySmooth_.skip(numSamples);
        
        // Get smoothed values at BLOCK END for coefficient computation
        const float blockMorph = morphSmooth_.getCurrentValue();
        const float blockIntensity = intensitySmooth_.getCurrentValue();

        // Compute end coefficients with smoothed parameters
        const float intensityBoost = 1.0f + blockIntensity * 0.06f;
        for (size_t i = 0; i < 6; ++i)
        {
            auto p = emu::interpolatePole(polesA_[i], polesB_[i], blockMorph);
            p = emu::remapPole48kToFs(p, sampleRate_);
            p.r = std::min(p.r * intensityBoost, emu::MAX_POLE_RADIUS);

            float a1, a2, b0, b1, b2;
            emu::poleToBiquad(p, a1, a2, b0, b1, b2);
            coeffsEnd_[i] = CoeffSet{ b0, b1, b2, a1, a2 };
        }

        // Compute per-sample deltas
        const float invSamples = 1.0f / static_cast<float>(numSamples);
        for (size_t i = 0; i < 6; ++i)
        {
            coeffsDelta_[i].b0 = (coeffsEnd_[i].b0 - coeffsStart_[i].b0) * invSamples;
            coeffsDelta_[i].b1 = (coeffsEnd_[i].b1 - coeffsStart_[i].b1) * invSamples;
            coeffsDelta_[i].b2 = (coeffsEnd_[i].b2 - coeffsStart_[i].b2) * invSamples;
            coeffsDelta_[i].a1 = (coeffsEnd_[i].a1 - coeffsStart_[i].a1) * invSamples;
            coeffsDelta_[i].a2 = (coeffsEnd_[i].a2 - coeffsStart_[i].a2) * invSamples;
        }

        samplesUntilUpdate_ = numSamples;
    }

    // Drive and saturation smoothing setup (per-sample for nonlinear stages)
    const bool driveIsSmoothing = driveSmooth_.isSmoothing();
    const bool saturationIsSmoothing = saturationSmooth_.isSmoothing();

    // Per-sample processing with coefficient interpolation and LFO
    for (int i = 0; i < numSamples; ++i)
    {
        // Get smoothed parameters (per-sample for nonlinear stages)
        const float currentDrive = driveIsSmoothing ? driveSmooth_.getNextValue() : driveSmooth_.getCurrentValue();
        const float currentSaturation = saturationIsSmoothing ? saturationSmooth_.getNextValue() : saturationSmooth_.getCurrentValue();

        // Per-sample LFO modulation (continuous, not stepped)
        // LFO modulates the base morph but doesn't affect coefficient interpolation
        // (coefficients are already interpolating smoothly via block-rate updates)
        if (lfoDepth_ > 0.0f)
        {
            lfoPhase_ += phaseInc;
            if (lfoPhase_ >= juce::MathConstants<float>::twoPi)
                lfoPhase_ -= juce::MathConstants<float>::twoPi;
        }

        // Track effective morph for UI (includes LFO if active)
        effectiveMorph_ = morphSmooth_.getCurrentValue();
        if (lfoDepth_ > 0.0f)
        {
            const float lfo = std::sin(lfoPhase_);
            effectiveMorph_ = juce::jlimit(0.0f, 1.0f, effectiveMorph_ + lfo * lfoDepth_ * 0.5f);
        }

        // Interpolate coefficients (smooth block-to-block transition)
        for (size_t s = 0; s < 6; ++s)
        {
            auto& current = coeffsStart_[s];
            const auto& delta = coeffsDelta_[s];

            cascadeL_.sections[s].setCoeffs(current.b0, current.b1, current.b2, current.a1, current.a2);
            cascadeR_.sections[s].setCoeffs(current.b0, current.b1, current.b2, current.a1, current.a2);
            
            // Apply smoothed saturation per-sample (prevents zipper in feedback path)
            cascadeL_.sections[s].setSaturation(currentSaturation);
            cascadeR_.sections[s].setSaturation(currentSaturation);

            current.b0 += delta.b0;
            current.b1 += delta.b1;
            current.b2 += delta.b2;
            current.a1 += delta.a1;
            current.a2 += delta.a2;
        }

        --samplesUntilUpdate_;

        // Process audio with smoothed drive
        float L = left[i] * currentDrive;
        float R = right[i] * currentDrive;

        L = std::tanh(L);
        R = std::tanh(R);

        L = cascadeL_.process(L);
        R = cascadeR_.process(R);

        // Apply auto makeup gain if enabled
        if (autoMakeup_)
        {
            L *= makeupGain_;
            R *= makeupGain_;
            outputBuffer_[i] = (L + R) * 0.5f;
        }

        // FINAL SAFETY LIMITER (CRITICAL - prevents ear damage)
        // MUST be last operation before output (after all gain stages)
        // Hard clamp at ±1.0 to prevent DAC overload and hearing damage
        L = std::clamp(L, -1.0f, 1.0f);
        R = std::clamp(R, -1.0f, 1.0f);

        left[i] = L;
        right[i] = R;
    }

    // Compute auto makeup gain for next block
    if (autoMakeup_)
    {
        const float inputRMS = computeRMS(inputBuffer_.data(), numSamples);
        const float outputRMS = computeRMS(outputBuffer_.data(), numSamples);
        computeAutoGain(inputRMS, outputRMS);
    }
}

void ZPlaneEngineV2::updateCoefficients()
{
    const float intensityBoost = 1.0f + intensitySmooth_.getCurrentValue() * 0.06f;

    for (size_t i = 0; i < 6; ++i)
    {
        // Interpolate
        auto p = emu::interpolatePole(polesA_[i], polesB_[i], morphSmooth_.getCurrentValue());

        // Remap for sample rate
        p = emu::remapPole48kToFs(p, sampleRate_);

        // Apply intensity
        p.r = std::min(p.r * intensityBoost, emu::MAX_POLE_RADIUS);

        currentPoles_[i] = p;

        // Convert to biquad coefficients
        float a1, a2, b0, b1, b2;
        emu::poleToBiquad(p, a1, a2, b0, b1, b2);

        // Store as END target for interpolation (prevents zipper noise)
        coeffsEnd_[i] = CoeffSet{ b0, b1, b2, a1, a2 };
    }
}

void ZPlaneEngineV2::getSnapshot(SosSnapshot* dest, size_t count) const
{
    for (size_t i = 0; i < std::min(count, size_t(6)); ++i)
    {
        const auto& s = cascadeL_.sections[i];
        dest[i] = SosSnapshot{ s.b0, s.b1, s.b2, s.a1, s.a2 };
    }
}

float ZPlaneEngineV2::computeRMS(const float* buffer, int numSamples)
{
    if (numSamples == 0) return 0.0f;

    float sumSquares = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        sumSquares += buffer[i] * buffer[i];
    }

    return std::sqrt(sumSquares / static_cast<float>(numSamples));
}

void ZPlaneEngineV2::computeAutoGain(float inputRMS, float outputRMS)
{
    // Simple RMS-based makeup gain with smoothing (ported from legacy ZPlaneEngine)
    if (outputRMS > 0.001f && inputRMS > 0.001f)
    {
        const float targetGain = inputRMS / outputRMS;
        const float clampedGain = juce::jlimit(0.5f, 2.0f, targetGain);

        // Smooth gain changes (1 ms time constant)
        const float alpha = 1.0f - std::exp(-1.0f / (0.001f * static_cast<float>(sampleRate_)));
        makeupGain_ += alpha * (clampedGain - makeupGain_);
    }
}
