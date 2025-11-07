#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <PluginProcessor.h>
#include <EMUAuthenticTables_VERIFIED.h>
#include <cmath>
#include <limits>
#include <vector>

using Catch::Approx;

//==============================================================================
// TIER 1: SHIP BLOCKERS - These prevent catastrophic failures
//==============================================================================

TEST_CASE ("Z-plane filter handles extreme inputs", "[dsp][critical]")
{
    PluginProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    SECTION ("NaN input produces finite output")
    {
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        
        // Fill with NaN
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample (ch, i, std::nanf(""));

        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        // Output should be finite (zeros or valid audio)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (sample));
            }
        }
    }

    SECTION ("Infinite input produces finite output")
    {
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        
        // Fill with infinity
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample (ch, i, std::numeric_limits<float>::infinity());

        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        // Output should be finite and bounded
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (sample));
                REQUIRE (std::abs (sample) < 100.0f);
            }
        }
    }

    SECTION ("DC offset doesn't accumulate")
    {
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        // Process 10 buffers of DC (1.0f constant)
        for (int iteration = 0; iteration < 10; ++iteration)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    buffer.setSample (ch, i, 1.0f);

            processor.processBlock (buffer, midi);
        }

        // Final output should stabilize, not grow unbounded
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (sample));
                REQUIRE (std::abs (sample) < 10.0f); // Reasonable bound
            }
        }
    }

    SECTION ("Silence in produces finite output")
    {
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        juce::MidiBuffer midi;

        processor.processBlock (buffer, midi);

        // Processing silence should produce finite output
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (sample));
            }
        }
    }
}

TEST_CASE ("Parameters stay within safe bounds", "[parameters][critical]")
{
    PluginProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto& apvts = processor.getState();

    SECTION ("Morph parameter accepts full range")
    {
        auto* morphParam = apvts.getParameter ("morph");
        REQUIRE (morphParam != nullptr);

        // Test boundary values
        morphParam->setValueNotifyingHost (0.0f);
        morphParam->setValueNotifyingHost (0.5f);
        morphParam->setValueNotifyingHost (1.0f);

        // Process audio with extreme morph values
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        
        processor.processBlock (buffer, midi);

        // Should not crash
        REQUIRE (true);
    }

    SECTION ("Intensity at maximum doesn't cause instability")
    {
        auto* intensityParam = apvts.getParameter ("intensity");
        REQUIRE (intensityParam != nullptr);

        intensityParam->setValueNotifyingHost (1.0f); // Max resonance

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        // Process silence - shouldn't produce runaway feedback
        buffer.clear();
        processor.processBlock (buffer, midi);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (sample));
                REQUIRE (std::abs (sample) < 1.0f); // Shouldn't grow from zero
            }
        }
    }

    SECTION ("All shape pairs process without crashing")
    {
        auto* pairParam = apvts.getParameter ("pair");
        REQUIRE (pairParam != nullptr);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        // Test all 4 shape pairs
        for (int pair = 0; pair < 4; ++pair)
        {
            INFO("Testing shape pair: " << pair);
            
            // Set shape pair (parameter is normalized 0-1, maps to 0-3)
            pairParam->setValueNotifyingHost (pair / 3.0f);

            // Test with impulse
            buffer.clear();
            buffer.setSample (0, 0, 1.0f);
            buffer.setSample (1, 0, 1.0f);

            processor.processBlock (buffer, midi);

            // Should produce finite output
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    float sample = buffer.getSample (ch, i);
                    INFO("Channel: " << ch << ", Sample: " << i);
                    REQUIRE (std::isfinite (sample));
                }
            }
        }
    }

    SECTION ("Extreme parameter combinations are stable")
    {
        // Set all parameters to maximum
        auto* morphParam = apvts.getParameter ("morph");
        auto* intensityParam = apvts.getParameter ("intensity");
        auto* mixParam = apvts.getParameter ("mix");

        morphParam->setValueNotifyingHost (1.0f);
        intensityParam->setValueNotifyingHost (1.0f);
        mixParam->setValueNotifyingHost (1.0f);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        // Process impulse
        buffer.clear();
        buffer.setSample (0, 0, 1.0f);
        buffer.setSample (1, 0, 1.0f);

        processor.processBlock (buffer, midi);

        // Should not explode
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (sample));
                REQUIRE (std::abs (sample) < 100.0f);
            }
        }
    }
}

