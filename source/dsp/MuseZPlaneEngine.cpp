#include "MuseZPlaneEngine.h"
#include <zplane/EMUAuthenticTables.h>
#include <algorithm>
#include <cmath>
#include <cstring>

MuseZPlaneEngine::MuseZPlaneEngine()
    : engine_(std::in_place_type<FastEngine>)
{
}

MuseZPlaneEngine::~MuseZPlaneEngine() = default;

void MuseZPlaneEngine::prepare(double sampleRate, int samplesPerBlock)
{
    std::visit([=](auto& eng) { eng.prepare(sampleRate, samplesPerBlock); }, engine_);
}

void MuseZPlaneEngine::reset()
{
    std::visit([](auto& eng) { eng.reset(); }, engine_);
}

void MuseZPlaneEngine::setShapePair(int pairIndex)
{
    std::visit([=](auto& eng) { eng.setShapePair(pairIndex); }, engine_);
}

void MuseZPlaneEngine::setMorph(float morph)
{
    std::visit([=](auto& eng) { eng.setMorph(morph); }, engine_);
}

void MuseZPlaneEngine::setIntensity(float intensity)
{
    std::visit([=](auto& eng) { eng.setIntensity(intensity); }, engine_);
}

void MuseZPlaneEngine::setMix(float mix)
{
    std::visit([=](auto& eng) { eng.setMix(mix); }, engine_);
}

void MuseZPlaneEngine::setDrive(float drive)
{
    std::visit([=](auto& eng) { eng.setDrive(drive); }, engine_);
}

void MuseZPlaneEngine::updateCoeffsBlock(int numSamples)
{
    std::visit([=](auto& eng) { eng.updateCoeffsBlock(numSamples); }, engine_);
}

void MuseZPlaneEngine::process(float* left, float* right, int numSamples)
{
    std::visit([=](auto& eng) { eng.process(left, right, numSamples); }, engine_);
}

std::vector<MuseZPlaneEngine::PoleData> MuseZPlaneEngine::getLastPoles() const
{
    return std::visit([](const auto& eng) { return eng.getLastPoles(); }, engine_);
}

void MuseZPlaneEngine::setMode(Mode mode)
{
    if (mode == currentMode_)
        return;

    currentMode_ = mode;

    // Switch engine variant (message thread only)
    // Use emplace to construct in-place (avoids copy/move requirement)
    if (mode == Mode::Fast)
        engine_.emplace<FastEngine>();
    else
        engine_.emplace<AuthenticEngine>();
}

void MuseZPlaneEngine::setPerformanceMode(emu::PerformanceMode perfMode)
{
    std::visit([=](auto& eng) { eng.setPerformanceMode(perfMode); }, engine_);
}

void MuseZPlaneEngine::setSectionSaturation(float saturation)
{
    std::visit([=](auto& eng) { eng.setSectionSaturation(saturation); }, engine_);
}

void MuseZPlaneEngine::setDangerMode(bool enabled)
{
    std::visit([=](auto& eng) { eng.setDangerMode(enabled); }, engine_);
}

//==============================================================================
// FastEngine implementation (thin wrapper over emu::ZPlaneFilter_fast)
//==============================================================================

void MuseZPlaneEngine::FastEngine::prepare(double sampleRate, int samplesPerBlock)
{
    filter.prepare(sampleRate, samplesPerBlock);
    filter.reset();
    filter.setPerformanceMode(emu::PerformanceMode::Authentic);
    filter.setSectionSaturation(0.0f);
    filter.setMix(1.0f);
    ensureDryBuffer(samplesPerBlock);
    adaptiveMakeup_ = 1.0f;
    resetNonlinearState();
}

void MuseZPlaneEngine::FastEngine::reset()
{
    filter.reset();
    dryBuffer.clear();
    adaptiveMakeup_ = 1.0f;
    resetNonlinearState();
}

