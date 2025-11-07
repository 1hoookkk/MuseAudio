#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
class EngineFieldAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    EngineFieldAudioProcessorEditor (EngineFieldAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~EngineFieldAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    //==========================================================================
    // Impact Meter Component (11 horizontal boxes)
    class ImpactMeter : public juce::Component
    {
    public:
        void setImpact(float value01) { impactValue = value01; repaint(); }
        void paint(juce::Graphics& g) override;
    private:
        float impactValue = 0.65f;
    };

    //==========================================================================
    EngineFieldAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& parameters;

    // UI Components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::ToggleButton> bypassButton;
    std::unique_ptr<ImpactMeter> impactMeter;
    std::unique_ptr<juce::Slider> characterKnob;
    std::unique_ptr<juce::Label> characterLabel;
    std::unique_ptr<juce::Slider> outputSlider;
    std::unique_ptr<juce::Label> outputLabel;

    // APVTS Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> characterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EngineFieldAudioProcessorEditor)
};

