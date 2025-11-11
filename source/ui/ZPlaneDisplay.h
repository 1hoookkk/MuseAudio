#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <complex>

/**
 * ZPlaneDisplay - Simple OLED-style pole visualizer
 *
 * Shows:
 * - Unit circle (thin mint line)
 * - 2 pole positions (solid mint dots with bloom)
 * - Pure black background
 * - 10 FPS update rate (snapping animation)
 */
class ZPlaneDisplay : public juce::Component, private juce::Timer
{
public:
    ZPlaneDisplay()
    {
        // 10 FPS for hardware snap aesthetic
        startTimerHz(10);
    }

    ~ZPlaneDisplay() override
    {
        stopTimer();
    }

    // Thread-safe setters (called from audio thread)
    void setPolePositions(std::complex<float> pole1, std::complex<float> pole2)
    {
        targetPole1_ = pole1;
        targetPole2_ = pole2;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Pure black OLED background
        g.fillAll(juce::Colour(0xFF000000));

        // Calculate center and radius
        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getCentreY();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;

        // Mint phosphor color
        auto mint = juce::Colour(0xFFd8f3dc);

        // Draw unit circle with subtle glow
        drawCircleWithBloom(g, centerX, centerY, radius, mint, 1.0f);

        // Draw pole 1
        auto pole1Screen = complexToScreen(currentPole1_, centerX, centerY, radius);
        drawPoleWithBloom(g, pole1Screen.x, pole1Screen.y, mint);

        // Draw pole 2
        auto pole2Screen = complexToScreen(currentPole2_, centerX, centerY, radius);
        drawPoleWithBloom(g, pole2Screen.x, pole2Screen.y, mint);
    }

private:
    void timerCallback() override
    {
        // Snap to target positions (10 FPS)
        currentPole1_ = targetPole1_;
        currentPole2_ = targetPole2_;
        repaint();
    }

    void drawCircleWithBloom(juce::Graphics& g, float cx, float cy, float r,
                             juce::Colour color, float thickness)
    {
        // Outer bloom (soft glow)
        g.setColour(color.withAlpha(0.2f));
        g.drawEllipse(cx - r - 2, cy - r - 2,
                     (r + 2) * 2, (r + 2) * 2, thickness + 2);

        // Inner bloom
        g.setColour(color.withAlpha(0.4f));
        g.drawEllipse(cx - r - 1, cy - r - 1,
                     (r + 1) * 2, (r + 1) * 2, thickness + 1);

        // Sharp core
        g.setColour(color);
        g.drawEllipse(cx - r, cy - r, r * 2, r * 2, thickness);
    }

    void drawPoleWithBloom(juce::Graphics& g, float x, float y, juce::Colour color)
    {
        const float dotRadius = 4.0f;

        // Outer bloom (soft glow)
        g.setColour(color.withAlpha(0.15f));
        g.fillEllipse(x - dotRadius - 4, y - dotRadius - 4,
                     (dotRadius + 4) * 2, (dotRadius + 4) * 2);

        // Mid bloom
        g.setColour(color.withAlpha(0.3f));
        g.fillEllipse(x - dotRadius - 2, y - dotRadius - 2,
                     (dotRadius + 2) * 2, (dotRadius + 2) * 2);

        // Sharp core dot
        g.setColour(color);
        g.fillEllipse(x - dotRadius, y - dotRadius, dotRadius * 2, dotRadius * 2);
    }

    juce::Point<float> complexToScreen(std::complex<float> z, float cx, float cy, float r)
    {
        // Map complex plane to screen coordinates
        float x = cx + z.real() * r;
        float y = cy - z.imag() * r;  // Flip Y (screen Y goes down)
        return {x, y};
    }

    // Current pole positions (UI thread)
    std::complex<float> currentPole1_ {0.5f, 0.5f};
    std::complex<float> currentPole2_ {0.5f, -0.5f};

    // Target pole positions (set from audio thread)
    std::complex<float> targetPole1_ {0.5f, 0.5f};
    std::complex<float> targetPole2_ {0.5f, -0.5f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZPlaneDisplay)
};
