#include "PluginEditor.h"
#include <cmath>

// Active Visual Skin: Industrial Instrument (shipping)
// Reference: CLAUDE.md (Visual Modes). If a task requests OLED/Seance,
// switch palette and bezel rendering accordingly and avoid mixing skins.

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p)
{
    // Add HalftoneMouth visualizer (shows actual DSP vowel shapes)
    halftoneMouth.setTintColor(juce::Colour(LED_MINT));
    addAndMakeVisible(halftoneMouth);

    // PHASE 1.2: Melatonin Inspector - debug builds only (saves ~300KB in release)
    #if JUCE_DEBUG
        inspector = std::make_unique<melatonin::Inspector>(*this);
        inspector->onClose = [this]() { inspector.reset(); };
    #endif

    // Configure knobs (OLED look via custom paint; sliders hidden, used for input)
    for (auto* knob : {&morphKnob, &intensityKnob, &mixKnob})
    {
        knob->setSliderStyle(juce::Slider::RotaryVerticalDrag); // Vertical drag only (pro plugin standard)
        knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // We draw our own value readout
        knob->setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                  juce::MathConstants<float>::pi * 2.75f,
                                  true);
        // Interaction improvements
        knob->setVelocityBasedMode(true);            // Slower drag = finer control
        knob->setVelocityModeParameters(0.3,         // Sensitivity (lower = finer, more precise)
                                        1.0,         // Threshold
                                        0.0,         // Offset
                                        false);
        knob->setMouseDragSensitivity(150);          // Less travel needed (was 180)
        knob->setDoubleClickReturnValue(true, 0.5f); // Double-click resets to center
        knob->setPopupDisplayEnabled(true, true, this); // Value tooltip on drag
        knob->setScrollWheelEnabled(true);

        // Hide default slider drawing; we paint the knob ourselves
        knob->setAlpha(0.0f);
        knob->setWantsKeyboardFocus(false);
        addAndMakeVisible(knob);
    }

    // Connect to APVTS parameters
    morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getState(), "morph", morphKnob);
    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getState(), "intensity", intensityKnob);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getState(), "mix", mixKnob);

    // Auto mode button (content-aware intelligence toggle)
    autoButton.setClickingTogglesState(true);
    autoButton.onClick = [this]() { repaint(); };
    autoButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    autoButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(LED_MINT).withAlpha(0.2f));
    autoButton.setColour(juce::TextButton::textColourOffId, juce::Colour(LED_MINT).withAlpha(0.6f));
    autoButton.setColour(juce::TextButton::textColourOnId, juce::Colour(LED_MINT));
    addAndMakeVisible(autoButton);
    autoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getState(), "auto", autoButton);

    // Pair badge label (shows active shape pair: VOWEL/BELL/LOW/SUB)
    pairBadgeLabel.setText("[VOWEL]", juce::dontSendNotification);
    pairBadgeLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    pairBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(LED_MINT).withAlpha(0.8f));
    pairBadgeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pairBadgeLabel);

    // Preset combo box
    presetComboBox.setTextWhenNothingSelected("-- PRESETS --");
    presetComboBox.setTextWhenNoChoicesAvailable("No presets");
    presetComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(CHASSIS_MOSS).darker(0.5f));
    presetComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(LED_MINT));
    presetComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(LED_MINT).withAlpha(0.3f));
    presetComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(LED_MINT));
    presetComboBox.onChange = [this]() {
        auto presetName = presetComboBox.getText();
        if (presetName.isNotEmpty())
            processorRef.getPresetManager().loadPreset(presetName);
    };
    addAndMakeVisible(presetComboBox);

    // Save preset button
    savePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    savePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(LED_MINT).withAlpha(0.7f));
    savePresetButton.onClick = [this]() { showSavePresetDialog(); };
    addAndMakeVisible(savePresetButton);

    // Delete preset button
    deletePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    deletePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(LED_MINT).withAlpha(0.5f));
    deletePresetButton.onClick = [this]() {
        auto presetName = presetComboBox.getText();
        if (presetName.isNotEmpty())
        {
            juce::AlertWindow::showOkCancelBox(
                juce::AlertWindow::QuestionIcon,
                "Delete Preset",
                "Delete preset '" + presetName + "'?",
                "Delete", "Cancel",
                nullptr,
                juce::ModalCallbackFunction::create([this, presetName](int result) {
                    if (result == 1)
                    {
                        processorRef.getPresetManager().deletePreset(presetName);
                        refreshPresetList();
                    }
                }));
        }
    };
    addAndMakeVisible(deletePresetButton);

    // Load initial preset list
    refreshPresetList();

    // PHASE 1.1: Pre-render powder coat texture for performance (95% paint time reduction)
    regeneratePowderCoatTexture(400, 600);

    startTimerHz(30);
    setSize(400, 600);
}

