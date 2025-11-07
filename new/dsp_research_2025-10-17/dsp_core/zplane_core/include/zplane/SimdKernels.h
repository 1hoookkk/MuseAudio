#pragma once
#include "BiquadCascade.h"
#include <juce_dsp/juce_dsp.h>
#include <immintrin.h> // For AVX intrinsics

namespace simd
{
    // CPU capability detection and function dispatch
    class CPUCapabilities
    {
    public:
        enum class SIMDLevel { Scalar, SSE2, AVX, AVX2, AVX512 };

        static SIMDLevel getSIMDLevel()
        {
            static SIMDLevel level = detectSIMDLevel();
            return level;
        }

    private:
        static SIMDLevel detectSIMDLevel()
        {
            // Check for AVX512
            if (__builtin_cpu_supports("avx512f"))
                return SIMDLevel::AVX512;

            // Check for AVX2
            if (__builtin_cpu_supports("avx2"))
                return SIMDLevel::AVX2;

            // Check for AVX
            if (__builtin_cpu_supports("avx"))
                return SIMDLevel::AVX;

            // Check for SSE2
            if (__builtin_cpu_supports("sse2"))
                return SIMDLevel::SSE2;

            // Fallback to scalar
            return SIMDLevel::Scalar;
        }
    };

    // Structure-of-Arrays layout for SIMD processing
    alignas(32) struct BiquadSOA
    {
        // Coefficients (structure-of-arrays for vectorization)
        float b0[6];
        float b1[6];
        float b2[6];
        float a1[6];
        float a2[6];

        // State variables (structure-of-arrays)
        float z1[6];
        float z2[6];

        // Initialize all coefficients and states to zero
        void reset()
        {
            for (int i = 0; i < 6; ++i)
            {
                b0[i] = b1[i] = b2[i] = a1[i] = a2[i] = 0.0f;
                z1[i] = z2[i] = 0.0f;
            }
        }

        // Convert from AoS (Array-of-Structures) to SoA
        void fromAoS(const BiquadSection sections[6])
        {
            for (int i = 0; i < 6; ++i)
            {
                b0[i] = sections[i].b0;
                b1[i] = sections[i].b1;
                b2[i] = sections[i].b2;
                a1[i] = sections[i].a1;
                a2[i] = sections[i].a2;
                z1[i] = sections[i].z1;
                z2[i] = sections[i].z2;
            }
        }

        // Convert back to AoS
        void toAoS(BiquadSection sections[6]) const
        {
            for (int i = 0; i < 6; ++i)
            {
                sections[i].b0 = b0[i];
                sections[i].b1 = b1[i];
                sections[i].b2 = b2[i];
                sections[i].a1 = a1[i];
                sections[i].a2 = a2[i];
                sections[i].z1 = z1[i];
                sections[i].z2 = z2[i];
            }
        }
    };

    // Scalar implementation (fallback)
    struct ScalarProcessor
    {
        static float processSample(BiquadSOA& soa, float input)
        {
            float x = input;

            for (int i = 0; i < 6; ++i)
            {
                // Transposed Direct Form II
                const float y = soa.b0[i] * x + soa.z1[i];
                soa.z1[i] = soa.b1[i] * x - soa.a1[i] * y + soa.z2[i];
                soa.z2[i] = soa.b2[i] * x - soa.a2[i] * y;

                // Denormal protection
                if (std::abs(soa.z1[i]) < 1e-20f) soa.z1[i] = 0.0f;
                if (std::abs(soa.z2[i]) < 1e-20f) soa.z2[i] = 0.0f;

                x = y;
            }

            return x;
        }

        static void processBlock(BiquadSOA& soa, const float* input, float* output, int numSamples)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                output[n] = processSample(soa, input[n]);
            }
        }
    };