void MuseZPlaneEngine::FastEngine::setShapePair(int pairIndex)
{
    if (pairIndex == lastPairIndex)
        return;

    // Map pair index to EMU shape enums
    switch (pairIndex)
    {
        case 0: filter.setShapePair(emu::VOWEL_A, emu::VOWEL_B); break;
        case 1: filter.setShapePair(emu::BELL_A, emu::BELL_B); break;
        case 2: filter.setShapePair(emu::LOW_A, emu::LOW_B); break;
        case 3: filter.setShapePair(emu::SUB_A, emu::SUB_B); break;
        default: filter.setShapePair(emu::VOWEL_A, emu::VOWEL_B); break;
    }
    lastPairIndex = pairIndex;
}

void MuseZPlaneEngine::FastEngine::setMorph(float morph)
{
    filter.setMorph(morph);
}

void MuseZPlaneEngine::FastEngine::setIntensity(float intensity)
{
    currentIntensity = intensity;
    filter.setIntensity(intensity);
}

void MuseZPlaneEngine::FastEngine::setMix(float mix)
{
    currentMix = juce::jlimit(0.0f, 1.0f, mix);
    filter.setMix(1.0f); // always process 100% wet internally
}

void MuseZPlaneEngine::FastEngine::setDrive(float drive)
{
    filter.setDrive(drive);
}

void MuseZPlaneEngine::FastEngine::setDangerMode(bool enabled)
{
    dangerMode.store(enabled, std::memory_order_relaxed);
}

void MuseZPlaneEngine::FastEngine::setPerformanceMode(emu::PerformanceMode mode)
{
    filter.setPerformanceMode(mode);
}

void MuseZPlaneEngine::FastEngine::setSectionSaturation(float saturation)
{
    filter.setSectionSaturation(saturation);
}

void MuseZPlaneEngine::FastEngine::updateCoeffsBlock(int numSamples)
{
    filter.updateCoeffsBlock(numSamples);
}

void MuseZPlaneEngine::FastEngine::process(float* left, float* right, int numSamples)
{
    ensureDryBuffer(numSamples);

    const bool isMono = (right == left);
    copyDrySamples(left, right, numSamples, isMono);

    const bool effectActive = currentIntensity >= 0.01f;

    if (effectActive)
    {
        filter.process(left, right, numSamples);

        const float drive = 1.0f + currentIntensity * 0.5f;
        for (int i = 0; i < numSamples; ++i)
        {
            left[i] = std::tanh(left[i] * drive);
            if (!isMono)
                right[i] = std::tanh(right[i] * drive);
        }
    }
    else
    {
        auto* dryL = dryBuffer.getReadPointer(0);
        auto* dryR = dryBuffer.getReadPointer(1);
        std::copy(dryL, dryL + numSamples, left);
        if (!isMono)
            std::copy(dryR, dryR + numSamples, right);
    }

    applyAdaptiveGain(left, right, numSamples, isMono);
}

std::vector<MuseZPlaneEngine::PoleData> MuseZPlaneEngine::FastEngine::getLastPoles() const
{
    const auto& poles = filter.getLastPoles();
    std::vector<PoleData> result;
    result.reserve(poles.size());
    for (const auto& p : poles)
        result.push_back({p.r, p.theta});
    return result;
}

