#pragma once
#include <zplane/ZPlaneFilter_fast.h>
#include "../../new/01_EMU_ZPlane/src/emu_extracted/EMUFilter.h"
#include <vector>
#include <variant>
#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/**
 * MuseZPlaneEngine - Unified Z-plane filter interface
 *
 * Abstracts Fast (emu::ZPlaneFilter_fast) and Authentic (AuthenticEMUZPlane)
 * engines behind a single API for seamless mode switching.
 *
 * Design:
 * - Fast mode: thin wrapper over existing validated filter (default)
 * - Authentic mode: dual AuthenticEMUZPlane for stereo (future integration)
 * - Zero-cost abstraction when not switching modes
 * - RT-safe: mode changes handled on message thread before prepare()
 */
class MuseZPlaneEngine
{
public:
    enum class Mode
    {
        Fast,       // emu::ZPlaneFilter_fast (current validated engine)
        Authentic   // AuthenticEMUZPlane (future EMU hardware mode)
    };

    // Pole data structure (normalized across engines)
    struct PoleData
    {
        float r;      // Radius (0-1)
        float theta;  // Angle (radians)
    };

    MuseZPlaneEngine();
    ~MuseZPlaneEngine();

    // Lifecycle (RT-safe after prepare)
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Parameter control (RT-safe)
    void setShapePair(int pairIndex);     // 0=Vowel, 1=Bell, 2=Low, 3=Sub
    void setMorph(float morph);           // 0-1
    void setIntensity(float intensity);   // 0-1
    void setMix(float mix);               // 0-1
    void setDrive(float drive);           // 0-1 normalized
    void setDangerMode(bool enabled);     // optional “danger” voicing

    // Processing (RT-safe)
    void updateCoeffsBlock(int numSamples);
    void process(float* left, float* right, int numSamples);

    // State query (RT-safe read)
    std::vector<PoleData> getLastPoles() const;

    // Mode control (message thread only, call before prepare)
    void setMode(Mode mode);
    Mode getMode() const { return currentMode_; }

    // Performance tuning (RT-safe)
    void setPerformanceMode(emu::PerformanceMode perfMode);
    void setSectionSaturation(float saturation);

private:
    Mode currentMode_ = Mode::Fast;

    // Fast engine adapter (wraps emu::ZPlaneFilter_fast)
        struct FastEngine
    {
        static constexpr size_t oversampleStages = 1;
        static constexpr int oversampleFactor = 1 << oversampleStages;

        struct ChannelState
        {
            float prevSample = 0.0f;
            float feedback = 0.0f;
            float dcBlocker = 0.0f;
            std::array<float, 3> noiseState { 0.0f, 0.0f, 0.0f };
        };

        emu::ZPlaneFilter_fast filter;
        int lastPairIndex = -1;
        float currentIntensity = 0.0f;
        float currentMix = 1.0f;
        juce::AudioBuffer<float> dryBuffer;
        juce::Random rng;
        std::array<ChannelState, 2> channelState {};
        float adaptiveMakeup_ = 1.0f;
        std::atomic<bool> dangerMode { false };

        void prepare(double sampleRate, int samplesPerBlock);
        void reset();
        void setShapePair(int pairIndex);
        void setMorph(float morph);
        void setIntensity(float intensity);
        void setMix(float mix);
        void setDrive(float drive);
        void setDangerMode(bool enabled);
        void setPerformanceMode(emu::PerformanceMode mode);
        void setSectionSaturation(float saturation);
        void updateCoeffsBlock(int numSamples);
        void process(float* left, float* right, int numSamples);
        std::vector<PoleData> getLastPoles() const;

    private:
        void ensureDryBuffer(int numSamples);
        void resetNonlinearState();
        void copyDrySamples(float* left, float* right, int numSamples, bool isMono);
        void applyAdaptiveGain(float* left, float* right, int numSamples, bool isMono);
    };

    // Authentic engine adapter (dual AuthenticEMUZPlane for stereo)
    struct AuthenticEngine
    {
        AuthenticEMUZPlane filterL;
        AuthenticEMUZPlane filterR;

        // External dry/wet mix (AuthenticEMUZPlane is 100% wet)
        float mixAmount = 1.0f;
        juce::AudioBuffer<float> dryBuffer;
        int lastPairIndex = -1;
        float currentIntensity = 0.0f;

        void prepare(double sampleRate, int samplesPerBlock);
        void reset();
        void setShapePair(int pairIndex);
        void setMorph(float morph);
        void setIntensity(float intensity);
        void setMix(float mix);
        void setDrive(float drive);
        void setDangerMode(bool /*enabled*/) {}
        void setPerformanceMode(emu::PerformanceMode mode);
        void setSectionSaturation(float saturation);
        void updateCoeffsBlock(int numSamples);
        void process(float* left, float* right, int numSamples);
        std::vector<PoleData> getLastPoles() const;

        // Internal makeup gain (matches FastEngine)
        inline float calculateMakeupGain(float intensity) const noexcept
        {
            return 1.0f - (intensity * 0.3f);  // Linear taper 1.0 → 0.7
        }
    };

    std::variant<FastEngine, AuthenticEngine> engine_;
};





