#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <juce_dsp/juce_dsp.h>

namespace emu
{
    inline constexpr float AUTHENTIC_INTENSITY   = 0.4f;   // 40%
    inline constexpr float AUTHENTIC_DRIVE       = 0.2f;   // ~3 dB
    inline constexpr float AUTHENTIC_SATURATION  = 0.2f;   // per section tanh
    inline constexpr float MAX_POLE_RADIUS       = 0.9950f; // hardware limit
    inline constexpr float MIN_POLE_RADIUS       = 0.10f;
    inline constexpr double REFERENCE_SR         = 48000.0;

    // Geodesic (log-space) radius interpolation - more "EMU-ish" morphing
    // Set to false to revert to linear radius interpolation
    inline constexpr bool GEODESIC_RADIUS        = true;

    struct PolePair { float r; float theta; };

    struct BiquadSection
    {
        void setCoeffs(float nb0, float nb1, float nb2, float na1, float na2) noexcept
        {
            b0 = nb0; b1 = nb1; b2 = nb2; a1 = na1; a2 = na2;
        }

        void setSaturation(float amt) noexcept { sat = std::clamp(amt, 0.0f, 1.0f); }

        void reset() noexcept { z1 = z2 = 0.0f; }

        inline float process(float x) noexcept
        {
            // Direct Form II Transposed (canonical biquad structure)
            // Efficient: minimal operations, numerically stable for audio
            float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;

            // Per-section saturation (authentic EMU nonlinearity)
            if (sat > 0.0f) {
                const float g = 1.0f + sat * 4.0f;  // 4.0 scaling → soft clipping at ±0.25
                y = std::tanh(y * g);
            }

            // Safety: catch NaN/Inf from extreme coefficients (defense in depth)
            if (!std::isfinite(y)) y = 0.0f;
            return y;
        }

        float z1{0}, z2{0};
        float b0{1}, b1{0}, b2{0}, a1{0}, a2{0};
        float sat{AUTHENTIC_SATURATION};
    };

    template <size_t N>
    struct BiquadCascade
    {
        void reset() noexcept { for (auto& s: sections) s.reset(); }
        inline float process(float x) noexcept
        {
            for (auto& s: sections) x = s.process(x);
            return x;
        }
        std::array<BiquadSection, N> sections;
    };

    inline float wrapAngle(float a) noexcept
    {
        const float pi = juce::MathConstants<float>::pi;
        while (a > pi)  a -= 2.0f * pi;
        while (a < -pi) a += 2.0f * pi;
        return a;
    }

    // Interpolate pole pair at 48k reference (before bilinear remap)
    // Intensity boost is applied AFTER interpolation and remap in updateCoeffsBlock
    inline PolePair interpolatePole(const PolePair& A, const PolePair& B, float t) noexcept
    {
        PolePair result;

        // Radius: geodesic (log-space) or linear interpolation
        if constexpr (GEODESIC_RADIUS) {
            const float lnA = std::log(std::max(1.0e-9f, A.r));
            const float lnB = std::log(std::max(1.0e-9f, B.r));
            result.r = std::exp((1.0f - t) * lnA + t * lnB);
        } else {
            result.r = A.r + t * (B.r - A.r); // linear
        }

        // Angle: shortest path
        float d = wrapAngle(B.theta - A.theta);
        result.theta = A.theta + t * d;

        return result;
    }

