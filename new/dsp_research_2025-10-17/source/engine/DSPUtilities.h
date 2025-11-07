#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>
#include <array>
#include <memory>
#include <cmath>
#include <immintrin.h>

namespace ConsolidatedDSP {

/**
 * @brief Consolidated DSP utilities helper functions
 * 
 * This namespace contains utility functions and helper classes that are
 * used across all DSP components in the consolidated library.
 */

namespace MathUtils
{
    // Mathematical constants
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = PI * 0.5f;
    constexpr float LOG_2 = 0.69314718055994530942f;
    constexpr float LOG_10 = 2.30258509299404568402f;
    constexpr float SQRT_2 = 1.41421356237309504880f;
    constexpr float SQRT_2_INV = 1.0f / SQRT_2;
    
    // Fast mathematical functions
    inline float fastSin(float x) noexcept
    {
        // Fast sine approximation using polynomial
        x = x - TWO_PI * std::floor(x / TWO_PI);
        if (x > PI) x -= TWO_PI;
        if (x < -PI) x += TWO_PI;
        
        const float x2 = x * x;
        const float x3 = x2 * x;
        const float x5 = x3 * x2;
        
        return x * (1.0f - x2 * (0.16666667f - x2 * (0.00833333f - x2 * 0.00019841f)));
    }
    
    inline float fastCos(float x) noexcept
    {
        return fastSin(x + HALF_PI);
    }
    
    inline float fastExp(float x) noexcept
    {
        // Fast exponential approximation
        if (x < -87.0f) return 0.0f;
        if (x > 88.0f) return std::numeric_limits<float>::infinity();
        
        // Use bit manipulation for fast exp2, then convert to exp
        union { float f; int32_t i; } v;
        v.i = (int32_t)(12102203.0f * x) + 1064866808;
        return v.f;
    }
    
    inline float fastLog(float x) noexcept
    {
        if (x <= 0.0f) return -std::numeric_limits<float>::infinity();
        
        // Use bit manipulation for fast log2, then convert to log
        union { float f; int32_t i; } v;
        v.f = x;
        return (v.i * 8.2629582881927490e-8f) - 87.989971088f;
    }
    
    inline float fastPow(float base, float exponent) noexcept
    {
        if (base <= 0.0f) return 0.0f;
        return fastExp(exponent * fastLog(base));
    }
    
    // Interpolation functions
    inline float linearInterpolate(float a, float b, float t) noexcept
    {
        return a + t * (b - a);
    }
    
    inline float cubicInterpolate(float y0, float y1, float y2, float y3, float t) noexcept
    {
        const float t2 = t * t;
        const float t3 = t2 * t;
        
        const float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
        const float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c = -0.5f * y0 + 0.5f * y2;
        const float d = y1;
        
        return a * t3 + b * t2 + c * t + d;
    }
    
    inline float hermiteInterpolate(float y0, float y1, float y2, float y3, float t) noexcept
    {
        const float t2 = t * t;
        const float t3 = t2 * t;
        
        const float a0 = y3 - y2 - y0 + y1;
        const float a1 = y0 - y1 - a0;
        const float a2 = y2 - y0;
        const float a3 = y1;
        
        return a0 * t3 + a1 * t2 + a2 * t + a3;
    }
    
    // Window functions
    inline float hannWindow(float n, float N) noexcept
    {
        return 0.5f * (1.0f - fastCos(TWO_PI * n / (N - 1.0f)));
    }
    
    inline float hammingWindow(float n, float N) noexcept
    {
        return 0.54f - 0.46f * fastCos(TWO_PI * n / (N - 1.0f));
    }
    
    inline float blackmanWindow(float n, float N) noexcept
    {
        const float phase = TWO_PI * n / (N - 1.0f);
        return 0.42f - 0.5f * fastCos(phase) + 0.08f * fastCos(2.0f * phase);
    }
    
    inline float kaiserWindow(float n, float N, float beta = 6.0f) noexcept
    {
        const float alpha = (N - 1.0f) * 0.5f;
        const float x = (n - alpha) / alpha;
        return fastExp(beta * std::sqrt(1.0f - x * x));
    }
    
    // Frequency conversion functions
    inline float midiToFreq(float midiNote) noexcept
    {
        return 440.0f * fastPow(2.0f, (midiNote - 69.0f) * (1.0f / 12.0f));
    }
    
