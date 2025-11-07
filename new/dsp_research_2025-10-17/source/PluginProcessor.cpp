#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

#include <algorithm>
#include <cmath>
#include <mutex>

#if FIELD_USE_DSP_PLUGIN_READY
// DSP_PLUGIN_READY adapter provides no-op implementations for EMU-specific methods
static constexpr int kAuthenticEmuModel = 0;
static constexpr int kVintageEmuType = 0;
#elif FIELD_ENABLE_RUST_EMU
static constexpr EmuFilterModel kAuthenticEmuModel = EmuFilterModel_AuthenticEmu;
static constexpr EmuFilterType kVintageEmuType = EmuFilterType_VintageEMU;
#else
static constexpr auto kAuthenticEmuModel = ConsolidatedDSP::EMUZPlaneFilter::FilterModel::AuthenticEmu;
static constexpr auto kVintageEmuType = ConsolidatedDSP::EMUZPlaneFilter::FilterType::VintageEMU;
#endif

namespace
{
    juce::File ensureDefaultEmuBankOnDisk()
    {
        static std::once_flag onceFlag;
        static juce::File cachedFile;

        std::call_once(onceFlag, []()
        {
            auto bankDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getChildFile("Field")
                               .getChildFile("emu_banks");
            bankDir.createDirectory();

            auto file = bankDir.getChildFile("Proteus1_fixed.json");
            const auto expectedSize = static_cast<juce::int64>(BinaryData::Proteus1_fixed_jsonSize);

            if (file.existsAsFile() && file.getSize() != expectedSize)
                file.deleteFile();

            if (!file.existsAsFile())
            {
                const bool wroteFile = file.replaceWithData(BinaryData::Proteus1_fixed_json,
                                                            static_cast<size_t>(BinaryData::Proteus1_fixed_jsonSize));
                if (!wroteFile)
                    DBG("Failed to persist Proteus coefficient bank to disk.");
            }

            cachedFile = file;
        });

        return cachedFile;
    }
}

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS",
      {
          std::make_unique<juce::AudioParameterBool>(
              bypassID, "Bypass", false),
          std::make_unique<juce::AudioParameterFloat>(
              characterID, "Character", 0.0f, 100.0f, 50.0f),
          std::make_unique<juce::AudioParameterFloat>(
              outputID, "Output", -24.0f, 6.0f, 0.0f)
      })
{
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate_ = sampleRate;
    safetyMuteSamplesLeft_ = 0;

    const auto bankFile = ensureDefaultEmuBankOnDisk();

    const auto configureFilter = [sampleRate, samplesPerBlock, &bankFile](EmuFilterImpl& filter)
    {
        filter.prepare(sampleRate, samplesPerBlock);
        filter.setFilterModel(kAuthenticEmuModel);
        filter.setFilterType(kVintageEmuType);
        filter.enableOversampling(1);
        filter.enableNonlinearStage(true);

#if FIELD_USE_DSP_PLUGIN_READY
        // DSP_PLUGIN_READY doesn't support coefficient banks - skip loading
#elif FIELD_ENABLE_RUST_EMU
        if (bankFile.existsAsFile())
        {
            juce::MemoryBlock bankData;
            if (bankFile.loadFileAsData(bankData))
            {
                if (!filter.loadCoefficientBankFromJson(bankData.getData(), bankData.getSize()))
                    DBG("Failed to load Proteus coefficient bank for Rust EMU filter.");
                else
                    filter.setActiveBank("Proteus 1");
            }
            else
            {
                DBG("Failed to read Proteus coefficient bank from disk.");
            }
        }
        else
        {
            if (!filter.loadCoefficientBankFromJson(BinaryData::Proteus1_fixed_json,
                                                    BinaryData::Proteus1_fixed_jsonSize))
                DBG("Proteus coefficient bank data unavailable for Rust EMU filter.");
            else
                filter.setActiveBank("Proteus 1");
        }
#else
        // Try to load coefficient bank (optional - filter works without it)
        bool bankLoaded = false;
        if (bankFile.existsAsFile())
        {
            bankLoaded = filter.loadCoefficientBank(bankFile.getFullPathName());
            if (!bankLoaded)
                DBG("Failed to load Proteus coefficient bank from disk. Falling back to embedded data.");
        }
        else
        {
            DBG("Proteus coefficient bank file not found on disk. Falling back to embedded data.");
        }

        if (!bankLoaded)
        {
            bankLoaded = filter.loadCoefficientBankFromJson(BinaryData::Proteus1_fixed_json,
                                                            BinaryData::Proteus1_fixed_jsonSize);
            if (bankLoaded)
            {
                DBG("Loaded Proteus coefficient bank from embedded data.");
            }
            else
            {
                DBG("Proteus coefficient bank unavailable - using default filter response.");
            }
        }

        if (bankLoaded)
            filter.setActiveBank("Proteus 1");
#endif

        filter.setMorphPosition(0.5f);
        filter.setCharacter(0.5f);
        filter.setResonance(0.35f);
        filter.setDrive(0.15f);
    };

    configureFilter(leftFilter);
    configureFilter(rightFilter);

    // Setup parameter smoothing
    optimisticCharacter.setSmoothingRate(sampleRate, 20.0f);

    // ENGINE_UI_REBUILD: Impact meter moved to UI side
    // impactMeter_.prepare(sampleRate);

    // Initialize filters to current parameter values with perceptual mapping
    if (auto* charParam = parameters.getRawParameterValue(characterID))
    {
        const float charValue = charParam->load() / 100.0f; // Convert 0-100 to 0-1

        // Apply same perceptual mapping as processBlock
        const auto sCurveEP = [](float x) noexcept {
            x = juce::jlimit(0.0f, 1.0f, x);
            return 0.5f - 0.5f * std::cos(juce::MathConstants<float>::pi * x);
        };
        const float u = sCurveEP(charValue);

        const auto symExp = [](float u01, float center, float lo, float hi, float gamma) noexcept {
            const float s = 2.0f * u01 - 1.0f;
            const float val = center * std::pow(gamma, s);
            return juce::jlimit(lo, hi, val);
        };

        const float morph = u;
        const float resonance = symExp(u, 0.45f, 0.20f, 0.70f, 1.60f);
        const float drive = juce::jlimit(0.0f, 1.0f, 0.325f * std::pow(2.0f, (2.0f*u - 1.0f)));
        const float frequency = std::exp((1.0f - u) * std::log(80.0f) + u * std::log(16000.0f));
        const float quality = symExp(u, 1.05f, 0.60f, 1.50f, 1.40f);

        leftFilter.setCharacter(morph);
        leftFilter.setMorphPosition(morph);
        leftFilter.setResonance(resonance);
        leftFilter.setDrive(drive);
        leftFilter.setFrequency(frequency);
        leftFilter.setQuality(quality);

        rightFilter.setCharacter(morph);
        rightFilter.setMorphPosition(morph);
        rightFilter.setResonance(resonance);
        rightFilter.setDrive(drive);
        rightFilter.setFrequency(frequency);
        rightFilter.setQuality(quality);

        optimisticCharacter.seed(charValue);  // Initialize smoothing state to prevent audible fade
    }
}

