#pragma once

#include "PluginProcessor.h"

/**
 * PluginEditor - Minimal Functional UI (North Star)
 *
 * Simple, clean baseline UI with essential controls only.
 * Purpose: Foundation to build upon, not final design.
 *
 * Contains:
 * - 3 rotary knobs (Morph, Intensity, Mix)
 * - Parameter labels
 * - Basic layout
 */
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& processorRef;

    // Knobs
    juce::Slider morphKnob;
    juce::Slider intensityKnob;
    juce::Slider mixKnob;

    // Labels
    juce::Label morphLabel;
    juce::Label intensityLabel;
    juce::Label mixLabel;
    juce::Label titleLabel;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
