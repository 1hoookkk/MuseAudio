#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "OLEDLookAndFeel.h"
#include "ui/themes/Theme.h"

/**
 * StatusBar - Real-time status information
 * 
 * Shows pole radius, stability, and CPU usage.
 * Matches the enhanced HTML prototype footer.
 */
class StatusBar : public juce::Component, private juce::Timer
{
public:
    StatusBar()
    {
        // Start timer for status updates
        startTimerHz(4); // Update every 250ms
    }
    
    ~StatusBar() override
    {
        stopTimer();
    }
    
    /**
     * Update pole radius from filter state
     */
    void setPoleRadius(float radius) noexcept
    {
        poleRadius.store(std::clamp(radius, 0.0f, 1.0f));
    }
    
    /**
     * Update CPU usage (0.0 to 1.0)
     */
    void setCpuUsage(float cpu) noexcept
    {
        cpuUsage.store(std::clamp(cpu, 0.0f, 1.0f));
    }
    
    /**
     * Update stability status
     */
    void setStability(bool stable) noexcept
    {
        isStable.store(stable);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Dark background with top border
        g.setColour(ModernMuseTheme::oledDarkTeal);
        g.fillRect(bounds);
        
        // Top border line
        g.setColour(ModernMuseTheme::mintGreen.withAlpha(0.2f));
        g.drawHorizontalLine(0, 0.0f, static_cast<float>(bounds.getWidth()));
        
        // Get current values
        const float radius = poleRadius.load();
        const float cpu = cpuUsage.load();
        const bool stable = isStable.load();
        
        // Build status string
        juce::String statusText = juce::String::formatted(
            "POLE RADIUS: %.3f | %s | CPU: %.0f%%",
            radius,
            stable ? "STABLE" : "UNSTABLE",
            cpu * 100.0f
        );
        
        // Draw status text with OLED styling
        g.setColour(ModernMuseTheme::mintGreen.withAlpha(0.8f));
        g.setFont(juce::Font("Space Grotesk", 10.0f, juce::Font::plain));
        
        // Center text vertically
        auto textBounds = bounds.toFloat();
        g.drawText(statusText, textBounds, juce::Justification::centred, false);
        
        // Add stability indicator (green dot if stable, red if not)
        const float dotX = bounds.getWidth() - 60.0f;
        const float dotY = bounds.getCentreY();
        const float dotRadius = 3.0f;
        
        if (stable)
        {
            // Green glowing dot for stable
            g.setColour(ModernMuseTheme::statusGreen.withAlpha(0.8f));
            g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2, dotRadius * 2);
            
            // Glow effect
            g.setColour(ModernMuseTheme::statusGreen.withAlpha(0.3f));
            g.fillEllipse(dotX - dotRadius * 2, dotY - dotRadius * 2, dotRadius * 4, dotRadius * 4);
        }
        else
        {
            // Red blinking dot for unstable
            static bool blinkState = false;
            if ((juce::Time::getMillisecondCounter() / 500) % 2 == 0)
            {
                g.setColour(ModernMuseTheme::statusRed.withAlpha(0.8f));
                g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2, dotRadius * 2);
            }
        }
    }
    
    void resized() override
    {
        // Nothing needed - drawn dynamically
    }
    
private:
    void timerCallback() override
    {
        // Trigger repaint for status updates
        repaint();
    }
    
    // Atomic state
    std::atomic<float> poleRadius{0.5f};
    std::atomic<float> cpuUsage{0.08f}; // Default 8%
    std::atomic<bool> isStable{true};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};