void PluginEditor::regeneratePowderCoatTexture(int width, int height)
{
    // Pre-render texture once instead of drawing 1200 pixels every paint call
    cachedPowderCoatTexture_ = juce::Image(juce::Image::ARGB, width, height, true);
    juce::Graphics g(cachedPowderCoatTexture_);

    // Fine-grained noise for matte powder-coat finish (deterministic seed)
    juce::Random random(42);

    for (int i = 0; i < 1200; ++i)
    {
        float x = random.nextFloat() * width;
        float y = random.nextFloat() * height;
        float alpha = random.nextFloat() * 0.04f;  // Very subtle grain

        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.fillRect(x, y, 1.0f, 1.0f);
    }
}
void PluginEditor::drawOLEDGlowText(juce::Graphics& g, const juce::String& text,
                                   juce::Rectangle<int> area, float baseAlpha,
                                   juce::Justification just, juce::Font font)
{
    auto mint = juce::Colour(LED_MINT);
    auto savedFont = g.getCurrentFont();
    g.setFont(font);
    g.setColour(mint.withAlpha(0.4f * baseAlpha));
    g.drawText(text, area.translated(0, -1), just);
    g.setColour(mint.withAlpha(0.3f * baseAlpha));
    g.drawText(text, area.translated(0, 1), just);
    g.setColour(mint.withAlpha(1.0f * baseAlpha));
    g.drawText(text, area, just);
    g.setFont(savedFont);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // ===== CHASSIS =====
    g.fillAll(juce::Colour(CHASSIS_MOSS));

    // PHASE 1.1: Powder-coat texture overlay (cached - 95% faster)
    if (cachedPowderCoatTexture_.isValid())
        g.drawImageAt(cachedPowderCoatTexture_, 0, 0);
    else
        drawPowderCoatTexture(g, bounds); // Fallback (shouldn't happen)

    // ===== HEADER - "MUSE" =====
    {
        juce::Font titleFont(juce::FontOptions().withHeight(16.0f).withStyle("Bold"));
        drawOLEDGlowText(g, "MUSE", {0, 24, 400, 20}, 1.0f, juce::Justification::centred,
                         titleFont);
    }

    // ===== BLACK DISPLAY PANEL (HalftoneMouth renders inside) =====
    juce::Rectangle<float> displayPanel(24, 60, 352, 150);

    // Pure black panel background
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(displayPanel, 2.0f);

    // Inner shadow (recessed look)
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawRoundedRectangle(displayPanel.reduced(1.0f), 2.0f, 2.0f);

    // ===== OLED HORIZONTAL GLOW LINE (center of display) =====
    {
        const float lineHeight = 4.0f;
        const float centerY = displayPanel.getCentreY();
        auto coreLine = juce::Rectangle<float>(displayPanel.getX() + 8.0f,
                                               centerY - lineHeight * 0.5f,
                                               displayPanel.getWidth() - 16.0f,
                                               lineHeight);

        // Outer glow
        g.setColour(juce::Colour(LED_MINT).withAlpha(0.30f));
        g.fillRoundedRectangle(coreLine.expanded(0.0f, 4.0f), 2.0f);

        // Middle glow
        g.setColour(juce::Colour(LED_MINT).withAlpha(0.20f));
        g.fillRoundedRectangle(coreLine.expanded(0.0f, 2.0f), 2.0f);

        // Core line
        g.setColour(juce::Colour(LED_MINT));
        g.fillRoundedRectangle(coreLine, 2.0f);
    }

    // HalftoneMouth component renders the actual DSP vowel shapes here

    // ===== KNOBS =====
    float morphVal = (float)morphKnob.getValue();
    float intensityVal = (float)intensityKnob.getValue();
    float mixVal = (float)mixKnob.getValue();

    // Top row: MORPH and INTENSITY (72×72 per code.html)
    drawKnob(g, {90, 250, 72, 72}, morphVal, "MORPH");
    drawKnob(g, {238, 250, 72, 72}, intensityVal, "INTENSITY");

    // Bottom row: MIX (centered, 72×72 per code.html)
    drawKnob(g, {164, 400, 72, 72}, mixVal, "MIX");

    // ===== FOOTER =====
    // Subtle border line
    g.setColour(juce::Colour(0xFF3A5A5A));
    g.drawHorizontalLine(520, 24.0f, 376.0f);

    // Muse status message (DSP-driven state)
    {
        juce::Font footerFont(juce::FontOptions().withHeight(10.0f));

        // Get current Muse state from processor
        auto state = processorRef.getMuseState();
        juce::String statusMsg;

        switch (state)
        {
            case PluginProcessor::MuseState::Flow:
                statusMsg = "FLOW";
                break;
            case PluginProcessor::MuseState::Struggle:
                statusMsg = "STRUGGLE";
                break;
            case PluginProcessor::MuseState::Meltdown:
                statusMsg = "MELTDOWN";
                break;
        }

        drawOLEDGlowText(g, statusMsg, {0, 550, 400, 20}, 0.7f,
                         juce::Justification::centred, footerFont);
    }

    // ===== OLED overlay: synesthetic utterance (optional) =====
    auto utter = processorRef.getLatestUtterance();
    if (utter.isNotEmpty())
    {
        juce::Font overlayFont(juce::FontOptions().withHeight(12.0f));
        overlayFont.setBold(true);
        drawOLEDGlowText(g, utter, displayPanel.toNearestInt().withY((int)displayPanel.getY() + 8).reduced(8),
                         0.9f, juce::Justification::centredTop, overlayFont);
    }
}

