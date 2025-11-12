#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MuseLookAndFeel.h"

/**
 * MuseKnob - Rotary slider with integrated label, value display, and tooltips.
 *
 * Features:
 * - Velocity-sensitive drag (slower = finer control)
 * - Double-click to reset to default
 * - Scroll wheel support
 * - Keyboard accessibility (up/down arrows, page up/down)
 * - Screen reader labels
 * - Hover tooltips showing value
 */
class MuseKnob : public juce::Component
{
public:
    MuseKnob(const juce::String& label = {}, const juce::String& unit = {})
        : label_(label), unit_(unit)
    {
        // Configure slider
        slider_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        slider_.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                                    juce::MathConstants<float>::pi * 2.8f,
                                    true);

        // Interaction tuning
        slider_.setVelocityBasedMode(true);
        slider_.setVelocityModeParameters(0.3, 1.0, 0.0, false); // Sensitive, fine control
        slider_.setMouseDragSensitivity(120); // Less travel needed
        slider_.setDoubleClickReturnValue(true, 0.5); // Middle = default
        slider_.setScrollWheelEnabled(true);
        slider_.setPopupDisplayEnabled(true, true, this);

        // Accessibility
        slider_.setWantsKeyboardFocus(true);
        slider_.setTitle(label_); // Screen reader label
        slider_.setDescription(label_ + " control"); // Long description

        addAndMakeVisible(slider_);

        // Label
        labelComponent_.setText(label_, juce::dontSendNotification);
        labelComponent_.setFont(MuseTheme::Typography::small());
        labelComponent_.setJustificationType(juce::Justification::centred);
        labelComponent_.setColour(juce::Label::textColourId, MuseTheme::Cream);
        addAndMakeVisible(labelComponent_);
    }

    // === Public API ===
    juce::Slider& getSlider() { return slider_; }

    void setRange(double min, double max, double interval = 0.0) {
        slider_.setRange(min, max, interval);
    }

    void setValue(double value, juce::NotificationType notification = juce::sendNotificationAsync) {
        slider_.setValue(value, notification);
    }

    double getValue() const {
        return slider_.getValue();
    }

    void setTextValueSuffix(const juce::String& suffix) {
        slider_.setTextValueSuffix(suffix);
    }

    // === Layout ===
    void resized() override
    {
        auto bounds = getLocalBounds();

        // Top label
        labelComponent_.setBounds(bounds.removeFromTop(20));

        // Knob (square, centered)
        int knobSize = juce::jmin(bounds.getWidth(), bounds.getHeight() - 20);
        auto knobBounds = bounds.removeFromTop(knobSize);
        knobBounds = knobBounds.withSizeKeepingCentre(knobSize, knobSize);
        slider_.setBounds(knobBounds);

        // Value readout handled by slider's text box (TextBoxBelow)
    }

    void paint(juce::Graphics& g) override
    {
        // Optional: draw subtle background card
        // (currently transparent, knob draws itself)
    }

private:
    juce::Slider slider_;
    juce::Label labelComponent_;
    juce::String label_;
    juce::String unit_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseKnob)
};
