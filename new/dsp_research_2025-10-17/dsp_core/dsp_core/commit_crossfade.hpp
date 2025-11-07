#pragma once

#include <cmath>
#include <utility>
#include <memory>
#include <type_traits>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace dsp
{

template <typename DSP>
class CommitXFade
{
public:
    CommitXFade() { ensureProcessors(); }

    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        sampleRate_ = sampleRate;
        ensureProcessors();
        prepareProcessor (*active, sampleRate, maxBlockSize, numChannels);
        prepareProcessor (*pending, sampleRate, maxBlockSize, numChannels);
        scratchActive.setSize (numChannels, maxBlockSize, false, false, true);
        scratchPending.setSize (numChannels, maxBlockSize, false, false, true);
        fade_ = 1.0f;
        crossfading_ = false;
    }

    void reset()
    {
        ensureProcessors();
        active->reset();
        pending->reset();
        fade_ = 1.0f;
        crossfading_ = false;
    }

    void beginCommit (double sampleRate, float fadeMs)
    {
        sampleRate_ = sampleRate;
        ensureProcessors();
        if (fadeMs <= 0.0f)
        {
            std::swap (active, pending);
            fade_ = 1.0f;
            crossfading_ = false;
            return;
        }

        fade_ = 0.0f;
        crossfading_ = true;
        const float samples = (fadeMs / 1000.0f) * static_cast<float> (sampleRate_);
        const int steps = std::max (1, static_cast<int> (std::ceil (samples)));
        fadeIncrement_ = 1.0f / static_cast<float> (steps);
    }

    DSP& getActive() noexcept            { ensureProcessors(); return *active; }
    const DSP& getActive() const noexcept { ensureProcessors(); return *active; }
    DSP& getPending() noexcept           { ensureProcessors(); return *pending; }
    const DSP& getPending() const noexcept { ensureProcessors(); return *pending; }

    void process (juce::AudioBuffer<float>& buffer)
    {
        ensureProcessors();
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        if (scratchActive.getNumChannels() < numChannels || scratchActive.getNumSamples() < numSamples)
            scratchActive.setSize (numChannels, numSamples, false, false, true);
        if (scratchPending.getNumChannels() < numChannels || scratchPending.getNumSamples() < numSamples)
            scratchPending.setSize (numChannels, numSamples, false, false, true);

        if (! crossfading_ || fade_ >= 1.0f)
        {
            active->process (buffer);
            return;
        }

        scratchActive.makeCopyOf (buffer, true);
        scratchPending.makeCopyOf (buffer, true);

        active->process (scratchActive);
        pending->process (scratchPending);

        float fadeSample = fade_;
        for (int i = 0; i < numSamples; ++i)
        {
            const float gA = std::cos (0.5f * juce::MathConstants<float>::pi * fadeSample);
            const float gB = std::sin (0.5f * juce::MathConstants<float>::pi * fadeSample);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* a = scratchActive.getReadPointer (ch);
                const float* b = scratchPending.getReadPointer (ch);
                float* dest    = buffer.getWritePointer (ch);
                dest[i] = gA * a[i] + gB * b[i];
            }

            fadeSample = std::min (1.0f, fadeSample + fadeIncrement_);
        }

        fade_ = fadeSample;
        if (fade_ >= 1.0f - 1.0e-5f)
        {
            std::swap (active, pending);
            crossfading_ = false;
            fade_ = 1.0f;
        }
    }

    template <typename... Args>
    decltype(auto) getSectionCoeffs (Args&&... args)
    {
        ensureProcessors();
        return active->getSectionCoeffs (std::forward<Args> (args)...);
    }

    decltype(auto) getCurrentPoles()
    {
        ensureProcessors();
        return active->getCurrentPoles();
    }

    decltype(auto) getCurrentMorph()
    {
        ensureProcessors();
        return active->getCurrentMorph();
    }

    decltype(auto) getCurrentIntensity()
    {
        ensureProcessors();
        return active->getCurrentIntensity();
    }

private:
    void ensureProcessors() const
    {
        if (! active)
            active.reset (new DSP{});
        if (! pending)
            pending.reset (new DSP{});
    }

    template <typename Processor>
    static void prepareProcessor (Processor& proc, double sampleRate, int maxBlockSize, int numChannels)
    {
        if constexpr (requires { proc.prepare (sampleRate, maxBlockSize, numChannels); })
        {
            proc.prepare (sampleRate, maxBlockSize, numChannels);
        }
        else if constexpr (requires { proc.prepareToPlay (sampleRate); })
        {
            (void) maxBlockSize;
            (void) numChannels;
            proc.prepareToPlay (sampleRate);
        }
        else if constexpr (requires { proc.prepare (juce::dsp::ProcessSpec{}); })
        {
            juce::dsp::ProcessSpec spec { static_cast<double> (sampleRate), static_cast<juce::uint32> (numChannels), static_cast<juce::uint32> (numChannels) };
            proc.prepare (spec);
        }
        else
        {
            static_assert (!std::is_same_v<Processor, Processor>, "CommitXFade requires DSP to expose prepare or prepareToPlay");
        }
    }

    double sampleRate_ = 48000.0;
    float fade_ = 1.0f;
    float fadeIncrement_ = 1.0f;
    bool crossfading_ = false;

    mutable std::unique_ptr<DSP> active;
    mutable std::unique_ptr<DSP> pending;

    juce::AudioBuffer<float> scratchActive;
    juce::AudioBuffer<float> scratchPending;
};

} // namespace dsp


