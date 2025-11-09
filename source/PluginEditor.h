#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

/**
 * Professional Dark UI - JUCE 8.0.10 Best Practices
 *
 * Image 2 aesthetic: Dark background, glowing knobs, LCD display
 * - All gradients/paths stored as members (no paint() allocations)
 * - Modern FontOptions API
 * - Proper component lifecycle
 * - Subtle depth with shadows and glows
 */
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    PluginEditor(PluginProcessor& p);
    ~PluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    void drawKnob(juce::Graphics& g, juce::Rectangle<float> bounds,
                  float value, const juce::String& label);
    void drawLCD(juce::Graphics& g, juce::Rectangle<float> bounds);

    // Pre-calculated paths/gradients (JUCE 8 best practice)
    void prepareGraphics();

    PluginProcessor& processorRef;

    // Interactive knob controls
    juce::Slider morphKnob, intensityKnob, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // Cached graphics objects
    juce::ColourGradient backgroundGradient;
    juce::Path knobPath;
    juce::DropShadow knobShadow;

    // Colors (const for performance)
    static constexpr juce::uint32 BG_TOP = 0xFF364150;
    static constexpr juce::uint32 BG_BOTTOM = 0xFF1F2832;
    static constexpr juce::uint32 LCD_BG = 0xFFA8FF60;
    static constexpr juce::uint32 LCD_CONTENT = 0xFF1A2028;
    static constexpr juce::uint32 KNOB_BODY = 0xFFE8EEF5;
    static constexpr juce::uint32 TEXT_PRIMARY = 0xFFE8EEF5;
    static constexpr juce::uint32 TEXT_LABEL = 0xFF8B95A5;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