    inline float freqToMidi(float frequency) noexcept
    {
        return 69.0f + 12.0f * fastLog(frequency / 440.0f) / LOG_2;
    }
    
    inline float semitonesToRatio(float semitones) noexcept
    {
        return fastPow(2.0f, semitones * (1.0f / 12.0f));
    }
    
    inline float ratioToSemitones(float ratio) noexcept
    {
        return 12.0f * fastLog(ratio) / LOG_2;
    }
    
    // Decibel conversion
    inline float dBToLinear(float dB) noexcept
    {
        return fastPow(10.0f, dB * 0.05f);
    }
    
    inline float linearTodB(float linear) noexcept
    {
        return linear > 0.0f ? 20.0f * fastLog10(linear) : -std::numeric_limits<float>::infinity();
    }
    
    // Clipping and limiting
    inline float softClip(float x, float threshold = 1.0f) noexcept
    {
        const float absX = std::abs(x);
        if (absX <= threshold) return x;
        
        const float sign = x < 0.0f ? -1.0f : 1.0f;
        return sign * threshold * (1.0f - fastExp(-absX / threshold));
    }
    
    inline float hardClip(float x, float min = -1.0f, float max = 1.0f) noexcept
    {
        return juce::jlimit(min, max, x);
    }
    
    inline float tanhClip(float x) noexcept
    {
        return std::tanh(x);
    }
    
    // Random number generation
    class FastRandom
    {
    public:
        FastRandom(uint32_t seed = 0) : state(seed ? seed : 1)
        {
            if (state == 0) state = 1;
        }
        
        float nextFloat() noexcept
        {
            state = state * 1103515245 + 12345;
            return (state & 0x7fffffff) * (1.0f / 2147483647.0f);
        }
        
        float nextFloat(float min, float max) noexcept
        {
            return min + (max - min) * nextFloat();
        }
        
        int nextInt(int min, int max) noexcept
        {
            return min + (nextFloat() * (max - min + 1));
        }
        
        float nextGaussian() noexcept
        {
            if (hasSpare)
            {
                hasSpare = false;
                return spare;
            }
            
            float u, v, s;
            do
            {
                u = 2.0f * nextFloat() - 1.0f;
                v = 2.0f * nextFloat() - 1.0f;
                s = u * u + v * v;
            } while (s >= 1.0f || s == 0.0f);
            
            s = std::sqrt(-2.0f * fastLog(s) / s);
            spare = v * s;
            hasSpare = true;
            return u * s;
        }
        
    private:
        uint32_t state;
        float spare = 0.0f;
        bool hasSpare = false;
    };
}

namespace AudioUtils
{
    // Buffer operations
    void clearBuffer(juce::AudioBuffer<float>& buffer) noexcept;
    void copyBuffer(const juce::AudioBuffer<float>& source, juce::AudioBuffer<float>& destination) noexcept;
    void addBuffer(const juce::AudioBuffer<float>& source, juce::AudioBuffer<float>& destination, float gain = 1.0f) noexcept;
    void multiplyBuffer(juce::AudioBuffer<float>& buffer, float gain) noexcept;
    
    // Channel operations
    void mixChannels(juce::AudioBuffer<float>& buffer) noexcept;
    void extractChannel(const juce::AudioBuffer<float>& source, int channel, std::vector<float>& output) noexcept;
    void insertChannel(const std::vector<float>& input, int channel, juce::AudioBuffer<float>& output) noexcept;
    
    // Sample operations
    void applyGain(juce::AudioBuffer<float>& buffer, float gain) noexcept;
    void applyGainRamp(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, float startGain, float endGain) noexcept;
    void applyFadeIn(juce::AudioBuffer<float>& buffer, int fadeLength) noexcept;
    void applyFadeOut(juce::AudioBuffer<float>& buffer, int fadeLength) noexcept;
    
    // Buffer analysis
    float calculateRMS(const juce::AudioBuffer<float>& buffer) noexcept;
    float calculatePeak(const juce::AudioBuffer<float>& buffer) noexcept;
    float calculateCrestFactor(const juce::AudioBuffer<float>& buffer) noexcept;
    float calculateZeroCrossingRate(const juce::AudioBuffer<float>& buffer) noexcept;
    std::vector<float> calculateHistogram(const juce::AudioBuffer<float>& buffer, int bins = 256) noexcept;
    
