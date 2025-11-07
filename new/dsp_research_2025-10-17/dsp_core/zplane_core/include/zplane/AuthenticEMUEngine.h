#pragma once
#include "IZPlaneEngine.h"
#include "IShapeBank.h"
#include "BiquadCascade.h"
#include "ZPoleMath.h"
#include "NonlinearStage.h"
#include "ResourcePool.h"
#include "StateTransform.h"
#include "SimdKernels.h"
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
        left.reset();  right.reset();
        prevLeft.reset();  prevRight.reset();
        simdCascade.reset();
        lfoPhase = 0.0f;
        needsStateTransform = false;
    }

    void setParams (const ZPlaneParams& p) override
    {
        if (p.morphPair != params.morphPair)
        {
            prevLeft = left;
            prevRight = right;
            needsStateTransform = true;
        }
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

    // base-rate linear cascade (SIMD-optimized stereo processing)
    void processLinear (juce::AudioBuffer<float>& wet) override
    {
        updateCoeffsBlock();

        if (wet.getNumChannels() >= 2)
        {
            // SIMD stereo processing
            simdCascade.processBlockStereo(
                wet.getReadPointer(0),
                wet.getReadPointer(1),
                wet.getWritePointer(0),
                wet.getWritePointer(1),
                wet.getNumSamples()
            );
        }
        else if (wet.getNumChannels() == 1)
        {
            // Mono processing
            simdCascade.processBlockMono(
                wet.getReadPointer(0),
                wet.getWritePointer(0),
                wet.getNumSamples()
            );
        }

        // Extract state back to regular cascades for visualization
        simdCascade.extractCoefficients(left, right);
    }

    // Coefficient access for visualization (ResponseCurveView, ZPlanePad)
    const BiquadCascade6& getLeftCascade() const { return left; }
    const BiquadCascade6& getRightCascade() const { return right; }
    const ZPlaneParams& getParams() const { return params; }

    // drive + saturation (+ optional makeup)
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
    static constexpr bool kEnableFixedGrid = true;
    static constexpr float kMinPoleRadius = 0.10f;
    static constexpr float kMaxPoleRadius = 0.9995f;
    static constexpr float kRadiusQuantStep = 1.0f / 16384.0f;
    static constexpr float kThetaQuantStep = juce::MathConstants<float>::twoPi / 8192.0f;
    static constexpr float kRadiusDitherAmount = kRadiusQuantStep * 0.5f;
    static constexpr float kThetaDitherAmount = kThetaQuantStep * 0.5f;
    static constexpr float kStereoSpreadRadians = juce::MathConstants<float>::twoPi / 180000.0f;

    static inline uint64_t splitmix64(uint64_t x) noexcept
    {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }

    static inline float uniformFromSeed(uint64_t seed) noexcept
    {
        const uint64_t mixed = splitmix64(seed);
        constexpr double inv = 1.0 / static_cast<double>(1ull << 53);
        return static_cast<float>(static_cast<double>(mixed >> 11) * inv);
    }

    static inline float tpdfNoise(uint64_t seed) noexcept
    {
        const float a = uniformFromSeed(seed);
        const float b = uniformFromSeed(seed ^ 0xA529F9A0DE2D3B1Full);
        return (a + b) - 1.0f;
    }

    static inline float makeQuantNoise(uint64_t frameSeed, int channel, int section, uint64_t axisTag) noexcept
    {
        uint64_t seed = frameSeed;
        seed ^= static_cast<uint64_t>(channel) * 0x9E3779B185EBCA87ull;
        seed ^= static_cast<uint64_t>(section + 1) * 0xC2B2AE3D27D4EB4Full;
        seed ^= axisTag * 0x165667B19E3779F9ull;
        return tpdfNoise(seed);
    }

    static inline float quantizeValue(float value, float step, float ditherAmp, float noise) noexcept
    {
        return std::round((value + ditherAmp * noise) / step) * step;
    }

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

        const uint64_t frameSeed = ++ditherFrameCounter;
        const float stereoOffset = (I > 1.0e-3f ? kStereoSpreadRadians : 0.0f);

        for (int s = 0; s < sectionsActive; ++s)
        {
            const int ri = s * 2, ti = s * 2 + 1;
            const float rA = std::clamp (A[ri], kMinPoleRadius, kMaxPoleRadius);
            const float rB = std::clamp (B[ri], kMinPoleRadius, kMaxPoleRadius);
            const float thA = A[ti], thB = B[ti];

            // Log-space geodesic interpolation for stable morphing
            const auto [r, th] = zpm::interpolatePoleLogSpace (rA, thA, rB, thB, morph);
            const float rI = std::clamp (r * (0.80f + 0.20f * I), kMinPoleRadius, kMaxPoleRadius);

            float thCoupled = th;
            if (params.formantLock && params.pitchRatio > 1e-6f && std::abs(params.pitchRatio - 1.0f) > 1e-3f)
                thCoupled = zpm::wrapAngle(th / params.pitchRatio);

            const auto [rF, thF] = zpm::remapPolar48kToFs(rI, thCoupled, fsProc);

            float rLeft = rF;
            float rRight = rF;
            float thLeft = thF;
            float thRight = zpm::wrapAngle(thF + stereoOffset);

            if (kEnableFixedGrid)
            {
                const float noiseRL = makeQuantNoise(frameSeed, 0, s, 0ull);
                const float noiseRR = makeQuantNoise(frameSeed, 1, s, 0ull);
                const float noiseTL = makeQuantNoise(frameSeed, 0, s, 1ull);
                const float noiseTR = makeQuantNoise(frameSeed, 1, s, 1ull);

                rLeft  = std::clamp(quantizeValue(rLeft,  kRadiusQuantStep, kRadiusDitherAmount, noiseRL), kMinPoleRadius, kMaxPoleRadius);
                rRight = std::clamp(quantizeValue(rRight, kRadiusQuantStep, kRadiusDitherAmount, noiseRR), kMinPoleRadius, kMaxPoleRadius);
                thLeft = zpm::wrapAngle(quantizeValue(thLeft, kThetaQuantStep, kThetaDitherAmount, noiseTL));
                thRight = zpm::wrapAngle(quantizeValue(thRight, kThetaQuantStep, kThetaDitherAmount, noiseTR));
            }
            else
            {
                rLeft  = std::clamp(rLeft,  kMinPoleRadius, kMaxPoleRadius);
                rRight = std::clamp(rRight, kMinPoleRadius, kMaxPoleRadius);
                thLeft = zpm::wrapAngle(thLeft);
                thRight = zpm::wrapAngle(thRight);
            }

            BiquadCascade6::poleToBandpass (rLeft, thLeft, left.s [s]);
            BiquadCascade6::poleToBandpass (rRight, thRight, right.s[s]);
        }

        // Apply state transform if needed (on preset/morph pair changes)
        if (needsStateTransform)
        {
            st::retargetCascade(prevLeft, left, left);
            st::retargetCascade(prevRight, right, right);
            needsStateTransform = false;
        }

        // Sync coefficients to SIMD cascade
        simdCascade.updateCoefficients(left, right);
    }
    IShapeBank& shapes;
    ZPlaneParams params{};           // defaults are all zeros (null-friendly)
    float fsHost = 48000.0f, fsProc = 48000.0f;
    int sectionsActive = 6;          // 6th order (3 sections) or 12th order (6 sections)

    // Coefficient storage (for visualization and state transform)
    BiquadCascade6 left, right;
    BiquadCascade6 prevLeft, prevRight;  // For state transform

    // SIMD-optimized processing engine
    simd::SIMDBiquadCascade simdCascade;

    juce::LinearSmoothedValue<float> morphSm, intensSm;
    juce::LinearSmoothedValue<float> driveSm, satSm;
    float lfoPhase = 0.0f;
    uint64_t ditherFrameCounter = 0;
    bool needsStateTransform = false;
};
