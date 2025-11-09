#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      knobShadow(juce::Colour(0x4D000000), 8, {0, 4})
{
    prepareGraphics();

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
    setSize(800, 400);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::prepareGraphics()
{
    // Background gradient (pre-calculated)
    backgroundGradient = juce::ColourGradient(
        juce::Colour(BG_TOP), 0.0f, 0.0f,
        juce::Colour(BG_BOTTOM), 0.0f, 400.0f,
        false
    );
}

void PluginEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // ===== BACKGROUND GRADIENT =====
    g.setGradientFill(backgroundGradient);
    g.fillAll();

    // ===== HEADER - ONLY "MUSE" =====
    g.setColour(juce::Colour(TEXT_PRIMARY));
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("MUSE", 30, 15, 200, 30, juce::Justification::centredLeft);

    // ===== KNOBS - PERFECTLY ALIGNED =====
    // Use slider values (normalized 0-1)
    float morphVal = (float)morphKnob.getValue();
    float intensityVal = (float)intensityKnob.getValue();
    float mixVal = (float)mixKnob.getValue();

    // Left column: MORPH (top), MIX (bottom)
    // Perfectly centered at x=120
    drawKnob(g, {60, 90, 120, 120}, morphVal, "");
    drawKnob(g, {60, 260, 120, 120}, mixVal, "");

    // Right of left column: INTENSITY
    // Perfectly centered at x=280
    drawKnob(g, {220, 90, 120, 120}, intensityVal, "");

    // ===== LCD DISPLAY - ALIGNED WITH KNOBS =====
    // Starts after knobs, perfectly aligned
    drawLCD(g, {380, 90, 390, 290});
}

void PluginEditor::drawKnob(juce::Graphics& g, juce::Rectangle<float> bounds,
                            float value, const juce::String& label)
{
    auto center = bounds.getCentre();
    float radius = bounds.getWidth() * 0.5f;

    // ===== PREMIUM DEPTH: Drop shadow (more pronounced) =====
    juce::DropShadow premiumShadow(juce::Colour(0x60000000), 12, {0, 6});
    juce::Path ellipsePath;
    ellipsePath.addEllipse(bounds);
    premiumShadow.drawForPath(g, ellipsePath);

    // ===== PREMIUM DEPTH: Outer glow (brighter) =====
    g.setColour(juce::Colour(0x50FFFFFF));
    g.fillEllipse(bounds.expanded(8));

    // ===== KNOB BODY: Better gradient for depth =====
    juce::ColourGradient knobGrad(
        juce::Colour(KNOB_BODY).brighter(0.15f), center.x, bounds.getY() + 20,
        juce::Colour(KNOB_BODY).darker(0.08f), center.x, bounds.getBottom() - 20,
        false
    );
    g.setGradientFill(knobGrad);
    g.fillEllipse(bounds);

    // ===== SUBTLE HIGHLIGHT (top-left) =====
    g.setColour(juce::Colour(0x20FFFFFF));
    auto highlight = bounds.reduced(10).translated(-2, -2);
    g.fillEllipse(highlight.removeFromTop(bounds.getHeight() * 0.3f));

    // ===== INDICATOR LINE (thicker, better contrast) =====
    float angle = juce::MathConstants<float>::pi * 1.25f +
                  value * juce::MathConstants<float>::pi * 1.5f;
    float lineLength = radius * 0.65f;
    juce::Point<float> lineEnd(
        center.x + std::cos(angle) * lineLength,
        center.y + std::sin(angle) * lineLength
    );

    g.setColour(juce::Colour(BG_BOTTOM).withAlpha(0.8f));
    g.drawLine(center.x, center.y, lineEnd.x, lineEnd.y, 4.0f);

    // NO LABEL - clean like FabFilter
}

void PluginEditor::drawLCD(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // ===== PREMIUM BEZEL: Deeper shadow =====
    juce::DropShadow bezelShadow(juce::Colour(0x70000000), 10, {0, 4});
    juce::Path bezelPath;
    bezelPath.addRoundedRectangle(bounds, 8.0f);
    bezelShadow.drawForPath(g, bezelPath);

    // Dark bezel background
    g.setColour(juce::Colour(0xFF0F1419));
    g.fillRoundedRectangle(bounds, 8.0f);

    // ===== PREMIUM DEPTH: Stronger inset shadow =====
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0x60000000), bounds.getX(), bounds.getY(),
        juce::Colour(0x00000000), bounds.getX(), bounds.getY() + 40,
        false
    ));
    g.fillPath(bezelPath);

    // Bright lime LCD screen
    auto lcdInner = bounds.reduced(8);
    g.setColour(juce::Colour(LCD_BG));
    g.fillRoundedRectangle(lcdInner, 4.0f);

    // Scanline texture (subtle retro feel)
    for (float y = lcdInner.getY(); y < lcdInner.getBottom(); y += 2.0f)
    {
        g.setColour(juce::Colour(0x08000000));
        g.drawHorizontalLine((int)y, lcdInner.getX(), lcdInner.getRight());
    }

    // ===== CLEAN LCD - NO TEXT (like premium hardware) =====
    // Future: This is where visualizations will go
    // (Z-plane poles, frequency response, etc.)
}

void PluginEditor::resized()
{
    // Position invisible sliders over drawn knobs for interaction
    morphKnob.setBounds(60, 90, 120, 120);        // Top left
    intensityKnob.setBounds(220, 90, 120, 120);   // Top middle
    mixKnob.setBounds(60, 260, 120, 120);         // Bottom left
}

void PluginEditor::timerCallback()
{
    repaint();
}
