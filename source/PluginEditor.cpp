#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // === Apply OLED Look and Feel ===
    setLookAndFeel(&oledLookAndFeel);

    // === Setup Knobs ===
    auto setupKnob = [this](juce::Slider& knob)
    {
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                 juce::MathConstants<float>::pi * 2.75f,
                                 true);
        knob.setRange(0.0, 1.0, 0.001);
        knob.setMouseDragSensitivity(300); // High resolution
        addAndMakeVisible(knob);
    };

    setupKnob(morphKnob);
    setupKnob(intensityKnob);
    setupKnob(mixKnob);

    // === Setup Labels ===
    auto setupLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
        label.setColour(juce::Label::textColourId, juce::Colour(OLEDLookAndFeel::MintGreen));
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };

    auto setupValueLabel = [this](juce::Label& label)
    {
        label.setText("0.0", juce::dontSendNotification);
        label.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
        label.setColour(juce::Label::textColourId, juce::Colour(OLEDLookAndFeel::MintGreen));
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };

    setupLabel(morphLabel, "MORPH");
    setupLabel(intensityLabel, "INTENSITY");
    setupLabel(mixLabel, "MIX");

    setupValueLabel(morphValue);
    setupValueLabel(intensityValue);
    setupValueLabel(mixValue);

    // === Setup OLED Mouth (generative, audio-reactive) ===
    addAndMakeVisible(generativeMouth);

    // === Setup Transmission Area ===
    addAndMakeVisible(transmissionArea);

    // === Setup Status Bar ===
    addAndMakeVisible(statusBar);

    // === Setup Shape Pair Selector ===
    addAndMakeVisible(shapePairSelector);

    // === Setup Header ===
    headerLabel.setText("MUSE", juce::dontSendNotification);
    headerLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::bold));
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(OLEDLookAndFeel::MintGreen));
    headerLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(headerLabel);

    // === Setup Footer ===
    footerLabel.setText("AUDIOFABRICA V 1.0", juce::dontSendNotification);
    footerLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));
    footerLabel.setColour(juce::Label::textColourId, juce::Colour(OLEDLookAndFeel::MintGreen).withAlpha(0.5f));
    footerLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(footerLabel);

    // === Parameter Attachments ===
    morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getState(), "morph", morphKnob);

    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getState(), "intensity", intensityKnob);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getState(), "mix", mixKnob);

    // === Attach Shape Pair Selector ===
    shapePairSelector.attachToParameter(processorRef.getState(), "pair");

    // === Value Update Callbacks ===
    // THREAD-SAFE: Parameter callbacks can be triggered from audio thread during automation
    // Always marshal UI updates to message thread
    morphKnob.onValueChange = [this]()
    {
        auto value = morphKnob.getValue();
        morphValue.setText(juce::String(value, 2), juce::dontSendNotification);
        transmissionArea.setMorphPosition((float)value);
    };

    intensityKnob.onValueChange = [this]()
    {
        auto value = intensityKnob.getValue();
        intensityValue.setText(juce::String(value, 2), juce::dontSendNotification);
    };

    mixKnob.onValueChange = [this]()
    {
        auto value = mixKnob.getValue();
        mixValue.setText(juce::String(value, 2), juce::dontSendNotification);
    };

    // Initialize values from APVTS after attachments are created
    // This ensures UI shows actual parameter values, not slider defaults
    morphValue.setText(juce::String(morphKnob.getValue(), 2), juce::dontSendNotification);
    intensityValue.setText(juce::String(intensityKnob.getValue(), 2), juce::dontSendNotification);
    mixValue.setText(juce::String(mixKnob.getValue(), 2), juce::dontSendNotification);

    // === Start lock-free state polling timer (30fps) ===
    // Polls audio level and filter state from audio thread → updates UI components
    startTimerHz(30);
    
    // Set up shape change callback (for messaging only; mouth follows processor state)
    shapePairSelector.setShapeChangeCallback([this](const juce::String& shape) {
        transmissionArea.setShapePair(shape);
    });
    
    // Initialize with current shape (for messaging only)
    {
        auto currentPair = processorRef.getState().getParameter("pair")->getCurrentValueAsText();
        if (currentPair.isEmpty()) {
            currentPair = "VOWEL"; // Default
        }
        transmissionArea.setShapePair(currentPair);
    }

    // === Set Plugin Size (400x600 from prototype) ===
    setSize(400, 600);
    setResizable(false, false);
}

PluginEditor::~PluginEditor()
{
    stopTimer(); // Stop polling before destruction
    setLookAndFeel(nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    // === Background (Dark Teal) ===
    g.fillAll(juce::Colour(OLEDLookAndFeel::DarkTeal));

    // === Draw OLED Screen ===
    // Position matches resized() - below shape selector
    const int padding = 24;
    auto selectorTop = padding + 28;
    auto selectorHeight = 32;
    auto screenBounds = juce::Rectangle<int>(50, selectorTop + selectorHeight + 8, 300, 150);
    drawOLEDScreen(g, screenBounds);

    // === Draw divider line above footer ===
    auto dividerY = getHeight() - 60;
    g.setColour(juce::Colour(0xFF3A5A5A)); // Slightly lighter teal
    g.drawLine(40.0f, (float)dividerY, (float)getWidth() - 40.0f, (float)dividerY, 1.0f);

    // Add subtle shadow to divider
    g.setColour(juce::Colour(OLEDLookAndFeel::MintGreen).withAlpha(0.1f));
    g.drawLine(40.0f, (float)dividerY + 1.0f, (float)getWidth() - 40.0f, (float)dividerY + 1.0f, 1.0f);
}

void PluginEditor::drawOLEDScreen(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // === Black screen background ===
    g.setColour(juce::Colour(OLEDLookAndFeel::Black));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // === Inset shadow for depth ===
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1.0f), 4.0f, 2.0f);

    // Note: The mouth is now rendered by the OLEDMouth component
    // positioned in resized()
}

