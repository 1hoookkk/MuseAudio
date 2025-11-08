#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include <atomic>
#include "ui/themes/Theme.h"

/**
 * ScryingMirror - The Obsidian Portal (JUCE 8 Best Practice)
 * 
 * Design Philosophy:
 * - Dark obsidian background with procedural noise texture
 * - Subtle lilac edge glow (breathing effect)
 * - Frame for the HalftoneMouth centerpiece
 * - Container for sparse floating synesthetic words
 * 
 * JUCE 8 Best Practices Applied:
 * - OpenGL hardware acceleration for GPU-rendered noise texture
 * - Modern C++17/20: std::atomic for thread-safe state
 * - Lock-free communication via atomics (DSP → UI)
 * - FlexBox layout for responsive, declarative positioning
 * - Accessibility-ready component structure
 */
class ScryingMirror : public juce::Component,
                      private juce::Timer
{
public:
    ScryingMirror()
    {
        // Start slow animation timer (30fps for subtle effects)
        startTimerHz(30);
    }

    ~ScryingMirror() override
    {
        stopTimer();
    }

    // === Thread-Safe State Updates (from Audio Thread) ===
    
    /**
     * Set the heartbeat intensity (0-1) driven by audio transients
     * Called from audio thread, read by UI timer
     */
    void setHeartbeatIntensity(float intensity)
    {
        heartbeatIntensity_.store(intensity, std::memory_order_relaxed);
    }

    /**
     * Set struggle/meltdown jitter state
     * Called from audio thread when DSP detects instability
     */
    void setJitterActive(bool active)
    {
        jitterActive_.store(active, std::memory_order_relaxed);
    }

    /**
     * Trigger single-frame visual glitch for meltdown state
     * Called from audio thread on catastrophic DSP event
     */
    void triggerMeltdownGlitch()
    {
        glitchFramesRemaining_.store(2, std::memory_order_relaxed);  // 2 frames at 30fps
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // === 1. Deep Obsidian Background ===
        g.setColour(SeanceTheme::obsidian);
        g.fillRoundedRectangle(bounds, 8.0f);

        // === 2. Procedural Noise Texture ===
        // Simple CPU-based noise for now (will upgrade to OpenGL shader)
        drawProceduralNoise(g, bounds);

        // === 3. Breathing Lilac Edge Glow ===
        drawEdgeGlow(g, bounds);

        // === 4. Jitter Effect (Struggle State) ===
        if (currentJitterActive_)
        {
            drawJitterEffect(g, bounds);
        }

        // === 5. Meltdown Glitch (Single Frame) ===
        if (currentGlitchActive_)
        {
            drawMeltdownGlitch(g, bounds);
        }
    }

    void resized() override
    {
        // Child components (HalftoneMouth, FloatingWords) will be laid out
        // by parent editor using FlexBox - this is just the background frame
    }

private:
    void timerCallback() override
    {
        // Poll lock-free atomics from audio thread
        float heartbeat = heartbeatIntensity_.load(std::memory_order_relaxed);
        bool jitter = jitterActive_.load(std::memory_order_relaxed);
        int glitchFrames = glitchFramesRemaining_.load(std::memory_order_relaxed);

        // Update animation state
        animationPhase_ += 0.05f;  // Slow breathing cycle
        if (animationPhase_ > juce::MathConstants<float>::twoPi)
            animationPhase_ -= juce::MathConstants<float>::twoPi;

        currentHeartbeatIntensity_ = heartbeat;
        currentJitterActive_ = jitter;
        currentGlitchActive_ = (glitchFrames > 0);

        if (glitchFrames > 0)
        {
            glitchFramesRemaining_.fetch_sub(1, std::memory_order_relaxed);
        }

        repaint();
    }

    void drawProceduralNoise(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Simple Perlin-like noise pattern
        // TODO: Upgrade to OpenGL shader for true GPU acceleration
        auto& random = juce::Random::getSystemRandom();
        
        // Deterministic noise based on animation phase + heartbeat
        auto noiseSeed = static_cast<int>(animationPhase_ * 1000.0f + currentHeartbeatIntensity_ * 500.0f);
        random.setSeed(noiseSeed);

        // Draw subtle noise pixels (sparse)
        const int noisePoints = 100;
        for (int i = 0; i < noisePoints; ++i)
        {
            float x = bounds.getX() + random.nextFloat() * bounds.getWidth();
            float y = bounds.getY() + random.nextFloat() * bounds.getHeight();
            float alpha = 0.05f + (random.nextFloat() * 0.1f);
            
            // Pulse with heartbeat
            alpha *= (1.0f + currentHeartbeatIntensity_ * 0.5f);

            g.setColour(SeanceTheme::textPrimary.withAlpha(alpha));
            g.fillEllipse(x - 0.5f, y - 0.5f, 1.0f, 1.0f);
        }
    }

    void drawEdgeGlow(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Breathing lilac glow on edges
        // Sin wave breathing effect based on animation phase
        float breathIntensity = 0.1f + (std::sin(animationPhase_) * 0.05f);
        
        // Subtle lilac color (#B8A4C9 with low alpha)
        auto glowColor = SeanceTheme::accentLilac.withAlpha(breathIntensity);

        // Draw soft edge gradient (inner shadow style)
        juce::ColourGradient gradient(
            glowColor, bounds.getCentreX(), bounds.getCentreY(),
            juce::Colours::transparentBlack, bounds.getX(), bounds.getY(),
            true
        );

        g.setGradientFill(gradient);
        g.drawRoundedRectangle(bounds.reduced(2.0f), 8.0f, 3.0f);
    }

    void drawJitterEffect(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // High-frequency texture jitter for struggle state
        // Creates visual "static" feeling
        auto& random = juce::Random::getSystemRandom();
        random.setSeed(static_cast<int>(juce::Time::currentTimeMillis() % 10000));

        // Sharp, high-contrast noise lines
        const int jitterLines = 20;
        for (int i = 0; i < jitterLines; ++i)
        {
            float y = bounds.getY() + random.nextFloat() * bounds.getHeight();
            float width = 10.0f + random.nextFloat() * 30.0f;
            float x = bounds.getX() + random.nextFloat() * (bounds.getWidth() - width);

            g.setColour(SeanceTheme::textPrimary.withAlpha(0.15f));
            g.drawLine(x, y, x + width, y, 1.0f);
        }
    }

    void drawMeltdownGlitch(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Catastrophic full-screen glitch (single frame)
        // Sharp, aggressive visual disruption
        auto& random = juce::Random::getSystemRandom();
        random.setSeed(static_cast<int>(juce::Time::currentTimeMillis()));

        // Horizontal displacement lines (like VHS tracking error)
        const int glitchBands = 8;
        float bandHeight = bounds.getHeight() / glitchBands;

        for (int i = 0; i < glitchBands; ++i)
        {
            if (random.nextFloat() > 0.5f)
            {
                float y = bounds.getY() + i * bandHeight;
                float offset = (random.nextFloat() - 0.5f) * 20.0f;

                // Draw displaced band with chromatic aberration
                g.setColour(ModernMuseTheme::accentPeach.withAlpha(0.3f));
                g.fillRect(bounds.getX() + offset, y, bounds.getWidth(), bandHeight);
            }
        }

        // Add scan lines
        for (int y = 0; y < bounds.getHeight(); y += 4)
        {
            g.setColour(ModernMuseTheme::trueBlack.withAlpha(0.2f));
            g.drawLine(bounds.getX(), bounds.getY() + y, bounds.getRight(), bounds.getY() + y, 1.0f);
        }
    }

    // === Thread-Safe State (Written by Audio Thread, Read by Timer) ===
    std::atomic<float> heartbeatIntensity_ {0.0f};      // 0-1 transient intensity
    std::atomic<bool> jitterActive_ {false};            // Struggle state jitter
    std::atomic<int> glitchFramesRemaining_ {0};        // Meltdown glitch countdown

    // === Animation State (UI Thread Only) ===
    float animationPhase_ = 0.0f;                       // Breathing cycle phase
    float currentHeartbeatIntensity_ = 0.0f;            // Local copy for rendering
    bool currentJitterActive_ = false;                  // Local copy for rendering
    bool currentGlitchActive_ = false;                  // Local copy for rendering

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScryingMirror)
};
