#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "OLEDLookAndFeel.h"
#include "ui/themes/Theme.h"

/**
 * OLEDMouth - Audio‑reactive vector mouth (AA/AH/EE/OH/OO)
 *
 * Only this component changes. Everything else in the UI stays the same.
 *
 * Goals:
 * - Look like an actual mouth rather than an LED grid
 * - React to the filter: shape follows pair+morph (AA/EE/OO etc.)
 * - React to audio: louder → wider opening/stronger glow
 * - 10 FPS stepped morph aesthetic (100 ms), while keeping smooth 30 FPS paints
 */
class OLEDMouth : public juce::Component, private juce::Timer
{
public:
    enum class VowelShape { AA, AH, EE, OH, OO, Wide, Narrow, Neutral };

    OLEDMouth()
    {
        startTimerHz(30); // keep editor’s cadence; we step logic to 10 FPS internally
    }

    ~OLEDMouth() override { stopTimer(); }

    // Called from editor timer
    void setVowelShape(VowelShape newShape)
    {
        if (currentVowelShape != newShape)
        {
            currentVowelShape = newShape;
            // update targets immediately; drawing updates on next 10 FPS tick
            computeTargets();
        }
    }

    // Called from editor timer (RMS 0..1)
    void setActivityLevel(float newActivity)
    {
        newActivity = juce::jlimit(0.0f, 1.0f, newActivity);
        // small hysteresis to avoid repaint storms
        if (std::abs(activityLevel - newActivity) > 0.005f)
            activityLevel = newActivity;
    }

    // Optional legacy API (now used for subtle morph offset if ever connected)
    void setMorphValue(float newMorph) { morphHint = juce::jlimit(0.0f, 1.0f, newMorph); }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        // Transparent background; editor paints the black OLED behind us
        // Render the inner opening first (black), then lip glow + outline

        // Derived dimensions
        const float cx = r.getCentreX();
        const float cy = r.getCentreY();

        const float W = r.getWidth() * juce::jlimit(0.3f, 0.98f, widthDisplay);
        const float H = r.getHeight() * juce::jlimit(0.04f, 0.60f, openDisplay);

        const float smile = juce::jlimit(0.0f, 1.0f, smileDisplay);   // 0=flat, 1=big smile
        const float round = juce::jlimit(0.0f, 1.0f, roundDisplay);   // 0=capsule, 1=round

        juce::Path mouth;

        // Anchor points
        juce::Point<float> L(cx - W * 0.5f, cy);
        juce::Point<float> R(cx + W * 0.5f, cy);

        // Control offsets
        const float curveY = H * (0.9f + 0.6f * smile); // more smile → higher top arc
        const float ctrlX  = W * juce::jmap(round, 0.22f, 0.35f);

        // Top lip: L → R
        mouth.startNewSubPath(L);
        mouth.cubicTo({ L.x + ctrlX, cy - curveY },
                      { R.x - ctrlX, cy - curveY },
                      R);
        // Bottom lip: R → L
        mouth.cubicTo({ R.x - ctrlX, cy + curveY },
                      { L.x + ctrlX, cy + curveY },
                      L);
        mouth.closeSubPath();

        // Inner opening fill (OLED black) to emphasize a real mouth
        g.setColour(ModernMuseTheme::trueBlack);
        g.fillPath(mouth);

        // Audio-reactive lip colour + glow
        const float pulse = 0.35f + 0.65f * (0.4f + 0.6f * activityLevel);
        const auto mint = ModernMuseTheme::mintGreen;

