/*
  JUCE Plugin Template using DSP_PLUGIN_READY

  This is a complete, working example of integrating the DSP library
  into a JUCE audio plugin.

  To use:
  1. Copy this into your JUCE plugin Source/ folder
  2. Make sure dsp_plugin.h is in your include path
  3. Link libdsp_plugin.a in your build
  4. Build and run!
*/

#pragma once

#include <JuceHeader.h>
#include "dsp_plugin.h"

//==============================================================================
// PROCESSOR CLASS
//==============================================================================

class DspPluginProcessor : public juce::AudioProcessor
{
public:
    //==========================================================================
    DspPluginProcessor()
        : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters(*this, nullptr, "Parameters", createParameterLayout())
    {
        // Get parameter pointers
        freqParam = parameters.getRawParameterValue("frequency");
        resParam = parameters.getRawParameterValue("resonance");
    }

    ~DspPluginProcessor() override
    {
        cleanupFilters();
    }

    //==========================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        juce::ignoreUnused(samplesPerBlock);

        // Clean up old filters
        cleanupFilters();

        // Create new filters at correct sample rate
        filterLeft = dsp_filter_create(static_cast<float>(sampleRate));
        filterRight = dsp_filter_create(static_cast<float>(sampleRate));

        // Initialize with current parameter values
        updateFilterParameters();
    }

    void releaseResources() override
    {
        // Reset filter state
        if (filterLeft) dsp_filter_reset(filterLeft);
        if (filterRight) dsp_filter_reset(filterRight);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;

        // Safety check
        if (!filterLeft || !filterRight)
            return;

        // Update filter parameters if changed
        float newFreq = freqParam->load();
        float newRes = resParam->load();

        if (std::abs(newFreq - lastFreq) > 0.1f)
        {
            dsp_filter_set_frequency(filterLeft, newFreq);
            dsp_filter_set_frequency(filterRight, newFreq);
            lastFreq = newFreq;
        }

        if (std::abs(newRes - lastRes) > 0.001f)
        {
            dsp_filter_set_resonance(filterLeft, newRes);
            dsp_filter_set_resonance(filterRight, newRes);
            lastRes = newRes;
        }

        // Process audio
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        // Process left channel
        if (numChannels >= 1)
        {
            float* leftData = buffer.getWritePointer(0);
            dsp_filter_process(filterLeft, leftData, static_cast<uint32_t>(numSamples));
        }

        // Process right channel (or copy from left if mono)
        if (numChannels >= 2)
        {
            float* rightData = buffer.getWritePointer(1);
            dsp_filter_process(filterRight, rightData, static_cast<uint32_t>(numSamples));
        }
        else if (numChannels == 1 && buffer.getNumChannels() == 2)
        {
            // Copy left to right for mono-to-stereo
            buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
        }
    }

    //==========================================================================
    // Editor
    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor(*this);
    }

    bool hasEditor() const override { return true; }

    //==========================================================================
    // Plugin info
    const juce::String getName() const override { return "DSP Filter Plugin"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    // Programs
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==========================================================================
    // State
    void getStateInformation(juce::MemoryBlock& destData) override
    {
        auto state = parameters.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

        if (xmlState && xmlState->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
            updateFilterParameters();
        }
    }

    //==========================================================================
    // Public access to parameters for custom editor
    juce::AudioProcessorValueTreeState parameters;

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // Frequency parameter
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "frequency",
            "Frequency",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
            1000.0f,
            "Hz",
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 1) + " Hz"; }
        ));

        // Resonance parameter
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "resonance",
            "Resonance",
            juce::NormalisableRange<float>(0.0f, 0.99f, 0.01f),
            0.5f,
            "",
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 2); }
        ));

        return { params.begin(), params.end() };
    }

    void updateFilterParameters()
    {
        if (!filterLeft || !filterRight)
            return;

        float freq = freqParam->load();
        float res = resParam->load();

        dsp_filter_set_frequency(filterLeft, freq);
        dsp_filter_set_frequency(filterRight, freq);

        dsp_filter_set_resonance(filterLeft, res);
        dsp_filter_set_resonance(filterRight, res);

        dsp_filter_set_smoothing(filterLeft, 0.001f);
        dsp_filter_set_smoothing(filterRight, 0.001f);

        lastFreq = freq;
        lastRes = res;
    }

    void cleanupFilters()
    {
        if (filterLeft)
        {
            dsp_filter_destroy(filterLeft);
            filterLeft = nullptr;
        }

        if (filterRight)
        {
            dsp_filter_destroy(filterRight);
            filterRight = nullptr;
        }
    }

    //==========================================================================
    // DSP components
    DspFilter* filterLeft = nullptr;
    DspFilter* filterRight = nullptr;

    // Parameter pointers
    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* resParam = nullptr;

    // Cached values for change detection
    float lastFreq = 1000.0f;
    float lastRes = 0.5f;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspPluginProcessor)
};

//==============================================================================
// PLUGIN ENTRY POINT
//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DspPluginProcessor();
}
