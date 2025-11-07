#pragma once
#include "IZPlaneEngine.h"
#include "IShapeBank.h"
#include "BiquadCascade.h"
#include "ZPoleMath.h"
#include "NonlinearStage.h"
#include <cstdint>
#include <cmath>
#include <juce_dsp/juce_dsp.h>

class AuthenticEMUEngine final : public IZPlaneEngine
{
public:
    explicit AuthenticEMUEngine (IShapeBank& bank) : shapes (bank) {}

    void prepare (double fs, int blockSize, int /*numChannels*/) override
    {
        fsHost = (float) fs;  fsProc = fsHost;
        morphSm.reset (fsHost, 0.02f);
        intensSm.reset (fsHost, 0.02f);
        driveSm.reset (fsHost, 0.003f);
        satSm.reset   (fsHost, 0.003f);
        morphSm.setCurrentAndTargetValue (params.morph);
        intensSm.setCurrentAndTargetValue (params.intensity);
        driveSm.setCurrentAndTargetValue  (params.driveDb);
        satSm.setCurrentAndTargetValue    (params.sat);
        (void) blockSize;
        reset();
    }

    void reset() override
    {
        left.reset();
        right.reset();
        lfoPhase = 0.0f;
    }

    void setParams (const ZPlaneParams& p) override
    {
        params = p;
    }

    void setProcessingSampleRate (double fs) override { fsProc = (float) fs; }
    void setSectionsActive (int count) { sectionsActive = juce::jlimit(3, 6, count); }

    bool isEffectivelyBypassed() const override
    {
        const float driveLin = std::pow(10.0f, params.driveDb / 20.0f);
        return params.intensity <= 1e-3f
            && std::abs (driveLin - 1.0f) < 1e-6f
            && params.sat <= 1e-6f
            && params.lfoDepth <= 1e-6f;
    }

    // Scalar processing (simple stereo biquad cascade)
    void processLinear (juce::AudioBuffer<float>& wet) override
    {
        updateCoeffsBlock();

        const int numChannels = wet.getNumChannels();
        const int numSamples = wet.getNumSamples();

        // Process left channel
        if (numChannels >= 1)
        {
            auto* data = wet.getWritePointer(0);
            for (int n = 0; n < numSamples; ++n)
            {
                float x = data[n];
                for (int s = 0; s < sectionsActive; ++s)
                    x = left.s[s].tick(x);
                data[n] = x;
            }
        }

        // Process right channel
        if (numChannels >= 2)
        {
            auto* data = wet.getWritePointer(1);
            for (int n = 0; n < numSamples; ++n)
            {
                float x = data[n];
                for (int s = 0; s < sectionsActive; ++s)
                    x = right.s[s].tick(x);
                data[n] = x;
            }
        }
    }

    // Coefficient access for visualization
    const BiquadCascade6& getLeftCascade() const { return left; }
    const BiquadCascade6& getRightCascade() const { return right; }
    const ZPlaneParams& getParams() const { return params; }

    // drive + saturation
    void processNonlinear (juce::AudioBuffer<float>& wet) override
    {
        if (isEffectivelyBypassed()) return;

        driveSm.setTargetValue (params.driveDb);
        satSm.setTargetValue   (params.sat);
        const float driveDb = driveSm.getCurrentValue();
        const float satAmt  = satSm.getCurrentValue();

        const float driveLin = std::pow(10.0f, driveDb / 20.0f);
        for (int ch=0; ch<wet.getNumChannels(); ++ch)
        {
            auto* x = wet.getWritePointer (ch);
            nlin::applyDrive (x, wet.getNumSamples(), driveLin);
            nlin::applySaturation (x, wet.getNumSamples(), satAmt);
        }

        if (params.autoMakeup)
        {
            const float I = intensSm.getCurrentValue();
            const float g = 1.0f / (1.0f + 0.5f * I);
            for (int ch=0; ch<wet.getNumChannels(); ++ch)
                for (int i=0; i<wet.getNumSamples(); ++i)
                    wet.getWritePointer (ch)[i] *= g;
        }
    }

private:
    static constexpr float kMinPoleRadius = 0.10f;
    static constexpr float kMaxPoleRadius = 0.9995f;

    void updateCoeffsBlock()
    {
        if (params.lfoRate > 0.0f)
        {
            const float inc = juce::MathConstants<float>::twoPi * (params.lfoRate / fsProc);
            lfoPhase += inc * 64.0f;
            if (lfoPhase >= juce::MathConstants<float>::twoPi) lfoPhase -= juce::MathConstants<float>::twoPi;
        }
        const float lfo = 0.5f * (1.0f + std::sin (lfoPhase)) * params.lfoDepth;

        morphSm.setTargetValue (std::clamp (params.morph + lfo, 0.0f, 1.0f));
        intensSm.setTargetValue (params.intensity);

        const float rawMorph = morphSm.getCurrentValue();
        const float morph = rawMorph * rawMorph * (3.0f - 2.0f * rawMorph);
        const float I = intensSm.getCurrentValue();

        const auto [iA, iB] = shapes.morphPairIndices (params.morphPair);
        const auto& A = shapes.shape (iA);
        const auto& B = shapes.shape (iB);

        for (int s = 0; s < sectionsActive; ++s)
        {
            const int ri = s * 2, ti = s * 2 + 1;
            const float rA = std::clamp (A[ri], kMinPoleRadius, kMaxPoleRadius);
            const float rB = std::clamp (B[ri], kMinPoleRadius, kMaxPoleRadius);
            const float thA = A[ti], thB = B[ti];

            // Log-space geodesic interpolation
            const auto [r, th] = zpm::interpolatePoleLogSpace (rA, thA, rB, thB, morph);
            const float rI = std::clamp (r * (0.80f + 0.20f * I), kMinPoleRadius, kMaxPoleRadius);

            float thCoupled = th;
            if (params.formantLock && params.pitchRatio > 1e-6f && std::abs(params.pitchRatio - 1.0f) > 1e-3f)
                thCoupled = zpm::wrapAngle(th / params.pitchRatio);

            const auto [rF, thF] = zpm::remapPolar48kToFs(rI, thCoupled, fsProc);

            BiquadCascade6::poleToBandpass (rF, thF, left.s[s]);
            BiquadCascade6::poleToBandpass (rF, thF, right.s[s]);
        }
    }

    IShapeBank& shapes;
    ZPlaneParams params{};
    float fsHost = 48000.0f, fsProc = 48000.0f;
    int sectionsActive = 6;

    // Coefficient storage
    BiquadCascade6 left, right;

    juce::LinearSmoothedValue<float> morphSm, intensSm;
    juce::LinearSmoothedValue<float> driveSm, satSm;
    float lfoPhase = 0.0f;
};
