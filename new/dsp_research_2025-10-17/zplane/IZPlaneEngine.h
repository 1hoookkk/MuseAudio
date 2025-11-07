#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "ZPlaneParams.h"

struct IZPlaneEngine
{
    virtual ~IZPlaneEngine() = default;

    virtual void prepare (double fs, int blockSize, int numChannels) = 0;
    virtual void reset() = 0;
    virtual void setParams (const ZPlaneParams&) = 0;

    // base-rate linear cascade (always)
    virtual void processLinear   (juce::AudioBuffer<float>& wet) = 0;
    // nonlinear stage (drive/saturation), may be called at base or OS rate
    virtual void processNonlinear(juce::AudioBuffer<float>& wet) = 0;

    // for OS wrapper
    virtual void setProcessingSampleRate (double fs) = 0;

    // intensity≈0 & drive≈1 & sat≈0 & lfoDepth≈0
    virtual bool isEffectivelyBypassed() const = 0;
};