void PluginEditor::drawGlowText(juce::Graphics& g, const juce::String& text,
                                juce::Rectangle<float> bounds, float fontSize,
                                juce::Justification justification)
{
    auto font = juce::Font(juce::Font::getDefaultSansSerifFontName(), fontSize, juce::Font::bold);
    g.setFont(font);

    // Outer glow (largest, most subtle)
    g.setColour(juce::Colour(OLEDLookAndFeel::MintGreen).withAlpha(0.2f));
    for (int i = -2; i <= 2; ++i)
    {
        for (int j = -2; j <= 2; ++j)
        {
            if (i == 0 && j == 0) continue;
            g.drawText(text, bounds.translated((float)i, (float)j), justification, true);
        }
    }

    // Middle glow
    g.setColour(juce::Colour(OLEDLookAndFeel::MintGreen).withAlpha(0.4f));
    g.drawText(text, bounds.translated(-1.0f, -1.0f), justification, true);
    g.drawText(text, bounds.translated(1.0f, 1.0f), justification, true);

    // Main text
    g.setColour(juce::Colour(OLEDLookAndFeel::MintGreen));
    g.drawText(text, bounds, justification, true);
}

bool PluginEditor::keyPressed(const juce::KeyPress& key)
{
    // Cmd+I (macOS) or Ctrl+I (Windows/Linux) toggles Melatonin Inspector
    if (key == juce::KeyPress('i', juce::ModifierKeys::commandModifier, 0))
    {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector>(*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }
        inspector->setVisible(!inspector->isVisible());
        return true;
    }

    return false;
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds();
    const int padding = 24;
    const int knobSize = 72; // Match HTML prototype (72px)
    const int knobRadius = knobSize / 2;

    // === Header (top) ===
    headerLabel.setBounds(0, padding, getWidth(), 20);

    // === Shape Pair Selector (below header) ===
    auto selectorBounds = juce::Rectangle<int>(50, padding + 28, 300, 32);
    shapePairSelector.setBounds(selectorBounds);

    // === OLED Screen (below selector) ===
    auto screenBounds = juce::Rectangle<int>(50, selectorBounds.getBottom() + 8, 300, 150);
    // Mouth sits inside the black OLED area with a small inset
    generativeMouth.setBounds(screenBounds.reduced(6));

    // === Transmission Area (below OLED screen) ===
    auto transmissionBounds = juce::Rectangle<int>(50, screenBounds.getBottom() + 16, 300, 40);
    transmissionArea.setBounds(transmissionBounds);

    // === Knobs (two rows) ===
    auto knobArea = bounds.reduced(padding);
    // Space for header (24+20) + selector (32+8) + screen (150+8) = ~242px
    knobArea.removeFromTop(242);

    // First row: MORPH and INTENSITY
    auto firstRow = knobArea.removeFromTop(140);
    auto morphArea = firstRow.removeFromLeft(getWidth() / 2);
    auto intensityArea = firstRow;

    // MORPH knob (left)
    {
        auto centerX = morphArea.getCentreX();
        auto centerY = morphArea.getCentreY() + 10;
        morphKnob.setBounds(centerX - knobRadius, centerY - knobRadius, knobSize, knobSize);
        morphLabel.setBounds(centerX - 50, centerY - knobRadius - 25, 100, 20);
        morphValue.setBounds(centerX - 30, centerY + knobRadius + 8, 60, 16);
    }

    // INTENSITY knob (right)
    {
        auto centerX = intensityArea.getCentreX();
        auto centerY = intensityArea.getCentreY() + 10;
        intensityKnob.setBounds(centerX - knobRadius, centerY - knobRadius, knobSize, knobSize);
        intensityLabel.setBounds(centerX - 50, centerY - knobRadius - 25, 100, 20);
        intensityValue.setBounds(centerX - 30, centerY + knobRadius + 8, 60, 16);
    }

    // Second row: MIX (centered)
    auto secondRow = knobArea.removeFromTop(140);
    {
        auto centerX = secondRow.getCentreX();
        auto centerY = secondRow.getCentreY() + 10;
        mixKnob.setBounds(centerX - knobRadius, centerY - knobRadius, knobSize, knobSize);
        mixLabel.setBounds(centerX - 50, centerY - knobRadius - 25, 100, 20);
        mixValue.setBounds(centerX - 30, centerY + knobRadius + 8, 60, 16);
    }

    // === Status Bar (above footer) ===
    auto statusBarBounds = juce::Rectangle<int>(0, getHeight() - 50, getWidth(), 20);
    statusBar.setBounds(statusBarBounds);

    // === Footer (bottom) ===
    footerLabel.setBounds(0, getHeight() - 25, getWidth(), 20);
}

void PluginEditor::timerCallback()
{
    // Lock-free read of audio level from audio thread (via atomic)
    // This makes the UI respond to ACTUAL audio activity, not just knob movements
    float audioLevel = processorRef.getAudioLevel();

    // Update generative mouth with audio activity and vowel shape
    generativeMouth.setAudioLevel(audioLevel);
    
    auto vowel = processorRef.getCurrentVowelShape();
    generativeMouth.setVowel(static_cast<GenerativeMouth::Vowel>(vowel));
    
    // Pass morph value to mouth for subtle shape influence
    generativeMouth.setMorph((float)morphKnob.getValue());
    
    // Update transmission area
    transmissionArea.setAudioLevel(audioLevel);
    
    // Update status bar with filter state
    statusBar.setCpuUsage(audioLevel * 0.1f);
    statusBar.setStability(audioLevel < 0.8f);
}
