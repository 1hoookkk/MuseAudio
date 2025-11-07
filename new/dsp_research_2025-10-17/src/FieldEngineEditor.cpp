#include "FieldEngineEditor.h"
#include "FieldEngineProcessor.h"

FieldEngineEditor::FieldEngineEditor (FieldEngineProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setSize (400, 300);
}

void FieldEngineEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::orange);
    g.setFont (18.0f);
    g.drawFittedText ("engine Field — clean", getLocalBounds(), juce::Justification::centred, 1);
}

void FieldEngineEditor::resized()
{
}

