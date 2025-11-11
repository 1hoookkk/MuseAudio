#pragma once

#include <array>
#include <complex>
#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * ZPlaneLEDDisplay - Lo-Fi Z-Plane Analyzer
 *
 * 16x6 LED dot-matrix display showing:
 * - Pixelated unit circle
 * - Two moving pole clusters (from DSP)
 * - 10-12 FPS refresh rate with pixel ghosting
 * - High-contrast off-white/mint LEDs
 *
 * This is the diagnostic display from a 1990s rack unit.
 */
class ZPlaneLEDDisplay : public juce::Component,
                         private juce::Timer
{
public:
    ZPlaneLEDDisplay()
    {
        // 10 FPS for lo-fi diagnostic feel
        startTimerHz(10);
        initializeCircleTemplate();
    }

    ~ZPlaneLEDDisplay() override
    {
        stopTimer();
    }

    // Thread-safe setters (called from audio/UI thread)
    void setPolePositions(std::complex<float> pole1, std::complex<float> pole2)
    {
        targetPole1_ = pole1;
        targetPole2_ = pole2;
    }

    void setLEDColor(juce::Colour color)
    {
        ledColor_ = color;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Pure black background (diagnostic display)
        g.setColour(juce::Colours::black);
        g.fillRect(bounds);

        const int cols = kCols;
        const int rows = kRows;
        const float cellW = bounds.getWidth() / (float)cols;
        const float cellH = bounds.getHeight() / (float)rows;

        // Map poles to grid coordinates
        auto pole1Grid = complexToGrid(currentPole1_);
        auto pole2Grid = complexToGrid(currentPole2_);

        // Render each LED cell
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                int idx = r * cols + c;
                float brightness = 0.0f;

                // Check if this cell should be lit

                // 1. Unit circle
                if (circleTemplate_[idx] > 0.0f)
                {
                    brightness = juce::jmax(brightness, 0.7f);
                }

                // 2. Pole 1 cluster (3x3 area)
                if (isNearPole(c, r, pole1Grid.first, pole1Grid.second))
                {
                    brightness = 1.0f;
                }

                // 3. Pole 2 cluster (3x3 area)
                if (isNearPole(c, r, pole2Grid.first, pole2Grid.second))
                {
                    brightness = 1.0f;
                }

                // Apply ghosting effect from previous frame
                float ghosting = ghostBuffer_[idx] * 0.4f;
                brightness = juce::jmax(brightness, ghosting);

                if (brightness < 0.1f)
                    continue; // Skip dark LEDs

                // Draw LED dot
                float cx = bounds.getX() + c * cellW + cellW * 0.5f;
                float cy = bounds.getY() + r * cellH + cellH * 0.5f;
                float dotSize = juce::jmin(cellW, cellH) * 0.7f;

                // LED glow (bloom effect)
                g.setColour(ledColor_.withAlpha(brightness * 0.3f));
                g.fillEllipse(cx - dotSize, cy - dotSize, dotSize * 2, dotSize * 2);

                // Core LED
                g.setColour(ledColor_.withAlpha(brightness));
                g.fillEllipse(cx - dotSize * 0.5f, cy - dotSize * 0.5f, dotSize, dotSize);

                // Update ghost buffer
                ghostBuffer_[idx] = brightness;
            }
        }
    }

    void resized() override {}

private:
    void timerCallback() override
    {
        // Snap to target positions at 10 FPS (diagnostic update rate)
        currentPole1_ = targetPole1_;
        currentPole2_ = targetPole2_;

        // Decay ghost buffer
        for (int i = 0; i < kTotal; ++i)
        {
            ghostBuffer_[i] *= 0.7f; // Ghosting decay
        }

        repaint();
    }

    void initializeCircleTemplate()
    {
        // Pre-calculate which LEDs belong to unit circle on 16x6 grid
        const float centerX = kCols / 2.0f;
        const float centerY = kRows / 2.0f;
        const float radius = kRows / 2.0f - 0.5f; // Slightly inset

        for (int r = 0; r < kRows; ++r)
        {
            for (int c = 0; c < kCols; ++c)
            {
                float dx = c + 0.5f - centerX;
                float dy = r + 0.5f - centerY;
                float dist = std::sqrt(dx * dx + dy * dy);

                // Circle edge (1 pixel wide)
                if (std::abs(dist - radius) < 0.8f)
                {
                    circleTemplate_[r * kCols + c] = 1.0f;
                }
                else
                {
                    circleTemplate_[r * kCols + c] = 0.0f;
                }
            }
        }
    }

    // Convert complex pole (-1 to +1 range) to grid coordinates (0 to cols/rows)
    std::pair<int, int> complexToGrid(std::complex<float> pole) const
    {
        float x = (pole.real() + 1.0f) * 0.5f; // -1..1 -> 0..1
        float y = (1.0f - pole.imag()) * 0.5f; // -1..1 -> 0..1, flip Y

        int gridX = (int)(x * kCols);
        int gridY = (int)(y * kRows);

        return {juce::jlimit(0, kCols - 1, gridX), juce::jlimit(0, kRows - 1, gridY)};
    }

    // Check if grid cell (c, r) is within pole cluster area
    bool isNearPole(int c, int r, int poleC, int poleR) const
    {
        int dc = std::abs(c - poleC);
        int dr = std::abs(r - poleR);
        return (dc <= 1 && dr <= 1); // 3x3 cluster
    }

    // === Constants ===
    static constexpr int kCols = 16;
    static constexpr int kRows = 6;
    static constexpr int kTotal = kCols * kRows;

    // State
    std::complex<float> targetPole1_{0.5f, 0.3f};
    std::complex<float> targetPole2_{0.5f, -0.3f};
    std::complex<float> currentPole1_{0.5f, 0.3f};
    std::complex<float> currentPole2_{0.5f, -0.3f};

    // Rendering
    std::array<float, kTotal> circleTemplate_{};
    std::array<float, kTotal> ghostBuffer_{}; // Pixel ghosting trails
    juce::Colour ledColor_{0xFFd8f3dc}; // Off-white/mint

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZPlaneLEDDisplay)
};