    // Resampling
    std::vector<float> resample_LINEAR(const std::vector<float>& input, double inputRate, double outputRate) noexcept;
    void resample_LINEAR(const float* input, int inputLength, float* output, int outputLength, 
                        double inputRate, double outputRate) noexcept;
    std::vector<float> resample_CUBIC(const std::vector<float>& input, double inputRate, double outputRate) noexcept;
    void resample_SINC(const float* input, int inputLength, float* output, int outputLength,
                       double inputRate, double outputRate, int kernelSize = 64) noexcept;
    
    // Convolution
    void convolve(const float* input, int inputLength, const float* impulse, int impulseLength, float* output, int outputLength) noexcept;
    void convolveFFT(const std::vector<float>& input, const std::vector<float>& impulse, std::vector<float>& output) noexcept;
    void convolvePartitioned(const float* input, int inputLength, const float* impulse, int impulseLength, 
                             float* output, int outputLength, int partitionSize = 1024) noexcept;
    
    // FFT utilities
    void realFFT(const std::vector<float>& input, std::vector<std::complex<float>>& output) noexcept;
    void realIFFT(const std::vector<std::complex<float>>& input, std::vector<float>& output) noexcept;
    void calculateMagnitudeSpectrum(const std::vector<std::complex<float>>& fftData, std::vector<float>& magnitudes) noexcept;
    void calculatePhaseSpectrum(const std::vector<std::complex<float>>& fftData, std::vector<float>& phases) noexcept;
    void calculatePowerSpectrum(const std::vector<std::complex<float>>& fftData, std::vector<float>& power) noexcept;
    
    // Filtering utilities
    void applyBiquadFilter(juce::AudioBuffer<float>& buffer, float b0, float b1, float b2, float a1, float a2) noexcept;
    void applyOnePoleLowpass(juce::AudioBuffer<float>& buffer, float cutoff, float sampleRate) noexcept;
    void applyOnePoleHighpass(juce::AudioBuffer<float>& buffer, float cutoff, float sampleRate) noexcept;
    void applySimpleMovingAverage(juce::AudioBuffer<float>& buffer, int windowSize) noexcept;
    
    // Denormal handling
    inline float flushDenormal(float x) noexcept
    {
        const float absX = std::abs(x);
        return (absX < 1e-10f) ? 0.0f : x;
    }
    
    inline void flushDenormals(juce::AudioBuffer<float>& buffer) noexcept
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                channelData[sample] = flushDenormal(channelData[sample]);
            }
        }
    }
}

namespace SIMDUtils
{
    // SIMD detection and initialization
    bool hasSSE2() noexcept;
    bool hasSSE4_1() noexcept;
    bool hasAVX() noexcept;
    bool hasAVX2() noexcept;
    bool hasFMA() noexcept;
    
    // SIMD optimized operations
    void multiplyAddSIMD(const float* a, const float* b, float* result, int numSamples) noexcept;
    void multiplySIMD(const float* a, float scalar, float* result, int numSamples) noexcept;
    void addSIMD(const float* a, const float* b, float* result, int numSamples) noexcept;
    void copySIMD(const float* source, float* destination, int numSamples) noexcept;
    void clearSIMD(float* buffer, int numSamples) noexcept;
    
    // SIMD optimized filtering
    void biquadFilterSIMD(const float* input, float* output, int numSamples,
                         float b0, float b1, float b2, float a1, float a2,
                         std::array<float, 4>& state) noexcept;
    
    // SIMD optimized windowing
    void applyHannWindowSIMD(float* data, int numSamples) noexcept;
    void applyHammingWindowSIMD(float* data, int numSamples) noexcept;
    void applyBlackmanWindowSIMD(float* data, int numSamples) noexcept;
    
    // SIMD optimized interpolation
    void linearInterpolateSIMD(const float* input, float* output, int inputLength, int outputLength) noexcept;
    void cubicInterpolateSIMD(const float* input, float* output, int inputLength, int outputLength) noexcept;
}

