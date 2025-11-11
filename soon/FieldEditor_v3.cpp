/*
  ==============================================================================

    FieldEditor_v3.cpp
    Engine:Field v3.0 - FabFilter-Quality Professional UI

  ==============================================================================
*/

#include "FieldEditor_v3.h"

//==============================================================================
// Main Editor Implementation
//==============================================================================

FieldEditor_v3::FieldEditor_v3(FieldProcessor& p)
    : AudioProcessorEditor(&p),
      processor_(p)
{
    setSize(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Use clean professional sans-serif font (system default)
    pixelFont_ = juce::Font(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::plain);

    // Create UI components
    presetBrowser_ = std::make_unique<PresetBrowser>(processor_.getPresetManager(), processor_.getAPVTS(), pixelFont_);
    addAndMakeVisible(presetBrowser_.get());

    spectrumAnalyzer_ = std::make_unique<SpectrumAnalyzer>(processor_, pixelFont_);
    addAndMakeVisible(spectrumAnalyzer_.get());

    zPlaneVisualizer_ = std::make_unique<ZPlaneVisualizer>(processor_, pixelFont_);
    addAndMakeVisible(zPlaneVisualizer_.get());

    shapeSelector_ = std::make_unique<ShapeSelector>(processor_.getAPVTS(), pixelFont_);
    addAndMakeVisible(shapeSelector_.get());

    characterBar_ = std::make_unique<CharacterBar>(pixelFont_);
    characterBar_->setSliderStyle(juce::Slider::LinearHorizontal);
    characterBar_->setRange(0.0, 100.0, 0.1);
    characterBar_->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(characterBar_.get());
    characterAttachment_ = std::make_unique<SliderAttachment>(processor_.getAPVTS(), "character", *characterBar_);

    mixKnob_ = std::make_unique<MixKnob>(pixelFont_);
    mixKnob_->setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixKnob_->setRange(0.0, 100.0, 0.1);
    mixKnob_->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(mixKnob_.get());
    mixAttachment_ = std::make_unique<SliderAttachment>(processor_.getAPVTS(), "mix", *mixKnob_);

    // Start 60Hz timer for visual updates
    startTimerHz(60);
}

FieldEditor_v3::~FieldEditor_v3()
{
    stopTimer();
}

void FieldEditor_v3::paint(juce::Graphics& g)
{
    g.fillAll(FieldColors_v3::background);
}

void FieldEditor_v3::resized()
{
    auto area = getLocalBounds().reduced(PADDING);

    // Preset browser (top)
    presetBrowser_->setBounds(area.removeFromTop(PRESET_HEIGHT));
    area.removeFromTop(PADDING);

    // Spectrum analyzer
    spectrumAnalyzer_->setBounds(area.removeFromTop(SPECTRUM_HEIGHT));
    area.removeFromTop(PADDING);

    // Z-plane visualizer
    zPlaneVisualizer_->setBounds(area.removeFromTop(ZPLANE_HEIGHT));
    area.removeFromTop(PADDING);

    // Shape selector
    shapeSelector_->setBounds(area.removeFromTop(SHAPE_HEIGHT));
    area.removeFromTop(PADDING);

    // Character bar
    characterBar_->setBounds(area.removeFromTop(CHARACTER_HEIGHT));
    area.removeFromTop(PADDING);

    // Mix knob (centered in remaining space)
    auto mixArea = area.removeFromTop(MIX_HEIGHT);
    int knobSize = 60;
    mixKnob_->setBounds(mixArea.withSizeKeepingCentre(knobSize, knobSize));
}

void FieldEditor_v3::timerCallback()
{
    // Update visualizers at 60Hz for smooth animation
    repaint();
}

//==============================================================================
// Preset Browser Implementation
//==============================================================================

FieldEditor_v3::PresetBrowser::PresetBrowser(PresetManager& pm, juce::AudioProcessorValueTreeState& apvts, juce::Font& font)
    : presetManager_(pm), apvts_(apvts), pixelFont_(font)
{
}

void FieldEditor_v3::PresetBrowser::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Shadow
    g.setColour(FieldColors_v3::shadow);
    g.fillRect(bounds.translated(0, 2));

    // White background panel
    g.setColour(FieldColors_v3::panelLight);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Preset name (center)
    g.setColour(FieldColors_v3::textPrimary);
    g.setFont(pixelFont_.withHeight(15.0f).boldened());
    g.drawText(presetManager_.getCurrentPresetName(), bounds, juce::Justification::centred);

    // Arrow buttons
    g.setColour(FieldColors_v3::textSecondary);
    g.setFont(pixelFont_.withHeight(16.0f));
    g.drawText("◄", prevButton_, juce::Justification::centred);
    g.drawText("►", nextButton_, juce::Justification::centred);

    // Button backgrounds on hover would go here
    g.setColour(FieldColors_v3::outline);
    g.drawRoundedRectangle(prevButton_.toFloat(), 2.0f, 1.0f);
    g.drawRoundedRectangle(nextButton_.toFloat(), 2.0f, 1.0f);
}

