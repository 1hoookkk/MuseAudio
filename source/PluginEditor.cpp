#include "PluginEditor.h"

// Active Visual Skin: Industrial Instrument (shipping)
// Reference: CLAUDE.md (Visual Modes). If a task requests OLED/Seance,
// switch palette and bezel rendering accordingly and avoid mixing skins.

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p)
{
    // Add Z-Plane LED diagnostic display
    ledDisplay.setLEDColor(juce::Colour(LED_MINT));
    addAndMakeVisible(ledDisplay);

    // Configure knobs (invisible sliders for interaction)
    for (auto* knob : {&morphKnob, &intensityKnob, &mixKnob})
    {
        knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob->setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                  juce::MathConstants<float>::pi * 2.75f,
                                  true);
        knob->setAlpha(0.0f);  // Invisible - only custom drawKnob shows
        addAndMakeVisible(knob);
    }

    // Connect to APVTS parameters
    morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getState(), "morph", morphKnob);
    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getState(), "intensity", intensityKnob);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getState(), "mix", mixKnob);

    startTimerHz(30);
    setSize(400, 600);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // ===== MOSS GREEN CHASSIS (#3C5850) =====
    g.fillAll(juce::Colour(CHASSIS_MOSS));

    // Powder-coat texture overlay
    drawPowderCoatTexture(g, bounds);

    // ===== HEADER - "MUSE" =====
    g.setColour(juce::Colour(LED_MINT));
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));

    // OLED glow effect
    g.setColour(juce::Colour(LED_MINT).withAlpha(0.4f));
    g.drawText("MUSE", 0, 23, 400, 20, juce::Justification::centred);
    g.setColour(juce::Colour(LED_MINT).withAlpha(0.3f));
    g.drawText("MUSE", 0, 25, 400, 20, juce::Justification::centred);
    g.setColour(juce::Colour(LED_MINT));
    g.drawText("MUSE", 0, 24, 400, 20, juce::Justification::centred);

    // ===== BLACK DISPLAY PANEL WITH HORIZONTAL LINE =====
    juce::Rectangle<float> displayPanel(24, 60, 352, 150);

    // Pure black panel
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(displayPanel, 2.0f);

    // Inner shadow (recessed look)
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawRoundedRectangle(displayPanel.reduced(1.0f), 2.0f, 2.0f);

    // Horizontal mint line (centered, glowing)
    float lineY = displayPanel.getCentreY();
    float lineX1 = displayPanel.getX() + 40;
    float lineX2 = displayPanel.getRight() - 40;

    // Glow layers
    g.setColour(juce::Colour(LED_MINT).withAlpha(0.2f));
    g.drawLine(lineX1, lineY - 1, lineX2, lineY - 1, 3.0f);
    g.setColour(juce::Colour(LED_MINT).withAlpha(0.3f));
    g.drawLine(lineX1, lineY, lineX2, lineY, 2.0f);
    g.setColour(juce::Colour(LED_MINT));
    g.drawLine(lineX1, lineY, lineX2, lineY, 0.5f);

    // ===== KNOBS =====
    float morphVal = (float)morphKnob.getValue();
    float intensityVal = (float)intensityKnob.getValue();
    float mixVal = (float)mixKnob.getValue();

    // Top row: MORPH and INTENSITY
    drawKnob(g, {90, 250, 72, 72}, morphVal, "MORPH");
    drawKnob(g, {238, 250, 72, 72}, intensityVal, "INTENSITY");

    // Bottom row: MIX (centered)
    drawKnob(g, {164, 390, 72, 72}, mixVal, "MIX");

    // ===== FOOTER =====
    // Subtle border line
    g.setColour(juce::Colour(0xFF3A5A5A));
    g.drawHorizontalLine(520, 24.0f, 376.0f);

    // Footer text
    g.setColour(juce::Colour(LED_MINT).withAlpha(0.5f));
    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.drawText("AUDIOFABRICA V 1.0", 0, 550, 400, 20, juce::Justification::centred);
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

void PluginEditor::drawLEDBezel(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Dark recessed bezel around LED display
    auto bezelOuter = bounds;
    auto bezelInner = bounds.reduced(16);

    // Bezel background (darker moss)
    g.setColour(juce::Colour(BEZEL_DARK));
    g.fillRect(bezelOuter);

    // Inner shadow effect (recessed look)
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(bezelInner.reduced(0.5f), 2.0f);

    // Subtle highlight on bottom-right (light catching edge)
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(bezelInner.getX(), bezelInner.getBottom(),
               bezelInner.getRight(), bezelInner.getBottom(), 1.0f);
    g.drawLine(bezelInner.getRight(), bezelInner.getY(),
               bezelInner.getRight(), bezelInner.getBottom(), 1.0f);
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
    // Dark inset (top-left)
    g.setColour(juce::Colour(0xFF263e3e).withAlpha(0.6f));
    g.fillEllipse(bounds.reduced(2).translated(2, 2));

    // Light inset (bottom-right)
    g.setColour(juce::Colour(0xFF385f5f).withAlpha(0.5f));
    g.fillEllipse(bounds.reduced(2).translated(-2, -2));

    // ===== CENTER CIRCLE (moss green) =====
    float centerRadius = radius * 0.8f;
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
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));

    // Glow layers
    g.setColour(juce::Colour(LED_MINT).withAlpha(0.3f));
    g.drawText(label, bounds.getX() - 30, bounds.getY() - 27,
               bounds.getWidth() + 60, 20, juce::Justification::centred);
    g.setColour(juce::Colour(LED_MINT));
    g.drawText(label, bounds.getX() - 30, bounds.getY() - 26,
               bounds.getWidth() + 60, 20, juce::Justification::centred);

    // ===== VALUE READOUT =====
    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.setColour(juce::Colour(LED_MINT));
    g.drawText(juce::String(value, 1),
               bounds.getX() - 20, bounds.getBottom() + 6,
               bounds.getWidth() + 40, 16,
               juce::Justification::centred);
}

void PluginEditor::resized()
{
    // Knobs matching code.html layout (400×600 vertical)
    // Top row: MORPH and INTENSITY
    morphKnob.setBounds(90, 250, 72, 72);
    intensityKnob.setBounds(238, 250, 72, 72);

    // Bottom row: MIX (centered)
    mixKnob.setBounds(164, 390, 72, 72);
}

void PluginEditor::timerCallback()
{
    repaint();
}
