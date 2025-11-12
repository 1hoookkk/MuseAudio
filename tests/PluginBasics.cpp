#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include "dsp/MuseZPlaneEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Plugin instance", "[instance]")
{
    PluginProcessor testPlugin;

    SECTION ("name")
    {
        CHECK_THAT (testPlugin.getName().toStdString(),
            Catch::Matchers::Equals ("Pamplejuce Demo"));
    }
}

TEST_CASE ("Adaptive gain wet/dry and danger mode", "[adaptive-gain]")
{
    MuseZPlaneEngine engine;
    engine.setMode(MuseZPlaneEngine::Mode::Fast);
    engine.prepare(48000.0, 64);
    engine.reset();
    engine.setShapePair(0);
    engine.setMorph(0.5f);
    engine.setIntensity(0.4f);
    engine.setMix(0.0f);
    engine.setDangerMode(false);
    engine.updateCoeffsBlock(64);

    juce::AudioBuffer<float> buffer(2, 64);
    buffer.clear();
    buffer.setSample(0, 0, 0.5f);
    buffer.setSample(1, 0, 0.5f);

    engine.process(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());

    REQUIRE(buffer.getSample(0, 0) == Approx(0.5f).margin(0.15f));

    engine.setMix(1.0f);
    engine.setIntensity(0.0f);
    engine.setDangerMode(true);
    engine.updateCoeffsBlock(64);
    buffer.clear();
    buffer.setSample(0, 0, 0.25f);
    buffer.setSample(1, 0, 0.25f);

    engine.process(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());

    const float expected = 0.25f * juce::Decibels::decibelsToGain(3.0f);
    REQUIRE(buffer.getSample(0, 0) == Approx(expected).margin(0.05f));
}


#ifdef PAMPLEJUCE_IPP
    #include <ipp.h>

TEST_CASE ("IPP version", "[ipp]")
{
    CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2022.2.0 (r0x42db1a66)"));
}
#endif