void PluginEditor::drawPowderCoatTexture(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Fine-grained noise for matte powder-coat finish
    juce::Random random(42);  // Fixed seed for consistent pattern

    for (int i = 0; i < 1200; ++i)
    {
        float x = random.nextFloat() * bounds.getWidth();
        float y = random.nextFloat() * bounds.getHeight();
        float alpha = random.nextFloat() * 0.04f;  // Very subtle grain

        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.fillRect(x, y, 1.0f, 1.0f);
    }
}

void PluginEditor::drawKnob(juce::Graphics& g, juce::Rectangle<float> bounds,
                            float value, const juce::String& label)
{
    auto center = bounds.getCentre();
    float radius = bounds.getWidth() * 0.5f;

    // ===== KNOB OUTER SHADOW =====
    g.setColour(juce::Colour(0x80000000));
    g.fillEllipse(bounds.translated(1, 1));

    // ===== KNOB BODY - 3D GRADIENT (from code.html: linear-gradient(145deg, #325555, #2c4949)) =====
    juce::ColourGradient gradient(
        juce::Colour(0xFF325555), center.x - radius * 0.7f, center.y - radius * 0.7f,
        juce::Colour(0xFF2c4949), center.x + radius * 0.7f, center.y + radius * 0.7f,
        false
    );
    g.setGradientFill(gradient);
    g.fillEllipse(bounds);

    // ===== INSET SHADOWS (from code.html: inset 2px 2px 4px #263e3e, inset -2px -2px 4px #385f5f) =====
    // Simulate inset shadows with layered gradients
    // Dark inset (top-left) - creates shadow effect
    {
        juce::ColourGradient darkInset(
            juce::Colour(0xFF263e3e).withAlpha(0.6f), center.x - radius * 0.3f, center.y - radius * 0.3f,
            juce::Colours::transparentBlack, center.x + radius * 0.3f, center.y + radius * 0.3f,
            true  // Radial gradient
        );
        g.setGradientFill(darkInset);
        g.fillEllipse(bounds.reduced(2));
    }

    // Light inset (bottom-right) - creates highlight effect
    {
        juce::ColourGradient lightInset(
            juce::Colours::transparentBlack, center.x - radius * 0.3f, center.y - radius * 0.3f,
            juce::Colour(0xFF385f5f).withAlpha(0.5f), center.x + radius * 0.3f, center.y + radius * 0.3f,
            true  // Radial gradient
        );
        g.setGradientFill(lightInset);
        g.fillEllipse(bounds.reduced(2));
    }

    // ===== CENTER CIRCLE (moss green) =====
    float centerRadius = radius * 0.88f;  // Increased from 0.8f for thinner rim
    juce::Rectangle<float> centerBounds(
        center.x - centerRadius, center.y - centerRadius,
        centerRadius * 2, centerRadius * 2
    );
    g.setColour(juce::Colour(CHASSIS_MOSS));
    g.fillEllipse(centerBounds);

    // ===== INDICATOR DOT ON RIM (from code.html) =====
    float angle = juce::MathConstants<float>::pi * 1.25f +
                  value * juce::MathConstants<float>::pi * 1.5f;

    float dotDistance = radius - 6.0f;
    juce::Point<float> dotPos(
        center.x + std::cos(angle) * dotDistance,
        center.y + std::sin(angle) * dotDistance
    );

    // Dot with glow
    g.setColour(juce::Colour(LED_MINT).withAlpha(0.4f));
    g.fillEllipse(dotPos.x - 3.0f, dotPos.y - 3.0f, 6, 6);
    g.setColour(juce::Colour(LED_MINT));
    g.fillEllipse(dotPos.x - 2.0f, dotPos.y - 2.0f, 4, 4);

    // ===== CENTER LINE INDICATOR =====
    float lineLength = centerRadius - 6.0f;
    juce::Point<float> lineStart(center.x, center.y);
    juce::Point<float> lineEnd(
        center.x + std::cos(angle) * lineLength,
        center.y + std::sin(angle) * lineLength
    );

    g.setColour(juce::Colour(LED_MINT).withAlpha(0.4f));
    g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 3.0f);
    g.setColour(juce::Colour(LED_MINT));
    g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 2.0f);

    // ===== LABEL (OLED GLOW) =====
    {
        juce::Font labelFont(juce::FontOptions().withHeight(11.0f));
        labelFont.setBold(true);
        drawOLEDGlowText(g, label,
                         juce::Rectangle<int>((int)(bounds.getX() - 30), (int)(bounds.getY() - 27),
                                              (int)(bounds.getWidth() + 60), 20),
                         1.0f, juce::Justification::centred, labelFont);
    }

    // ===== VALUE READOUT =====
    {
        juce::Font valueFont(juce::FontOptions().withHeight(10.0f));
        valueFont.setTypefaceName("Consolas"); // Monospace if available
        auto valueArea = juce::Rectangle<int>((int)(bounds.getX() - 20), (int)(bounds.getBottom() + 6),
                                              (int)(bounds.getWidth() + 40), 16);
        drawOLEDGlowText(g, juce::String(value, 1), valueArea, 1.0f,
                         juce::Justification::centred, valueFont);
    }
}