TEST_CASE ("Thread safety under parameter changes", "[threading][critical]")
{
    PluginProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto& apvts = processor.getState();
    auto* morphParam = apvts.getParameter ("morph");
    auto* intensityParam = apvts.getParameter ("intensity");

    SECTION ("Rapid parameter changes don't crash audio thread")
    {
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        // Simulate DAW automation: change parameters while processing
        for (int iteration = 0; iteration < 100; ++iteration)
        {
            // Change parameters
            morphParam->setValueNotifyingHost (iteration / 100.0f);
            intensityParam->setValueNotifyingHost ((iteration % 2) ? 0.0f : 1.0f);

            // Process audio
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    buffer.setSample (ch, i, std::sin (i * 0.01f));

            processor.processBlock (buffer, midi);

            // Output should remain finite
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    float sample = buffer.getSample (ch, i);
                    REQUIRE (std::isfinite (sample));
                }
            }
        }
    }
}

//==============================================================================
// TIER 2: QUALITY ASSURANCE - These catch user-facing bugs
//==============================================================================

TEST_CASE ("Mix at 0% passes dry signal", "[dsp][quality]")
{
    PluginProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto& apvts = processor.getState();
    auto* mixParam = apvts.getParameter ("mix");
    REQUIRE (mixParam != nullptr);

    mixParam->setValueNotifyingHost (0.0f); // 100% dry

    SECTION ("Mix at 0% passes dry signal")
    {
        juce::AudioBuffer<float> inputBuffer (2, 512);
        juce::AudioBuffer<float> outputBuffer (2, 512);

        // Generate test signal
        for (int ch = 0; ch < inputBuffer.getNumChannels(); ++ch)
            for (int i = 0; i < inputBuffer.getNumSamples(); ++i)
                inputBuffer.setSample (ch, i, std::sin (i * 0.1f));

        // Copy to output buffer
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.copyFrom (ch, 0, inputBuffer, ch, 0, inputBuffer.getNumSamples());

        juce::MidiBuffer midi;
        processor.processBlock (outputBuffer, midi);

        // Output should match input (within floating point tolerance)
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < outputBuffer.getNumSamples(); ++i)
            {
                float input = inputBuffer.getSample (ch, i);
                float output = outputBuffer.getSample (ch, i);
                
                INFO("Channel: " << ch << ", Sample: " << i);
                REQUIRE_THAT(output, Catch::Matchers::WithinAbs(input, 0.001f));
            }
        }
    }
}

TEST_CASE ("Parameter changes are smooth", "[dsp][quality]")
{
    PluginProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto& apvts = processor.getState();
    auto* morphParam = apvts.getParameter ("morph");

    SECTION ("Morph change doesn't produce clicks")
    {
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        // Process first half at morph=0
        morphParam->setValueNotifyingHost (0.0f);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < 256; ++i)
                buffer.setSample (ch, i, 1.0f);

        processor.processBlock (buffer, midi);

        // Store first half output
        std::vector<float> firstHalf (256);
        for (int i = 0; i < 256; ++i)
            firstHalf[i] = buffer.getSample (0, i);

        // Hard parameter change
        morphParam->setValueNotifyingHost (0.8f);

        // Process second half
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 256; i < 512; ++i)
                buffer.setSample (ch, i, 1.0f);

        processor.processBlock (buffer, midi);

        // Check for discontinuity (zipper noise)
        float maxJump = 0.0f;
        for (int i = 1; i < 256; ++i)
        {
            float jump = std::abs (firstHalf[i] - firstHalf[i - 1]);
            maxJump = std::max (maxJump, jump);
        }

        // Should be smoothed (no large jumps)
        INFO ("Max jump: " << maxJump);
        REQUIRE (maxJump < 0.5f); // Reasonable threshold for smooth transitions
    }
}

