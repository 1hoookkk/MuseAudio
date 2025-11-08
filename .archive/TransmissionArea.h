#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "OLEDLookAndFeel.h"
#include "ui/themes/Theme.h"

/**
 * TransmissionArea - Context-aware status messages
 * 
 * Shows dynamic messages based on audio analysis and filter state.
 * Matches the enhanced HTML prototype design.
 */
class TransmissionArea : public juce::Component, private juce::Timer
{
public:
    TransmissionArea()
    {
        // Start timer for message updates
        startTimerHz(2); // Update every 500ms
    }
    
    ~TransmissionArea() override
    {
        stopTimer();
    }
    
    /**
     * Update audio level for context-aware messages
     */
    void setAudioLevel(float level) noexcept
    {
        audioLevel = std::clamp(level, 0.0f, 1.0f);
    }
    
    /**
     * Update current shape pair for context
     */
    void setShapePair(const juce::String& shape) noexcept
    {
        currentShape = shape;
    }
    
    /**
     * Update morph position for dynamic messages
     */
    void setMorphPosition(float morph) noexcept
    {
        morphPosition = std::clamp(morph, 0.0f, 1.0f);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Dark background matching HTML prototype
        g.setColour(ModernMuseTheme::panelBackground);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
        
        // Get current state (simple read - already on message thread)
        const float level = audioLevel;
        const juce::String shape = currentShape;
        const float morph = morphPosition;
        
        // Determine message based on context
        juce::String message = getContextualMessage(level, shape, morph);
        
        // Draw message with OLED glow effect
        auto textBounds = bounds.toFloat();
        g.setColour(ModernMuseTheme::mintGreen.withAlpha(0.9f));
        g.setFont(juce::Font("Space Grotesk", 14.0f, juce::Font::bold));
        
        // Add subtle glow
        g.setColour(ModernMuseTheme::mintGreen.withAlpha(0.3f));
        g.drawText(message, textBounds.translated(1, 1), juce::Justification::centred, false);
        
        // Main text
        g.setColour(ModernMuseTheme::mintGreen);
        g.drawText(message, textBounds, juce::Justification::centred, false);
    }
    
    void resized() override
    {
        // Nothing needed - drawn dynamically
    }
    
private:
    void timerCallback() override
    {
        // Trigger repaint for message updates
        repaint();
    }
    
    juce::String getContextualMessage(float level, const juce::String& shape, float morph)
    {
        // No audio: listening state
        if (level < 0.01f)
        {
            if (shape.isEmpty())
                return "Listening...";
            else
                return "Ready for " + shape.toLowerCase();
        }
        
        // Low audio: analyzing state
        if (level < 0.1f)
        {
            return "Analyzing...";
        }
        
        // Medium audio: shape-specific messages
        if (level < 0.3f)
        {
            if (shape == "VOWEL")
                return "Shaping vowels...";
            else if (shape == "BELL")
                return "Resonating...";
            else if (shape == "LOW")
                return "Deepening...";
            else if (shape == "SUB")
                return "Submerging...";
            else
                return "Processing...";
        }
        
        // High audio: poetic messages based on morph
        if (level < 0.6f)
        {
            if (morph < 0.3f)
                return "Forming...";
            else if (morph < 0.7f)
                return "Morphing...";
            else
                return "Transforming...";
        }
        
        // Very high audio: peak messages
        static int messageIndex = 0;
        static const juce::StringArray peakMessages = {
            "Resonating...",
            "Vibrating...",
            "Sustaining...",
            "Peaking...",
            "Glowing..."
        };
        
        return peakMessages[messageIndex++ % peakMessages.size()];
    }
    
    // State (not atomic - juce::String can't be atomic)
    float audioLevel{0.0f};
    juce::String currentShape{""};
    float morphPosition{0.5f};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransmissionArea)
};