void MuseZPlaneEngine::FastEngine::ensureDryBuffer(int numSamples)
{
    if (dryBuffer.getNumChannels() < 2 || dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize(2, numSamples, false, false, true);
}

void MuseZPlaneEngine::FastEngine::resetNonlinearState()
{
    for (auto& ch : channelState)
    {
        ch.prevSample = 0.0f;
        ch.feedback = 0.0f;
        ch.dcBlocker = 0.0f;
        ch.noiseState.fill(0.0f);
    }
}

void MuseZPlaneEngine::FastEngine::copyDrySamples(float* left, float* right, int numSamples, bool isMono)
{
    auto* dryL = dryBuffer.getWritePointer(0);
    auto* dryR = dryBuffer.getWritePointer(1);

    std::memcpy(dryL, left, sizeof(float) * numSamples);
    if (isMono)
    {
        std::memcpy(dryR, left, sizeof(float) * numSamples);
    }
    else
    {
        std::memcpy(dryR, right, sizeof(float) * numSamples);
    }
}

void MuseZPlaneEngine::FastEngine::applyAdaptiveGain(float* left, float* right, int numSamples, bool isMono)
{
    const float* dryL = dryBuffer.getReadPointer(0);
    const float* dryR = dryBuffer.getReadPointer(1);

    float dryEnergy = 0.0f;
    float wetEnergy = 0.0f;
    float* wetR = isMono ? left : right;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dl = dryL[i];
        const float wl = left[i];
        dryEnergy += dl * dl;
        wetEnergy += wl * wl;

        if (!isMono)
        {
            const float dr = dryR[i];
            const float wr = wetR[i];
            dryEnergy += dr * dr;
            wetEnergy += wr * wr;
        }
    }

    const float channels = isMono ? 1.0f : 2.0f;
    const float denom = juce::jmax(1.0f, channels * static_cast<float>(numSamples));
    const float dryRms = std::sqrt(dryEnergy / denom);
    const float wetRms = std::sqrt(wetEnergy / denom);

    if (!dangerMode.load(std::memory_order_relaxed))
    {
        float target = 1.0f;
        if (wetRms > 0.0001f && dryRms > 0.0001f)
            target = juce::jlimit(0.25f, 2.5f, dryRms / wetRms);

        adaptiveMakeup_ += 0.2f * (target - adaptiveMakeup_);
        adaptiveMakeup_ = juce::jlimit(0.1f, 4.0f, adaptiveMakeup_);
    }

    const float appliedGain = dangerMode.load(std::memory_order_relaxed)
        ? juce::Decibels::decibelsToGain(3.0f)
        : adaptiveMakeup_;

    const float dryBlend = 1.0f - currentMix;
    const float wetBlend = currentMix;

    for (int i = 0; i < numSamples; ++i)
    {
        const float wetL = left[i] * appliedGain;
        const float dryLVal = dryL[i];
        left[i] = dryLVal * dryBlend + wetL * wetBlend;

        if (!isMono)
        {
            const float wetRVal = right[i] * appliedGain;
            const float dryRVal = dryR[i];
            right[i] = dryRVal * dryBlend + wetRVal * wetBlend;
        }
        else
        {
            // mono buffer already updated via left pointer
        }
    }
}

//==============================================================================
// AuthenticEngine implementation (dual AuthenticEMUZPlane for stereo)
//==============================================================================

void MuseZPlaneEngine::AuthenticEngine::prepare(double sampleRate, int samplesPerBlock)
{
    // Initialize stereo filters
    filterL.prepareToPlay(sampleRate);
    filterR.prepareToPlay(sampleRate);

    // Pre-allocate dry buffer to worst-case size (prevents RT allocation on buffer size changes)
    // Only allocate if buffer is too small (avoids reallocation on every prepare)
    constexpr int maxBufferSize = 8192;  // Worst case: 8192 samples @ 44.1kHz = ~185ms
    if (dryBuffer.getNumSamples() < maxBufferSize)
    {
        dryBuffer.setSize(2, maxBufferSize, false, false, true);  // Clear new allocation
    }

    // Set default authentic configuration
    filterL.setAutoMakeup(false);  // We handle makeup gain externally
    filterR.setAutoMakeup(false);

    // Disable LFO modulation (controlled externally via morph parameter)
    filterL.setLFODepth(0.0f);
    filterR.setLFODepth(0.0f);

    reset();
}

void MuseZPlaneEngine::AuthenticEngine::reset()
{
    filterL.reset();
    filterR.reset();
    dryBuffer.clear();
}

