#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class FieldEngineProcessor;

class FieldEngineEditor : public juce::AudioProcessorEditor
{
public:
    explicit FieldEngineEditor (FieldEngineProcessor&);
    ~FieldEngineEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FieldEngineProcessor& processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FieldEngineEditor)
};
