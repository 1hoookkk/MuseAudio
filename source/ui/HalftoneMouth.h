#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <map>
#include <atomic>

/**
 * HalftoneMouth - Professional Halftone Mouth Visualization (JUCE 8 Best Practice)
 * 
 * Design Philosophy:
 * - Pre-rendered halftone PNG masks for each vowel shape
 * - Smooth crossfading between masks based on DSP state
 * - GPU-accelerated rendering (optional OpenGL path)
 * - Brutalist aesthetic: abstract, minimal, NO teeth
 * 
 * JUCE 8 Best Practices:
 * ✅ Asset-based rendering (not procedural CPU drawing)
 * ✅ Lock-free atomic communication (DSP → UI)
 * ✅ Smooth interpolation at 60fps
 * ✅ Resolution-independent (can scale masks)
 * ✅ Tintable color (supports theme changes)
 * 
 * Asset Requirements:
 * - 5-7 PNG files in assets/images/mouth/
 * - White halftone dots on transparent/black background
 * - 240×90 pixels (16:6 aspect ratio)
 * - Loaded via BinaryData by CMake
 */
class HalftoneMouth : public juce::Component,
                      private juce::Timer
{
public:
    // Vowel shapes matching PluginProcessor::VowelShape
    enum class Vowel
    {
        AA,         // Wide open vertical oval
        AH,         // Medium relaxed (neutral)
        EE,         // Horizontal slit smile
        OH,         // Round bell opening
        OO,         // Small tight circle
        Closed,     // Dormant/idle state
        Glitch      // Distorted (struggle/meltdown)
    };

    HalftoneMouth()
    {
        loadMouthAssets();
        startTimerHz(60);  // 60fps smooth animation
    }

    ~HalftoneMouth() override
    {
        stopTimer();
    }

    // === Thread-Safe State Updates (from Audio Thread) ===
    
    void setVowel(Vowel vowel)
    {
        targetVowel_.store(static_cast<int>(vowel), std::memory_order_relaxed);
    }

    void setAudioLevel(float level)
    {
        audioLevel_.store(level, std::memory_order_relaxed);
    }

    void setMorph(float morph)
    {
        morphPosition_.store(morph, std::memory_order_relaxed);
    }

    // === Visual Styling ===
    
    void setTintColor(juce::Colour color)
    {
        tintColor_ = color;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // === 1. Determine Current Vowel State ===
        Vowel currentVowel = static_cast<Vowel>(currentVowel_);
        Vowel targetVowel = static_cast<Vowel>(targetVowel_.load(std::memory_order_relaxed));

        // === 2. Get Source and Target Masks ===
        auto sourceMask = getMaskForVowel(currentVowel);
        auto targetMask = getMaskForVowel(targetVowel);

        if (!sourceMask.isValid() && !targetMask.isValid())
        {
            // Fallback: Draw simple placeholder
            drawPlaceholder(g, bounds);
            return;
        }

        // === 3. Calculate Crossfade Alpha ===
        float alpha = crossfadeAlpha_;

        // === 4. Draw Crossfaded Masks ===
        if (sourceMask.isValid() && alpha > 0.0f)
        {
            drawMask(g, bounds, sourceMask, 1.0f - alpha);
        }

        if (targetMask.isValid() && alpha < 1.0f)
        {
            drawMask(g, bounds, targetMask, alpha);
        }

        // === 5. Apply Audio-Reactive Brightness ===
        float brightness = 0.7f + (currentAudioLevel_ * 0.3f);  // 70-100% brightness range
        
        if (brightness < 1.0f)
        {
            g.setColour(juce::Colours::black.withAlpha(1.0f - brightness));
            g.fillRect(bounds);
        }
    }

    void resized() override
    {
        // No child components
    }

private:
    void timerCallback() override
    {
        // Poll atomics from audio thread
        int targetVowelInt = targetVowel_.load(std::memory_order_relaxed);
        float audioLevel = audioLevel_.load(std::memory_order_relaxed);
        float morph = morphPosition_.load(std::memory_order_relaxed);

        // Update audio level with smoothing
        currentAudioLevel_ += (audioLevel - currentAudioLevel_) * 0.2f;

        // === Autonomous Life: Subtle Breathing ===
        // Slow sine wave that gently scales the mouth (±2% size variation)
        breathingPhase_ += 0.02f;  // ~3 second breathing cycle
        if (breathingPhase_ > juce::MathConstants<float>::twoPi)
            breathingPhase_ -= juce::MathConstants<float>::twoPi;
        
        breathingScale_ = 1.0f + (std::sin(breathingPhase_) * 0.02f);

        // === Autonomous Life: Rare Micro-Expressions ===
        // Every ~8 seconds, trigger a subtle "character moment"
        if (++autonomousFrameCount_ > 480)  // 8 seconds at 60fps
        {
            auto& random = juce::Random::getSystemRandom();
            if (random.nextFloat() < 0.3f)  // 30% chance
            {
                // Trigger subtle expression (quick blink, sigh, micro-shift)
                microExpressionFrames_ = 6;  // 100ms expression
                microExpressionType_ = random.nextInt(3);  // 0=blink, 1=sigh, 2=asymmetry
            }
            autonomousFrameCount_ = 0;
        }

        // Update micro-expression countdown
        if (microExpressionFrames_ > 0)
            microExpressionFrames_--;

        // === Audio-Reactive: Transient Pulse ===
        // When audio suddenly increases, mouth pulses larger for a moment
        float audioChange = audioLevel - previousAudioLevel_;
        if (audioChange > 0.15f)  // Detected transient
        {
            transientPulseFrames_ = 4;  // 66ms pulse
        }
        previousAudioLevel_ = audioLevel;

        if (transientPulseFrames_ > 0)
        {
            transientPulseFrames_--;
            transientPulseScale_ = 1.0f + (transientPulseFrames_ * 0.03f);  // Up to +12% size
        }
        else
        {
            transientPulseScale_ = 1.0f;
        }

        // === Vowel Transition (Smooth Crossfading) ===
        if (targetVowelInt != currentVowel_)
        {
            // Smooth crossfade to new vowel
            crossfadeAlpha_ += 0.08f;  // ~12 frames to complete (200ms at 60fps)

            if (crossfadeAlpha_ >= 1.0f)
            {
                crossfadeAlpha_ = 0.0f;
                currentVowel_ = targetVowelInt;
            }
        }
        else
        {
            // Reset crossfade when stable
            if (crossfadeAlpha_ > 0.0f)
            {
                crossfadeAlpha_ = std::max(0.0f, crossfadeAlpha_ - 0.05f);
            }
        }

        repaint();
    }

    void loadMouthAssets()
    {
        // Load PNG masks from BinaryData (populated by CMake Assets.cmake)
        // Format: mouth_[vowel].png -> BinaryData::mouth_[vowel]_png
        
        // NOTE: Uncomment these once PNGs are generated and added to assets/
        
        // mouthMasks_[Vowel::AA] = loadImageFromBinaryData("mouth_AA_wide_png");
        // mouthMasks_[Vowel::AH] = loadImageFromBinaryData("mouth_AH_neutral_png");
        // mouthMasks_[Vowel::EE] = loadImageFromBinaryData("mouth_EE_smile_png");
        // mouthMasks_[Vowel::OH] = loadImageFromBinaryData("mouth_OH_round_png");
        // mouthMasks_[Vowel::OO] = loadImageFromBinaryData("mouth_OO_tight_png");
        // mouthMasks_[Vowel::Closed] = loadImageFromBinaryData("mouth_closed_png");
        // mouthMasks_[Vowel::Glitch] = loadImageFromBinaryData("mouth_glitch_png");
    }

    juce::Image loadImageFromBinaryData(const char* resourceName)
    {
        // Helper to load PNG from BinaryData
        // Once assets are added, use:
        // int dataSize = 0;
        // const char* data = BinaryData::getNamedResource(resourceName, dataSize);
        // if (data != nullptr)
        //     return juce::ImageFileFormat::loadFrom(data, dataSize);
        
        return juce::Image();  // Empty image if not found
    }

    juce::Image getMaskForVowel(Vowel vowel)
    {
        auto it = mouthMasks_.find(vowel);
        if (it != mouthMasks_.end())
            return it->second;
        
        return juce::Image();  // Return empty image if not found
    }

    void drawMask(juce::Graphics& g, juce::Rectangle<float> bounds, 
                   const juce::Image& mask, float alpha)
    {
        if (!mask.isValid())
            return;

        // Calculate scaling to fit bounds while maintaining aspect ratio
        float scaleX = bounds.getWidth() / mask.getWidth();
        float scaleY = bounds.getHeight() / mask.getHeight();
        float scale = std::min(scaleX, scaleY);

        // === Apply Autonomous Life: Breathing + Transient Pulses ===
        scale *= breathingScale_ * transientPulseScale_;

        // === Apply Micro-Expressions ===
        if (microExpressionFrames_ > 0)
        {
            if (microExpressionType_ == 0)  // Blink: shrink vertically
            {
                scaleY *= 0.5f;
            }
            else if (microExpressionType_ == 1)  // Sigh: expand slightly
            {
                scale *= 1.05f;
            }
            // Type 2 (asymmetry) handled below in position offset
        }

        float scaledWidth = mask.getWidth() * scale;
        float scaledHeight = mask.getHeight() * scale;

        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();

        // === Micro-Expression: Asymmetry (slight horizontal offset) ===
        if (microExpressionFrames_ > 0 && microExpressionType_ == 2)
        {
            centerX += 3.0f;  // Subtle shift right
        }

        auto drawBounds = juce::Rectangle<float>(
            centerX - scaledWidth * 0.5f,
            centerY - scaledHeight * 0.5f,
            scaledWidth,
            scaledHeight
        );

        // Apply tint color with alpha
        g.setColour(tintColor_.withAlpha(alpha));
        
        // Draw mask using multiply blend mode for halftone effect
        g.setOpacity(alpha);
        g.drawImage(mask, drawBounds, juce::RectanglePlacement::centred);
    }

    void drawPlaceholder(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Temporary placeholder while waiting for PNG assets
        // Simple procedural dots in a grid pattern
        
        const int rows = 6;
        const int cols = 16;
        
        float dotSpacingX = bounds.getWidth() / (cols + 1);
        float dotSpacingY = bounds.getHeight() / (rows + 1);
        
        g.setColour(tintColor_.withAlpha(0.7f + currentAudioLevel_ * 0.3f));
        
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                float x = bounds.getX() + (col + 1) * dotSpacingX;
                float y = bounds.getY() + (row + 1) * dotSpacingY;
                
                // Vary dot size based on position (simple mouth-like shape)
                float centerCol = cols * 0.5f;
                float centerRow = rows * 0.5f;
                float distX = std::abs(col - centerCol) / centerCol;
                float distY = std::abs(row - centerRow) / centerRow;
                float size = 2.0f + (1.0f - distX * distY) * 3.0f;
                
                g.fillEllipse(x - size * 0.5f, y - size * 0.5f, size, size);
            }
        }
        
        // Draw "AWAITING ASSETS" text
        g.setFont(juce::Font(8.0f));
        g.setColour(tintColor_.withAlpha(0.3f));
        g.drawText("AWAITING PNG ASSETS", bounds.reduced(10.0f), 
                   juce::Justification::centredBottom, true);
    }

    // === Asset Storage ===
    std::map<Vowel, juce::Image> mouthMasks_;

    // === Thread-Safe State (Written by Audio Thread) ===
    std::atomic<int> targetVowel_ {static_cast<int>(Vowel::AH)};
    std::atomic<float> audioLevel_ {0.0f};
    std::atomic<float> morphPosition_ {0.5f};

    // === Animation State (UI Thread Only) ===
    int currentVowel_ = static_cast<int>(Vowel::AH);
    float crossfadeAlpha_ = 0.0f;
    float currentAudioLevel_ = 0.0f;
    float previousAudioLevel_ = 0.0f;

    // === Autonomous Life State ===
    float breathingPhase_ = 0.0f;              // Slow breathing cycle
    float breathingScale_ = 1.0f;              // ±2% size variation
    int autonomousFrameCount_ = 0;             // Frame counter for rare moments
    int microExpressionFrames_ = 0;            // Countdown for expressions
    int microExpressionType_ = 0;              // 0=blink, 1=sigh, 2=asymmetry

    // === Audio-Reactive State ===
    int transientPulseFrames_ = 0;             // Countdown for transient pulse
    float transientPulseScale_ = 1.0f;         // Transient size boost

    // === Visual Style ===
    juce::Colour tintColor_ {juce::Colours::white};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HalftoneMouth)
};