void MuseZPlaneEngine::AuthenticEngine::setShapePair(int pairIndex)
{
    if (pairIndex == lastPairIndex)
        return;

    // Map Muse pair index (0-3) to AuthenticEMUZPlane::MorphPair enum
    AuthenticEMUZPlane::MorphPair morphPair;
    switch (pairIndex)
    {
        case 0: morphPair = AuthenticEMUZPlane::VowelAe_to_VowelOo; break;           // Vowel
        case 1: morphPair = AuthenticEMUZPlane::BellMetallic_to_MetallicCluster; break; // Bell
        case 2: morphPair = AuthenticEMUZPlane::LowLPPunch_to_FormantPad; break;     // Low
        case 3: morphPair = AuthenticEMUZPlane::ResonantPeak_to_WideSpectrum; break; // Sub
        default: morphPair = AuthenticEMUZPlane::VowelAe_to_VowelOo; break;
    }

    filterL.setMorphPair(morphPair);
    filterR.setMorphPair(morphPair);
    lastPairIndex = pairIndex;
}

void MuseZPlaneEngine::AuthenticEngine::setMorph(float morph)
{
    filterL.setMorphPosition(morph);
    filterR.setMorphPosition(morph);
}

void MuseZPlaneEngine::AuthenticEngine::setIntensity(float intensity)
{
    currentIntensity = intensity;
    filterL.setIntensity(intensity);
    filterR.setIntensity(intensity);
}

void MuseZPlaneEngine::AuthenticEngine::setMix(float mix)
{
    mixAmount = mix;
}

void MuseZPlaneEngine::AuthenticEngine::setDrive(float drive)
{
    // Convert normalized 0-1 to dB (0-12dB range)
    float driveDB = drive * 12.0f;
    filterL.setDrive(driveDB);
    filterR.setDrive(driveDB);
}

void MuseZPlaneEngine::AuthenticEngine::setPerformanceMode(emu::PerformanceMode /*mode*/)
{
    // AuthenticEMUZPlane doesn't have performance modes (always authentic)
    // This is a no-op for compatibility with FastEngine interface
}

void MuseZPlaneEngine::AuthenticEngine::setSectionSaturation(float saturation)
{
    filterL.setSectionSaturation(saturation);
    filterR.setSectionSaturation(saturation);
}

void MuseZPlaneEngine::AuthenticEngine::updateCoeffsBlock(int /*numSamples*/)
{
    // AuthenticEMUZPlane updates coefficients internally in processBlock()
    // This is a no-op for compatibility with FastEngine interface
}

void MuseZPlaneEngine::AuthenticEngine::process(float* left, float* right, int numSamples)
{
    // True bypass at near-zero intensity (< 1%) for transparent passthrough
    if (currentIntensity < 0.01f)
    {
        return;  // Bypass mode: pass through unprocessed
    }

    // Save dry signal for wet/dry mixing
    if (mixAmount < 0.999f)
    {
        auto* dryL = dryBuffer.getWritePointer(0);
        auto* dryR = dryBuffer.getWritePointer(1);
        std::copy(left, left + numSamples, dryL);
        std::copy(right, right + numSamples, dryR);
    }

    // Process stereo through authentic EMU filters
    filterL.processBlock(left, numSamples);
    filterR.processBlock(right, numSamples);

    // Apply dry/wet mix if needed
    if (mixAmount < 0.999f)
    {
        const auto* dryL = dryBuffer.getReadPointer(0);
        const auto* dryR = dryBuffer.getReadPointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            left[i] = dryL[i] + (left[i] - dryL[i]) * mixAmount;
            right[i] = dryR[i] + (right[i] - dryR[i]) * mixAmount;
        }
    }

    // Apply automatic makeup gain to compensate for resonance boost
    const float makeup = calculateMakeupGain(currentIntensity);
    if (makeup < 0.999f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            left[i] *= makeup;
            right[i] *= makeup;
        }
    }
}

std::vector<MuseZPlaneEngine::PoleData> MuseZPlaneEngine::AuthenticEngine::getLastPoles() const
{
    // Return left channel poles (stereo poles should be identical)
    const auto& poles = filterL.getCurrentPoles();
    std::vector<PoleData> result;
    result.reserve(poles.size());
    for (const auto& p : poles)
        result.push_back({p.r, p.theta});
    return result;
}







