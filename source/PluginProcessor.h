#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_events/juce_events.h>
#include "dsp/MuseZPlaneEngine.h"
#include "../dsp/ZPlaneShapes.hpp"
#include "PresetManager.h"
#include <psycho/PsychoacousticDescriptors.h>

// NOTE: Now inherits juce::AsyncUpdater to safely communicate sparse "utterances"
// from the audio thread to the UI without ever touching UI objects off the message thread.
class PluginProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
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

    bool isDangerModeEnabled() const
    {
        return dangerParam_ && dangerParam_->load(std::memory_order_relaxed) > 0.5f;
    }

    struct ParameterSnapshot
    {
        int pair = 0;
        float morph = 0.5f;
        float intensity = 0.0f;
        float mix = 1.0f;
        float drive = 0.0f;
    };

    ParameterSnapshot getParameterSnapshot() const { return parameterState_.getSnapshot(); }

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
    std::atomic<bool> nanDetected_ {false};         // NaN/Inf detected flag
    
    MuseState getMuseState() const
    {
        return static_cast<MuseState>(currentMuseState_.load(std::memory_order_relaxed));
    }
    
    float getMaxPoleRadius() const
    {
        return maxPoleRadius_.load(std::memory_order_relaxed);
    }
    
    bool wasNanDetected()
    {
        return nanDetected_.exchange(false, std::memory_order_relaxed);  // Read and clear
    }

    // === PHASE 5: Content-Aware Intelligence ===
    // Thread-safe psychoacoustic analysis results for auto shape selection
    // Written by audio thread (processBlock @ 10 Hz), read by UI thread for feedback
    std::atomic<float> detectedVowelness_ {0.0f};
    std::atomic<float> detectedMetallicity_ {0.0f};
    std::atomic<float> detectedWarmth_ {0.0f};
    std::atomic<float> detectedPunch_ {0.0f};
    std::atomic<int> suggestedPairIndex_ {0};  // Auto-selected pair (0-3: Vowel/Bell/Low/Sub)

    float getDetectedVowelness() const { return detectedVowelness_.load(std::memory_order_relaxed); }
    float getDetectedMetallicity() const { return detectedMetallicity_.load(std::memory_order_relaxed); }
    float getDetectedWarmth() const { return detectedWarmth_.load(std::memory_order_relaxed); }
    float getDetectedPunch() const { return detectedPunch_.load(std::memory_order_relaxed); }
    int getSuggestedPairIndex() const { return suggestedPairIndex_.load(std::memory_order_relaxed); }

    // === Synesthetic Intelligence (Phase 4) ===
    // Public accessor so the Editor could, in future, poll recent spectral features
    // (currently we only emit sparse textual utterances).
    struct SpectralFeatures
    {
        float peakFrequency = 0.0f;
        float spectralCentroid = 0.0f;
        float lowEnergyRatio = 0.0f;
        float highEnergyRatio = 0.0f;
        bool hasStrongResonance = false;
        bool isFlat = false;
    };
    SpectralFeatures getLatestSpectralFeatures() const { return latestFeatures_; }

    // Latest delivered textual synesthetic utterance (non-blocking UI poll)
    juce::String getLatestUtterance() const { return pendingMessage_; }

    // === PHASE 3.1: Performance Monitoring ===
    // Thread-safe CPU load tracking (JUCE best practice: AudioProcessLoadMeasurer)
    // UI can poll this at ~10 Hz to display CPU usage metrics
    float getProcessorLoad() const
    {
        return static_cast<float>(loadMeasurer_.getLoadAsPercentage());
    }

    double getXRunCount() const
    {
        return loadMeasurer_.getXRunCount();
    }

    // === PHASE 4.1: Preset Management ===
    // Access to preset manager for UI (message thread only)
    PresetManager& getPresetManager() { return presetManager_; }

    // === PHASE 1: Direct Pole Visualization ===
    // Thread-safe pole data access for UI visualization (10 FPS timer safe)
    std::vector<MuseZPlaneEngine::PoleData> getLastPoles() const;

