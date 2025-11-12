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
    // Steel grey tint for dark dots on lime LCD (retro hardware look)
    halftoneMouth.setTintColor(juce::Colour(0xFF3B4A52));
    // Dense almond lip with superellipse SDF (thousands of dots)
    halftoneMouth.setStyle(HalftoneMouth::Style::LipHalftone);
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

    // PHASE 3: AUTO mode toggle (content-aware pair selection)
    autoButton.setClickingTogglesState(true);
    autoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(CHASSIS_MOSS).darker(0.3f));
    autoButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(LED_MINT).withAlpha(0.3f));
    autoButton.setColour(juce::TextButton::textColourOffId, juce::Colour(LED_MINT).withAlpha(0.5f));
    autoButton.setColour(juce::TextButton::textColourOnId, juce::Colour(LED_MINT));
    addAndMakeVisible(autoButton);

    autoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getState(), "auto", autoButton);

    pairBadgeLabel.setJustificationType(juce::Justification::centred);
    pairBadgeLabel.setFont(juce::Font(11.0f));
    pairBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(LED_MINT).withAlpha(0.8f));
    pairBadgeLabel.setText("MANUAL", juce::dontSendNotification);
    addAndMakeVisible(pairBadgeLabel);

    dangerButton.setClickingTogglesState(true);
    dangerButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(60, 20, 20));
    dangerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.withAlpha(0.4f));
    dangerButton.setColour(juce::TextButton::textColourOffId, juce::Colour(LED_MINT).withAlpha(0.6f));
    dangerButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    dangerButton.setTooltip("Danger Mode bypasses adaptive gain and adds +3 dB boost");
    addAndMakeVisible(dangerButton);

    dangerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getState(), "danger", dangerButton);

    // PHASE 1.1: Pre-render powder coat texture for performance (95% paint time reduction)
    regeneratePowderCoatTexture(400, 600);

    // PHASE 2: Pre-render chassis corruption (burn marks, scratches, wear)
    regenerateChassisCorruption(400, 600);

    startTimerHz(60);  // 60 FPS for smooth knob interaction; mouth throttled to 10 FPS
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

void PluginEditor::regenerateChassisCorruption(int width, int height)
{
    // PHASE 2: Procedural burn marks, scratches, and wear (EMU hardware character)
    // Deterministic seed 1993 (EMU Z-Plane year) for consistent appearance
    cachedChassisCorruption_ = juce::Image(juce::Image::ARGB, width, height, true);
    juce::Graphics g(cachedChassisCorruption_);
    juce::Random random(1993);

    // === BURN MARKS (3-5 organic scorched areas) ===
    int burnCount = random.nextInt(juce::Range<int>(3, 6));
    for (int i = 0; i < burnCount; ++i)
    {
        float x = random.nextFloat() * width;
        float y = random.nextFloat() * height;
        float size = 15.0f + random.nextFloat() * 25.0f;  // 15-40px
        float alpha = 0.08f + random.nextFloat() * 0.12f;  // Subtle dark stain

        // Organic irregular shape (multiple overlapping circles)
        for (int j = 0; j < 5; ++j)
        {
            float offsetX = (random.nextFloat() - 0.5f) * size * 0.6f;
            float offsetY = (random.nextFloat() - 0.5f) * size * 0.6f;
            float subSize = size * (0.6f + random.nextFloat() * 0.4f);

            g.setColour(juce::Colours::black.withAlpha(alpha * (0.5f + random.nextFloat() * 0.5f)));
            g.fillEllipse(x + offsetX - subSize/2, y + offsetY - subSize/2, subSize, subSize);
        }
    }

    // === SCRATCHES (8-12 thin random lines) ===
    int scratchCount = random.nextInt(juce::Range<int>(8, 13));
    for (int i = 0; i < scratchCount; ++i)
    {
        float x1 = random.nextFloat() * width;
        float y1 = random.nextFloat() * height;
        float angle = random.nextFloat() * juce::MathConstants<float>::twoPi;
        float length = 20.0f + random.nextFloat() * 60.0f;  // 20-80px
        float x2 = x1 + std::cos(angle) * length;
        float y2 = y1 + std::sin(angle) * length;
        float alpha = 0.05f + random.nextFloat() * 0.08f;

        g.setColour(juce::Colours::black.withAlpha(alpha));
        g.drawLine(x1, y1, x2, y2, 0.8f);  // Thin scratch
    }

    // === WEAR SPOTS (knob areas - subtle darkening) ===
    // Top row knobs: MORPH (90, 250), INTENSITY (238, 250)
    // Bottom knob: MIX (164, 400)
    std::array<juce::Point<float>, 3> knobCenters = {
        juce::Point<float>(90 + 36, 250 + 36),   // MORPH center
        juce::Point<float>(238 + 36, 250 + 36),  // INTENSITY center
        juce::Point<float>(164 + 36, 400 + 36)   // MIX center
    };

    for (const auto& center : knobCenters)
    {
        float wearRadius = 45.0f + random.nextFloat() * 10.0f;
        g.setColour(juce::Colours::black.withAlpha(0.04f));
        g.fillEllipse(center.x - wearRadius, center.y - wearRadius,
                      wearRadius * 2, wearRadius * 2);
    }
}

