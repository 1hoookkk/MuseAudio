#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * ParametersV2 - APVTS layout for new architecture
 * Coexists with old Parameters.h
 */
namespace ParamsV2
{
    // Parameter IDs
    inline constexpr auto pairId        = "pairV2";
    inline constexpr auto morphId       = "morphV2";
    inline constexpr auto intensityId   = "intensityV2";
    inline constexpr auto driveId       = "driveDbV2";
    inline constexpr auto satId         = "satV2";
    inline constexpr auto lfoRateId     = "lfoRateHzV2";
    inline constexpr auto lfoDepthId    = "lfoDepthV2";
    inline constexpr auto mixId         = "mixV2";
    inline constexpr auto autoMakeupId  = "autoMakeupV2";

    // Shape pair names
    inline const juce::StringArray pairNames = { "Vowel", "Bell", "Low", "Sub" };

    /**
     * Create parameter layout for APVTS
     */
    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // PAIR - Shape selector
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            pairId,
            "Shape Pair",
            pairNames,
            0 // default: Vowel
        ));

        // MORPH - X axis (character)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            morphId,
            "Morph",
            juce::NormalisableRange<float>(0.0f, 1.0f),
            0.5f // default: center
        ));

        // INTENSITY - Y axis (resonance)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            intensityId,
            "Intensity",
            juce::NormalisableRange<float>(0.0f, 1.0f),
            0.4f // default: 40% (EMU authentic)
        ));

        // DRIVE - Pre-gain (-12 to +12 dB)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            driveId,
            "Drive",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f // default: unity
        ));

        // SATURATION - Per-section warmth (0-100%)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            satId,
            "Warmth",
            juce::NormalisableRange<float>(0.0f, 1.0f),
            0.2f // default: 20% (EMU authentic)
        ));

        // LFO RATE - Motion speed (0.02 to 8 Hz, log scale)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            lfoRateId,
            "Motion Rate",
            juce::NormalisableRange<float>(0.02f, 8.0f, 0.01f, 0.3f), // skew for log feel
            0.5f // default: 0.5 Hz
        ));

        // LFO DEPTH - Motion amount (0-100%)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            lfoDepthId,
            "Motion Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f),
            0.0f // default: off
        ));

        // MIX - Wet/dry blend (0-100%)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            mixId,
            "Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f),
            1.0f // default: 100% wet
        ));

        // AUTO MAKEUP - Automatic gain compensation
        layout.add(std::make_unique<juce::AudioParameterBool>(
            autoMakeupId,
            "Auto Makeup",
            true // default: on
        ));

        return layout;
    }

} // namespace ParamsV2
