#pragma once

#include <array>
#include <atomic>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/AuthenticShapeLoader.h"

/**
 * HalftoneMouth (Procedural) – JUCE 8.0.10 Best Practice
 *
 * NO external PNG assets. We render procedurally from AUTHENTIC EMU Z-plane pole data.
 * Each vowel shape (AA, AH, EE, OH, OO) is a template of target radii (0..1) for each cell.
 * Morphing blends between two active templates derived from DSP vowel state + morph value.
 * Audio level animates global brightness and subtle per-dot breathing.
 * Micro-expressions & transient pulses layer organic life without allocating.
 *
 * SACRED AESTHETIC LAW:
 * - 10 FPS update cadence (haunted hardware stutter) enforced by PluginEditor
 * - Double-pass phosphor bloom (glow layer + sharp dots)
 * - Occasional controlled flicker (broken hardware feel)
 * - NEVER smooth this to 60 FPS - the stutter IS the soul
 */
class HalftoneMouth : public juce::Component
{
public:
    enum class Style { LEDGrid, LipHalftone }; // LipHalftone = filled lip/no teeth
    enum class Vowel { AA,
        AH,
        EE,
        OH,
        OO };

    HalftoneMouth()
    {
        initialiseShapeTemplates();
        setJitter(true);  // Enable subtle hardware LED flicker
        // No internal timer - PluginEditor drives 10 FPS updates via triggerUpdate()
    }

    ~HalftoneMouth() override = default;

    // === Thread-safe setters (audio thread writes) ===
    void setPair (int pairIndex) { pair_.store (juce::jlimit (0, 3, pairIndex), std::memory_order_relaxed); }
    void setAudioLevel (float level) { audioLevel_.store (level, std::memory_order_relaxed); }
    void setMorph (float morph) { morph_.store (juce::jlimit (0.0f, 1.0f, morph), std::memory_order_relaxed); }
    void setTintColor (juce::Colour c)
    {
        tint_ = c;
        repaint();
    }
    void setJitter (bool enabled) { jitterActive_.store (enabled, std::memory_order_relaxed); }
    void triggerGlitchFrame() { glitchFrames_.store (2, std::memory_order_relaxed); }
    void setIntensity (float intensity) { intensity_.store (juce::jlimit (0.0f, 1.0f, intensity), std::memory_order_relaxed); }
    void setStyle (Style s) { style_.store (static_cast<int>(s), std::memory_order_relaxed); repaint(); }

    // Phase 1: Direct pole visualization - bypass vowel templates
    void setDotPattern(const std::array<float, 96>& pattern)
    {
        directDotPattern_ = pattern;
        useDirectPattern_.store(true, std::memory_order_relaxed);
    }

    void clearDotPattern()
    {
        useDirectPattern_.store(false, std::memory_order_relaxed);
    }

    // Called by PluginEditor at 10 FPS to trigger animation update + repaint
    void triggerUpdate()
    {
        updateAnimation();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        renderCPU (g);
    }

    void resized() override {}

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
        int pairIndex = pair_.load (std::memory_order_relaxed);
        int glitchLeft = glitchFrames_.load (std::memory_order_relaxed);
        bool jitter = jitterActive_.load (std::memory_order_relaxed);

        // Continuous vowel blending based on pair + morph (no discrete quantization)
        // VOWEL (0): AA → AH → EE (3-stage)
        // BELL (1): OH → OO (2-stage)
        // LOW (2): AA → OO (2-stage, wide → narrow)
        // SUB (3): AH (static, sub-bass has no formants)

        // Background (lime green for LipHalftone, black for LED grid)
        const bool useLip = static_cast<Style>(style_.load(std::memory_order_relaxed)) == Style::LipHalftone;
        g.setColour(useLip ? juce::Colour(0xFF9FFF9F) : juce::Colours::black);
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

        // Calculate continuous vowel weights based on pair + morph
        float aaWeight = 0.0f, ahWeight = 0.0f, eeWeight = 0.0f, ohWeight = 0.0f, ooWeight = 0.0f;

        switch (pairIndex)
        {
            case 0: // VOWEL: AA → AH → EE (3-stage)
                if (morphPos < 0.5f)
                {
                    // AA → AH
                    aaWeight = 1.0f - morphPos * 2.0f;
                    ahWeight = morphPos * 2.0f;
                }
                else
                {
                    // AH → EE
                    ahWeight = 1.0f - (morphPos - 0.5f) * 2.0f;
                    eeWeight = (morphPos - 0.5f) * 2.0f;
                }
                break;
            case 1: // BELL: OH → OO
                ohWeight = 1.0f - morphPos;
                ooWeight = morphPos;
                break;
            case 2: // LOW: AA (wide) → OO (narrow)
                aaWeight = 1.0f - morphPos;
                ooWeight = morphPos;
                break;
            case 3: // SUB: AH (static, sub-bass has no formants)
            default:
                ahWeight = 1.0f;
                break;
        }

