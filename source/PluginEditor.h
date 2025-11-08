#pragma once

#include "PluginProcessor.h"
#include "ui/HalftoneMouth.h"
#include "ui/MuseColors.h"   // existing token colors

class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PluginProcessor& processorRef;

    // Visualizer
    HalftoneMouth halftoneMouth;

    // Controls
    juce::Slider morphKnob, intensityKnob, mixKnob;
    juce::Label  morphLabel, intensityLabel, mixLabel;
    juce::Label  morphValue, intensityValue, mixValue;

    // Header/footer
    juce::Label headerLabel, footerLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment,
                                                                          intensityAttachment,
                                                                          mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
