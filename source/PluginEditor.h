#pragma once

// Active Visual Skin: Industrial Instrument (shipping)
// Guidance: See CLAUDE.md (Sources of Truth, Visual Modes). Do NOT mix skins in one view.
// If switching to OLED or Seance skins, update colours and bezel rendering in PluginEditor.*
// to use either OLEDLookAndFeel (Alt A) or MuseSeanceTokens (Alt B).

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ui/HalftoneMouth.h"
#include <melatonin_inspector/melatonin_inspector.h>

/**
 * Muse - Moss Green OLED Aesthetic (code.html layout)
 *
 * Layout: 400×600 vertical from code.html
 * - Moss green #3C5850 chassis (not code.html's teal)
 * - Black display with HalftoneMouth showing actual DSP vowel shapes
 * - 3D gradient knobs from code.html CSS
 * - Mint OLED text with glow effects
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
                  float value, const juce::String& label, int knobId = 0);
    void drawPowderCoatTexture(juce::Graphics& g, juce::Rectangle<float> bounds);
    void regeneratePowderCoatTexture(int width, int height);
    void drawChassisCorruption(juce::Graphics& g, juce::Rectangle<float> bounds);
    void regenerateChassisCorruption(int width, int height);
    void drawOLEDGlowText(juce::Graphics& g, const juce::String& text,
                          juce::Rectangle<int> area, float baseAlpha = 1.0f,
                          juce::Justification just = juce::Justification::centred,
                          juce::Font font = juce::Font());
    void drawStatusLED(juce::Graphics& g);

    PluginProcessor& processorRef;

    // PHASE 1.1: Cached powder coat texture (pre-rendered for performance)
    juce::Image cachedPowderCoatTexture_;

    // PHASE 2: Cached chassis corruption (burn marks, scratches, wear)
    juce::Image cachedChassisCorruption_;

    // HalftoneMouth - shows actual DSP vowel shapes
    HalftoneMouth halftoneMouth;

    // PHASE 1.2: Melatonin Inspector - debug builds only (saves ~300KB in release)
    #if JUCE_DEBUG
        std::unique_ptr<melatonin::Inspector> inspector;
    #endif

    // Interactive knob controls
    juce::Slider morphKnob, intensityKnob, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // Auto mode toggle (content-aware shape selection)
    juce::TextButton autoButton { "AUTO" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAttachment;
    juce::TextButton dangerButton { "DANGER" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dangerAttachment;
    juce::Label pairBadgeLabel;  // Shows active pair: [VOWEL] [BELL] [LOW] [SUB]

    // Preset management UI
    juce::ComboBox presetComboBox;
    juce::TextButton savePresetButton { "SAVE" };
    juce::TextButton deletePresetButton { "DEL" };

    void showSavePresetDialog();
    void refreshPresetList();

    // Phase 1: Direct pole visualization
    std::array<float, 96> convertPolesToDots(const std::vector<MuseZPlaneEngine::PoleData>& poles);

    // Frame counter for 10 FPS mouth throttling (60 FPS / 6 = 10 FPS)
    int frameCounter_ = 0;

    // PHASE 2: Occasional glitch timing (~30 seconds)
    int glitchTimerFrames_ = 0;
    int nextGlitchFrame_ = 1800;  // 30 seconds @ 60 FPS

    // Colors - 1980s Hardware LCD Palette
    static constexpr juce::uint32 CHASSIS_MOSS = 0xFF2F4F4F;    // Dark teal chassis
    static constexpr juce::uint32 LCD_BG = 0xFF000000;           // Pure black LCD background (LED grid mode)
    static constexpr juce::uint32 LCD_LIME = 0xFF9FFF9F;         // Lime green LCD (LipHalftone mode)
    static constexpr juce::uint32 LED_MINT = 0xFFD8F3DC;         // Mint text with glow
    static constexpr juce::uint32 KNOB_GRAD_LIGHT = 0xFF325555;  // Knob gradient light (teal-tinted)
    static constexpr juce::uint32 KNOB_GRAD_DARK = 0xFF2c4949;   // Knob gradient dark (teal-tinted)
    static constexpr juce::uint32 KNOB_INSET_DARK = 0xFF263e3e;  // Inset shadow dark
    static constexpr juce::uint32 KNOB_INSET_LIGHT = 0xFF385f5f; // Inset shadow light

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
