#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// Self-contained Z-plane DSP - all in one folder!
#include "zplane/ZPlaneParams.h"
#include "zplane/IZPlaneEngine.h"
#include "zplane/IShapeBank.h"
#include "zplane/AuthenticEMUEngine.h"
#include "zplane/StaticShapeBank.h"
#include "zplane/OversampledEngine.h"
#include "zplane/BiquadCascade.h"
#include "zplane/ZPoleMath.h"
#include "zplane/NonlinearStage.h"

//==============================================================================
/**
 * Engine Field Plugin Processor
 *
 * Professional EMU Z-plane morphing filter with authentic DSP implementation.
 * RT-safe with proper APVTS parameter binding and lock-free telemetry.
 */
class EngineFieldAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    EngineFieldAudioProcessor();
    ~EngineFieldAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    // APVTS and Parameters
    juce::AudioProcessorValueTreeState parameters;

    // Z-plane DSP Engine
    std::unique_ptr<StaticShapeBank> shapeBank;
    std::unique_ptr<AuthenticEMUEngine> zplaneEngine;
    std::unique_ptr<OversampledEngine> oversampler;

    // Parameter pointers (atomic)
    std::atomic<float>* characterParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    // DSP buffers
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> wetBuffer;

    // Utility functions
    void updateDSPParameters();
    void initializeDSP();
    void initializeParameters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EngineFieldAudioProcessor)
};

