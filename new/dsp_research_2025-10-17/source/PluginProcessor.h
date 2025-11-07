#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#if FIELD_USE_DSP_PLUGIN_READY
#include "engine/dsp_ready/DspFilterAdapter.h"
#elif FIELD_ENABLE_RUST_EMU
#include "engine/rust/RustEmuFilter.h"
#else
#include "engine/EMUZPlaneFilter.h"
#endif

#if FIELD_USE_DSP_PLUGIN_READY
using EmuFilterImpl = DspReady::DspFilterAdapter;
#elif FIELD_ENABLE_RUST_EMU
using EmuFilterImpl = RustDSP::RustEmuFilter;
#else
using EmuFilterImpl = ConsolidatedDSP::EMUZPlaneFilter;
#endif

//==============================================================================
// Field Plugin Processor - handles audio processing and parameter management
class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "Field"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter access
    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }
    float getImpactValue() const noexcept { return impactValueAtomic.load(std::memory_order_relaxed); }

    // Telemetry access for UI meter
    float getDeltaTiltDb() const noexcept { return deltaTiltDbAtomic.load(std::memory_order_relaxed); }
    float getDeltaRmsDb() const noexcept { return deltaRmsDbAtomic.load(std::memory_order_relaxed); }
    float getInputRmsDb() const noexcept { return inputRmsDbAtomic.load(std::memory_order_relaxed); }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> impactValueAtomic { 0.0f };

    // Parameter IDs
    static constexpr const char* bypassID = "bypass";
    static constexpr const char* characterID = "character";
    static constexpr const char* outputID = "output";

    // DSP Engine
    EmuFilterImpl leftFilter;
    EmuFilterImpl rightFilter;

    // Parameter smoothing for optimistic UI updates
    struct OptimisticParameter
    {
        std::atomic<float> uiValue{0.0f};      // Immediate UI feedback
        std::atomic<float> targetValue{0.0f};  // Target for DSP
        float currentValue{0.0f};               // Actual DSP value (audio thread only)
        float smoothingRate{0.01f};

        void setFromUI(float value)
        {
            uiValue.store(value, std::memory_order_relaxed);
            targetValue.store(value, std::memory_order_relaxed);
        }

        float getForUI() const
        {
            return uiValue.load(std::memory_order_relaxed);
        }

        float getSmoothed()
        {
            float target = targetValue.load(std::memory_order_relaxed);
            currentValue += (target - currentValue) * smoothingRate;
            return currentValue;
        }

        void setSmoothingRate(double sampleRate, float timeMs = 20.0f)
        {
            smoothingRate = 1.0f - std::exp(-1.0f / (sampleRate * timeMs * 0.001f));
        }

        void seed(float value)
        {
            // Use release ordering to ensure non-atomic currentValue write
            // is visible to other threads when they acquire these atomics.
            // This is critical during initialization where we mix atomic
            // and non-atomic writes that must be properly synchronized.
            uiValue.store(value, std::memory_order_release);
            targetValue.store(value, std::memory_order_release);
            currentValue = value;
        }
    };

    OptimisticParameter optimisticCharacter;

    // Spectral tilt measurement for enhanced impact meter
    struct TiltBandsState {
        float lpIn = 0.0f, hpIn = 0.0f;
        float lpOut = 0.0f, hpOut = 0.0f;
        float alphaLP = 0.0f, alphaHP = 0.0f;

        void prepare(double fs) {
            alphaLP = std::exp(-2.0 * juce::MathConstants<double>::pi * 400.0 / fs);
            alphaHP = std::exp(-2.0 * juce::MathConstants<double>::pi * 1000.0 / fs);
        }

        inline float lpStep(float x, float &z) const noexcept {
            const float y = (1.0f - alphaLP) * x + alphaLP * z;
            z = y;
            return y;
        }

        inline float hpStep(float x, float &z) const noexcept {
            const float ylp = (1.0f - alphaHP) * x + alphaHP * z;
            z = ylp;
            return x - ylp;
        }
    };

    TiltBandsState tiltBands_;

    // Telemetry atomics for UI meter
    std::atomic<float> inputRmsDbAtomic { -100.0f };
    std::atomic<float> deltaTiltDbAtomic { 0.0f };
    std::atomic<float> deltaRmsDbAtomic { 0.0f };

    // Safety and state tracking
    double lastSampleRate_ = 48000.0;
    int safetyMuteSamplesLeft_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