void PluginEditor::resized()
{
    // HalftoneMouth display (inside black panel, with padding)
    halftoneMouth.setBounds(32, 68, 336, 134);

    // Knobs matching code.html layout (all 72×72)
    // Top row
    morphKnob.setBounds(90, 250, 72, 72);
    intensityKnob.setBounds(238, 250, 72, 72);

    // Bottom row
    mixKnob.setBounds(164, 400, 72, 72);

    // Auto button (below display, centered)
    autoButton.setBounds(164, 220, 72, 22);

    // Pair badge (below AUTO button, shows active pair)
    pairBadgeLabel.setBounds(164, 245, 72, 14);

    // Preset management (below title, left side)
    presetComboBox.setBounds(12, 50, 150, 22);
    savePresetButton.setBounds(168, 50, 50, 22);
    deletePresetButton.setBounds(224, 50, 40, 22);
}

void PluginEditor::timerCallback()
{
    // Update HalftoneMouth with continuous DSP state (no discrete quantization)
    float audioLevel = processorRef.getAudioLevel();
    float morphValue = *processorRef.getState().getRawParameterValue("morph");
    int pairIndex = static_cast<int>(*processorRef.getState().getRawParameterValue("pair"));

    halftoneMouth.setAudioLevel(audioLevel);
    halftoneMouth.setMorph(morphValue);
    halftoneMouth.setPair(pairIndex);
    // Note: morph + pair drive continuous vowel template blending inside HalftoneMouth

    // Update pair badge label
    const char* pairNames[] = { "[VOWEL]", "[BELL]", "[LOW]", "[SUB]" };
    if (pairIndex >= 0 && pairIndex < 4)
        pairBadgeLabel.setText(pairNames[pairIndex], juce::dontSendNotification);

    repaint();
}

void PluginEditor::showSavePresetDialog()
{
    auto window = std::make_unique<juce::AlertWindow>("Save Preset",
                                                       "Enter preset name:",
                                                       juce::AlertWindow::QuestionIcon);
    window->addTextEditor("name", "", "Preset Name:");
    window->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, safeWindow = window.release()](int result) {
        std::unique_ptr<juce::AlertWindow> windowPtr(safeWindow);
        if (result == 1)
        {
            auto presetName = windowPtr->getTextEditorContents("name").trim();
            if (presetName.isNotEmpty())
            {
                if (processorRef.getPresetManager().savePreset(presetName))
                {
                    refreshPresetList();
                    presetComboBox.setText(presetName, juce::dontSendNotification);
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Save Failed",
                        "Could not save preset '" + presetName + "'");
                }
            }
        }
    }));
}

void PluginEditor::refreshPresetList()
{
    presetComboBox.clear(juce::dontSendNotification);

    auto presets = processorRef.getPresetManager().getAvailablePresets();
    for (int i = 0; i < presets.size(); ++i)
    {
        presetComboBox.addItem(presets[i], i + 1);
    }
}