void FieldEditor_v3::PresetBrowser::resized()
{
    auto bounds = getLocalBounds();
    int buttonWidth = 40;

    prevButton_ = bounds.removeFromLeft(buttonWidth).reduced(4);
    nextButton_ = bounds.removeFromRight(buttonWidth).reduced(4);
}

void FieldEditor_v3::PresetBrowser::mouseDown(const juce::MouseEvent& e)
{
    if (prevButton_.contains(e.getPosition()))
    {
        presetManager_.loadPrevious();
        presetManager_.applyToAPVTS(apvts_);
        repaint();
    }
    else if (nextButton_.contains(e.getPosition()))
    {
        presetManager_.loadNext();
        presetManager_.applyToAPVTS(apvts_);
        repaint();
    }
}

void FieldEditor_v3::PresetBrowser::updateDisplay()
{
    repaint();
}

//==============================================================================
// Spectrum Analyzer Implementation
//==============================================================================

FieldEditor_v3::SpectrumAnalyzer::SpectrumAnalyzer(FieldProcessor& p, juce::Font& font)
    : processor_(p), pixelFont_(font)
{
    inputBuffer_.fill(0.0f);
    outputBuffer_.fill(0.0f);
}

void FieldEditor_v3::SpectrumAnalyzer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Shadow
    g.setColour(FieldColors_v3::shadow);
    g.fillRoundedRectangle(bounds.translated(0, 2).toFloat(), 4.0f);

    // White background panel
    g.setColour(FieldColors_v3::panelLight);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Title
    g.setColour(FieldColors_v3::textSecondary);
    g.setFont(pixelFont_.withHeight(11.0f).boldened());
    auto titleArea = bounds.removeFromTop(24).reduced(12, 4);
    g.drawText("FREQUENCY RESPONSE", titleArea, juce::Justification::centredLeft);

    // Visualization area
    auto vizBounds = bounds.reduced(12);

    // Draw grid
    g.setColour(FieldColors_v3::gridLine);
    int gridSpacing = 30;
    for (int y = gridSpacing; y < vizBounds.getHeight(); y += gridSpacing)
    {
        g.drawLine(vizBounds.getX(), vizBounds.getY() + y, vizBounds.getRight(), vizBounds.getY() + y, 1.0f);
    }

    // Draw frequency response
    drawFrequencyResponse(g);

    // Labels
    g.setColour(FieldColors_v3::textDim);
    g.setFont(pixelFont_.withHeight(9.0f));
    g.drawText("20Hz", vizBounds.getX(), vizBounds.getBottom() - 12, 40, 12, juce::Justification::left);
    g.drawText("20kHz", vizBounds.getRight() - 40, vizBounds.getBottom() - 12, 40, 12, juce::Justification::right);
}

void FieldEditor_v3::SpectrumAnalyzer::drawFrequencyResponse(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(4);

    // Draw input spectrum (dim)
    g.setColour(FieldColors_v3::spectrumInput);
    juce::Path inputPath;
    inputPath.startNewSubPath(bounds.getX(), bounds.getCentreY());

    // Simple visualization - just draw a horizontal line for now
    // TODO: Implement actual FFT analysis
    inputPath.lineTo(bounds.getRight(), bounds.getCentreY());
    g.strokePath(inputPath, juce::PathStrokeType(1.0f));

    // Draw output spectrum (bright)
    g.setColour(FieldColors_v3::spectrumOutput);
    juce::Path outputPath;
    outputPath.startNewSubPath(bounds.getX(), bounds.getCentreY());

    // TODO: Draw actual filter response curve based on pole positions
    outputPath.lineTo(bounds.getRight(), bounds.getCentreY());
    g.strokePath(outputPath, juce::PathStrokeType(2.0f));
}

void FieldEditor_v3::SpectrumAnalyzer::update()
{
    // TODO: Collect audio data from processor for FFT analysis
    repaint();
}

//==============================================================================
// Z-Plane Visualizer Implementation
//==============================================================================

FieldEditor_v3::ZPlaneVisualizer::ZPlaneVisualizer(FieldProcessor& p, juce::Font& font)
    : processor_(p), pixelFont_(font)
{
    currentPoles_.fill(0.0f);
}

