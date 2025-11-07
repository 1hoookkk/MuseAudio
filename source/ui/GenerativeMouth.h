#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "OLEDLookAndFeel.h"
#include <array>
#include <vector>

/**
 * GenerativeMouth - Procedurally generated LED/pixel mouth
 * 
 * This is a GENERATIVE visual that creates a new mouth pattern on every frame.
 * Unlike interpolated animations, each frame is procedurally generated based on:
 * - Vowel shape (AA/AH/EE/OH/OO from Z-plane filter)
 * - Audio activity level (RMS from DSP)
 * - Morph parameter (shape pair interpolation)
 * - Organic noise/jitter for "living" feel
 * 
 * The mouth updates at 10 FPS (100ms per frame) for intentional stutter aesthetic.
 * Each frame is unique - not smoothed, not cached, truly generative.
 * 
 * Visual style:
 * - 16×6 LED/pixel matrix (higher resolution than 8×3)
 * - Mint green (#d8f3dc) with audio-reactive brightness
 * - Per-pixel organic noise (~3-5% flicker)
 * - Breathing motion even when idle
 * - Asymmetric patterns for naturalism
 */
class GenerativeMouth : public juce::Component, private juce::Timer
{
public:
    enum class Vowel 
    { 
        AA,        // Wide open (like "father")
        AH,        // Mid open (like "hut") 
        EE,        // Smile (like "see")
        OH,        // Round medium (like "go")
        OO,        // Round tight (like "boot")
        Wide,      // Maximum width (low formants)
        Narrow,    // Reduced width
        Neutral    // Flat/minimal (sub bass)
    };

    GenerativeMouth()
    {
        // Initialize grid to all off
        currentFrame.fill(false);
        
        // Seed random for organic variation
        random.setSeedRandomly();
        
        // 10 FPS for intentional stutter aesthetic (spec requirement)
        startTimerHz(10);
    }

    ~GenerativeMouth() override 
    { 
        stopTimer(); 
    }

    // Called from editor timer (vowel shape from Z-plane filter)
    void setVowel(Vowel v) 
    { 
        if (currentVowel != v)
        {
            currentVowel = v;
            // Immediate regeneration on vowel change
            generateNextFrame();
        }
    }

    // Called from editor timer (0-1 morph parameter)
    void setMorph(float m) 
    { 
        morphValue = juce::jlimit(0.0f, 1.0f, m);
    }

    // Called from editor timer (RMS audio level 0-1)
    void setAudioLevel(float level) 
    { 
        // Exponential smoothing for natural feel
        const float attack = 0.6f;
        const float release = 0.92f;
        
        if (level > audioLevel)
            audioLevel += (level - audioLevel) * attack;
        else
            audioLevel *= release;
            
        audioLevel = juce::jlimit(0.0f, 1.0f, audioLevel);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Calculate cell dimensions
        const float cellW = bounds.getWidth() / (float)COLS;
        const float cellH = bounds.getHeight() / (float)ROWS;
        
        // Audio-reactive brightness pulse
        const float breathPhase = (float)juce::Time::getMillisecondCounter() * 0.002f;
        const float breathPulse = 0.5f + 0.5f * std::sin(breathPhase);
        const float baseBrightness = 0.4f + (audioLevel * 0.6f);
        const float brightness = baseBrightness * (0.85f + 0.15f * breathPulse);
        
        // Draw LED grid
        for (int row = 0; row < ROWS; ++row)
        {
            for (int col = 0; col < COLS; ++col)
            {
                const int idx = row * COLS + col;
                const bool isLit = currentFrame[idx];
                
                auto cellBounds = juce::Rectangle<float>(
                    col * cellW,
                    row * cellH,
                    cellW,
                    cellH
                ).reduced(0.5f);
                
                if (isLit)
                {
                    // Lit LED with glow
                    const auto mint = juce::Colour(OLEDLookAndFeel::MintGreen);
                    
                    // Outer glow (audio-reactive)
                    g.setColour(mint.withAlpha(0.15f * brightness));
                    g.fillRoundedRectangle(cellBounds.expanded(1.5f), 1.0f);
                    
                    // Core LED
                    g.setColour(mint.withAlpha(brightness));
                    g.fillRoundedRectangle(cellBounds, 0.8f);
                }
                else
                {
                    // Dim LED (barely visible)
                    g.setColour(juce::Colour(OLEDLookAndFeel::MintGreen).withAlpha(0.03f));
                    g.fillRoundedRectangle(cellBounds, 0.8f);
                }
            }
        }
    }

private:
    static constexpr int COLS = 16;  // Higher resolution than 8×3
    static constexpr int ROWS = 6;
    static constexpr int TOTAL_PIXELS = COLS * ROWS;
    
