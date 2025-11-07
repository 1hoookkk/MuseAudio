#include "PluginEditor.h"

//==============================================================================
EngineFieldAudioProcessorEditor::EngineFieldAudioProcessorEditor (
    EngineFieldAudioProcessor& p,
    juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), processor (p), parameters (vts)
{
    // Window size - fixed to mockup dimensions
    setSize (400, 600);
    setResizable (false, false);

    // Title label
    titleLabel = std::make_unique<juce::Label>();
    titleLabel->setText ("engine: Field", juce::dontSendNotification);
    titleLabel->setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::plain));
    titleLabel->setColour (juce::Label::textColourId, juce::Colour (0xFF1A1A1A));
    addAndMakeVisible (*titleLabel);

    // Bypass button
    bypassButton = std::make_unique<juce::ToggleButton>("bypass");
    bypassButton->setButtonText ("");
    addAndMakeVisible (*bypassButton);

    // Impact meter
    impactMeter = std::make_unique<ImpactMeter>();
    addAndMakeVisible (*impactMeter);

    // Character knob (main hero control)
    characterKnob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                    juce::Slider::TextBoxBelow);
    characterKnob->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    characterKnob->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFC73E1D));
    characterKnob->setColour (juce::Slider::thumbColourId, juce::Colour (0xFFC73E1D));
    characterKnob->setColour (juce::Slider::trackColourId, juce::Colour (0xFF2C2C2C));
    addAndMakeVisible (*characterKnob);

    characterLabel = std::make_unique<juce::Label>();
    characterLabel->setText ("Character", juce::dontSendNotification);
    characterLabel->setFont (juce::Font (16.0f, juce::Font::bold));
    characterLabel->setJustificationType (juce::Justification::centred);
    characterLabel->setColour (juce::Label::textColourId, juce::Colour (0xFF1A1A1A));
    addAndMakeVisible (*characterLabel);

    // Output slider
    outputSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                   juce::Slider::TextBoxRight);
    outputSlider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    outputSlider->setColour (juce::Slider::trackColourId, juce::Colour (0xFF1A1A1A));
    outputSlider->setColour (juce::Slider::thumbColourId, juce::Colour (0xFF2C2C2C));
    addAndMakeVisible (*outputSlider);

    outputLabel = std::make_unique<juce::Label>();
    outputLabel->setText ("Output", juce::dontSendNotification);
    outputLabel->setFont (juce::Font (14.0f, juce::Font::bold));
    outputLabel->setColour (juce::Label::textColourId, juce::Colour (0xFF1A1A1A));
    addAndMakeVisible (*outputLabel);

    // Create APVTS attachments
    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (parameters, "character", *characterKnob);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (parameters, "outputGain", *outputSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (parameters, "bypass", *bypassButton);

    // Start telemetry timer at 30Hz
    startTimerHz (30);
}

EngineFieldAudioProcessorEditor::~EngineFieldAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void EngineFieldAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Off-white background (matches mockup)
    g.fillAll (juce::Colour (0xFFF5F5F0));

    // Border
    g.setColour (juce::Colour (0xFF1A1A1A));
    g.drawRect (getLocalBounds(), 1);

    // Horizontal line under title
    g.drawLine (20.0f, 60.0f, getWidth() - 20.0f, 60.0f, 2.0f);
}

void EngineFieldAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title area (60px)
    auto titleArea = bounds.removeFromTop (60);
    titleLabel->setBounds (titleArea.removeFromLeft (200).reduced (20, 20));
    bypassButton->setBounds (titleArea.removeFromRight (100).reduced (30, 20));

    bounds.removeFromTop (20); // Spacing

    // Impact meter (80px)
    impactMeter->setBounds (bounds.removeFromTop (80).reduced (40, 10));

    bounds.removeFromTop (40); // Spacing

    // Character knob (centered, 200px tall)
    int knobSize = 180;
    auto knobArea = bounds.removeFromTop (200);
    characterKnob->setBounds ((getWidth() - knobSize) / 2, knobArea.getY(), knobSize, 180);
    characterLabel->setBounds (characterKnob->getBounds().withY (characterKnob->getBottom()).withHeight (30));

    bounds.removeFromTop (20); // Spacing

    // Output slider (60px)
    auto outputArea = bounds.removeFromTop (60).reduced (40, 10);
    outputLabel->setBounds (outputArea.removeFromLeft (60));
    outputSlider->setBounds (outputArea);
}

void EngineFieldAudioProcessorEditor::timerCallback()
{
    // Update impact meter from character value
    if (auto* charParam = parameters.getRawParameterValue ("character"))
    {
        impactMeter->setImpact (*charParam);
    }
}

//==============================================================================
// Impact Meter Implementation
void EngineFieldAudioProcessorEditor::ImpactMeter::paint (juce::Graphics& g)
{
    const int numBoxes = 11;
    const int boxWidth = 40;
    const int boxHeight = 50;
    const int spacing = 4;
    const int totalWidth = (numBoxes * boxWidth) + ((numBoxes - 1) * spacing);
    const int startX = (getWidth() - totalWidth) / 2;
    const int startY = 5;

    // Draw "Impact" label above
    g.setColour (juce::Colour (0xFF1A1A1A));
    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.drawText ("Impact", getLocalBounds().withHeight (startY), juce::Justification::centredTop, false);

    // Colors
    juce::Colour fillColor (0xFFC73E1D);  // Terracotta
    juce::Colour emptyColor (0xFF2C2C2C); // Dark gray

    int filledBoxes = juce::jmax (0, juce::jmin (numBoxes, (int)(impactValue * numBoxes)));

    // Draw boxes
    for (int i = 0; i < numBoxes; ++i)
    {
        int x = startX + (i * (boxWidth + spacing));

        g.setColour (i < filledBoxes ? fillColor : emptyColor);
        g.fillRect (x, startY + 20, boxWidth, boxHeight);

        // Border
        g.setColour (juce::Colour (0xFF1A1A1A));
        g.drawRect (x, startY + 20, boxWidth, boxHeight, 1);
    }

    // Draw percentage
    g.setColour (juce::Colour (0xFF1A1A1A));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));
    g.drawText (juce::String ((int)(impactValue * 100)) + "%",
                getLocalBounds().withY (startY + 20 + boxHeight + 5),
                juce::Justification::centredTop, false);
}