void FieldEditor_v3::ZPlaneVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Shadow
    g.setColour(FieldColors_v3::shadow);
    g.fillRoundedRectangle(bounds.translated(0, 2).toFloat(), 4.0f);

    // White background panel
    g.setColour(FieldColors_v3::panelLight);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Title
    g.setColour(FieldColors_v3::textSecondary);
    g.setFont(pixelFont_.withHeight(11.0f).boldened());
    auto titleArea = bounds.removeFromTop(24).reduced(12, 4);
    g.drawText("Z-PLANE: 6 POLES", titleArea, juce::Justification::centredLeft);

    // Draw visualization
    auto vizArea = bounds.reduced(16);
    float size = juce::jmin(vizArea.getWidth(), vizArea.getHeight());
    auto circleBounds = vizArea.withSizeKeepingCentre(size, size).toFloat();

    drawUnitCircle(g, circleBounds);
    drawPoles(g, circleBounds);
}

void FieldEditor_v3::ZPlaneVisualizer::drawUnitCircle(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Unit circle
    g.setColour(FieldColors_v3::outline);
    g.drawEllipse(bounds, 1.0f);

    // Crosshair
    g.drawLine(bounds.getCentreX(), bounds.getY(), bounds.getCentreX(), bounds.getBottom(), 1.0f);
    g.drawLine(bounds.getX(), bounds.getCentreY(), bounds.getRight(), bounds.getCentreY(), 1.0f);
}

void FieldEditor_v3::ZPlaneVisualizer::drawPoles(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto centre = bounds.getCentre();
    float radius = bounds.getWidth() * 0.5f;

    // Draw 6 poles from processor data
    for (int i = 0; i < 6; ++i)
    {
        float r = currentPoles_[i * 2];
        float theta = currentPoles_[i * 2 + 1];

        float x = centre.x + r * radius * std::cos(theta);
        float y = centre.y + r * radius * std::sin(theta);

        // Draw pole as circle with teal fill and dark stroke
        g.setColour(FieldColors_v3::pole);
        g.fillEllipse(x - 4, y - 4, 8, 8);
        g.setColour(FieldColors_v3::poleStroke);
        g.drawEllipse(x - 4, y - 4, 8, 8, 1.5f);

        // Draw conjugate pole (mirror across x-axis)
        float yConj = centre.y - r * radius * std::sin(theta);
        g.setColour(FieldColors_v3::pole);
        g.fillEllipse(x - 4, yConj - 4, 8, 8);
        g.setColour(FieldColors_v3::poleStroke);
        g.drawEllipse(x - 4, yConj - 4, 8, 8, 1.5f);
    }
}

void FieldEditor_v3::ZPlaneVisualizer::update()
{
    // Read pole positions from processor
    const auto& poles = processor_.getUIPoles();
    for (int i = 0; i < 12; ++i)
    {
        currentPoles_[i] = poles[i].load(std::memory_order_relaxed);
    }
    repaint();
}

//==============================================================================
// Shape Selector Implementation
//==============================================================================

FieldEditor_v3::ShapeSelector::ShapeSelector(juce::AudioProcessorValueTreeState& apvts, juce::Font& font)
    : apvts_(apvts), pixelFont_(font)
{
}

void FieldEditor_v3::ShapeSelector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Get current shape from APVTS
    if (auto* shapeParam = apvts_.getRawParameterValue("shape"))
    {
        currentShape_ = static_cast<int>(shapeParam->load(std::memory_order_relaxed));
    }

    // Draw 4 buttons
    for (int i = 0; i < 4; ++i)
    {
        bool isSelected = (i == currentShape_);

        // Shadow
        if (isSelected)
        {
            g.setColour(FieldColors_v3::shadow);
            g.fillRoundedRectangle(buttons_[i].translated(0, 1).toFloat(), 4.0f);
        }

        // Background
        g.setColour(isSelected ? FieldColors_v3::accent : FieldColors_v3::panelLight);
        g.fillRoundedRectangle(buttons_[i].toFloat(), 4.0f);

        // Border
        if (!isSelected)
        {
            g.setColour(FieldColors_v3::outline);
            g.drawRoundedRectangle(buttons_[i].toFloat(), 4.0f, 1.0f);
        }

        // Text
        g.setColour(isSelected ? juce::Colours::white : FieldColors_v3::textPrimary);
        g.setFont(pixelFont_.withHeight(12.0f).boldened());
        g.drawText(shapeNames_[i], buttons_[i], juce::Justification::centred);
    }
}

