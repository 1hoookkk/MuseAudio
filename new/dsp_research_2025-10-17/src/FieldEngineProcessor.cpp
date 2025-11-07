#include "FieldEngineProcessor.h"
#include "FieldEngineEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>

FieldEngineProcessor::FieldEngineProcessor()
    : juce::AudioProcessor (BusesProperties()
                            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void FieldEngineProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/)
{
}

void FieldEngineProcessor::releaseResources()
{
}

void FieldEngineProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Pass-through (placeholder). Do not touch UI or allocate here.
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);
}

juce::AudioProcessorEditor* FieldEngineProcessor::createEditor() { return new FieldEngineEditor (*this); }

void FieldEngineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void FieldEngineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FieldEngineProcessor();
}