#ifdef __AVX__
    // AVX implementation (8-wide float processing)
    struct AVXProcessor
    {
        static float processSample(BiquadSOA& soa, float input)
        {
            // For single sample processing, AVX doesn't provide much benefit
            // Fall back to scalar for single samples
            return ScalarProcessor::processSample(soa, input);
        }

        static void processBlock(BiquadSOA& soa, const float* input, float* output, int numSamples)
        {
            // Process 8 samples at a time
            const int numSIMD = numSamples & ~7; // Round down to multiple of 8

            for (int n = 0; n < numSIMD; n += 8)
            {
                // Load 8 input samples
                __m256 x = _mm256_loadu_ps(&input[n]);

                // Process all 6 biquad sections
                for (int section = 0; section < 6; ++section)
                {
                    // Load coefficients for this section (broadcast to 8 lanes)
                    const __m256 b0 = _mm256_set1_ps(soa.b0[section]);
                    const __m256 b1 = _mm256_set1_ps(soa.b1[section]);
                    const __m256 b2 = _mm256_set1_ps(soa.b2[section]);
                    const __m256 a1 = _mm256_set1_ps(soa.a1[section]);
                    const __m256 a2 = _mm256_set1_ps(soa.a2[section]);

                    // Load state variables (broadcast current values)
                    const __m256 z1 = _mm256_set1_ps(soa.z1[section]);
                    const __m256 z2 = _mm256_set1_ps(soa.z2[section]);

                    // Compute output: y = b0*x + z1
                    const __m256 y = _mm256_fmadd_ps(b0, x, z1);

                    // Update state: z1 = b1*x - a1*y + z2
                    const __m256 new_z1 = _mm256_fmsub_ps(b1, x, _mm256_fmadd_ps(a1, y, z2));

                    // Update state: z2 = b2*x - a2*y
                    const __m256 new_z2 = _mm256_fmsub_ps(b2, x, _mm256_mul_ps(a2, y));

                    // Store updated state (extract first element since we're processing cascaded)
                    soa.z1[section] = _mm256_cvtss_f32(new_z1);
                    soa.z2[section] = _mm256_cvtss_f32(new_z2);

                    // Input for next section
                    x = y;
                }

                // Store results
                _mm256_storeu_ps(&output[n], x);
            }

            // Handle remaining samples with scalar
            for (int n = numSIMD; n < numSamples; ++n)
            {
                output[n] = ScalarProcessor::processSample(soa, input[n]);
            }
        }
    };
#endif

#ifdef __AVX2__
    // AVX2 implementation with FMA and improved operations
    struct AVX2Processor
    {
        static float processSample(BiquadSOA& soa, float input)
        {
            return ScalarProcessor::processSample(soa, input);
        }

        static void processBlock(BiquadSOA& soa, const float* input, float* output, int numSamples)
        {
            const int numSIMD = numSamples & ~7;

            for (int n = 0; n < numSIMD; n += 8)
            {
                __m256 x = _mm256_loadu_ps(&input[n]);

                for (int section = 0; section < 6; ++section)
                {
                    const __m256 b0 = _mm256_set1_ps(soa.b0[section]);
                    const __m256 b1 = _mm256_set1_ps(soa.b1[section]);
                    const __m256 b2 = _mm256_set1_ps(soa.b2[section]);
                    const __m256 a1 = _mm256_set1_ps(soa.a1[section]);
                    const __m256 a2 = _mm256_set1_ps(soa.a2[section]);

                    const __m256 z1 = _mm256_set1_ps(soa.z1[section]);
                    const __m256 z2 = _mm256_set1_ps(soa.z2[section]);

                    // AVX2 FMA operations
                    const __m256 y = _mm256_fmadd_ps(b0, x, z1);
                    const __m256 new_z1 = _mm256_fmsub_ps(b1, x, _mm256_fmadd_ps(a1, y, z2));
                    const __m256 new_z2 = _mm256_fmsub_ps(b2, x, _mm256_mul_ps(a2, y));

                    soa.z1[section] = _mm256_cvtss_f32(new_z1);
                    soa.z2[section] = _mm256_cvtss_f32(new_z2);

                    x = y;
                }

                _mm256_storeu_ps(&output[n], x);
            }

            // Handle remaining samples
            for (int n = numSIMD; n < numSamples; ++n)
            {
                output[n] = ScalarProcessor::processSample(soa, input[n]);
            }
        }
    };
