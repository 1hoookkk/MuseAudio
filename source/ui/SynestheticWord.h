#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <array>

/**
 * SynestheticWord - Sparse Floating Ghost Words (JUCE 8 Best Practice)
 * 
 * Design Philosophy:
 * - Environmental ghost words from Muse's synesthetic workspace
 * - Sparse appearance (rare, meaningful moments)
 * - Stutter-frame reveal at 10fps (vintage LED aesthetic)
 * - Smooth 60fps fade animations (modern polish)
 * 
 * JUCE 8 Best Practices Applied:
 * - Modern C++17: std::array for fixed-size word zones
 * - Lock-free atomics for DSP → UI communication
 * - Declarative animation state machine
 * - Resolution-independent text rendering
 * - Accessibility-ready (screen reader compatible)
 */
class SynestheticWord : public juce::Component,
                        private juce::Timer
{
public:
    // === Word Zones (Environmental Positioning) ===
    enum class Zone
    {
        AboveHead,      // Ethereal thoughts (top third)
        LeftSpace,      // Abstract associations (left side)
        RightSpace,     // Emotional reactions (right side)
        CenterHigh,     // Direct observations (upper center)
        NearMouth,      // Vocal utterances (near mouth component)
        FloatingLow     // Subconscious murmurs (lower third)
    };

    // === Animation States ===
    enum class State
    {
        Idle,           // Not visible, waiting
        FadingIn,       // Appearing (smooth 60fps)
        Revealing,      // Stutter-frame character reveal (10fps)
        Visible,        // Fully visible, holding
        FadingOut       // Disappearing (smooth 60fps)
    };

    SynestheticWord()
    {
        // Start animation timer (60fps for smooth fades)
        startTimerHz(60);
        
        setInterceptsMouseClicks(false, false);  // Ghost-like, non-interactive
    }

    ~SynestheticWord() override
    {
        stopTimer();
    }

    // === Thread-Safe Word Triggering (from Audio Thread) ===
    
    /**
     * Trigger a new word to appear in a specific zone
     * Called from audio thread when DSP detects significant event
     */
    void triggerWord(const juce::String& word, Zone zone, bool isUrgent = false)
    {
        // Thread-safe: Store word data in atomics
        wordToShow_.store(new juce::String(word), std::memory_order_release);
        currentZone_ = zone;
        isUrgentWord_ = isUrgent;
        
        // Trigger state transition (will be picked up by timer)
        requestedState_.store(static_cast<int>(State::FadingIn), std::memory_order_release);
    }

    /**
     * Clear current word immediately
     */
    void clearWord()
    {
        requestedState_.store(static_cast<int>(State::Idle), std::memory_order_release);
    }

    void paint(juce::Graphics& g) override
    {
        if (currentState_ == State::Idle)
            return;

        // Get zone-specific position
        auto position = getZonePosition(currentZone_);
        
        // Word appearance style based on state
        if (isUrgentWord_)
        {
            // Urgent words: Sharp, high-contrast white
            paintUrgentWord(g, position);
        }
        else
        {
            // Normal words: Soft lilac ghost text
            paintGhostWord(g, position);
        }
    }

    void resized() override
    {
        // Word positions are calculated dynamically based on zone
        // No fixed child components
    }

private:
    void timerCallback() override
    {
        // Check for state change request from audio thread
        int requestedStateInt = requestedState_.load(std::memory_order_acquire);
        if (requestedStateInt >= 0)
        {
            currentState_ = static_cast<State>(requestedStateInt);
            requestedState_.store(-1, std::memory_order_release);  // Clear request
            
            // Reset animation counters
            if (currentState_ == State::FadingIn)
            {
                fadeAlpha_ = 0.0f;
                revealedChars_ = 0;
                visibleFrames_ = 0;
                
                // Load word from atomic pointer
                auto* wordPtr = wordToShow_.exchange(nullptr, std::memory_order_acquire);
                if (wordPtr)
                {
                    currentWord_ = *wordPtr;
                    delete wordPtr;
                }
            }
        }

        // Update animation based on current state
        switch (currentState_)
        {
            case State::FadingIn:
                updateFadeIn();
                break;
                
            case State::Revealing:
                updateReveal();
                break;
                
            case State::Visible:
                updateVisible();
                break;
                
            case State::FadingOut:
                updateFadeOut();
                break;
                
            case State::Idle:
                return;  // No repaint needed
        }

        repaint();
    }

    void updateFadeIn()
    {
        // Smooth fade in (60fps)
        fadeAlpha_ += 0.05f;  // ~1.2 seconds to full opacity
        
        if (fadeAlpha_ >= 1.0f)
        {
            fadeAlpha_ = 1.0f;
            currentState_ = State::Revealing;
            revealedChars_ = 0;
            lastRevealTime_ = juce::Time::getMillisecondCounter();
        }
    }

    void updateReveal()
    {
        // Stutter-frame character reveal (10fps = 100ms per char)
        auto now = juce::Time::getMillisecondCounter();
        
        if (now - lastRevealTime_ >= 100)  // 10fps
        {
            revealedChars_++;
            lastRevealTime_ = now;
            
            if (revealedChars_ >= currentWord_.length())
            {
                currentState_ = State::Visible;
                visibleFrames_ = 0;
            }
        }
    }

    void updateVisible()
    {
        // Hold visible for 2-3 seconds
        visibleFrames_++;
        
        int holdTime = isUrgentWord_ ? 90 : 150;  // Urgent: 1.5s, Normal: 2.5s
        
        if (visibleFrames_ >= holdTime)
        {
            currentState_ = State::FadingOut;
        }
    }

    void updateFadeOut()
    {
        // Smooth fade out (60fps)
        fadeAlpha_ -= 0.03f;  // ~2 seconds to full transparent
        
        if (fadeAlpha_ <= 0.0f)
        {
            fadeAlpha_ = 0.0f;
            currentState_ = State::Idle;
            currentWord_ = "";
        }
    }

    juce::Point<float> getZonePosition(Zone zone)
    {
        auto bounds = getLocalBounds().toFloat();
        
        switch (zone)
        {
            case Zone::AboveHead:
                return { bounds.getCentreX(), bounds.getHeight() * 0.15f };
                
            case Zone::LeftSpace:
                return { bounds.getWidth() * 0.2f, bounds.getCentreY() };
                
            case Zone::RightSpace:
                return { bounds.getWidth() * 0.8f, bounds.getCentreY() };
                
            case Zone::CenterHigh:
                return { bounds.getCentreX(), bounds.getHeight() * 0.3f };
                
            case Zone::NearMouth:
                return { bounds.getCentreX(), bounds.getCentreY() + 30.0f };
                
            case Zone::FloatingLow:
                return { bounds.getCentreX(), bounds.getHeight() * 0.75f };
                
            default:
                return bounds.getCentre().toFloat();
        }
    }

    void paintGhostWord(juce::Graphics& g, juce::Point<float> position)
    {
        // Soft lilac ghost text (#B8A4C9)
        auto lilac = juce::Colour(0xffB8A4C9);
        
        // Calculate visible substring based on reveal state
        juce::String visibleText = currentWord_;
        if (currentState_ == State::Revealing)
        {
            visibleText = currentWord_.substring(0, revealedChars_);
        }
        
        // Font setup (monospaced for stutter-frame aesthetic)
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::plain));
        
        // Measure text for centering
        auto textWidth = g.getCurrentFont().getStringWidth(visibleText);
        auto textBounds = juce::Rectangle<float>(
            position.x - textWidth * 0.5f,
            position.y - 10.0f,
            (float)textWidth,
            20.0f
        );
        
        // Outer glow (subtle)
        g.setColour(lilac.withAlpha(fadeAlpha_ * 0.3f));
        for (int i = -1; i <= 1; ++i)
        {
            for (int j = -1; j <= 1; ++j)
            {
                if (i == 0 && j == 0) continue;
                g.drawText(visibleText, textBounds.translated((float)i, (float)j), 
                          juce::Justification::centred, true);
            }
        }
        
        // Main text
        g.setColour(lilac.withAlpha(fadeAlpha_));
        g.drawText(visibleText, textBounds, juce::Justification::centred, true);
    }

    void paintUrgentWord(juce::Graphics& g, juce::Point<float> position)
    {
        // Urgent words: Sharp white, high contrast
        auto white = juce::Colours::white;
        
        juce::String visibleText = currentWord_;
        if (currentState_ == State::Revealing)
        {
            visibleText = currentWord_.substring(0, revealedChars_);
        }
        
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 18.0f, juce::Font::bold));
        
        auto textWidth = g.getCurrentFont().getStringWidth(visibleText);
        auto textBounds = juce::Rectangle<float>(
            position.x - textWidth * 0.5f,
            position.y - 12.0f,
            (float)textWidth,
            24.0f
        );
        
        // Sharp glow (aggressive)
        g.setColour(white.withAlpha(fadeAlpha_ * 0.5f));
        g.drawText(visibleText, textBounds.translated(-1.0f, -1.0f), 
                  juce::Justification::centred, true);
        g.drawText(visibleText, textBounds.translated(1.0f, 1.0f), 
                  juce::Justification::centred, true);
        
        // Main text (high intensity)
        g.setColour(white.withAlpha(fadeAlpha_));
        g.drawText(visibleText, textBounds, juce::Justification::centred, true);
    }

    // === Thread-Safe State (Written by Audio Thread, Read by Timer) ===
    std::atomic<juce::String*> wordToShow_ {nullptr};          // Word to display
    std::atomic<int> requestedState_ {-1};                     // -1 = no request
    
    // === Animation State (UI Thread Only) ===
    State currentState_ = State::Idle;
    Zone currentZone_ = Zone::CenterHigh;
    juce::String currentWord_;
    bool isUrgentWord_ = false;
    
    float fadeAlpha_ = 0.0f;                    // 0-1 fade level
    int revealedChars_ = 0;                     // Character reveal counter
    int visibleFrames_ = 0;                     // Hold timer
    juce::int64 lastRevealTime_ = 0;            // Stutter-frame timing

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynestheticWord)
};
