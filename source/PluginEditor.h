#pragma once

// Active Visual Skin: Industrial Instrument (shipping)
// Guidance: See CLAUDE.md (Sources of Truth, Visual Modes). Do NOT mix skins in one view.
// If switching to OLED or Seance skins, update colours and bezel rendering in PluginEditor.*
// to use either OLEDLookAndFeel (Alt A) or MuseSeanceTokens (Alt B).

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ui/ZPlaneLEDDisplay.h"

/**
 * Industrial Instrument - Boutique Hardware Aesthetic (Active Skin)
 *
 * Design: Military/studio hardware from 1990s
 * - Industrial Moss Green chassis (#3C5850) with powder-coat texture
 * - Inset 16x6 LED diagnostic display showing Z-Plane poles
 * - 2.5D knobs with hard-edged drop shadows
 * - DIN-style all-caps typography
 * - High-contrast off-white/mint indicators
 *
 * Notes:
 * - Follow CLAUDE.md for precedence (implementation > tokens > vision).
 * - To switch skins, do so explicitly and only in the minimal files.
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
    void drawPowderCoatTexture(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawLEDBezel(juce::Graphics& g, juce::Rectangle<float> bounds);

    PluginProcessor& processorRef;

    // Z-Plane LED Diagnostic Display
    ZPlaneLEDDisplay ledDisplay;

    // Interactive knob controls
    juce::Slider morphKnob, intensityKnob, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // Colors - Industrial Instrument Palette
    static constexpr juce::uint32 CHASSIS_MOSS = 0xFF3C5850;  // Industrial moss green
    static constexpr juce::uint32 LED_MINT = 0xFFd8f3dc;      // Off-white/mint LEDs
    static constexpr juce::uint32 BEZEL_DARK = 0xFF2A3C34;    // Darker bezel inset
    static constexpr juce::uint32 KNOB_BODY = 0xFF4A6058;     // Knob base (lighter moss)
    static constexpr juce::uint32 KNOB_DARK = 0xFF2E4239;     // Knob shadow
    static constexpr juce::uint32 TEXT_PANEL = 0xFFE8ECE8;    // Panel text (silk-screen)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
