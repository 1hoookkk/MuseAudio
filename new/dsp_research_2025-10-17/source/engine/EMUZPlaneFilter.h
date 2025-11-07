#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>
#include <array>

namespace ConsolidatedDSP
{
    /**
     * Professional EMU Z-plane filter implementation with morphing, nonlinear drive,
     * oversampling and coefficient bank support.
     *
     * The class exposes a modern interface that mirrors the consolidated DSP
     * library used throughout EngineAudio projects. Internally it provides a
     * high-quality cascaded biquad network with parameter smoothing.
     */
    class EMUZPlaneFilter
    {
    public:
        enum class FilterType
        {
            Lowpass,
            Highpass,
            Bandpass,
            Notch,
            Peak,
            Allpass,
            VintageEMU,
            Morphing
        };

        enum class FilterModel
        {
            EmuClassic,
            EmuModern,
            ZPlaneMorph,
            AuthenticEmu,
            Hybrid
        };

        struct FilterParameters
        {
            FilterType type = FilterType::VintageEMU;
            FilterModel model = FilterModel::AuthenticEmu;
            float frequency = 1000.0f;
            float resonance = 0.5f;
            float gain = 0.0f;
            float morphPosition = 0.5f;
            float drive = 0.15f;
            float character = 0.5f;
            float quality = 1.0f;
        };

        struct CoefficientBank
        {
            juce::String bankName;
            juce::String description;
            std::vector<FilterParameters> morphTargets;
        };

        struct FilterResponse
        {
            std::vector<float> frequencies;
            std::vector<float> magnitudes;
            std::vector<float> phases;
            float currentFrequency = 0.0f;
            float currentResonance = 0.0f;
            float currentMorph = 0.0f;
        };

        EMUZPlaneFilter();
        ~EMUZPlaneFilter();

        void prepare(double sampleRate, int samplesPerBlock);
        void reset();

        void setFilterType(FilterType type) noexcept;
        void setFilterModel(FilterModel model) noexcept;
        void setFrequency(float frequency) noexcept;
        void setResonance(float resonance) noexcept;
        void setGain(float gain) noexcept;
        void setMorphPosition(float morph) noexcept;
        void setParameters(const FilterParameters& params) noexcept;

        void setDrive(float drive) noexcept;
        void setCharacter(float character) noexcept;
        void setQuality(float quality) noexcept;
        void enableNonlinearStage(bool enable) noexcept;
        void enableOversampling(int factor);

        float processSample(float input) noexcept;
        void processBlock(juce::AudioBuffer<float>& buffer) noexcept;
        void processStereo(float& left, float& right) noexcept;

        bool loadCoefficientBank(const juce::String& bankPath);
        bool loadCoefficientBankFromJson(const void* data, size_t sizeInBytes);
        bool loadCoefficientBankData(const CoefficientBank& bank);
        void setActiveBank(const juce::String& bankName);
        const CoefficientBank* getCurrentBank() const noexcept;

        FilterResponse getFrequencyResponse(int numPoints = 512) const;
        float getCurrentFrequency() const noexcept;
        float getCurrentResonance() const noexcept;
        float getCurrentMorph() const noexcept;

    private:
        struct SmoothParameter
        {
            void setSampleRate(double sampleRate, float timeMs = 20.0f) noexcept;
            void setTarget(float value) noexcept;
            float getNextValue() noexcept;
            void reset(float value) noexcept;

            float current { 0.0f };
            float target { 0.0f };
            float coefficient { 0.01f };
        };

        struct BiquadSection
        {
            void reset() noexcept;
            float process(float input) noexcept;

            float a0 { 1.0f }, a1 { 0.0f }, a2 { 0.0f };
            float b1 { 0.0f }, b2 { 0.0f };
            float x1 { 0.0f }, x2 { 0.0f };
            float y1 { 0.0f }, y2 { 0.0f };
        };

        using BiquadCascade = std::array<BiquadSection, 6>;  // 6 pole pairs for authentic EMU

        void updateFilterState();
        void rebuildFromParameters(const FilterParameters& params);
        void rebuildVintageModel(const FilterParameters& params);
        void rebuildModernModel(const FilterParameters& params);
        void rebuildMorphModel(const FilterParameters& params);
        void updateMorphTargets() noexcept;
        FilterParameters interpolateMorphTargets(float morph) const noexcept;
        float applyNonlinearStage(float input) noexcept;

        static float fastTanh(float x) noexcept;
        bool loadCoefficientBankFromJsonInternal(const juce::String& jsonText,
                                                 const juce::String& sourceLabel,
                                                 const juce::String& suggestedBankName);

        double sampleRate_ { 44100.0 };
        int samplesPerBlock_ { 512 };

        // Sample rate invariance infrastructure
        double refSampleRate_{44100.0};  // Reference SR for EMU coefficient banks
        float remapExp_{1.0f};            // Exponent for z-plane pole remapping

        FilterType filterType_ { FilterType::VintageEMU };
        FilterModel filterModel_ { FilterModel::AuthenticEmu };

        SmoothParameter frequency_;
        SmoothParameter resonance_;
        SmoothParameter gain_;
        SmoothParameter morph_;
        SmoothParameter drive_;
        SmoothParameter character_;

        float quality_ { 1.0f };
        bool nonlinearEnabled_ { true };
        int oversamplingFactor_ { 1 };

        BiquadCascade cascade_;
        FilterParameters effectiveParams_;

        std::vector<CoefficientBank> coefficientBanks_;
        std::atomic<int> currentBankIndex_ { -1 };

        // Cached bank for lock-free audio thread access
        std::atomic<bool> cachedBankValid_ { false };
        CoefficientBank cachedBank_;

        mutable juce::CriticalSection stateLock_; // Only for bank management, NOT audio thread
    };
} // namespace ConsolidatedDSP
