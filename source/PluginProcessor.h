#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <zplane/ZPlaneFilter.h>
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
    emu::ZPlaneFilter filter_;
    ZPlaneShapes shapes_;

    int lastPairIndex_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
