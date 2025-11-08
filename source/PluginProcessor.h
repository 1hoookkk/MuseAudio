#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <zplane/ZPlaneFilter_fast.h>
#include <zplane/EMUAuthenticTables.h>
#include "../dsp/ZPlaneShapes.hpp"

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Access for UI
    juce::AudioProcessorValueTreeState& getState() { return state_; }

    // Thread-safe vowel shape state for UI (read by PluginEditor Timer)
    // Maps Z-plane filter formants (pair + morph) to vowel shapes
    enum class VowelShape
    {
        AA, AH, EE,     // Vowel pair (0): formant transitions
        OH, OO,         // Bell pair (1): resonance shapes
        Wide, Narrow,   // Low pair (2): width variations
        Neutral         // Sub pair (3): minimal movement
    };
    std::atomic<int> currentVowelShape_ {static_cast<int>(VowelShape::AH)};

    VowelShape getCurrentVowelShape() const
    {
        return static_cast<VowelShape>(currentVowelShape_.load(std::memory_order_relaxed));
    }

    // Thread-safe audio level for UI visualization (read by PluginEditor Timer)
    // Written by audio thread in processBlock(), read by UI thread for mouth animation
    std::atomic<float> audioLevel_ {0.0f};  // 0-1 RMS level (smoothed)

    float getAudioLevel() const
    {
        return audioLevel_.load(std::memory_order_relaxed);
    }

    // === MUSE PERSONALITY: DSP-Driven State ===
    // Thread-safe DSP state monitoring for personality system
    enum class MuseState
    {
        Flow,           // 70% - Normal operation (r < 0.90)
        Struggle,       // 30% - Pushing limits (0.90 <= r < 0.93)
        Meltdown        // Ultra-rare - Catastrophic (r >= 0.93, NaN/Inf)
    };
    
    std::atomic<int> currentMuseState_ {static_cast<int>(MuseState::Flow)};
    std::atomic<float> maxPoleRadius_ {0.0f};      // Maximum pole radius this block
    mutable std::atomic<bool> nanDetected_ {false}; // NaN/Inf detected flag (mutable for const getter)
    
    MuseState getMuseState() const
    {
        return static_cast<MuseState>(currentMuseState_.load(std::memory_order_relaxed));
    }
    
    float getMaxPoleRadius() const
    {
        return maxPoleRadius_.load(std::memory_order_relaxed);
    }
    
    bool wasNanDetected() const
    {
        return nanDetected_.exchange(false, std::memory_order_relaxed);  // Read and clear
    }

private:
    // Parameter state (JUCE 8 best practice: APVTS with cached raw pointers)
    juce::AudioProcessorValueTreeState state_;

    // Cached parameter pointers for RT-safe audio thread access
    std::atomic<float>* pairParam_ = nullptr;
    std::atomic<float>* morphParam_ = nullptr;
    std::atomic<float>* intensityParam_ = nullptr;
    std::atomic<float>* mixParam_ = nullptr;
    std::atomic<float>* autoMakeupParam_ = nullptr;

    // DSP engine (validated EngineField implementation)
    emu::ZPlaneFilter_fast filter_;
    ZPlaneShapes shapes_;

    int lastPairIndex_ = -1;

    // Audio level smoothing for UI visualization (exponential decay)
    float smoothedLevel_ = 0.0f;
    static constexpr float levelDecay_ = 0.95f;  // Fast attack, slow release

    // === Phase 4: Synesthetic Intelligence ===
    // FFT analysis for sparse, truthful word generation
    static constexpr int fftOrder = 11;  // 2048 samples
    static constexpr int fftSize = 1 << fftOrder;  // 2048
    juce::dsp::FFT fft_ {fftOrder};
    std::array<float, fftSize * 2> fftData_ {};  // Input + output
    juce::AudioBuffer<float> analysisBuffer_;

    double lastAnalysisTime_ = 0.0;
    double lastUtteranceTime_ = 0.0;
    double nextUtteranceDelay_ = 60.0;  // Random 30-90 seconds

    // Instance-specific random generator (thread-safe usage from prepareToPlay)
    juce::Random instanceRandom_;

    struct SpectralFeatures
    {
        float peakFrequency = 0.0f;      // Strongest resonance (Hz)
        float spectralCentroid = 0.0f;   // Brightness measure
        float lowEnergyRatio = 0.0f;     // 100-300 Hz relative energy
        float highEnergyRatio = 0.0f;    // 3-7 kHz relative energy
        bool hasStrongResonance = false; // Peak > threshold
        bool isFlat = false;             // Low spectral variance
    };

    // DISABLED: Thread-unsafe implementations (accessed UI from audio thread)
    // TODO: Refactor using juce::AsyncUpdater or Timer-based approach
    // void analyzeAudioAndMaybeSpeak();
    // SpectralFeatures extractSpectralFeatures(const float* spectrum, int spectrumSize);
    // juce::String selectSynestheticMessage(const SpectralFeatures& features, float mix);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
