#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SeanceColors.h"
#include "ui/themes/Theme.h"
#include <vector>
#include <random>

/**
 * FloatingWord - Environmental Ghost Phenomenon
 * 
 * Her voice is NOT contained in a box.
 * Words materialize anywhere in empty space, then dissolve.
 * 
 * States:
 * - Flow (10fps stutter) - Her "performance", transmission from higher plane
 * - Glitch (60fps smooth) - Mask slip, her true voice leaking through
 */
class FloatingWord : public juce::Component, private juce::Timer
{
public:
    enum class RenderMode
    {
        Stutter,  // 10fps (default - her "performance")
        Smooth    // 60fps (rare - mask slips)
    };
    
    enum class AnimationPhase
    {
        Idle,       // Not visible, fully transparent
        FadingIn,   // Materializing
        Revealing,  // Stutter-frame character reveal
        Visible,    // Fully visible, holding
        FadingOut   // Dissolving
    };
    
    FloatingWord()
    {
        setInterceptsMouseClicks(false, false);  // Ghost, not interactive
    }
    
    void showWord(const juce::String& word, 
                  Muse::Layout::WordZone zone,
                  RenderMode mode = RenderMode::Stutter)
    {
        fullMessage = word;
        currentZone = zone;
        renderMode = mode;
        
        revealedCharCount = 0;
        fadeAlpha = 0.0f;
        currentPhase = AnimationPhase::FadingIn;
        
        // Position based on zone
        positionInZone();
        
        // Start animation timer
        int timerHz = (mode == RenderMode::Smooth) ? 60 : 10;
        startTimerHz(timerHz);
        
        setVisible(true);
        repaint();
    }
    
    void hide()
    {
        currentPhase = AnimationPhase::Idle;
        stopTimer();
        setVisible(false);
    }
    
    void paint(juce::Graphics& g) override
    {
        if (currentPhase == AnimationPhase::Idle)
            return;
        
        using namespace SeanceTheme;
        
        // Calculate current visible text
        juce::String visibleText = fullMessage.substring(0, revealedCharCount);
        
        // Apply fade alpha
        auto textColor = textPrimary.withAlpha(fadeAlpha);
        
        // Draw with subtle glow
        g.setColour(textColor.withAlpha(fadeAlpha * 0.3f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 18.0f, juce::Font::bold));
        g.drawText(visibleText, getLocalBounds().expanded(2), juce::Justification::centred);
        
        // Draw main text
        g.setColour(textColor);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 18.0f, juce::Font::bold));
        g.drawText(visibleText, getLocalBounds(), juce::Justification::centred);
    }
    
private:
    void timerCallback() override
    {
        switch (currentPhase)
        {
            case AnimationPhase::FadingIn:
                fadeAlpha += 0.05f;  // Fade in over ~300ms
                if (fadeAlpha >= 1.0f)
                {
                    fadeAlpha = 1.0f;
                    currentPhase = AnimationPhase::Revealing;
                }
                repaint();
                break;
                
            case AnimationPhase::Revealing:
                // Stutter-frame reveal (1-2 chars at a time)
                revealedCharCount += (renderMode == RenderMode::Stutter) ? 2 : 3;
                if (revealedCharCount >= fullMessage.length())
                {
                    revealedCharCount = fullMessage.length();
                    currentPhase = AnimationPhase::Visible;
                    holdTimer = 0;
                }
                repaint();
                break;
                
            case AnimationPhase::Visible:
                holdTimer++;
                // Hold for ~2 seconds (depends on timer frequency)
                int holdFrames = (renderMode == RenderMode::Smooth) ? 120 : 20;
                if (holdTimer >= holdFrames)
                {
                    currentPhase = AnimationPhase::FadingOut;
                }
                break;
                
            case AnimationPhase::FadingOut:
                fadeAlpha -= 0.02f;  // Slow dissolve over ~2 seconds
                if (fadeAlpha <= 0.0f)
                {
                    fadeAlpha = 0.0f;
                    hide();
                }
                repaint();
                break;
                
            case AnimationPhase::Idle:
                // Do nothing
                break;
        }
    }
    
    void positionInZone()
    {
        using namespace Muse::Layout;
        
        juce::Rectangle<int> zoneBounds;
        
        switch (currentZone)
        {
            case WordZone::AboveHead:
                zoneBounds = {200, 40, 240, 30};
                break;
            case WordZone::LeftSpace:
                zoneBounds = {50, 200, 150, 30};
                break;
            case WordZone::RightSpace:
                zoneBounds = {440, 180, 150, 30};
                break;
            case WordZone::CenterHigh:
                zoneBounds = {220, 100, 200, 30};
                break;
            case WordZone::NearMorphKnob:
                zoneBounds = {MorphKnobX - 80, MorphKnobY - 40, 160, 30};
                break;
            case WordZone::FloatingLow:
                zoneBounds = {240, 360, 160, 30};
                break;
        }
        
        setBounds(zoneBounds);
    }
    
    juce::String fullMessage;
    Muse::Layout::WordZone currentZone;
    RenderMode renderMode = RenderMode::Stutter;
    AnimationPhase currentPhase = AnimationPhase::Idle;
    
    int revealedCharCount = 0;
    float fadeAlpha = 0.0f;
    int holdTimer = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingWord)
};