    // Bilinear remap: 48k reference pole -> target sample rate
    // This provides proper frequency warping vs simple theta scaling
    inline PolePair remapPole48kToFs(const PolePair& p48k, double targetFs) noexcept
    {
        // Fast path: within ±0.1 Hz of reference — skip complex math
        if (std::abs(targetFs - REFERENCE_SR) < 0.1)
            return p48k;

        // Guard: pathological or unsupported sample rate
        if (targetFs < 1e3)
            return p48k;

        using cd = std::complex<double>;

        const double r48 = std::clamp<double>(p48k.r, 0.0, 0.999999);
        const double th  = static_cast<double>(p48k.theta);
        const cd z48 = std::polar(r48, th);

        // Avoid singularity at z ≈ -1 (rare with valid EMU shapes)
        const cd denom = z48 + cd{1.0, 0.0};
        if (std::abs(denom) < 1e-12)
            return p48k;

        // Inverse bilinear: z@48k -> s (analog domain)
        // s = (2*fs_ref) * (z - 1) / (z + 1)
        const cd s = (2.0 * REFERENCE_SR) * (z48 - cd{1.0, 0.0}) / denom;

        // Forward bilinear: s -> z@target_fs
        // z = (2*fs + s) / (2*fs - s)
        const cd denom_fwd = 2.0 * targetFs - s;
        if (std::abs(denom_fwd) < 1e-12)
            return p48k;  // Return original if transform would be unstable

        const cd z_new = (2.0 * targetFs + s) / denom_fwd;

        // Convert back to polar
        PolePair result;
        result.r  = static_cast<float>(std::min(std::abs(z_new), 0.999999));
        result.theta = static_cast<float>(std::atan2(z_new.imag(), z_new.real()));

        return result;
    }

    inline void poleToBiquad(const PolePair& p, float& a1, float& a2, float& b0, float& b1, float& b2) noexcept
    {
        // Denominator (poles): complex conjugate pair at radius r, angle θ
        // H(z) = N(z) / (1 + a₁z⁻¹ + a₂z⁻²)
        a1 = -2.0f * p.r * std::cos(p.theta);
        a2 = p.r * p.r;

        // Numerator (zeros): placed at 90% of pole radius for resonance control
        // Zeros too close to poles → unstable; too far → weak resonance
        // 0.9 factor empirically matches EMU hardware behavior
        const float rz = std::clamp(0.9f * p.r, 0.0f, 0.999f);
        const float c  = std::cos(p.theta);
        b0 = 1.0f;
        b1 = -2.0f * rz * c;
        b2 = rz * rz;

        // Normalize numerator to prevent gain explosion (sum of |coeffs| as denominator)
        // 0.25 minimum prevents divide-by-zero and extreme gains
        const float norm = 1.0f / std::max(0.25f, std::abs(b0) + std::abs(b1) + std::abs(b2));
        b0 *= norm; b1 *= norm; b2 *= norm;
    }

    template <size_t N>
    inline void loadShape(const std::array<float, N>& shape, std::array<PolePair, N/2>& out) noexcept
    {
        for (size_t i = 0; i < N/2; ++i)
        {
            out[i] = PolePair{ shape[2*i], shape[2*i + 1] };
        }
    }

    struct ZPlaneFilter
    {
        static constexpr int NumSections = 6;

        void prepare(double sampleRate, int /*samplesPerBlock*/)
        {
            sr = sampleRate;
            cascadeL.reset();
            cascadeR.reset();
            morphSmooth.reset(sr, 0.02);
            driveSmooth.reset(sr, 0.01);
            intensitySmooth.reset(sr, 0.02);
            mixSmooth.reset(sr, 0.02);
        }

        void setShapePair(const std::array<float,12>& a, const std::array<float,12>& b) noexcept
        {
            shapeA = a; shapeB = b;
            loadShape(shapeA, polesA);
            loadShape(shapeB, polesB);
        }

        void setMorph(float m) noexcept { morphSmooth.setTargetValue(std::clamp(m, 0.0f, 1.0f)); }
        void setIntensity(float i) noexcept { intensitySmooth.setTargetValue(std::clamp(i, 0.0f, 1.0f)); }
        void setDrive(float d) noexcept { driveSmooth.setTargetValue(std::clamp(d, 0.0f, 1.0f)); }
        void setSectionSaturation(float s) noexcept { for (auto& sct : cascadeL.sections) sct.setSaturation(s);
                                                      for (auto& sct : cascadeR.sections) sct.setSaturation(s); }
        void setMix(float m) noexcept { mixSmooth.setTargetValue(std::clamp(m, 0.0f, 1.0f)); }

        void reset() { cascadeL.reset(); cascadeR.reset(); morphSmooth.setCurrentAndTargetValue(0.5f); }