void PluginEditor::drawChassisCorruption(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Draw cached corruption texture (called once per paint)
    if (cachedChassisCorruption_.isValid())
        g.drawImageAt(cachedChassisCorruption_, 0, 0);
}

void PluginEditor::drawOLEDGlowText(juce::Graphics& g, const juce::String& text,
                                   juce::Rectangle<int> area, float baseAlpha,
                                   juce::Justification just, juce::Font font)
{
    // Subtle OLED glow (CSS: text-shadow: 0 0 2px, 0 0 5px, 0 0 8px)
    auto mint = juce::Colour(LED_MINT);
    auto savedFont = g.getCurrentFont();
    g.setFont(font);

    // Outer glow (8px - very subtle)
    g.setColour(mint.withAlpha(0.15f * baseAlpha));
    g.drawText(text, area.translated(-2, -2), just);
    g.drawText(text, area.translated(2, 2), just);
    g.drawText(text, area.translated(-2, 2), just);
    g.drawText(text, area.translated(2, -2), just);

    // Middle glow (5px)
    g.setColour(mint.withAlpha(0.25f * baseAlpha));
    g.drawText(text, area.translated(-1, -1), just);
    g.drawText(text, area.translated(1, 1), just);
    g.drawText(text, area.translated(-1, 1), just);
    g.drawText(text, area.translated(1, -1), just);

    // Inner glow (2px)
    g.setColour(mint.withAlpha(0.35f * baseAlpha));
    g.drawText(text, area.translated(0, -1), just);
    g.drawText(text, area.translated(0, 1), just);
    g.drawText(text, area.translated(-1, 0), just);
    g.drawText(text, area.translated(1, 0), just);

    // Core text (bright mint)
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

    // ===== CHASSIS - Dark Teal (#2F4F4F) =====
    g.fillAll(juce::Colour(CHASSIS_MOSS));

    // PHASE 1.1: Powder-coat texture overlay (cached - 95% faster)
    if (cachedPowderCoatTexture_.isValid())
        g.drawImageAt(cachedPowderCoatTexture_, 0, 0);
    else
        drawPowderCoatTexture(g, bounds); // Fallback (shouldn't happen)

    // PHASE 2: Chassis corruption overlay (burn marks, scratches, wear)
    drawChassisCorruption(g, bounds);

    // ===== HEADER - "MUSE" =====
    {
        // Sans-serif bold, 16px, very wide letter-spacing
        juce::Font titleFont(juce::FontOptions().withHeight(16.0f).withStyle("Bold"));
        titleFont.setExtraKerningFactor(0.25f); // tracking-[0.2em] = 25% extra spacing
        drawOLEDGlowText(g, "M U S E", juce::Rectangle<int>(0, 24, 400, 20), 1.0f,
                         juce::Justification::centred, titleFont);
    }

    drawStatusLED(g);

    // ===== BLACK LCD DISPLAY PANEL (HalftoneMouth renders inside) =====
    juce::Rectangle<float> displayPanel(24, 60, 352, 150);

    // Beveled panel border (inset 3D effect)
    // Outer highlight (top/left - catching light)
    g.setColour(juce::Colour(KNOB_INSET_LIGHT).withAlpha(0.4f));
    g.drawRoundedRectangle(displayPanel.expanded(2.0f), 2.0f, 2.0f);

    // Inner shadow (bottom/right - recessed)
    g.setColour(juce::Colour(KNOB_INSET_DARK));
    g.drawRoundedRectangle(displayPanel.expanded(1.0f), 2.0f, 1.5f);

    // Lime green LCD background (retro hardware aesthetic)
    g.setColour(juce::Colour(LCD_LIME));
    g.fillRoundedRectangle(displayPanel, 2.0f);

    // Deep inner shadow (recessed glass look)
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawRoundedRectangle(displayPanel.reduced(1.0f), 2.0f, 1.5f);

    // HalftoneMouth component renders the procedural dot matrix mouth here
    // (Component paints itself via renderCPU(), bounds set in resized())

    // PHASE 2: Serial number badge (faded hardware ID)
    {
        juce::Font serialFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        g.setFont(serialFont);
        g.setColour(juce::Colour(LED_MINT).withAlpha(0.15f));  // Very faded
        g.drawText("EMU-Z-1993-MUSE", juce::Rectangle<int>(0, 560, 400, 12),
                   juce::Justification::centred);
    }

    // ===== KNOBS =====
    float morphVal = (float)morphKnob.getValue();
    float intensityVal = (float)intensityKnob.getValue();
    float mixVal = (float)mixKnob.getValue();

    // Top row: MORPH and INTENSITY (72×72 per code.html)
    drawKnob(g, {90, 250, 72, 72}, morphVal, "MORPH", 0);
    drawKnob(g, {238, 250, 72, 72}, intensityVal, "INTENSITY", 1);

    // Bottom row: MIX (centered, 72×72 per code.html)
    drawKnob(g, {164, 400, 72, 72}, mixVal, "MIX", 2);

}

