#pragma once

#include <array>
#include <atomic>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

/**
 * HalftoneMouth (Procedural) – JUCE 8.0.10 Best Practice
 *
 * New approach: NO external PNG assets. We render a 16×6 dot matrix procedurally.
 * Each vowel shape (AA, AH, EE, OH, OO) is a template of target radii (0..1) for each cell.
 * Morphing blends between two active templates derived from DSP vowel state + morph value.
 * Audio level animates global brightness and subtle per-dot breathing.
 * Micro-expressions & transient pulses layer organic life without allocating.
 * Optional OpenGL acceleration: attach() enables GPU path; CPU fallback is default.
 */
class HalftoneMouth : public juce::Component,
                      private juce::Timer,
                      public juce::OpenGLRenderer
{
public:
    enum class Vowel { AA,
        AH,
        EE,
        OH,
        OO };

    HalftoneMouth (bool useOpenGL = false)
    {
        initialiseShapeTemplates();
        startTimerHz (60);
        if (useOpenGL)
            attachOpenGL();
    }

    ~HalftoneMouth() override
    {
        if (openGLAttached_)
            detachOpenGL();
        stopTimer();
    }

    // === Thread-safe setters (audio thread writes) ===
    void setVowel (Vowel v) { targetVowel_.store (static_cast<int> (v), std::memory_order_relaxed); }
    void setAudioLevel (float level) { audioLevel_.store (level, std::memory_order_relaxed); }
    void setMorph (float morph) { morph_.store (juce::jlimit (0.0f, 1.0f, morph), std::memory_order_relaxed); }
    void setTintColor (juce::Colour c)
    {
        tint_ = c;
        repaint();
    }
    void setJitter (bool enabled) { jitterActive_.store (enabled, std::memory_order_relaxed); }
    void triggerGlitchFrame() { glitchFrames_.store (2, std::memory_order_relaxed); }

    // === Public control for GPU path ===
    void attachOpenGL()
    {
        if (!openGLAttached_)
        {
            openGLContext_.setRenderer (this);
            openGLContext_.attachTo (*this);
            openGLContext_.setContinuousRepainting (true);
            openGLAttached_ = true;
        }
    }
    void detachOpenGL()
    {
        if (openGLAttached_)
        {
            openGLContext_.detach();
            openGLAttached_ = false;
        }
    }

    void paint (juce::Graphics& g) override
    {
        if (openGLAttached_)
            return; // GPU path handled in renderOpenGL()
        renderCPU (g);
    }

    void resized() override {}

    // === OpenGLRenderer callbacks ===
    void newOpenGLContextCreated() override {}
    void renderOpenGL() override
    {
        juce::OpenGLHelpers::clear (juce::Colours::black);
        juce::Graphics g (*juce::Image::null); // We will fallback to immediate mode via helper (simplified)
        // For simplicity in this prototype, reuse CPU renderer logic on a temporary Image.
        // A production path would write a GLSL shader using per-vertex circle impostors.
        renderCPU (glRenderGraphics_);
    }
    void openGLContextClosing() override {}

private:
    // === Core procedural render ===
    void renderCPU (juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const int cols = kCols;
        const int rows = kRows;

        // Animation / state snapshots
        float audio = audioLevel_.load (std::memory_order_relaxed);
        float morphPos = morph_.load (std::memory_order_relaxed);
        Vowel tv = static_cast<Vowel> (targetVowel_.load (std::memory_order_relaxed));
        int glitchLeft = glitchFrames_.load (std::memory_order_relaxed);
        bool jitter = jitterActive_.load (std::memory_order_relaxed);

        // Resolve current + target vowel for template selection
        Vowel currentVowel = static_cast<Vowel> (currentVowel_);

        // Morphing across vowel templates (source→target when crossfadeActive)
        if (tv != currentVowel)
        {
            crossfadeAlpha_ += 0.08f;
            if (crossfadeAlpha_ >= 1.0f)
            {
                crossfadeAlpha_ = 0.0f;
                currentVowel_ = static_cast<int> (tv);
            }
        }
        else if (crossfadeAlpha_ > 0.0f)
        {
            crossfadeAlpha_ = juce::jmax (0.0f, crossfadeAlpha_ - 0.05f);
        }

        // Background (pure black inside scrying mirror region)
        g.setColour (juce::Colours::black);
        g.fillRect (bounds);

        // Layout constants
        const float gridW = bounds.getWidth();
        const float gridH = bounds.getHeight();
        const float cellW = gridW / (float) cols;
        const float cellH = gridH / (float) rows;

        // Breathing + transient pulses
        float breathScale = 1.0f + std::sin (breathPhase_) * 0.02f;
        float transientScale = transientPulseScale_;
        float globalScale = breathScale * transientScale;

        // Iterate grid
        const auto& baseTemplate = shapes_[static_cast<int> (currentVowel_)];
        const auto& targetTemplate = shapes_[static_cast<int> (tv)];

        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                int idx = r * cols + c;
                float srcRadius = baseTemplate[idx];
                float dstRadius = targetTemplate[idx];

                // Crossfade between vowels + morph modulation (morph influences openness weighting)
                float vowelBlend = crossfadeAlpha_ * dstRadius + (1.0f - crossfadeAlpha_) * srcRadius;
                float opennessMod = juce::jmap (morphPos, 0.0f, 1.0f, 0.85f, 1.15f);
                float finalRadius = vowelBlend * opennessMod;

                // Audio brightness scaling (dot fill intensity)
                float brightness = 0.6f + audio * 0.4f; // 0.6 .. 1.0

                // Micro-expression adjustments
                if (microExpressionFrames_ > 0)
                {
                    switch (microExpressionType_)
                    {
                        case 0: // blink -> squash vertically
                            finalRadius *= (r == rows / 2 ? 0.4f : 0.8f);
                            break;
                        case 1: // sigh -> slightly larger overall
                            finalRadius *= 1.05f;
                            break;
                        case 2: // asymmetry -> shift columns right side bigger
                            if (c > cols / 2)
                                finalRadius *= 1.08f;
                            break;
                    }
                }

                if (glitchLeft > 0)
                {
                    // Glitch: random skip / random scale
                    auto& rand = juce::Random::getSystemRandom();
                    if (rand.nextFloat() < 0.15f)
                        continue;
                    finalRadius *= rand.nextFloat() * 1.5f;
                }
                else if (jitter)
                {
                    finalRadius *= 0.9f + juce::Random::getSystemRandom().nextFloat() * 0.2f;
                }

                // Convert radius (0..1) into pixel diameter relative to cell size
                float maxDot = juce::jmin (cellW, cellH) * 0.9f * globalScale;
                float diameter = maxDot * finalRadius;
                if (diameter < 0.8f)
                    continue; // Skip near-zero dots for clarity

                float cx = bounds.getX() + c * cellW + cellW * 0.5f;
                float cy = bounds.getY() + r * cellH + cellH * 0.5f;

                // Color: tint with brightness
                juce::Colour dotColour = tint_.withMultipliedAlpha (brightness);
                g.setColour (dotColour);
                g.fillEllipse (cx - diameter * 0.5f, cy - diameter * 0.5f, diameter, diameter);
            }
        }
    }

    void timerCallback() override
    {
        // Smooth audio level
        float lvl = audioLevel_.load (std::memory_order_relaxed);
        smoothedAudio_ += (lvl - smoothedAudio_) * 0.15f;

        // Breathing phase
        breathPhase_ += 0.02f;
        if (breathPhase_ > juce::MathConstants<float>::twoPi)
            breathPhase_ -= juce::MathConstants<float>::twoPi;

        // Transient detection
        float delta = lvl - prevAudio_;
        if (delta > 0.18f)
        {
            transientPulseFrames_ = 5;
        }
        prevAudio_ = lvl;

        if (transientPulseFrames_ > 0)
        {
            transientPulseScale_ = 1.0f + transientPulseFrames_ * 0.04f; // up to ~1.2
            --transientPulseFrames_;
        }
        else
            transientPulseScale_ = 1.0f;

        // Micro-expression scheduling
        if (++framesSinceExpression_ > 480) // ~8s
        {
            auto& rand = juce::Random::getSystemRandom();
            if (rand.nextFloat() < 0.35f)
            {
                microExpressionFrames_ = 6;
                microExpressionType_ = rand.nextInt (3);
            }
            framesSinceExpression_ = 0;
        }
        if (microExpressionFrames_ > 0)
            --microExpressionFrames_;

        // Glitch frame countdown
        int g = glitchFrames_.load (std::memory_order_relaxed);
        if (g > 0)
            glitchFrames_.store (g - 1, std::memory_order_relaxed);

        repaint();
    }

    void initialiseShapeTemplates()
    {
        // Each template: 16*6 = 96 floats (0..1). 0 means no dot, higher means larger relative radius.
        // Templates kept intentionally stylised, not anatomically perfect.
        auto make = [&] (std::initializer_list<float> values) {
            std::array<float, kTotal> arr{}; int i=0; for (auto v: values) arr[i++] = v; return arr; };

        // Helper lambdas for pattern generation
        auto ellipse = [&] (float xNorm, float yNorm, float rx, float ry) {
            float dx = (xNorm - 0.5f) / rx;
            float dy = (yNorm - 0.5f) / ry;
            float d = dx * dx + dy * dy;
            if (d <= 1.0f)
                return 1.0f - d; // radial falloff
            return 0.0f;
        };

        auto buildTemplate = [&] (float rx, float ry, float minCut) {
            std::array<float, kTotal> arr {};
            for (int r = 0; r < kRows; ++r)
            {
                for (int c = 0; c < kCols; ++c)
                {
                    float x = (c + 0.5f) / kCols;
                    float y = (r + 0.5f) / kRows;
                    float v = ellipse (x, y, rx, ry);
                    if (v < minCut)
                        v = 0.0f; // carve opening void
                    arr[r * kCols + c] = v;
                }
            }
            return arr;
        };

        // Distinct shapes:
        shapes_[(int) Vowel::AA] = buildTemplate (0.32f, 0.50f, 0.15f); // tall open
        shapes_[(int) Vowel::AH] = buildTemplate (0.38f, 0.42f, 0.20f); // neutral
        shapes_[(int) Vowel::EE] = buildTemplate (0.55f, 0.25f, 0.22f); // wide horizontal
        shapes_[(int) Vowel::OH] = buildTemplate (0.40f, 0.40f, 0.18f); // near circle
        shapes_[(int) Vowel::OO] = buildTemplate (0.22f, 0.30f, 0.10f); // small tight
    }

    // === Constants ===
    static constexpr int kCols = 16;
    static constexpr int kRows = 6;
    static constexpr int kTotal = kCols * kRows;

    // Templates
    std::array<std::array<float, kTotal>, 5> shapes_ {}; // AA, AH, EE, OH, OO

    // Atomics (audio thread writes)
    std::atomic<int> targetVowel_ { static_cast<int> (Vowel::AH) };
    std::atomic<float> audioLevel_ { 0.0f };
    std::atomic<float> morph_ { 0.5f };
    std::atomic<bool> jitterActive_ { false };
    std::atomic<int> glitchFrames_ { 0 }; // meltdown effect

    // UI thread state
    int currentVowel_ = static_cast<int> (Vowel::AH);
    float crossfadeAlpha_ = 0.0f;
    float smoothedAudio_ = 0.0f;
    float prevAudio_ = 0.0f;
    float breathPhase_ = 0.0f;
    int transientPulseFrames_ = 0;
    float transientPulseScale_ = 1.0f;
    int framesSinceExpression_ = 0;
    int microExpressionFrames_ = 0;
    int microExpressionType_ = 0; // 0 blink,1 sigh,2 asymmetry

    // Visual style
    juce::Colour tint_ { juce::Colours::white };

    // OpenGL context (optional)
    juce::OpenGLContext openGLContext_;
    bool openGLAttached_ = false;
    juce::Graphics glRenderGraphics_ { *juce::Image::null }; // placeholder simplification

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HalftoneMouth)
};