void PluginProcessor::releaseResources()
{
    // Release any resources
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support stereo only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    if (safetyMuteSamplesLeft_ > 0)
    {
        buffer.clear();
        safetyMuteSamplesLeft_ = std::max(0, safetyMuteSamplesLeft_ - numSamples);
        leftFilter.reset();
        rightFilter.reset();
        impactValueAtomic.store(0.0f, std::memory_order_relaxed);

        // ==== ENGINE_UI_REBUILD: Zero telemetry on safety mute ====
        inputRmsDbAtomic.store(-120.0f, std::memory_order_relaxed);
        deltaTiltDbAtomic.store(0.0f, std::memory_order_relaxed);
        deltaRmsDbAtomic.store(0.0f, std::memory_order_relaxed);
        // ==== END ENGINE_UI_REBUILD ====

        return;
    }

    // Clear any output channels that don't have input
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get parameter values
    auto bypass = parameters.getRawParameterValue(bypassID)->load();
    auto outputGain = parameters.getRawParameterValue(outputID)->load();

    // If bypassed, just apply output gain and reset filters
    if (bypass > 0.5f)
    {
        auto gain = juce::Decibels::decibelsToGain(outputGain);
        buffer.applyGain(gain);

        // Keep filters in sync but don't process
        leftFilter.reset();
        rightFilter.reset();

        // Set impact to 0 when bypassed
        impactValueAtomic.store(0.0f, std::memory_order_relaxed);

        // ==== ENGINE_UI_REBUILD: Zero telemetry on bypass ====
        inputRmsDbAtomic.store(-120.0f, std::memory_order_relaxed);
        deltaTiltDbAtomic.store(0.0f, std::memory_order_relaxed);
        deltaRmsDbAtomic.store(0.0f, std::memory_order_relaxed);
        // ==== END ENGINE_UI_REBUILD ====

        return;
    }

    const auto computeRMS = [](const float* data, int numSamples) -> float
    {
        if (data == nullptr || numSamples <= 0)
            return 0.0f;

        double sum = 0.0;
        for (int i = 0; i < numSamples; ++i)
            sum += static_cast<double>(data[i]) * static_cast<double>(data[i]);

        return static_cast<float>(std::sqrt(sum / juce::jmax(1, numSamples)));
    };

    const float inputLeftRMS = totalNumInputChannels > 0 ? computeRMS(buffer.getReadPointer(0), numSamples) : 0.0f;
    juce::ignoreUnused(totalNumInputChannels > 1 ? computeRMS(buffer.getReadPointer(1), numSamples) : 0.0f);

    // ==== ENGINE_UI_REBUILD: Store input RMS in dB ====
    const float inDb = (inputLeftRMS > 0.0f) ? 20.0f * std::log10(std::max(1.0e-9f, inputLeftRMS)) : -120.0f;
    inputRmsDbAtomic.store(inDb, std::memory_order_relaxed);
    // ==== END ENGINE_UI_REBUILD ====

    // Get character parameter and update filters with optimistic value
    if (auto* charParam = parameters.getRawParameterValue(characterID))
    {
        const float charValue = charParam->load() / 100.0f; // Convert 0-100 to 0-1
        optimisticCharacter.setFromUI(charValue);

        // Use smoothed value for DSP
        const float smoothedChar = juce::jlimit(0.0f, 1.0f, optimisticCharacter.getSmoothed());

        // --- Perceptual Character mapping (GPT-5 Pro Solution: Problem 1) ---
        // Equal-power S-curve (perceptual midpoint at 0.5)
        const auto sCurveEP = [](float x) noexcept {
            x = juce::jlimit(0.0f, 1.0f, x);
            return 0.5f - 0.5f * std::cos(juce::MathConstants<float>::pi * x);
        };
        const float u = sCurveEP(smoothedChar); // drives morph perceptually

        // Symmetric exponential helper around center c with span clamp
        const auto symExp = [](float u01, float center, float lo, float hi, float gamma) noexcept {
            const float s = 2.0f * u01 - 1.0f;           // [-1, +1]
            const float val = center * std::pow(gamma, s);
            return juce::jlimit(lo, hi, val);
        };

        // Map each parameter perceptually
        const float morph = u;

        // Resonance centered at mid of [0.20, 0.70] ≈ 0.45, gamma tunes "speed" to edges
        const float resonance = symExp(u, /*center*/ 0.45f, /*lo*/ 0.20f, /*hi*/ 0.70f, /*gamma*/ 1.60f);

        // Drive: old max was 0.65, perceptual center at ~0.325
        const float drive = juce::jlimit(0.0f, 1.0f, 0.325f * std::pow(2.0f, (2.0f*u - 1.0f)));

        // Frequency: perceptual (log) map between 80 Hz and 16 kHz
        const float frequency = std::exp((1.0f - u) * std::log(80.0f) + u * std::log(16000.0f));

        // Quality: symmetric around mid of [0.60, 1.50] ≈ 1.05; slightly gentler gamma
        const float quality = symExp(u, /*center*/ 1.05f, /*lo*/ 0.60f, /*hi*/ 1.50f, /*gamma*/ 1.40f);

        leftFilter.setCharacter(morph);
        leftFilter.setMorphPosition(morph);
        leftFilter.setResonance(resonance);
        leftFilter.setDrive(drive);
        leftFilter.setFrequency(frequency);
        leftFilter.setQuality(quality);

        rightFilter.setCharacter(morph);
        rightFilter.setMorphPosition(morph);
        rightFilter.setResonance(resonance);
        rightFilter.setDrive(drive);
        rightFilter.setFrequency(frequency);
        rightFilter.setQuality(quality);
    }

    // Process stereo channels through EMU filters
    // Feed samples to perceptual impact meter (input vs output comparison)
    float* leftChannel = totalNumOutputChannels > 0 ? buffer.getWritePointer(0) : nullptr;
    float* rightChannel = totalNumOutputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    if (leftChannel != nullptr)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float inputSample = leftChannel[sample];
            const float outputSample = leftFilter.processSample(inputSample);
            leftChannel[sample] = outputSample;

            // ENGINE_UI_REBUILD: Per-sample impact meter moved to UI side
            // impactMeter_.processSample(inputSample, outputSample);
        }
    }

    if (totalNumInputChannels >= 2 && rightChannel != nullptr)
    {
        for (int sample = 0; sample < numSamples; ++sample)
            rightChannel[sample] = rightFilter.processSample(rightChannel[sample]);
    }

    const float outputLeftRMS = leftChannel != nullptr ? computeRMS(leftChannel, numSamples) : 0.0f;
    const float outputRightRMS = (totalNumInputChannels >= 2 && rightChannel != nullptr)
                                     ? computeRMS(rightChannel, numSamples)
                                     : 0.0f;

    // Apply output gain
    auto gain = juce::Decibels::decibelsToGain(outputGain);
    buffer.applyGain(gain);

    // ENGINE_UI_REBUILD: Simplified tilt calculation (replaces 8-band impact meter)
    // Calculate simple RMS delta and use character as proxy for tilt
    const float charValue = juce::jlimit(0.0f, 1.0f, optimisticCharacter.getForUI());

    const float inputRMS = inputLeftRMS;
    const float outputRMS = outputLeftRMS;
    const float eps = 1.0e-9f;

    // Simple RMS delta in dB
    const float deltaRMS = 20.0f * std::log10((outputRMS + eps) / (inputRMS + eps));

    // Approximate tilt from character value (character controls filter slope)
    const float deltaTilt = (charValue - 0.5f) * 12.0f; // ±6dB tilt estimate

    // ==== ENGINE_UI_REBUILD: Store delta values for UI telemetry ====
    deltaTiltDbAtomic.store(deltaTilt, std::memory_order_relaxed);
    deltaRmsDbAtomic.store(deltaRMS, std::memory_order_relaxed);
    // ==== END ENGINE_UI_REBUILD ====

    // Blend tilt and RMS changes (0.7 tilt, 0.3 RMS)
    const float rawImpact = 0.7f * deltaTilt + 0.3f * deltaRMS;

    // Normalize to [0,1] range using +/-12 dB window
    float impactValue = juce::jlimit(0.0f, 1.0f, (rawImpact + 12.0f) / 24.0f);

    // Apply optimistic preview blend for responsive UI
    impactValue = juce::jlimit(0.0f, 1.0f, 0.15f * charValue + 0.85f * impactValue);

    impactValueAtomic.store(impactValue, std::memory_order_relaxed);

    bool catastrophic = false;
    for (int channel = 0; channel < totalNumOutputChannels && !catastrophic; ++channel)
    {
        const float* data = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float y = data[sample];
            if (!std::isfinite(y) || std::abs(y) > 8.0f)
            {
                catastrophic = true;
                break;
            }
        }
    }

    if (catastrophic)
    {
        buffer.clear();
        leftFilter.reset();
        rightFilter.reset();
        impactValueAtomic.store(0.0f, std::memory_order_relaxed);

        // ==== ENGINE_UI_REBUILD: Zero telemetry on catastrophic failure ====
        inputRmsDbAtomic.store(-120.0f, std::memory_order_relaxed);
        deltaTiltDbAtomic.store(0.0f, std::memory_order_relaxed);
        deltaRmsDbAtomic.store(0.0f, std::memory_order_relaxed);
        // ==== END ENGINE_UI_REBUILD ====

        const int muteSamples = static_cast<int>(std::max(0.0, lastSampleRate_ * 0.1));
        if (safetyMuteSamplesLeft_ == 0)
            DBG("Field safety mute engaged (non-finite or runaway sample detected).");
        safetyMuteSamplesLeft_ = std::max(safetyMuteSamplesLeft_, muteSamples);
    }
}

//==============================================================================
juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

//==============================================================================
void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
