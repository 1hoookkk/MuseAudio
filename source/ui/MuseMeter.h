#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include "MuseLookAndFeel.h"

/**
 * MuseMeter - Thread-safe level meter with peak hold.
 *
 * Thread safety:
 * - Audio thread writes to atomics via setLevel()
 * - UI thread reads from timer at 30Hz
 *
 * Features:
 * - Peak + RMS display
 * - Hold indicator (peak hold for 1 second)
 * - Colour zones: green (safe), yellow (hot), red (clip)
 * - Vertical or horizontal orientation
 */
class MuseMeter : public juce::Component,
                  private juce::Timer
{
public:
    enum class Orientation { Vertical, Horizontal };

    MuseMeter(Orientation orientation = Orientation::Horizontal)
        : orientation_(orientation)
    {
        startTimerHz(30); // 30Hz refresh
    }

    ~MuseMeter() override
    {
        stopTimer();
    }

    // === Thread-safe API (call from audio thread) ===
    void setLevel(float newLevel)
    {
        newLevel = juce::jlimit(0.0f, 1.0f, newLevel);
        level_.store(newLevel, std::memory_order_relaxed);
    }

    // === UI Configuration ===
    void setOrientation(Orientation orientation)
    {
        orientation_ = orientation;
        repaint();
    }

    void reset()
    {
        level_.store(0.0f, std::memory_order_relaxed);
        smoothedLevel_ = 0.0f;
        peakHold_ = 0.0f;
        peakHoldFrames_ = 0;
    }

    // === Component Overrides ===
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background track
        g.setColour(MuseTheme::Grey800);
        g.fillRoundedRectangle(bounds, MuseTheme::Radii::sm);

        // Active level bar
        float levelProp = smoothedLevel_;
        juce::Rectangle<float> levelBounds;

        if (orientation_ == Orientation::Horizontal)
        {
            levelBounds = bounds.withWidth(bounds.getWidth() * levelProp);
        }
        else // Vertical
        {
            float levelHeight = bounds.getHeight() * levelProp;
            levelBounds = bounds.withTop(bounds.getBottom() - levelHeight);
        }

        // Colour zones: green → yellow → red
        juce::Colour levelColour;
        if (levelProp < 0.7f)
            levelColour = MuseTheme::Success; // Green (safe)
        else if (levelProp < 0.9f)
            levelColour = MuseTheme::Warning; // Yellow (hot)
        else
            levelColour = MuseTheme::Error; // Red (clip)

        g.setColour(levelColour);
        g.fillRoundedRectangle(levelBounds, MuseTheme::Radii::sm);

        // Peak hold indicator (thin line)
        if (peakHold_ > 0.01f)
        {
            float peakPos;
            if (orientation_ == Orientation::Horizontal)
            {
                peakPos = bounds.getX() + bounds.getWidth() * peakHold_;
                g.setColour(MuseTheme::Cream);
                g.drawLine(peakPos, bounds.getY(), peakPos, bounds.getBottom(), 2.0f);
            }
            else // Vertical
            {
                peakPos = bounds.getBottom() - bounds.getHeight() * peakHold_;
                g.setColour(MuseTheme::Cream);
                g.drawLine(bounds.getX(), peakPos, bounds.getRight(), peakPos, 2.0f);
            }
        }

        // Border
        g.setColour(MuseTheme::Grey700);
        g.drawRoundedRectangle(bounds, MuseTheme::Radii::sm, 1.0f);
    }

private:
    void timerCallback() override
    {
        // Read atomic level
        float newLevel = level_.load(std::memory_order_relaxed);

        // Smooth level (fast attack, slow decay)
        if (newLevel > smoothedLevel_)
            smoothedLevel_ += (newLevel - smoothedLevel_) * 0.8f; // Fast attack
        else
            smoothedLevel_ += (newLevel - smoothedLevel_) * 0.05f; // Slow decay

        // Peak hold logic
        if (smoothedLevel_ > peakHold_)
        {
            peakHold_ = smoothedLevel_;
            peakHoldFrames_ = 30; // Hold for 1 second (30 frames @ 30Hz)
        }
        else if (peakHoldFrames_ > 0)
        {
            --peakHoldFrames_;
        }
        else
        {
            peakHold_ *= 0.95f; // Decay after hold expires
        }

        repaint();
    }

    Orientation orientation_;
    std::atomic<float> level_ { 0.0f };
    float smoothedLevel_ = 0.0f;
    float peakHold_ = 0.0f;
    int peakHoldFrames_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseMeter)
};