    void timerCallback() override
    {
        // Generate new frame at 10 FPS
        generateNextFrame();
        repaint();
    }
    
    /**
     * Procedurally generate a new mouth frame
     * 
     * This is the heart of the generative system. Each call creates a unique
     * mouth pattern based on current state + organic noise.
     */
    void generateNextFrame()
    {
        // Clear previous frame
        currentFrame.fill(false);
        
        // Get base shape parameters from vowel
        float widthNorm = 0.5f;   // 0-1: width of mouth
        float openNorm = 0.3f;    // 0-1: vertical opening
        float smileNorm = 0.0f;   // 0-1: upward curve (smile)
        float roundNorm = 0.0f;   // 0-1: roundness (vs elongated)
        
        getVowelParameters(widthNorm, openNorm, smileNorm, roundNorm);
        
        // Audio activity expands opening
        openNorm = juce::jlimit(0.05f, 0.95f, openNorm * (0.6f + 0.8f * audioLevel));
        
        // Morph can subtly influence shape (optional)
        widthNorm = juce::jlimit(0.2f, 0.95f, widthNorm * (0.85f + 0.3f * morphValue));
        
        // Convert normalized values to pixel dimensions
        const float centerX = COLS * 0.5f;
        const float centerY = ROWS * 0.5f;
        const float halfWidth = (COLS * 0.5f) * widthNorm;
        const float halfHeight = (ROWS * 0.5f) * openNorm;
        
        // Generate mouth shape with organic variation
        for (int row = 0; row < ROWS; ++row)
        {
            for (int col = 0; col < COLS; ++col)
            {
                const float x = col - centerX;
                const float y = row - centerY;
                
                // Elliptical base shape with smile curve
                const float smileCurve = smileNorm * (x * x) / (COLS * COLS) * 4.0f;
                const float adjustedY = y - smileCurve;
                
                // Distance from center (ellipse test)
                float ellipseDist;
                if (roundNorm > 0.5f)
                {
                    // More circular
                    const float avgRadius = (halfWidth + halfHeight) * 0.5f;
                    ellipseDist = std::sqrt(x*x + adjustedY*adjustedY) / avgRadius;
                }
                else
                {
                    // More elongated
                    ellipseDist = std::sqrt((x*x)/(halfWidth*halfWidth) + (adjustedY*adjustedY)/(halfHeight*halfHeight));
                }
                
                // Base threshold (edge of mouth)
                bool shouldLight = ellipseDist < 1.0f;
                
                // Organic noise: 3-5% chance of flipping any pixel
                const float noiseChance = 0.03f + (audioLevel * 0.02f);
                if (random.nextFloat() < noiseChance)
                    shouldLight = !shouldLight;
                
                // Edge softness: slight falloff near boundary
                if (ellipseDist > 0.85f && ellipseDist < 1.0f)
                {
                    const float edgeProb = juce::jmap(ellipseDist, 0.85f, 1.0f, 0.9f, 0.3f);
                    shouldLight = shouldLight && (random.nextFloat() < edgeProb);
                }
                
                const int idx = row * COLS + col;
                currentFrame[idx] = shouldLight;
            }
        }
        
        // Additional generative effects
        addAsymmetry();      // Living organisms aren't perfectly symmetrical
        addBreathingJitter(); // Subtle position noise even when idle
        addTeethHint();      // Tiny teeth suggestion for open vowels
    }
    
