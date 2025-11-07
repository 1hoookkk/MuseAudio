#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
    Minimal audio processor scaffold for the engineCLEAN research build.

    This processor is intentionally lean – it simply exposes a stereo I/O
    configuration so that the JUCE project can build while DSP modules are
    developed in isolation. Swap in the research processor implementation when
    ready.
*/
class FieldEngineProcessor final : public juce::AudioProcessor
{
public:
    FieldEngineProcessor();
    ~FieldEngineProcessor() override = default;

    //==============================================================================
    const juce::String getName() const override { return "engine Field"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#if JucePlugin_ProducesMidiOutput
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
#else
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
#endif

    //==============================================================================
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FieldEngineProcessor)
};

/** JUCE factory entry point */
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
