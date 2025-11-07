#pragma once

#include "PluginProcessor.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "ui/OLEDLookAndFeel.h"
#include "ui/GenerativeMouth.h"
#include "ui/TransmissionArea.h"
#include "ui/StatusBar.h"
#include "ui/ShapePairSelector.h"

//==============================================================================
/**
 * MuseEditor - OLED Style
 *
 * Design Philosophy (from HTML prototype):
 * - 400x600px retro hardware aesthetic
 * - Dark teal background (#2F4F4F)
 * - Mint green indicators (#d8f3dc) with glow effects
 * - 3D skeuomorphic knobs
 * - Simple OLED screen with horizontal line (neutral expression)
 * - Centered "MUSE" header
 * - "AUDIOFABRICA V 1.0" footer
 */
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    void drawOLEDScreen(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawGlowText(juce::Graphics& g, const juce::String& text,
                      juce::Rectangle<float> bounds, float fontSize,
                      juce::Justification justification);

    // Timer callback for lock-free vowel state polling (30fps)
    void timerCallback() override;

    PluginProcessor& processorRef;

    // === OLED Look and Feel ===
    OLEDLookAndFeel oledLookAndFeel;

    // === UI Components ===
    juce::Slider morphKnob;
    juce::Slider intensityKnob;
    juce::Slider mixKnob;

    juce::Label headerLabel;
    juce::Label footerLabel;

    juce::Label morphLabel;
    juce::Label intensityLabel;
    juce::Label mixLabel;

    juce::Label morphValue;
    juce::Label intensityValue;
    juce::Label mixValue;

    // === OLED-Specific Components ===
    GenerativeMouth generativeMouth;
    TransmissionArea transmissionArea;
    StatusBar statusBar;
    ShapePairSelector shapePairSelector;

    // === Parameter Attachments ===
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // === Debug (keyboard-only, no visible buttons) ===
    std::unique_ptr<melatonin::Inspector> inspector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
