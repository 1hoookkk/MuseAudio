#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Setup Knobs
    auto setupKnob = [this] (juce::Slider& knob) {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible (knob);
    };

    setupKnob (morphKnob);
    setupKnob (intensityKnob);
    setupKnob (mixKnob);

    // Setup Labels
    auto setupLabel = [this] (juce::Label& label, const juce::String& text) {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    setupLabel (morphLabel, "Morph");
    setupLabel (intensityLabel, "Intensity");
    setupLabel (mixLabel, "Mix");
    setupLabel (titleLabel, "MUSE");

    // Parameter Attachments
    morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.getState(), "morph", morphKnob);

    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.getState(), "intensity", intensityKnob);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.getState(), "mix", mixKnob);

    setSize (600, 400);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title at top
    titleLabel.setBounds (bounds.removeFromTop (60).reduced (20));

    // Knobs in horizontal row
    auto knobArea = bounds.reduced (40);
    int knobWidth = knobArea.getWidth() / 3;
    int knobHeight = 120;

    auto knobRow = knobArea.removeFromTop (knobHeight);

    // Morph
    auto morphArea = knobRow.removeFromLeft (knobWidth);
    morphLabel.setBounds (morphArea.removeFromTop (30));
    morphKnob.setBounds (morphArea.reduced (10));

    // Intensity
    auto intensityArea = knobRow.removeFromLeft (knobWidth);
    intensityLabel.setBounds (intensityArea.removeFromTop (30));
    intensityKnob.setBounds (intensityArea.reduced (10));

    // Mix
    auto mixArea = knobRow.removeFromLeft (knobWidth);
    mixLabel.setBounds (mixArea.removeFromTop (30));
    mixKnob.setBounds (mixArea.reduced (10));
}
