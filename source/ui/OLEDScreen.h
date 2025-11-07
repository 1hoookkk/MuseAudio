#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "OLEDLookAndFeel.h"

/**
 * OLEDScreen - Simple audio-reactive line visualization
 * 
 * Replaces the complex LED matrix with a clean horizontal line
 * that responds to audio levels and filter parameters.
 * 
 * Design matches HTML prototype:
 * - Black background (#000000)
 * - Mint green horizontal line (#d8f3dc) with glow
 * - Line responds to audio RMS (brightness/position)
 * - Line responds to morph parameter (subtle position shift)
 */
class OLEDScreen : public juce::Component, private juce::Timer
{
public:
    OLEDScreen()
    {
        // Start timer for 30fps updates (same as original)
        startTimerHz(30);
    }
    
    ~OLEDScreen() override
    {
        stopTimer();
    }
    
    /**
     * Update audio level from audio thread (atomic)
     */
    void setAudioLevel(float level) noexcept
    {
        audioLevel.store(std::clamp(level, 0.0f, 1.0f));
    }
    
    /**
     * Update morph position from UI thread
     */
    void setMorphPosition(float morph) noexcept
    {
        morphPosition.store(std::clamp(morph, 0.0f, 1.0f));
    }
    
    /**
     * Update vowel shape for visualization (UI thread only)
     */
    void setVowelShape(const juce::String& shape) noexcept
    {
        currentShape = shape;
    }
    
    void paint(juce::Graphics& g) override
    {
        // Black OLED background
        g.fillAll(juce::Colours::black);
        
        // Get current values atomically
        const float level = audioLevel.load();
        const float morph = morphPosition.load();
        const juce::String shape = currentShape; // Not atomic
        
        // Calculate line properties based on audio and parameters
        const auto bounds = getLocalBounds().toFloat();
        const float centerY = bounds.getCentreY();
        const float lineWidth = bounds.getWidth() * 0.8f;
        const float lineStartX = (bounds.getWidth() - lineWidth) * 0.5f;
        
        // Audio-reactive properties
        const float brightness = 0.3f + level * 0.7f;  // 30% to 100% brightness
        const float thickness = 2.0f + level * 2.0f;    // 2px to 4px thickness
        const float verticalOffset = (morph - 0.5f) * 20.0f; // ±10px based on morph
        
        // Create mint green color with brightness
        auto baseColor = juce::Colour(OLEDLookAndFeel::MintGreen);
        auto lineColor = baseColor.withBrightness(brightness);
        
        // Draw glow effect for active audio
        if (level > 0.01f)
        {
            // Outer glow
            g.setColour(lineColor.withAlpha(0.1f));
            g.drawRoundedRectangle(
                lineStartX - 5.0f,
                centerY + verticalOffset - thickness - 5.0f,
                lineWidth + 10.0f,
                thickness * 2.0f + 10.0f,
                2.0f,
                3.0f
            );
            
            // Inner glow
            g.setColour(lineColor.withAlpha(0.2f));
            g.drawRoundedRectangle(
                lineStartX - 2.0f,
                centerY + verticalOffset - thickness - 2.0f,
                lineWidth + 4.0f,
                thickness * 2.0f + 4.0f,
                1.0f,
                2.0f
            );
        }
        
        // Draw the main line
        g.setColour(lineColor);
        juce::Rectangle<float> lineRect(
            lineStartX,
            centerY + verticalOffset - thickness * 0.5f,
            lineWidth,
            thickness
        );
        
        // Add subtle glow to the line itself
        g.setColour(lineColor.withAlpha(0.8f));
        g.fillRect(lineRect);
        
        // Add bright center line when audio is present
        if (level > 0.05f)
        {
            g.setColour(baseColor.withBrightness(1.0f));
            g.fillRect(
                lineStartX,
                centerY + verticalOffset - 0.5f,
                lineWidth,
                1.0f
            );
        }
        
        // Optional: Show shape name when audio is active
        if (level > 0.1f && shape.isNotEmpty())
        {
            g.setColour(lineColor.withAlpha(0.6f));
            g.setFont(juce::Font("Space Grotesk", 10.0f, juce::Font::plain));
            g.drawText(shape, bounds, juce::Justification::centredBottom, false);
        }
    }
    
    void resized() override
    {
        // Nothing needed - drawn dynamically
    }
    
private:
    void timerCallback() override
    {
        // Trigger repaint at 30fps
        repaint();
    }
    
    // Atomic state from audio thread
    std::atomic<float> audioLevel{0.0f};
    std::atomic<float> morphPosition{0.5f};
    juce::String currentShape{""}; // Not atomic - updated only from UI thread
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OLEDScreen)
};