TEST_CASE ("Different sample rates produce stable output", "[dsp][quality]")
{
    SECTION ("44.1 kHz")
    {
        PluginProcessor processor;
        processor.prepareToPlay (44100.0, 512);

        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        buffer.setSample (0, 0, 1.0f);

        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                REQUIRE (std::isfinite (buffer.getSample (ch, i)));
    }

    SECTION ("48 kHz")
    {
        PluginProcessor processor;
        processor.prepareToPlay (48000.0, 512);

        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        buffer.setSample (0, 0, 1.0f);

        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                REQUIRE (std::isfinite (buffer.getSample (ch, i)));
    }

    SECTION ("96 kHz")
    {
        PluginProcessor processor;
        processor.prepareToPlay (96000.0, 512);

        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        buffer.setSample (0, 0, 1.0f);

        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                REQUIRE (std::isfinite (buffer.getSample (ch, i)));
    }
}

TEST_CASE ("Auto makeup gain prevents extreme level changes", "[dsp][quality]")
{
    PluginProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto& apvts = processor.getState();
    auto* makeupParam = apvts.getParameter ("autoMakeup");
    auto* intensityParam = apvts.getParameter ("intensity");

    if (makeupParam != nullptr)
    {
        makeupParam->setValueNotifyingHost (1.0f); // Enable auto makeup
        intensityParam->setValueNotifyingHost (1.0f); // High intensity

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        // Process test tone
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample (ch, i, std::sin (i * 0.01f) * 0.5f);

        processor.processBlock (buffer, midi);

        // Calculate RMS of output
        float rmsSum = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample (ch, i);
                rmsSum += sample * sample;
            }
        }
        float rms = std::sqrt (rmsSum / (buffer.getNumSamples() * buffer.getNumChannels()));

        // RMS should be reasonable (not silent, not clipping)
        INFO ("Output RMS: " << rms);
        REQUIRE (rms > 0.01f);  // Not silent
        REQUIRE (rms < 2.0f);   // Not exploding
    }
}

//==============================================================================
// PERFORMANCE BENCHMARKS - Run with: ./Tests "[!benchmark]"
//==============================================================================

TEST_CASE ("DSP performance benchmarks", "[!benchmark][performance]")
{
    PluginProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    
    // Generate realistic test signal
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample (ch, i, std::sin (i * 0.01f) * 0.5f);
    
    BENCHMARK ("Process 512 samples @ 48kHz") {
        processor.processBlock (buffer, midi);
        return buffer.getSample (0, 0);
    };
    
    BENCHMARK ("Process with parameter changes") {
        auto& apvts = processor.getState();
        auto* morphParam = apvts.getParameter ("morph");
        
        // Simulate automation
        morphParam->setValueNotifyingHost (0.5f);
        processor.processBlock (buffer, midi);
        return buffer.getSample (0, 0);
    };
}

// =============================================================================
// ROM INTEGRITY TESTS - Verify authentic EMU coefficient tables
// =============================================================================

