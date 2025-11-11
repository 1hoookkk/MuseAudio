/*
  ==============================================================================

    FieldEditor_v3.h
    Engine:Field v3.0 - FabFilter-Quality Professional UI

    400×600px compact window (2:3 ratio)
    Features:
    - Preset browser with EMU-style names
    - Real-time frequency spectrum analyzer
    - Z-plane pole position visualizer
    - 4-button shape selector (VOWEL/BELL/LOW/SUB)
    - Horizontal CHARACTER touch bar
    - Mix knob
    - C&C Red Alert pixel font
    - Deep slate blue + copper orange color scheme
    - Shows user exactly what they are hearing

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../FieldProcessor.h"
#include "../PresetManager.h"
#include "FieldColors_v3.h"

//==============================================================================
/**
    Main editor component for Engine:Field v3.0.

    Compact 400×600px professional interface with FabFilter-level visual feedback.
*/
class FieldEditor_v3 : public juce::AudioProcessorEditor,
                        public juce::Timer
{
public:
    explicit FieldEditor_v3(FieldProcessor& p);
    ~FieldEditor_v3() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    //==========================================================================
    // Data
    //==========================================================================
    FieldProcessor& processor_;
    PresetManager presetManager_;

    // Custom font
    juce::Font pixelFont_;

    //==========================================================================
    // Child Components
    //==========================================================================
    class PresetBrowser;
    class SpectrumAnalyzer;
    class ZPlaneVisualizer;
    class ShapeSelector;
    class CharacterBar;
    class MixKnob;

    std::unique_ptr<PresetBrowser> presetBrowser_;
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer_;
    std::unique_ptr<ZPlaneVisualizer> zPlaneVisualizer_;
    std::unique_ptr<ShapeSelector> shapeSelector_;
    std::unique_ptr<CharacterBar> characterBar_;
    std::unique_ptr<MixKnob> mixKnob_;

    //==========================================================================
    // APVTS Attachments
    //==========================================================================
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> characterAttachment_;
    std::unique_ptr<SliderAttachment> mixAttachment_;

    //==========================================================================
    // Layout Constants
    //==========================================================================
    static constexpr int WINDOW_WIDTH = 400;
    static constexpr int WINDOW_HEIGHT = 600;
    static constexpr int PRESET_HEIGHT = 40;
    static constexpr int SPECTRUM_HEIGHT = 200;
    static constexpr int ZPLANE_HEIGHT = 140;
    static constexpr int SHAPE_HEIGHT = 50;
    static constexpr int CHARACTER_HEIGHT = 60;
    static constexpr int MIX_HEIGHT = 80;
    static constexpr int PADDING = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldEditor_v3)
};

//==============================================================================
// Preset Browser Component
//==============================================================================
class FieldEditor_v3::PresetBrowser : public juce::Component
{
public:
    PresetBrowser(PresetManager& pm, juce::AudioProcessorValueTreeState& apvts, juce::Font& font);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void updateDisplay();

private:
    PresetManager& presetManager_;
    juce::AudioProcessorValueTreeState& apvts_;
    juce::Font& pixelFont_;
    juce::Rectangle<int> prevButton_;
    juce::Rectangle<int> nextButton_;
};

//==============================================================================
// Spectrum Analyzer Component
//==============================================================================
class FieldEditor_v3::SpectrumAnalyzer : public juce::Component
{
public:
    SpectrumAnalyzer(FieldProcessor& p, juce::Font& font);
    void paint(juce::Graphics& g) override;
    void update();

private:
    FieldProcessor& processor_;
    juce::Font& pixelFont_;

    // FFT visualization data
    static constexpr int fftOrder = 11;  // 2048 samples
    static constexpr int fftSize = 1 << fftOrder;

    std::array<float, fftSize> inputBuffer_;
    std::array<float, fftSize> outputBuffer_;
    int bufferPos_ = 0;

    void drawFrequencyResponse(juce::Graphics& g);
};

//==============================================================================
// Z-Plane Visualizer Component
//==============================================================================
class FieldEditor_v3::ZPlaneVisualizer : public juce::Component
{
public:
    ZPlaneVisualizer(FieldProcessor& p, juce::Font& font);
    void paint(juce::Graphics& g) override;
    void update();

private:
    FieldProcessor& processor_;
    juce::Font& pixelFont_;

    std::array<float, 12> currentPoles_;  // 6 poles × 2 (r, theta)

    void drawUnitCircle(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawPoles(juce::Graphics& g, juce::Rectangle<float> bounds);
};

//==============================================================================
// Shape Selector Component
//==============================================================================
class FieldEditor_v3::ShapeSelector : public juce::Component
{
public:
    ShapeSelector(juce::AudioProcessorValueTreeState& apvts, juce::Font& font);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    juce::AudioProcessorValueTreeState& apvts_;
    juce::Font& pixelFont_;

    std::array<juce::Rectangle<int>, 4> buttons_;
    int currentShape_ = 0;

    const std::array<juce::String, 4> shapeNames_ = { "VOWEL", "BELL", "LOW", "SUB" };
};

//==============================================================================
// Character Bar Component (horizontal touch bar)
//==============================================================================
class FieldEditor_v3::CharacterBar : public juce::Slider
{
public:
    CharacterBar(juce::Font& font);
    void paint(juce::Graphics& g) override;

private:
    juce::Font& pixelFont_;
};

//==============================================================================
// Mix Knob Component
//==============================================================================
class FieldEditor_v3::MixKnob : public juce::Slider
{
public:
    MixKnob(juce::Font& font);
    void paint(juce::Graphics& g) override;

private:
    juce::Font& pixelFont_;
};