#endif

    // Stereo SIMD processor (processes both channels)
    template<typename Processor>
    struct StereoProcessor
    {
        static void processBlock(BiquadSOA& left, BiquadSOA& right,
                                 const float* inputL, const float* inputR,
                                 float* outputL, float* outputR,
                                 int numSamples)
        {
            // Process both channels in parallel
            Processor::processBlock(left, inputL, outputL, numSamples);
            Processor::processBlock(right, inputR, outputR, numSamples);
        }
    };

    // Main SIMD-enabled biquad cascade
    class SIMDBiquadCascade
    {
    public:
        SIMDBiquadCascade()
        {
            reset();
            detectAndSetProcessor();
        }

        void reset()
        {
            left.reset();
            right.reset();
        }

        void updateCoefficients(const BiquadCascade6& leftCascade, const BiquadCascade6& rightCascade)
        {
            left.fromAoS(leftCascade.s);
            right.fromAoS(rightCascade.s);
        }

        void extractCoefficients(BiquadCascade6& leftCascade, BiquadCascade6& rightCascade) const
        {
            left.toAoS(leftCascade.s);
            right.toAoS(rightCascade.s);
        }

        float processSampleLeft(float input)
        {
            return processSampleFunc(left, input);
        }

        float processSampleRight(float input)
        {
            return processSampleFunc(right, input);
        }

        void processBlockStereo(const float* inputL, const float* inputR,
                                float* outputL, float* outputR,
                                int numSamples)
        {
            processBlockStereoFunc(left, right, inputL, inputR, outputL, outputR, numSamples);
        }

        void processBlockMono(const float* input, float* output, int numSamples)
        {
            processBlockFunc(left, input, output, numSamples);
        }

        // Get performance metrics
        struct PerformanceMetrics
        {
            float cyclesPerSample;
            CPUCapabilities::SIMDLevel simdLevel;
            bool isOptimized;
        };

        PerformanceMetrics getPerformanceMetrics() const
        {
            return {
                getCyclesPerSample(),
                simdLevel,
                simdLevel != CPUCapabilities::SIMDLevel::Scalar
            };
        }

    private:
        BiquadSOA left, right;
        CPUCapabilities::SIMDLevel simdLevel;

        // Function pointers for dispatch
        float (*processSampleFunc)(BiquadSOA&, float);
        void (*processBlockFunc)(BiquadSOA&, const float*, float*, int);
        void (*processBlockStereoFunc)(BiquadSOA&, BiquadSOA&, const float*, const float*, float*, float*, int);

        void detectAndSetProcessor()
        {
            simdLevel = CPUCapabilities::getSIMDLevel();

            switch (simdLevel)
            {
#ifdef __AVX2__
                case CPUCapabilities::SIMDLevel::AVX2:
                case CPUCapabilities::SIMDLevel::AVX512:
                    processSampleFunc = ScalarProcessor::processSample;
                    processBlockFunc = AVX2Processor::processBlock;
                    processBlockStereoFunc = StereoProcessor<AVX2Processor>::processBlock;
                    break;
#endif
#ifdef __AVX__
                case CPUCapabilities::SIMDLevel::AVX:
                    processSampleFunc = ScalarProcessor::processSample;
                    processBlockFunc = AVXProcessor::processBlock;
                    processBlockStereoFunc = StereoProcessor<AVXProcessor>::processBlock;
                    break;
#endif
                default:
                    processSampleFunc = ScalarProcessor::processSample;
                    processBlockFunc = ScalarProcessor::processBlock;
                    processBlockStereoFunc = StereoProcessor<ScalarProcessor>::processBlock;
                    break;
            }
        }

        float getCyclesPerSample() const
        {
            // Rough estimate based on SIMD level
            switch (simdLevel)
            {
                case CPUCapabilities::SIMDLevel::AVX512: return 2.0f;
                case CPUCapabilities::SIMDLevel::AVX2:   return 3.0f;
                case CPUCapabilities::SIMDLevel::AVX:    return 4.0f;
                case CPUCapabilities::SIMDLevel::SSE2:   return 6.0f;
                default:                                  return 12.0f;
            }
        }
    };

} // namespace simd