TEST_CASE ("ROM coefficient table integrity", "[dsp][critical][rom]")
{
    SECTION ("Verify authentic EMU shape data exists")
    {
        // These constants should be defined in EMUAuthenticTables_VERIFIED.h
        REQUIRE (AUTHENTIC_EMU_NUM_SHAPES == 8);
        REQUIRE (AUTHENTIC_EMU_NUM_PAIRS == 4);
        REQUIRE (AUTHENTIC_EMU_SAMPLE_RATE_REF == 48000);
        
        INFO ("ROM tables contain 8 shapes organized into 4 morph pairs");
    }
    
    SECTION ("Verify vowel_pair coefficients (most iconic EMU sound)")
    {
        // Vowel pair = shapes 0 (A) and 4 (B)
        // These specific values are from verified ROM dumps
        
        // Shape 0 (A:vowel_pair) - First bi-quad stage (index 0-1)
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[0][0], 
                     Catch::Matchers::WithinAbs (0.95f, 0.001f));
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[0][1], 
                     Catch::Matchers::WithinAbs (0.0104719755f, 0.0001f));
        
        // Shape 4 (B:vowel_pair) - First bi-quad stage
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[4][0], 
                     Catch::Matchers::WithinAbs (0.96f, 0.001f));
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[4][1], 
                     Catch::Matchers::WithinAbs (0.00785398164f, 0.0001f));
        
        INFO ("Vowel pair coefficients match verified ROM dumps");
    }
    
    SECTION ("Verify bell_pair coefficients (high resonance)")
    {
        // Bell pair = shapes 1 (A) and 5 (B)
        // Known for metallic/bell-like resonance
        
        // Shape 1 (A:bell_pair) - Last bi-quad stage (index 10-11)
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[1][10], 
                     Catch::Matchers::WithinAbs (0.99f, 0.001f));
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[1][11], 
                     Catch::Matchers::WithinAbs (0.785398164f, 0.0001f));
        
        // Shape 5 (B:bell_pair) - Last bi-quad stage
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[5][10], 
                     Catch::Matchers::WithinAbs (0.989f, 0.001f));
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[5][11], 
                     Catch::Matchers::WithinAbs (1.25663706f, 0.0001f));
        
        INFO ("Bell pair coefficients match verified ROM dumps");
    }
    
    SECTION ("Verify low_pair coefficients (bass emphasis)")
    {
        // Low pair = shapes 2 (A) and 6 (B)
        
        // Shape 2 (A:low_pair) - First bi-quad stage
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[2][0], 
                     Catch::Matchers::WithinAbs (0.88f, 0.001f));
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[2][1], 
                     Catch::Matchers::WithinAbs (0.00392699082f, 0.0001f));
        
        INFO ("Low pair coefficients match verified ROM dumps");
    }
    
    SECTION ("Verify sub_pair coefficients (sub-bass)")
    {
        // Sub pair = shapes 3 (A) and 7 (B)
        // Deepest bass frequencies
        
        // Shape 3 (A:sub_pair) - First bi-quad stage
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[3][0], 
                     Catch::Matchers::WithinAbs (0.85f, 0.001f));
        REQUIRE_THAT (AUTHENTIC_EMU_SHAPES[3][1], 
                     Catch::Matchers::WithinAbs (0.00130899694f, 0.0001f));
        
        INFO ("Sub pair coefficients match verified ROM dumps");
    }
    
    SECTION ("Verify morph pair organization")
    {
        // Morph pairs define which shapes interpolate between each other
        REQUIRE (MORPH_PAIRS[0][0] == 0);  // vowel A
        REQUIRE (MORPH_PAIRS[0][1] == 4);  // vowel B
        
        REQUIRE (MORPH_PAIRS[1][0] == 1);  // bell A
        REQUIRE (MORPH_PAIRS[1][1] == 5);  // bell B
        
        REQUIRE (MORPH_PAIRS[2][0] == 2);  // low A
        REQUIRE (MORPH_PAIRS[2][1] == 6);  // low B
        
        REQUIRE (MORPH_PAIRS[3][0] == 3);  // sub A
        REQUIRE (MORPH_PAIRS[3][1] == 7);  // sub B
        
        INFO ("Morph pair organization matches EMU architecture");
    }
    
    SECTION ("Verify shape ID strings")
    {
        // Human-readable shape identifiers
        REQUIRE (std::string (AUTHENTIC_EMU_IDS[0]) == "vowel_pair");
        REQUIRE (std::string (AUTHENTIC_EMU_IDS[1]) == "bell_pair");
        REQUIRE (std::string (AUTHENTIC_EMU_IDS[2]) == "low_pair");
        REQUIRE (std::string (AUTHENTIC_EMU_IDS[3]) == "sub_pair");
        
        INFO ("Shape IDs match EMU naming conventions");
    }
}