void PluginEditor::drawStatusLED(juce::Graphics& g)
{
    auto state = processorRef.getMuseState();
    juce::Colour colour;
    juce::String label;

    switch (state)
    {
        case PluginProcessor::MuseState::Flow:
            colour = juce::Colour::fromRGB(66, 214, 151);
            label = "FLOW";
            break;
        case PluginProcessor::MuseState::Struggle:
            colour = juce::Colour::fromRGB(232, 191, 61);
            label = "STRUGGLE";
            break;
        case PluginProcessor::MuseState::Meltdown:
        default:
            colour = juce::Colour::fromRGB(219, 63, 63);
            label = "MELTDOWN";
            break;
    }

    juce::Rectangle<float> ledBounds(24.0f, 20.0f, 12.0f, 12.0f);
    g.setColour(colour.withAlpha(0.25f));
    g.fillEllipse(ledBounds.expanded(4.0f));
    g.setColour(colour);
    g.fillEllipse(ledBounds);

    juce::Font statusFont(juce::FontOptions().withHeight(11.0f).withStyle("Bold"));
    g.setFont(statusFont);
    g.setColour(juce::Colour(LED_MINT).withAlpha(0.85f));
    g.drawText(label, juce::Rectangle<int>(44, 14, 120, 24), juce::Justification::left);

    if (processorRef.isDangerModeEnabled())
    {
        auto warningArea = juce::Rectangle<int>(260, 24, 120, 16);
        g.setColour(juce::Colours::red.withAlpha(0.6f));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText("DANGER ACTIVE", warningArea, juce::Justification::left);
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
                            float value, const juce::String& label, int knobId)
{
    // PHASE 2: Mechanical wobble (deterministic per knob)
    juce::Random wobbleRandom(knobId + 1993);  // Seed with knob ID + EMU year
    float wobbleX = (wobbleRandom.nextFloat() - 0.5f) * 0.4f;
    float wobbleY = (wobbleRandom.nextFloat() - 0.5f) * 0.4f;
    bounds = bounds.translated(wobbleX, wobbleY);

    auto center = bounds.getCentre();
    float radius = bounds.getWidth() * 0.5f;

    // Drop shadow (subtle depth)
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillEllipse(bounds.translated(1, 1));

    // ===== OUTER BEZEL (100% size) - Gradient + inset shadows =====
    // CSS: linear-gradient(145deg, #325555, #2c4949)
    // CSS: box-shadow: inset 2px 2px 4px #263e3e, inset -2px -2px 4px #385f5f
    juce::ColourGradient gradient(
        juce::Colour(KNOB_GRAD_LIGHT), center.x - radius * 0.6f, center.y - radius * 0.6f,
        juce::Colour(KNOB_GRAD_DARK), center.x + radius * 0.6f, center.y + radius * 0.6f,
        false
    );
    g.setGradientFill(gradient);
    g.fillEllipse(bounds);

    // Inset shadow (top-left darker, bottom-right lighter)
    g.setColour(juce::Colour(KNOB_INSET_DARK).withAlpha(0.4f));
    g.fillEllipse(bounds.getX() + 2, bounds.getY() + 2, bounds.getWidth() - 2, bounds.getHeight() - 2);

    g.setColour(juce::Colour(KNOB_INSET_LIGHT).withAlpha(0.3f));
    g.fillEllipse(bounds.getX(), bounds.getY(), bounds.getWidth() - 2, bounds.getHeight() - 2);

    // ===== CENTER CIRCLE (80% size) - Flat chassis color =====
    // CSS: width: 80%; background: #2F4F4F;
    auto centerCircle = bounds.reduced(radius * 0.2f);
    g.setColour(juce::Colour(CHASSIS_MOSS));
    g.fillEllipse(centerCircle);

    // PHASE 2: Knob wear patterns (scratches + finger darkening)
    juce::Random wearRandom(knobId + 42);  // Deterministic wear per knob
    for (int i = 0; i < 6; ++i)
    {
        float angle = wearRandom.nextFloat() * juce::MathConstants<float>::twoPi;
        float startR = wearRandom.nextFloat() * radius * 0.3f;
        float length = 8.0f + wearRandom.nextFloat() * 12.0f;
        juce::Point<float> p1(center.x + std::cos(angle) * startR,
                              center.y + std::sin(angle) * startR);
        juce::Point<float> p2(p1.x + std::cos(angle) * length,
                              p1.y + std::sin(angle) * length);
        g.setColour(juce::Colours::black.withAlpha(0.12f));
        g.drawLine(p1.x, p1.y, p2.x, p2.y, 0.6f);
    }

    // Finger wear darkening (center hotspot)
    g.setColour(juce::Colours::black.withAlpha(0.06f));
    g.fillEllipse(centerCircle.reduced(radius * 0.15f));

    // ===== MINT INDICATOR LINE (2px × 12px) =====
    // CSS: width: 2px; height: 12px; top: 6px; background: #d8f3dc;
    float angle = juce::MathConstants<float>::pi * 1.25f +
                  value * juce::MathConstants<float>::pi * 1.5f;

    float lineLength = 12.0f;
    float lineStartRadius = 6.0f;
    juce::Point<float> lineStart(
        center.x + std::cos(angle - juce::MathConstants<float>::halfPi) * lineStartRadius,
        center.y + std::sin(angle - juce::MathConstants<float>::halfPi) * lineStartRadius
    );
    juce::Point<float> lineEnd(
        center.x + std::cos(angle - juce::MathConstants<float>::halfPi) * (lineStartRadius + lineLength),
        center.y + std::sin(angle - juce::MathConstants<float>::halfPi) * (lineStartRadius + lineLength)
    );

    // Mint glow on indicator
    auto mint = juce::Colour(LED_MINT);
    g.setColour(mint.withAlpha(0.3f));
    g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 3.0f);
    g.setColour(mint);
    g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 2.0f);

    // ===== LABEL (Sans-serif semibold, 14px, tracking-widest) =====
    {
        juce::Font labelFont(juce::FontOptions().withHeight(14.0f).withStyle("Bold"));
        labelFont.setExtraKerningFactor(0.2f); // tracking-widest = ~0.1em = 20%
        drawOLEDGlowText(g, label,
                         juce::Rectangle<int>((int)(bounds.getX() - 30), (int)(bounds.getY() - 27),
                                              (int)(bounds.getWidth() + 60), 20),
                         1.0f, juce::Justification::centred, labelFont);
    }

    // ===== VALUE READOUT (Monospace 12px = text-xs) =====
    {
        juce::Font valueFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
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
    dangerButton.setBounds(260, 220, 90, 22);

    // Pair badge (below AUTO button, shows active pair)
    pairBadgeLabel.setBounds(164, 245, 72, 14);

    // Preset management (below title, left side)
    presetComboBox.setBounds(12, 50, 150, 22);
    savePresetButton.setBounds(168, 50, 50, 22);
    deletePresetButton.setBounds(224, 50, 40, 22);
}