namespace PerformanceUtils
{
    // High-resolution timing
    class HighResolutionTimer
    {
    public:
        void start() noexcept;
        double getElapsedSeconds() const noexcept;
        double getElapsedMilliseconds() const noexcept;
        double getElapsedMicroseconds() const noexcept;
        void reset() noexcept;
        
    private:
        std::chrono::high_resolution_clock::time_point startTime;
    };
    
    // CPU usage monitoring
    class CPUMonitor
    {
    public:
        CPUMonitor();
        void startMeasurement() noexcept;
        void endMeasurement() noexcept;
        double getCurrentCPUUsage() const noexcept;
        double getAverageCPUUsage() const noexcept;
        void reset() noexcept;
        
    private:
        std::chrono::high_resolution_clock::time_point measurementStart;
        std::vector<double> cpuUsages;
        double currentUsage = 0.0f;
        mutable std::mutex mutex;
    };
    
    // Memory usage monitoring
    class MemoryMonitor
    {
    public:
        size_t getCurrentMemoryUsage() const noexcept;
        size_t getPeakMemoryUsage() const noexcept;
        double getMemoryUsagePercentage() const noexcept;
        void reset() noexcept;
        
    private:
        size_t peakUsage = 0;
        mutable std::mutex mutex;
    };
    
    // Performance profiler
    class PerformanceProfiler
    {
    public:
        struct ProfileEntry
        {
            juce::String name;
           double totalTime = 0.0;
            int callCount = 0;
            double minTime = std::numeric_limits<double>::infinity();
            double maxTime = 0.0;
        };
        
        void beginProfile(const juce::String& name) noexcept;
        void endProfile(const juce::String& name) noexcept;
        std::vector<ProfileEntry> getProfileData() const noexcept;
        void reset() noexcept;
        void printProfileData() const noexcept;
        
    private:
        std::unordered_map<juce::String, ProfileEntry> profiles;
        std::unordered_map<juce::String, HighResolutionTimer> timers;
        mutable std::mutex mutex;
    };
}

namespace ValidationUtils
{
    // Audio buffer validation
    bool isValidAudioBuffer(const juce::AudioBuffer<float>& buffer) noexcept;
    bool hasNaNs(const juce::AudioBuffer<float>& buffer) noexcept;
    bool hasInfinities(const juce::AudioBuffer<float>& buffer) noexcept;
    bool isSilent(const juce::AudioBuffer<float>& buffer, float threshold = 1e-6f) noexcept;
    
    // Parameter validation
    bool isValidFrequency(float frequency, float sampleRate = 44100.0f) noexcept;
    bool isValidQFactor(float q) noexcept;
    bool isValidGain(float gain) noexcept;
    bool isValidPan(float pan) noexcept;
    bool isValidPhase(float phase) noexcept;
    
    // Coefficient validation
    bool areStableBiquadCoefficients(float b0, float b1, float b2, float a1, float a2) noexcept;
    bool areFiniteCoefficients(const std::vector<float>& coefficients) noexcept;
    float calculateFilterStabilityMargin(float b0, float b1, float b2, float a1, float a2) noexcept;
    
    // Test signal generation
    std::vector<float> generateSineWave(float frequency, float duration, float sampleRate, float amplitude = 1.0f) noexcept;
    std::vector<float> generateSquareWave(float frequency, float duration, float sampleRate, float amplitude = 1.0f) noexcept;
    std::vector<float> generateSawtoothWave(float frequency, float duration, float sampleRate, float amplitude = 1.0f) noexcept;
    std::vector<float> generateTriangleWave(float frequency, float duration, float sampleRate, float amplitude = 1.0f) noexcept;
    std::vector<float> generateWhiteNoise(float duration, float sampleRate, float amplitude = 1.0f) noexcept;
    std::vector<float> generatePinkNoise(float duration, float sampleRate, float amplitude = 1.0f) noexcept;
    
    // Signal analysis
    float calculateTHD(const std::vector<float>& signal, float fundamentalFreq, float sampleRate) noexcept;
    float calculateSNR(const std::vector<float>& signal, float sampleRate) noexcept;
    float calculateDynamicRange(const std::vector<float>& signal) noexcept;
    std::vector<float> calculateFFT(const std::vector<float>& signal) noexcept;
}

} // namespace ConsolidatedDSP