    /**
     * Get normalized shape parameters for each vowel
     */
    void getVowelParameters(float& width, float& open, float& smile, float& round)
    {
        switch (currentVowel)
        {
            case Vowel::AA:      // Wide open
                width = 0.85f; open = 0.70f; smile = 0.10f; round = 0.15f; break;
            case Vowel::AH:      // Mid open
                width = 0.75f; open = 0.50f; smile = 0.05f; round = 0.20f; break;
            case Vowel::EE:      // Smile
                width = 0.88f; open = 0.30f; smile = 0.75f; round = 0.10f; break;
            case Vowel::OH:      // Round medium
                width = 0.60f; open = 0.55f; smile = 0.08f; round = 0.70f; break;
            case Vowel::OO:      // Round tight
                width = 0.45f; open = 0.45f; smile = 0.05f; round = 0.90f; break;
            case Vowel::Wide:    // Maximum width
                width = 0.95f; open = 0.75f; smile = 0.02f; round = 0.10f; break;
            case Vowel::Narrow:  // Reduced width
                width = 0.35f; open = 0.40f; smile = 0.02f; round = 0.25f; break;
            case Vowel::Neutral: // Flat/minimal
                width = 0.70f; open = 0.25f; smile = 0.01f; round = 0.15f; break;
        }
    }
    
    /**
     * Add subtle left/right asymmetry for organic feel
     */
    void addAsymmetry()
    {
        // Randomly shift a few rows left or right by 1 pixel
        for (int row = 0; row < ROWS; ++row)
        {
            if (random.nextFloat() < 0.25f)  // 25% chance per row
            {
                const int shift = random.nextBool() ? 1 : -1;
                std::array<bool, COLS> rowCopy;
                
                // Copy row
                for (int col = 0; col < COLS; ++col)
                    rowCopy[col] = currentFrame[row * COLS + col];
                
                // Shift and wrap
                for (int col = 0; col < COLS; ++col)
                {
                    int srcCol = col - shift;
                    if (srcCol < 0) srcCol += COLS;
                    if (srcCol >= COLS) srcCol -= COLS;
                    currentFrame[row * COLS + col] = rowCopy[srcCol];
                }
            }
        }
    }
    
    /**
     * Add subtle positional jitter for "breathing" feel
     */
    void addBreathingJitter()
    {
        // Very low activity: add tiny random sparkles (1-2 pixels)
        if (audioLevel < 0.1f)
        {
            const int sparkleCount = 1 + random.nextInt(3);
            for (int i = 0; i < sparkleCount; ++i)
            {
                const int idx = random.nextInt(TOTAL_PIXELS);
                if (random.nextFloat() < 0.5f)
                    currentFrame[idx] = !currentFrame[idx];
            }
        }
    }
    
    /**
     * Add tiny teeth hint for wide-open vowels
     */
    void addTeethHint()
    {
        // Only for AA, AH, EE when mouth is open enough
        if ((currentVowel == Vowel::AA || currentVowel == Vowel::AH || currentVowel == Vowel::EE)
            && audioLevel > 0.2f)
        {
            // Top center: small horizontal line of 3-5 pixels
            const int teethRow = 1;  // Second row from top
            const int teethWidth = 3 + random.nextInt(3);
            const int startCol = (COLS - teethWidth) / 2;
            
            for (int i = 0; i < teethWidth; ++i)
            {
                const int col = startCol + i;
                if (col >= 0 && col < COLS)
                {
                    const int idx = teethRow * COLS + col;
                    currentFrame[idx] = true;
                }
            }
        }
    }
    
    // State
    Vowel currentVowel = Vowel::AH;
    float morphValue = 0.5f;
    float audioLevel = 0.0f;
    
    // Current frame buffer (regenerated each tick)
    std::array<bool, TOTAL_PIXELS> currentFrame;
    
    // Random generator for organic variation
    juce::Random random;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenerativeMouth)
};
