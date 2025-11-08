#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "OLEDLookAndFeel.h"
#include "ui/themes/Theme.h"

/**
 * ShapePairSelector - 4-button shape selector for Z-plane filter pairs
 *
 * Design:
 * - Horizontal row of 4 buttons: VOWEL, BELL, LOW, SUB
 * - Dark background (#141b1d), mint green text with glow when selected
 * - Radio button behavior (only one selected at a time)
 * - Wired to "pair" parameter (0-3) in PluginProcessor
 */
class ShapePairSelector : public juce::Component
{
public:
    ShapePairSelector()
    {
        // Create 4 buttons
        const juce::StringArray shapeNames = { "VOWEL", "BELL", "LOW", "SUB" };

        for (int i = 0; i < 4; ++i)
        {
            auto* button = new juce::TextButton(shapeNames[i]);
            button->setClickingTogglesState(true);
            button->setRadioGroupId(1001, juce::dontSendNotification);

            // Custom colors for OLED aesthetic
            button->setColour(juce::TextButton::buttonColourId, ModernMuseTheme::panelBackground);
            button->setColour(juce::TextButton::buttonOnColourId, ModernMuseTheme::panelBackgroundActive);
            button->setColour(juce::TextButton::textColourOffId, ModernMuseTheme::mintGreen.withAlpha(0.5f));
            button->setColour(juce::TextButton::textColourOnId, ModernMuseTheme::mintGreen);

            button->onClick = [this, i]() { onButtonClicked(i); };

            addAndMakeVisible(button);
            shapeButtons.add(button);
        }

        // Default to first button
        shapeButtons[0]->setToggleState(true, juce::dontSendNotification);
    }

    ~ShapePairSelector() override
    {
        // Clear attachments before buttons are destroyed
        parameterAttachment.reset();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int buttonWidth = bounds.getWidth() / 4;

        for (int i = 0; i < shapeButtons.size(); ++i)
        {
            auto buttonBounds = bounds.removeFromLeft(buttonWidth);
            if (i < 3)
                buttonBounds = buttonBounds.withTrimmedRight(2); // Small gap between buttons

            shapeButtons[i]->setBounds(buttonBounds);
        }
    }

    void paint(juce::Graphics& g) override
    {
        // Draw custom button backgrounds with glow effect for selected state
        for (int i = 0; i < shapeButtons.size(); ++i)
        {
            auto* button = shapeButtons[i];
            auto buttonBounds = button->getBounds().toFloat();

            if (button->getToggleState())
            {
                // Draw glow effect for selected button
                g.setColour(ModernMuseTheme::mintGreen.withAlpha(0.2f));
                g.fillRoundedRectangle(buttonBounds.expanded(2.0f), 2.0f);
            }
        }
    }

    /**
     * Attach to the "pair" parameter in APVTS
     */
    void attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& parameterID)
    {
        // Create a custom attachment that maps button index (0-3) to parameter value
        parameterAttachment = std::make_unique<juce::ParameterAttachment>(
            *apvts.getParameter(parameterID),
            [this](float value) { updateButtonState((int)value); }
        );

        // Set initial state
        auto* param = apvts.getRawParameterValue(parameterID);
        if (param != nullptr)
            updateButtonState((int)*param);
    }
    
    /**
     * Set callback for shape pair changes
     */
    void setShapeChangeCallback(std::function<void(const juce::String&)> callback)
    {
        onShapeChange = std::move(callback);
    }

private:
    void onButtonClicked(int buttonIndex)
    {
        // Update parameter when button clicked
        if (parameterAttachment)
        {
            parameterAttachment->setValueAsCompleteGesture((float)buttonIndex);
        }

        // Trigger callback for UI updates
        if (onShapeChange)
        {
            const juce::StringArray shapeNames = { "VOWEL", "BELL", "LOW", "SUB" };
            onShapeChange(shapeNames[buttonIndex]);
        }

        repaint(); // Trigger glow effect repaint
    }

    void updateButtonState(int selectedIndex)
    {
        // Update button toggle states when parameter changes
        for (int i = 0; i < shapeButtons.size(); ++i)
        {
            shapeButtons[i]->setToggleState(i == selectedIndex, juce::dontSendNotification);
        }

        repaint(); // Trigger glow effect repaint
    }

    juce::OwnedArray<juce::TextButton> shapeButtons;
    std::unique_ptr<juce::ParameterAttachment> parameterAttachment;
    std::function<void(const juce::String&)> onShapeChange;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShapePairSelector)
};