        // Outer glow
        g.setColour(mint.withAlpha(0.18f * pulse));
        g.strokePath(mouth, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Mid glow
        g.setColour(mint.withAlpha(0.28f * pulse));
        g.strokePath(mouth, juce::PathStrokeType(3.5f));

        // Lip outline
        g.setColour(mint.withAlpha(0.95f));
        g.strokePath(mouth, juce::PathStrokeType(1.8f));

        // Optional: tiny top teeth hint for AA/AH/EE when open enough
        if ((currentVowelShape == VowelShape::AA || currentVowelShape == VowelShape::AH || currentVowelShape == VowelShape::EE)
            && H > r.getHeight() * 0.10f)
        {
            const float teethW = juce::jlimit(12.0f, 40.0f, W * 0.16f);
            const float teethH = juce::jlimit(2.0f, 6.0f, H * 0.20f);
            g.setColour(mint.withAlpha(0.8f));
            g.fillRoundedRectangle({ cx - teethW * 0.5f, cy - H * 0.62f, teethW, teethH }, 1.5f);
        }
    }

private:
    // Shape target computation from vowel + activity
    void computeTargets()
    {
        // Base parameters by vowel shape
        // widthTarget: 0..1 relative to component width
        // openTarget:  0..1 relative to component height
        // smileTarget: 0..1 curvature upwards
        // roundTarget: 0..1 corner roundness (OO most rounded)
        switch (currentVowelShape)
        {
            case VowelShape::AA:      widthTarget = 0.92f; openTarget = 0.28f; smileTarget = 0.15f; roundTarget = 0.15f; break;
            case VowelShape::AH:      widthTarget = 0.88f; openTarget = 0.18f; smileTarget = 0.05f; roundTarget = 0.12f; break;
            case VowelShape::EE:      widthTarget = 0.96f; openTarget = 0.10f; smileTarget = 0.55f; roundTarget = 0.10f; break;
            case VowelShape::OH:      widthTarget = 0.74f; openTarget = 0.16f; smileTarget = 0.10f; roundTarget = 0.55f; break;
            case VowelShape::OO:      widthTarget = 0.56f; openTarget = 0.14f; smileTarget = 0.08f; roundTarget = 0.85f; break;
            case VowelShape::Wide:    widthTarget = 0.98f; openTarget = 0.32f; smileTarget = 0.05f; roundTarget = 0.10f; break;
            case VowelShape::Narrow:  widthTarget = 0.40f; openTarget = 0.12f; smileTarget = 0.05f; roundTarget = 0.20f; break;
            case VowelShape::Neutral: widthTarget = 0.80f; openTarget = 0.06f; smileTarget = 0.02f; roundTarget = 0.10f; break;
        }

        // Audio activity expands opening (more speaking when loud)
        openTarget = juce::jlimit(0.04f, 0.60f, openTarget * (0.80f + 0.40f * activityLevel));
    }

    void timerCallback() override
    {
        // 30 FPS timer; step morph every 3 ticks (≈10 FPS) to match spec
        if ((tickCounter++ % 3) == 0)
        {
            // update targets for current vowel + activity
            computeTargets();

            // simple eased approach to targets
            const auto ease = 0.35f; // 35% towards target per 100 ms
            widthDisplay  += (widthTarget  - widthDisplay)  * ease;
            openDisplay   += (openTarget   - openDisplay)   * ease;
            smileDisplay  += (smileTarget  - smileDisplay)  * ease;
            roundDisplay  += (roundTarget  - roundDisplay)  * ease;
        }

        // subtle breathing independent of 10 FPS stepping (low-level sine)
        breath += 0.05f; if (breath > juce::MathConstants<float>::twoPi) breath -= juce::MathConstants<float>::twoPi;
        const float breathAmt = 0.02f * (0.2f + 0.8f * activityLevel);
        openDisplay += std::sin(breath) * breathAmt; // tiny wobble

        repaint();
    }

    // Current state
    VowelShape currentVowelShape = VowelShape::AH;
    float activityLevel = 0.0f; // 0..1
    float morphHint = 0.5f;     // optional external hint (unused unless wired)

    // Targets (updated at 10 FPS)
    float widthTarget = 0.88f, openTarget = 0.18f, smileTarget = 0.05f, roundTarget = 0.12f;

    // Display values (interpolated)
    float widthDisplay = 0.88f, openDisplay = 0.18f, smileDisplay = 0.05f, roundDisplay = 0.12f;

    // Animation
    int   tickCounter = 0;
    float breath = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OLEDMouth)
};