        // Update coefficients once per block
        void updateCoeffsBlock(int samplesPerBlock)
        {
            // Advance smoothers by block size for per-sample effective stepping
            // (Note: these are only read once per block, but skip() ensures proper settling time)
            morphSmooth.skip(samplesPerBlock);
            intensitySmooth.skip(samplesPerBlock);

            // Fast-path: skip expensive pole computation if parameters are stable
            // Saves ~60-80% of updateCoeffsBlock cost when not morphing
            if (!morphSmooth.isSmoothing() && !intensitySmooth.isSmoothing())
            {
                // Still update cached values for consistency
                lastMorph     = morphSmooth.getCurrentValue();
                lastIntensity = intensitySmooth.getCurrentValue();
                return;  // Coefficients unchanged, skip computation
            }

            lastMorph     = morphSmooth.getCurrentValue();
            lastIntensity = intensitySmooth.getCurrentValue();

            // Intensity boost: scales pole radius (higher → sharper resonance)
            // 0.06 factor empirically calibrated to EMU hardware response curve
            const float intensityBoost = 1.0f + lastIntensity * 0.06f;

            for (int i = 0; i < NumSections; ++i)
            {
                // 1) Interpolate in 48k reference domain (geodesic or linear)
                PolePair p48k = interpolatePole(polesA[i], polesB[i], lastMorph);

                // 2) Bilinear remap from 48k to actual sample rate
                PolePair pm = remapPole48kToFs(p48k, sr);

                // 3) Apply intensity boost and EMU hardware clamp
                pm.r = std::min(pm.r * intensityBoost, MAX_POLE_RADIUS);

                lastInterpPoles[i] = pm;
            }

            for (int ch = 0; ch < 2; ++ch)
            {
                auto& cas = (ch == 0 ? cascadeL : cascadeR);
                for (int i = 0; i < NumSections; ++i)
                {
                    float a1, a2, b0, b1, b2;
                    poleToBiquad(lastInterpPoles[i], a1, a2, b0, b1, b2);
                    cas.sections[(size_t)i].setCoeffs(b0, b1, b2, a1, a2);
                }
            }
        }

        // Get last interpolated poles (for UI visualization)
        const std::array<PolePair, NumSections>& getLastPoles() const noexcept { return lastInterpPoles; }

        // Process block (stereo)
        void process(float* left, float* right, int num)
        {
            for (int n = 0; n < num; ++n)
            {
                // Advance smoothers per-sample for proper 20ms ramps
                const float drive = driveSmooth.getNextValue();
                const float mix   = mixSmooth.getNextValue();

                // Pre-drive gain (1.0 to 5.0 range) → tanh soft clipping
                // 4.0 scaling gives ~12dB boost at max drive setting
                const float driveGain = 1.0f + drive * 4.0f;

                // Capture true dry input BEFORE any processing
                const float inL = left[n];
                const float inR = right[n];

                // Pre-drive (authentic: tanh on input)
                float l = std::tanh(inL * driveGain);
                float r = std::tanh(inR * driveGain);

                float wetL = cascadeL.process(l);
                float wetR = cascadeR.process(r);

                // Mix (equal-power to avoid perceived dips around 50% and preserve tone with nonlinearities)
                // Use TRUE dry signal (inL/inR) not driven signal for authentic bypass tone
                const float wetG = std::sqrt(mix);
                const float dryG = std::sqrt(1.0f - mix);
                left[n]  = wetL * wetG + inL * dryG;
                right[n] = wetR * wetG + inR * dryG;
            }
        }

        double sr { REFERENCE_SR };
        BiquadCascade<NumSections> cascadeL, cascadeR;
        std::array<PolePair, NumSections> polesA{}, polesB{};
        std::array<PolePair, NumSections> lastInterpPoles{};
        std::array<float,12> shapeA{}, shapeB{};
        float lastMorph{0.5f}, lastIntensity{AUTHENTIC_INTENSITY};
        juce::LinearSmoothedValue<float> morphSmooth, driveSmooth, intensitySmooth, mixSmooth;
    };
}