        // Iterate grid (LED mode uses authentic dot templates)
        const auto& aaTemplate = shapes_[static_cast<int> (Vowel::AA)];
        const auto& ahTemplate = shapes_[static_cast<int> (Vowel::AH)];
        const auto& eeTemplate = shapes_[static_cast<int> (Vowel::EE)];
        const auto& ohTemplate = shapes_[static_cast<int> (Vowel::OH)];
        const auto& ooTemplate = shapes_[static_cast<int> (Vowel::OO)];

        // useLip already declared above for background color (line 105)
        if (!useLip)
        {
            // Phase 1: Check if using direct pole visualization
            bool useDirect = useDirectPattern_.load(std::memory_order_relaxed);

            for (int r = 0; r < rows; ++r)
            {
                for (int c = 0; c < cols; ++c)
                {
                    int idx = r * cols + c;

                    // Phase 1: Use direct pole pattern OR fallback to vowel templates
                    float blendedRadius = useDirect
                        ? directDotPattern_[idx]
                        : (aaWeight * aaTemplate[idx]
                           + ahWeight * ahTemplate[idx]
                           + eeWeight * eeTemplate[idx]
                           + ohWeight * ohTemplate[idx]
                           + ooWeight * ooTemplate[idx]);

                    float finalRadius = blendedRadius;
                    float brightness = 0.3f + audio * 0.7f; // 0.3 .. 1.0

                    // PHASE 2: Phosphor decay (CRT persistence - slow fade after peak)
                    float currentIntensity = brightness * finalRadius;
                    auto& dotState = dotStates_[idx];

                    // Fast attack (instant), slow decay (exponential fade)
                    if (currentIntensity > dotState.phosphor)
                        dotState.phosphor = currentIntensity;
                    else
                        dotState.phosphor *= 0.92f;  // Decay per frame (10 FPS)

                    if (microExpressionFrames_ > 0)
                    {
                        switch (microExpressionType_)
                        {
                            case 0: finalRadius *= (r == rows / 2 ? 0.4f : 0.8f); break; // blink
                            case 1: finalRadius *= 1.05f; break;                             // sigh
                            case 2: if (c > cols / 2) finalRadius *= 1.08f; break;           // asymmetry
                        }
                    }

                    if (glitchLeft > 0)
                    {
                        auto& rand = juce::Random::getSystemRandom();
                        if (rand.nextFloat() < 0.15f) continue;
                        finalRadius *= rand.nextFloat() * 1.5f;
                    }
                    else if (jitter)
                    {
                        finalRadius *= 0.95f + juce::Random::getSystemRandom().nextFloat() * 0.10f;
                    }

                    float maxDot = juce::jmin (cellW, cellH) * 0.45f * globalScale; // Small LED pixels with visible gaps
                    float diameter = maxDot * finalRadius;
                    if (diameter < 0.8f) continue;

                    float cx = bounds.getX() + c * cellW + cellW * 0.5f;
                    float cy = bounds.getY() + r * cellH + cellH * 0.5f;

                    // PHASE 2: Phosphor glow layer (CRT persistence trail)
                    if (dotState.phosphor > 0.1f)
                    {
                        float glowSize = diameter * 1.6f;
                        float glowAlpha = dotState.phosphor * 0.25f;
                        g.setColour(tint_.withMultipliedAlpha(glowAlpha));
                        g.fillEllipse(cx - glowSize * 0.5f, cy - glowSize * 0.5f, glowSize, glowSize);
                    }

                    // Sharp dot (main render)
                    juce::Colour dotColour = tint_.withMultipliedAlpha (brightness);
                    g.setColour (dotColour);
                    g.fillEllipse (cx - diameter * 0.5f, cy - diameter * 0.5f, diameter, diameter);
                }
            }
        }
        else
        {
            // LipHalftone mode: DENSE almond lip with superellipse SDF (thousands of dots)
            const int lipCols = 140;  // Much denser grid for reference image look
            const int lipRows = 58;
            const float lipCellW = gridW / (float) lipCols;
            const float lipCellH = gridH / (float) lipRows;

            // Superellipse helper (almond shape with subtle pointy corners)
            auto superellipseF = [](float x, float y, float ax, float ay, float n)
            {
                return std::pow(std::abs(x) / ax, n) + std::pow(std::abs(y) / ay, n); // <=1 inside
            };

            // Smooth step for edge falloff
            auto smoothstep = [](float a, float b, float v)
            {
                float t = juce::jlimit(0.0f, 1.0f, (v - a) / (b - a));
                return t * t * (3.0f - 2.0f * t);
            };

            const auto interp = [&](float aa, float ah, float ee, float oh, float oo)
            {
                return aaWeight * aa + ahWeight * ah + eeWeight * ee + ohWeight * oh + ooWeight * oo;
            };

            const float widthRatio  = interp(0.88f, 0.70f, 0.92f, 0.60f, 0.48f);
            const float heightRatio = interp(0.58f, 0.50f, 0.38f, 0.68f, 0.62f);

            const float intensity = juce::jlimit(0.0f, 1.0f, intensity_.load(std::memory_order_relaxed));
            const float heightGain = juce::jmap(intensity, 0.0f, 1.0f, 0.85f, 1.15f);

            // Clamp proportions for stable almond look
            const float rx = (gridW * 0.48f) * juce::jlimit(0.80f, 1.05f, widthRatio);
            const float ry = (gridH * 0.33f) * juce::jlimit(0.90f, 1.15f, heightRatio * heightGain);
            const float cx0 = bounds.getCentreX();
            const float cy0 = bounds.getCentreY();

            const float brightnessBase = 0.32f + audio * 0.68f;

            // DOUBLE-PASS PHOSPHOR BLOOM (sacred haunted aesthetic)
            for (int pass = 0; pass < 2; ++pass)
            {
                bool isGlowPass = (pass == 0);

                for (int r = 0; r < lipRows; ++r)
                {
                    for (int c = 0; c < lipCols; ++c)
                    {
                        const float px = bounds.getX() + c * lipCellW + lipCellW * 0.5f;
                        const float py = bounds.getY() + r * lipCellH + lipCellH * 0.5f;

                        const float nx = (px - cx0) / rx;
                        const float ny = (py - cy0) / ry;

                        // Superellipse test (almond shape, n=2.6 adds subtle corner point)
                        const float nExp = 2.6f;
                        const float f = superellipseF(nx, ny, 1.0f, 1.0f, nExp);
                        if (f > 1.0f) continue;

                        // Edge-weighted: bigger dots near rim, smaller in center
                        const float edge = smoothstep(0.70f, 1.00f, f);  // 0 at center, 1 at edge
                        float dotScale = 1.0f - std::pow(edge, 0.55f);    // Strong edge emphasis

                        // Vertical taper (thinner at top/bottom)
                        const float lipProfile = 1.0f - juce::jlimit(0.0f, 1.0f, std::abs(ny) * 0.75f);
                        dotScale *= 0.85f + 0.15f * lipProfile;

                        // Horizontal end pinch (sharper corners like almond)
                        const float pinch = 1.0f - smoothstep(0.82f, 1.0f, std::abs(nx));
                        dotScale *= 0.85f + 0.15f * pinch;

                        if (microExpressionFrames_ > 0)
                        {
                            if (microExpressionType_ == 0)
                                dotScale *= (std::abs(ny) < 0.2f ? 0.6f : 1.0f); // blink
                            else if (microExpressionType_ == 1)
                                dotScale *= 1.03f; // sigh = slightly larger
                            else if (microExpressionType_ == 2 && c > lipCols / 2)
                                dotScale *= 1.05f; // asymmetry
                        }

                        if (isGlowPass)
                        {
                            // Pass 0: Phosphor bloom (larger, dimmer)
                            float glowSize = juce::jmin(lipCellW, lipCellH) * 1.3f * globalScale * dotScale;
                            if (glowSize < 0.6f) continue;

                            float glowAlpha = brightnessBase * 0.18f * (1.0f - edge);  // Brighter toward center
                            g.setColour(tint_.withMultipliedAlpha(glowAlpha));
                            g.fillEllipse(px - glowSize * 0.5f, py - glowSize * 0.5f, glowSize, glowSize);
                        }
                        else
                        {
                            // Pass 1: Sharp dots with controlled flicker
                            if (jitter)
                                dotScale *= 0.97f + juce::Random::getSystemRandom().nextFloat() * 0.06f;

                            // Occasional flicker (broken hardware aesthetic)
                            float flicker = 1.0f;
                            if (frameCounter_ % 37 == 0 && juce::Random::getSystemRandom().nextFloat() < 0.02f)
                                flicker = 0.4f;

                            // Tighter, smaller dots for dense look
                            float maxDot = juce::jmin(lipCellW, lipCellH) * 0.85f * globalScale;
                            float diameter = juce::jmax(0.0f, maxDot * dotScale);
                            if (diameter < 0.35f)
                                continue;

                            // Edge-based brightness (brighter toward center) + dim upper lip
                            float brightness = brightnessBase * (0.90f + 0.10f * (1.0f - edge)) * flicker;
                            if (ny < 0.0f) brightness *= 0.9f;  // Dim upper lip slightly
                            juce::Colour dotColour = tint_.withMultipliedAlpha(brightness);
                            g.setColour(dotColour);
                            g.fillEllipse(px - diameter * 0.5f, py - diameter * 0.5f, diameter, diameter);
                        }
                    }
                }
            }
        }
    }

    // Animation state update (called by PluginEditor at 10 FPS)
    void updateAnimation()
    {
        ++frameCounter_;

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

        // Micro-expression scheduling (at 10 FPS: 80 frames ~= 8s)
        if (++framesSinceExpression_ > 80)
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
    }

    void initialiseShapeTemplates()
    {
        // Load AUTHENTIC EMU Z-plane pole formations (not procedural approximations)
        // Each EMU shape = 6 conjugate pole pairs converted to 16×6 dot matrix visualization

        using VowelMap = AuthenticShapeLoader::VowelMapping;

        // AA: VowelAe (bright open) - shape 0
        auto aaShape = AuthenticShapeLoader::getAuthenticShape(
            AuthenticShapeLoader::getShapeIndex(VowelMap::AA));
        shapes_[(int) Vowel::AA] = AuthenticShapeLoader::convertToHalftoneDots(aaShape);

        // AH: VowelEh (mid neutral) - shape 4
        auto ahShape = AuthenticShapeLoader::getAuthenticShape(
            AuthenticShapeLoader::getShapeIndex(VowelMap::AH));
        shapes_[(int) Vowel::AH] = AuthenticShapeLoader::convertToHalftoneDots(ahShape);

        // EE: FormantSweep (darker wide) - shape 2
        auto eeShape = AuthenticShapeLoader::getAuthenticShape(
            AuthenticShapeLoader::getShapeIndex(VowelMap::EE));
        shapes_[(int) Vowel::EE] = AuthenticShapeLoader::convertToHalftoneDots(eeShape);

        // OH: Bell (circular) - shape 3
        auto ohShape = AuthenticShapeLoader::getAuthenticShape(
            AuthenticShapeLoader::getShapeIndex(VowelMap::OH));
        shapes_[(int) Vowel::OH] = AuthenticShapeLoader::convertToHalftoneDots(ohShape);

        // OO: VowelIh (closed tight) - shape 5
        auto ooShape = AuthenticShapeLoader::getAuthenticShape(
            AuthenticShapeLoader::getShapeIndex(VowelMap::OO));
        shapes_[(int) Vowel::OO] = AuthenticShapeLoader::convertToHalftoneDots(ooShape);
    }

    // === Constants ===
    static constexpr int kCols = 16;
    static constexpr int kRows = 6;
    static constexpr int kTotal = kCols * kRows;

    // PHASE 2: Per-dot phosphor decay (CRT persistence)
    struct DotState
    {
        float phosphor = 0.0f;  // 0-1, decays slowly after brightness peak
    };

    // Templates
    std::array<std::array<float, kTotal>, 5> shapes_ {}; // AA, AH, EE, OH, OO

    // Atomics (audio thread writes)
    std::atomic<int> pair_ { 0 }; // 0=VOWEL, 1=BELL, 2=LOW, 3=SUB
    std::atomic<float> audioLevel_ { 0.0f };
    std::atomic<float> morph_ { 0.5f };
    std::atomic<bool> jitterActive_ { false };
    std::atomic<int> glitchFrames_ { 0 }; // meltdown effect
    std::atomic<float> intensity_ { 0.5f }; // drives lip thickness in LipHalftone mode
    std::atomic<int> style_ { static_cast<int>(Style::LEDGrid) };
    std::atomic<bool> useDirectPattern_ { false }; // Phase 1: Direct pole visualization mode

    // UI thread state
    float smoothedAudio_ = 0.0f;
    float prevAudio_ = 0.0f;
    float breathPhase_ = 0.0f;
    int transientPulseFrames_ = 0;
    float transientPulseScale_ = 1.0f;
    int framesSinceExpression_ = 0;
    int microExpressionFrames_ = 0;
    int microExpressionType_ = 0; // 0 blink,1 sigh,2 asymmetry
    int frameCounter_ = 0;        // For controlled flicker timing (frame % 37)

    // PHASE 2: Per-dot phosphor decay state (16×6 = 96 dots)
    std::array<DotState, kTotal> dotStates_ {};

    // Visual style
    juce::Colour tint_ { juce::Colours::white };

    // Phase 1: Direct pole visualization pattern (UI thread only)
    std::array<float, 96> directDotPattern_ {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HalftoneMouth)
};