void PluginEditor::timerCallback()
{
    // 60 FPS for smooth knob repaints
    repaint();

    // Throttle mouth updates to 10 FPS (sacred haunted stutter)
    if (++frameCounter_ % 6 != 0)
        return;

    // Update HalftoneMouth with continuous DSP state (no discrete quantization)
    float audioLevel = processorRef.getAudioLevel();

    // CRITICAL: Null-safe parameter reads (prevents crash if timer fires before APVTS init)
    auto* morphPtr = processorRef.getState().getRawParameterValue("morph");
    auto* intensityPtr = processorRef.getState().getRawParameterValue("intensity");
    auto* pairPtr = processorRef.getState().getRawParameterValue("pair");

    float morphValue = morphPtr ? morphPtr->load() : 0.5f;
    float intensityValue = intensityPtr ? intensityPtr->load() : 0.5f;
    int pairIndex = pairPtr ? static_cast<int>(pairPtr->load()) : 0;

    // Phase 1: Direct pole visualization - get actual Z-plane pole positions
    auto poles = processorRef.getLastPoles();
    if (!poles.empty())
    {
        auto dotPattern = convertPolesToDots(poles);
        halftoneMouth.setDotPattern(dotPattern);
    }

    halftoneMouth.setAudioLevel(audioLevel);
    halftoneMouth.setMorph(morphValue);
    halftoneMouth.setIntensity(intensityValue);
    halftoneMouth.setPair(pairIndex);

    // PHASE 2: Occasional glitch timing (~30 seconds, randomized)
    if (++glitchTimerFrames_ >= nextGlitchFrame_)
    {
        halftoneMouth.triggerGlitchFrame();
        glitchTimerFrames_ = 0;
        // Next glitch in 20-40 seconds @ 60 FPS = 1200-2400 frames
        nextGlitchFrame_ = 1200 + juce::Random::getSystemRandom().nextInt(1200);
    }

    // PHASE 3: AUTO mode visual feedback (show detected pair)
    auto* autoPtr = processorRef.getState().getRawParameterValue("auto");
    bool autoMode = autoPtr ? (autoPtr->load() > 0.5f) : false;
    if (autoMode)
    {
        int suggestedPair = processorRef.getSuggestedPairIndex();
        juce::String pairName;
        switch (suggestedPair)
        {
            case 0: pairName = "VOWEL"; break;
            case 1: pairName = "BELL"; break;
            case 2: pairName = "LOW"; break;
            case 3: pairName = "SUB"; break;
            default: pairName = "---"; break;
        }
        pairBadgeLabel.setText(pairName, juce::dontSendNotification);
    }
    else
    {
        pairBadgeLabel.setText("MANUAL", juce::dontSendNotification);
    }

    halftoneMouth.triggerUpdate();  // Explicit update trigger for 10 FPS cadence
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

// Phase 1: Convert Z-plane poles to 16×6 dot grid via radial influence
std::array<float, 96> PluginEditor::convertPolesToDots(const std::vector<MuseZPlaneEngine::PoleData>& poles)
{
    std::array<float, 96> dots{};

    // For each dot in 16×6 grid
    for (int y = 0; y < 6; ++y) {
        for (int x = 0; x < 16; ++x) {
            float dotX = (x + 0.5f) / 16.0f;
            float dotY = (y + 0.5f) / 6.0f;

            // Sum influence from all poles
            float intensity = 0.0f;
            for (const auto& pole : poles) {
                // Convert polar to cartesian (Z-plane: -1 to 1 → screen: 0 to 1)
                float poleX = (pole.r * std::cos(pole.theta) + 1.0f) * 0.5f;
                float poleY = (pole.r * std::sin(pole.theta) + 1.0f) * 0.5f;

                // Radial falloff from each pole (exponential decay)
                float dx = dotX - poleX;
                float dy = dotY - poleY;
                float dist = std::sqrt(dx * dx + dy * dy);
                intensity += pole.r * std::exp(-dist * 5.0f);
            }

            dots[y * 16 + x] = juce::jmin(1.0f, intensity);
        }
    }
    return dots;
}