private:
    struct ParameterState
    {
        void prepare(double sampleRate)
        {
            morphSmooth_.reset(sampleRate, 0.02);
            intensitySmooth_.reset(sampleRate, 0.02);
            mixSmooth_.reset(sampleRate, 0.02);
            driveSmooth_.reset(sampleRate, 0.02);
            morphSmooth_.setCurrentAndTargetValue(snapshot_.morph);
            intensitySmooth_.setCurrentAndTargetValue(snapshot_.intensity);
            mixSmooth_.setCurrentAndTargetValue(snapshot_.mix);
            driveSmooth_.setCurrentAndTargetValue(snapshot_.drive);
        }

        void setTargets(int pair, float morph, float intensity, float mix, float drive)
        {
            pairTarget_ = pair;
            morphSmooth_.setTargetValue(juce::jlimit(0.0f, 1.0f, morph));
            intensitySmooth_.setTargetValue(juce::jlimit(0.0f, 1.0f, intensity));
            mixSmooth_.setTargetValue(juce::jlimit(0.0f, 1.0f, mix));
            driveSmooth_.setTargetValue(drive);
        }

        ParameterSnapshot consume(int numSamples)
        {
            morphSmooth_.skip(numSamples);
            intensitySmooth_.skip(numSamples);
            mixSmooth_.skip(numSamples);
            driveSmooth_.skip(numSamples);

            snapshot_.pair = pairTarget_;
            snapshot_.morph = morphSmooth_.getCurrentValue();
            snapshot_.intensity = intensitySmooth_.getCurrentValue();
            snapshot_.mix = mixSmooth_.getCurrentValue();
            snapshot_.drive = driveSmooth_.getCurrentValue();
            return snapshot_;
        }

        ParameterSnapshot getSnapshot() const { return snapshot_; }

    private:
        int pairTarget_ = 0;
        ParameterSnapshot snapshot_{};
        juce::LinearSmoothedValue<float> morphSmooth_;
        juce::LinearSmoothedValue<float> intensitySmooth_;
        juce::LinearSmoothedValue<float> mixSmooth_;
        juce::LinearSmoothedValue<float> driveSmooth_;
    };

    // Parameter state (JUCE 8 best practice: APVTS with cached raw pointers)
    juce::AudioProcessorValueTreeState state_;

    // Cached parameter pointers for RT-safe audio thread access
    std::atomic<float>* pairParam_ = nullptr;
    std::atomic<float>* morphParam_ = nullptr;
    std::atomic<float>* intensityParam_ = nullptr;
    std::atomic<float>* mixParam_ = nullptr;
    std::atomic<float>* autoParam_ = nullptr;    // PHASE 3: Auto mode toggle
    std::atomic<float>* dangerParam_ = nullptr;  // PHASE 4: Danger mode

    // DSP engine (unified wrapper for Fast/Authentic modes)
    MuseZPlaneEngine engine_;
    ZPlaneShapes shapes_;
    ParameterState parameterState_;
    mutable juce::SpinLock poleLock_;
    std::vector<MuseZPlaneEngine::PoleData> cachedPoleFrame_;

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

    // === PHASE 5: Psychoacoustic Analysis Timing ===
    double lastPsychoAnalysisTime_ = 0.0;
    static constexpr double psychoAnalysisInterval_ = 0.1;  // 10 Hz analysis rate
    float smoothedPairTarget_ = 0.0f;  // Exponential smoothing for shape transitions

    // Instance-specific random generator (thread-safe usage from prepareToPlay)
    juce::Random instanceRandom_;

    // Latest snapshot of trivial spectral features (placeholder until full FFT pipeline)
    SpectralFeatures latestFeatures_{};

    // Async utterance system --------------------------------------------------
    std::atomic<bool> pendingUtterance_{ false }; // Set by audio thread; consumed on message thread
    juce::String pendingMessage_;                 // Built on message thread only
    void handleAsyncUpdate() override;            // Deliver message safely to UI
    juce::String selectSynestheticMessage(const SpectralFeatures& features, float mix, float intensity);

    // PHASE 3.1: CPU load monitoring (best practice from Context7)
    juce::AudioProcessLoadMeasurer loadMeasurer_;

    // PHASE 4.1: Preset management (message thread only, never in processBlock)
    PresetManager presetManager_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