void FieldEditor_v3::ShapeSelector::resized()
{
    auto bounds = getLocalBounds();
    int buttonWidth = (bounds.getWidth() - 3 * 8) / 4;  // 4 buttons with 8px gaps

    for (int i = 0; i < 4; ++i)
    {
        buttons_[i] = bounds.removeFromLeft(buttonWidth);
        if (i < 3) bounds.removeFromLeft(8);  // Gap between buttons
    }
}

void FieldEditor_v3::ShapeSelector::mouseDown(const juce::MouseEvent& e)
{
    for (int i = 0; i < 4; ++i)
    {
        if (buttons_[i].contains(e.getPosition()))
        {
            if (auto* shapeParam = apvts_.getParameter("shape"))
            {
                shapeParam->setValueNotifyingHost(shapeParam->convertTo0to1(static_cast<float>(i)));
            }
            repaint();
            break;
        }
    }
}

//==============================================================================
// Character Bar Implementation
//==============================================================================

FieldEditor_v3::CharacterBar::CharacterBar(juce::Font& font)
    : pixelFont_(font)
{
}

void FieldEditor_v3::CharacterBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Shadow
    g.setColour(FieldColors_v3::shadow);
    g.fillRoundedRectangle(bounds.translated(0, 2).toFloat(), 4.0f);

    // White background
    g.setColour(FieldColors_v3::panelLight);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Label
    g.setColour(FieldColors_v3::textSecondary);
    g.setFont(pixelFont_.withHeight(11.0f).boldened());
    auto labelArea = bounds.removeFromTop(20).reduced(12, 4);
    g.drawText("CHARACTER", labelArea, juce::Justification::centredLeft);

    // Slider bar area
    auto barArea = bounds.reduced(12, 8);
    auto barBounds = barArea.removeFromTop(20);

    // Background track
    g.setColour(FieldColors_v3::background);
    g.fillRoundedRectangle(barBounds.toFloat(), 3.0f);

    // Filled portion (teal)
    float fillProportion = (float)getValue() / 100.0f;
    auto fillBounds = barBounds.toFloat();
    fillBounds.setWidth(fillBounds.getWidth() * fillProportion);
    g.setColour(FieldColors_v3::accent);
    g.fillRoundedRectangle(fillBounds, 3.0f);

    // Value text
    g.setColour(FieldColors_v3::textPrimary);
    g.setFont(pixelFont_.withHeight(13.0f).boldened());
    juce::String valueText = juce::String((int)getValue()) + "%";
    g.drawText(valueText, barArea, juce::Justification::centredRight);
}

//==============================================================================
// Mix Knob Implementation
//==============================================================================

FieldEditor_v3::MixKnob::MixKnob(juce::Font& font)
    : pixelFont_(font)
{
}

void FieldEditor_v3::MixKnob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto centre = bounds.getCentre();
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;

    // Shadow
    g.setColour(FieldColors_v3::shadow);
    g.fillEllipse(centre.x - radius + 1, centre.y - radius + 2, radius * 2, radius * 2);

    // White background circle
    g.setColour(FieldColors_v3::panelLight);
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2, radius * 2);

    // Background track arc (light gray)
    juce::Path bgArc;
    bgArc.addCentredArc(centre.x, centre.y, radius - 4, radius - 4,
                        0.0f, juce::MathConstants<float>::pi * 1.25f,
                        juce::MathConstants<float>::pi * 2.75f, true);
    g.setColour(FieldColors_v3::background);
    g.strokePath(bgArc, juce::PathStrokeType(4.0f));

    // Value arc (teal)
    float startAngle = juce::MathConstants<float>::pi * 1.25f;
    float endAngle = startAngle + ((float)getValue() / 100.0f) * juce::MathConstants<float>::pi * 1.5f;

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius - 4, radius - 4,
                          0.0f, startAngle, endAngle, true);

    g.setColour(FieldColors_v3::accent);
    g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Border circle
    g.setColour(FieldColors_v3::outline);
    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2, radius * 2, 1.0f);

    // Label
    g.setColour(FieldColors_v3::textSecondary);
    g.setFont(pixelFont_.withHeight(10.0f).boldened());
    g.drawText("MIX", bounds.reduced(0, radius * 2 + 6), juce::Justification::centredTop);

    // Value text
    g.setColour(FieldColors_v3::textPrimary);
    g.setFont(pixelFont_.withHeight(16.0f).boldened());
    juce::String valueText = juce::String((int)getValue()) + "%";
    g.drawText(valueText, bounds, juce::Justification::centred);
}
