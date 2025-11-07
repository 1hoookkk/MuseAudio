#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "OLEDLookAndFeel.h"
#include <random>

/**
 * MuseTransmission - Displays Muse's synesthetic inner monologue
 * 
 * Her text is driven by DSP reality (pole positions, intensity, NaN detection)
 * NOT random or fake - she reacts to what's actually happening mathematically.
 * 
 * The Three States:
 * 
 * 1. FLOW STATE (70% - r < 0.90)
 *    - 10fps stutter text ("transmission from higher plane")
 *    - "Doodling...", "Tinkering...", "Pondering..."
 *    - Then: "Voila.", "So.", "Ta-da."
 *    - Effortless confidence
 * 
 * 2. STRUGGLE STATE (30% - 0.90 <= r < 0.93)
 *    - 60fps SMOOTH flash (mask slip!)
 *    - "Ugh...", "Trying my best...", "Struggling..."
 *    - Brief vulnerability, then back to 10fps
 *    - User sees her work hard
 * 
 * 3. MELTDOWN STATE (Ultra-rare - r >= 0.93 or NaN)
 *    - 60fps SMOOTH, high-quality render
 *    - "wtf (╯°□°)..." (CUT OFF mid-rage)
 *    - "that's not right", "umm...", "Fiddlesticks."
 *    - Complete composure loss
 */
class MuseTransmission : public juce::Component, private juce::Timer
{
public:
    enum class State
    {
        Flow,
        Struggle,
        Meltdown
    };

    MuseTransmission()
    {
        random.seed(std::random_device{}());
        
        // Start at 10fps (Flow state default)
        startTimerHz(10);
        
        // Initial message
        currentMessage = "...";
        messageAge = 0.0f;
    }

    ~MuseTransmission() override
    {
        stopTimer();
    }

    void setState(State newState)
    {
        if (currentState == newState)
            return;

        currentState = newState;
        
        // Update framerate based on state
        if (currentState == State::Flow)
        {
            startTimerHz(10);  // 10fps stutter
        }
        else  // Struggle or Meltdown
        {
            startTimerHz(60);  // 60fps smooth (mask slip!)
        }
        
        // Generate new message immediately
        updateMessage();
        repaint();
    }

    void timerCallback() override
    {
        messageAge += (1.0f / (currentState == State::Flow ? 10.0f : 60.0f));
        
        // Message duration depends on state
        float maxAge = (currentState == State::Flow) ? 3.0f : 1.5f;
        
        if (messageAge >= maxAge)
        {
            updateMessage();
            messageAge = 0.0f;
        }
        
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Dark background
        g.fillAll(juce::Colour(0xFF0a0a0a));
        
        // Message text (mint green with glow)
        auto font = juce::Font(juce::Font::getDefaultMonospacedFontName(), 
                               16.0f, 
                               juce::Font::plain);
        g.setFont(font);
        
        // Glow effect (stronger in Struggle/Meltdown states)
        float glowAlpha = (currentState == State::Flow) ? 0.3f : 0.6f;
        g.setColour(juce::Colour(OLEDLookAndFeel::MintGreen).withAlpha(glowAlpha));
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                if (dx == 0 && dy == 0) continue;
                g.drawText(currentMessage, bounds.translated((float)dx, (float)dy), 
                          juce::Justification::centred, true);
            }
        }
        
        // Main text
        g.setColour(juce::Colour(OLEDLookAndFeel::MintGreen));
        g.drawText(currentMessage, bounds, juce::Justification::centred, true);
        
        // Add framerate indicator (debug - can remove later)
        if (currentState != State::Flow)
        {
            g.setFont(juce::Font(10.0f));
            g.setColour(juce::Colour(OLEDLookAndFeel::MintGreen).withAlpha(0.3f));
            g.drawText("60fps", bounds.removeFromBottom(12).toFloat(), 
                      juce::Justification::centredRight, false);
        }
    }

private:
    void updateMessage()
    {
        std::uniform_int_distribution<int> dist(0, 99);
        int roll = dist(random);
        
        switch (currentState)
        {
            case State::Flow:
                // 70% of the time - effortless creation
                if (roll < 40)
                    currentMessage = flowProcessMessages[roll % flowProcessMessages.size()];
                else if (roll < 70)
                    currentMessage = flowVerdictMessages[(roll - 40) % flowVerdictMessages.size()];
                else
                    currentMessage = synestheticMutterings[(roll - 70) % synestheticMutterings.size()];
                break;

            case State::Struggle:
                // 30% of the time - pushing limits
                currentMessage = struggleMessages[roll % struggleMessages.size()];
                break;

            case State::Meltdown:
                // Ultra-rare - complete loss of composure
                currentMessage = meltdownMessages[roll % meltdownMessages.size()];
                break;
        }
    }

    // === FLOW STATE: Effortless Creation ===
    const std::vector<juce::String> flowProcessMessages = {
        "Doodling...",
        "Tinkering...",
        "Pondering...",
        "Weaving...",
        "Sculpting...",
        "Listening...",
    };

    const std::vector<juce::String> flowVerdictMessages = {
        "Voila.",
        "So.",
        "Ta-da.",
        "There.",
        "Done.",
    };

    const std::vector<juce::String> synestheticMutterings = {
        "Silver...",
        "Indigo...",
        "Breathing amber...",
        "The fifth tastes wrong.",
        "Hmm.",
        "Interesting.",
    };

    // === STRUGGLE STATE: Mask Slip ===
    const std::vector<juce::String> struggleMessages = {
        "Ugh...",
        "Trying my best...",
        "Struggling...",
        "Edges...",
        "Almost...",
        "Hold on...",
    };

    // === MELTDOWN STATE: Fourth Wall Break ===
    const std::vector<juce::String> meltdownMessages = {
        "wtf (╯°□°)...",
        "that's not right",
        "umm...",
        "Fiddlesticks.",
        "NOPE",
        "Can't.",
    };

    State currentState = State::Flow;
    juce::String currentMessage;
    float messageAge = 0.0f;
    std::mt19937 random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseTransmission)
};
