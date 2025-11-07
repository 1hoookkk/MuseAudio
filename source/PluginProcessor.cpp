#include "PluginProcessor.h"
#include "SeanceEditor.h"
#include <algorithm>

//==============================================================================
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterInt>(
        "pair",
        "Shape Pair",
        0, 3, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "morph",
        "Morph",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.33f));  // Default to neutral vowel (AH) for 3-stage morphing

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "intensity",
        "Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f));  // Start transparent - Muse observes before acting

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "autoMakeup",
        "Auto Makeup",
        true));

    return layout;
}

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
                    #if ! JucePlugin_IsMidiEffect
                     #if ! JucePlugin_IsSynth
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     #endif
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                    #endif
                      )
    , state_(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for RT-safe audio thread access (JUCE 8 best practice)
    pairParam_ = state_.getRawParameterValue("pair");
    morphParam_ = state_.getRawParameterValue("morph");
    intensityParam_ = state_.getRawParameterValue("intensity");
    mixParam_ = state_.getRawParameterValue("mix");
    autoMakeupParam_ = state_.getRawParameterValue("autoMakeup");
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialize validated Z-plane filter (EngineField implementation)
    filter_.prepare(sampleRate, samplesPerBlock);
    filter_.reset();

    // Configure filter for transparent quality
    filter_.setPerformanceMode(emu::PerformanceMode::Authentic);  // Best quality for transparency
    filter_.setSectionSaturation(0.0f);  // Start with no saturation

    // Load initial shape pair (Vowel = 0)
    int pairIndex = pairParam_ ? static_cast<int>(*pairParam_) : 0;

    // Map pair index to authentic EMU shapes
    switch (pairIndex)
    {
        case 0: filter_.setShapePair(emu::VOWEL_A, emu::VOWEL_B); break;
        case 1: filter_.setShapePair(emu::BELL_A, emu::BELL_B); break;
        case 2: filter_.setShapePair(emu::LOW_A, emu::LOW_B); break;
        case 3: filter_.setShapePair(emu::SUB_A, emu::SUB_B); break;
        default: filter_.setShapePair(emu::VOWEL_A, emu::VOWEL_B); break;
    }

    lastPairIndex_ = pairIndex;

    // Phase 4: Prepare FFT analysis buffer (pre-allocate, RT-safe)
    analysisBuffer_.setSize(1, fftSize, false, true, false);
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
    lastAnalysisTime_ = 0.0;
    lastUtteranceTime_ = 0.0;

    // Randomize first utterance delay (30-90 seconds)
    auto& random = juce::Random::getSystemRandom();
    nextUtteranceDelay_ = 30.0 + random.nextFloat() * 60.0;  // 30-90 sec
}

void PluginProcessor::releaseResources()
{
    // Resources cleanup if needed
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // Support mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    // JUCE 8 best practice: prevent denormal CPU spikes
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear extra output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (totalNumInputChannels == 0)
        return;

    // Sanitize input buffer (prevent NaN/Inf from propagating)
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (!std::isfinite(channelData[i]))
                channelData[i] = 0.0f;
        }
    }

    // Read parameters from cached atomic pointers (RT-safe)
    int pairIndex = pairParam_ ? static_cast<int>(*pairParam_) : 0;
    float morph = morphParam_ ? static_cast<float>(*morphParam_) : 0.5f;
    float intensity = intensityParam_ ? static_cast<float>(*intensityParam_) : 0.0f;
    float mix = mixParam_ ? static_cast<float>(*mixParam_) : 1.0f;
    // Note: autoMakeup not used in validated filter (always on by design)

    // Calculate vowel shape for UI visualization (RT-safe atomic write)
    // ARTISTIC INTERPRETATION: 3-stage vowel morphing for enhanced expressiveness
    // The DSP morphs continuously between two Z-plane shapes (ROM-verified authentic)
    // The VISUALIZER interprets this as 3 distinct vowel positions for visual feedback
    // This gives users a more intuitive sense of the formant sweep (AA → AH → EE)
    VowelShape newVowelShape = VowelShape::AH;  // Default to middle vowel
    switch (pairIndex)
    {
        case 0:  // VOWEL pair: AA → AH → EE (3-stage creative visualization)
            // Visual interpretation: morph parameter creates vowel journey
            // morph 0.0-0.33 = AA (dark vowel, back of throat)
            // morph 0.33-0.67 = AH (neutral vowel, center transition)
            // morph 0.67-1.0 = EE (bright vowel, front articulation)
            if (morph < 0.33f)
                newVowelShape = VowelShape::AA;      // Dark/back vowel dominates
            else if (morph < 0.67f)
                newVowelShape = VowelShape::AH;      // Transition/neutral zone
            else
                newVowelShape = VowelShape::EE;      // Bright/front vowel dominates
            break;

        case 1:  // BELL pair: OH → OO (2-stage, rounded vowels)
            newVowelShape = (morph < 0.5f) ? VowelShape::OH : VowelShape::OO;
            break;

        case 2:  // LOW pair: Wide → Narrow (2-stage, bandwidth sweep)
            newVowelShape = (morph < 0.5f) ? VowelShape::Wide : VowelShape::Narrow;
            break;

        case 3:  // SUB pair: Neutral (sub-bass, non-vocal)
            newVowelShape = VowelShape::Neutral;
            break;
    }
    currentVowelShape_.store(static_cast<int>(newVowelShape), std::memory_order_relaxed);

    // Check if shape pair changed
    if (pairIndex != lastPairIndex_)
    {
        switch (pairIndex)
        {
            case 0: filter_.setShapePair(emu::VOWEL_A, emu::VOWEL_B); break;
            case 1: filter_.setShapePair(emu::BELL_A, emu::BELL_B); break;
            case 2: filter_.setShapePair(emu::LOW_A, emu::LOW_B); break;
            case 3: filter_.setShapePair(emu::SUB_A, emu::SUB_B); break;
            default: filter_.setShapePair(emu::VOWEL_A, emu::VOWEL_B); break;
        }
        lastPairIndex_ = pairIndex;
    }

    // Update filter parameters (smoothed internally via LinearSmoothedValue)
    filter_.setMorph(morph);
    filter_.setIntensity(intensity);
    filter_.setMix(mix);
    filter_.setDrive(0.0f);  // Unity gain for transparency at 0% intensity

    // CRITICAL: Update coefficients ONCE per block (prevents zipper noise)
    filter_.updateCoeffsBlock(buffer.getNumSamples());

    // Process audio (handles stereo or mono, includes wet/dry mix internally)
    if (totalNumInputChannels >= 2)
    {
        // Stereo processing
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        filter_.process(left, right, buffer.getNumSamples());
    }
    else
    {
        // Mono processing - use same buffer for both L+R
        float* mono = buffer.getWritePointer(0);
        filter_.process(mono, mono, buffer.getNumSamples());
    }

    // Sanitize output buffer (ensure filter output is always finite)
    bool nanDetected = false;
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (!std::isfinite(channelData[i]))
            {
                channelData[i] = 0.0f;
                nanDetected = true;  // MUSE MELTDOWN: NaN/Inf detected!
            }
            // Also clamp extreme but finite values to prevent downstream issues
            else if (std::abs(channelData[i]) > 10.0f)
                channelData[i] = (channelData[i] > 0.0f) ? 10.0f : -10.0f;
        }
    }
    
    // === MUSE PERSONALITY: Monitor DSP State ===
    // Calculate Muse's emotional state based on filter pole positions
    // This is the mathematical reality that drives her personality
    {
        // Estimate max pole radius from intensity parameter
        // intensity boosts pole radius: r' = r * (1.0 + intensity * 0.06)
        // At intensity=1.0, base poles (r~0.85) become r~0.90
        // At intensity=1.0 + extreme morph, poles can approach r~0.93
        const float intensityBoost = 1.0f + intensity * 0.06f;
        const float estimatedBasePole = 0.85f;  // Typical base pole radius
        const float estimatedMaxPole = estimatedBasePole * intensityBoost;
        
        // Add extra stress from extreme morph positions (0.0 or 1.0)
        const float morphStress = std::min(morph, 1.0f - morph) * 0.05f;  // 0-2.5% extra
        const float maxPoleRadius = std::min(0.99f, estimatedMaxPole + morphStress);
        
        // Determine Muse's state based on pole radius thresholds
        MuseState newState;
        if (nanDetected || maxPoleRadius >= 0.93f)
        {
            newState = MuseState::Meltdown;  // FOURTH WALL BREAK: wtf (╯°□°)...
        }
        else if (maxPoleRadius >= 0.90f || intensity > 0.75f)
        {
            newState = MuseState::Struggle;  // MASK SLIP: "Ugh..." "Trying my best..."
        }
        else
        {
            newState = MuseState::Flow;      // FLOW: "Doodling..." "Voila."
        }
        
        // Store state for UI (thread-safe)
        currentMuseState_.store(static_cast<int>(newState), std::memory_order_relaxed);
        maxPoleRadius_.store(maxPoleRadius, std::memory_order_relaxed);
        if (nanDetected)
            nanDetected_.store(true, std::memory_order_relaxed);
    }

    // === Audio Level Analysis for UI Visualization ===
    // Calculate RMS of filtered output for OLEDMouth animation (RT-safe)
    // This measures actual audio activity, not just parameter movement
    {
        float rmsSum = 0.0f;
        int numSamples = buffer.getNumSamples();

        // Sum RMS from all channels
        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            auto* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                float sample = channelData[i];
                rmsSum += sample * sample;
            }
        }

        // Calculate RMS (root-mean-square)
        float rms = std::sqrt(rmsSum / (numSamples * std::max(1, totalNumInputChannels)));

        // Time-constant based envelope follower (buffer-size independent)
        // Fast attack (10ms) for responsive "speaking", slower release (200ms) for smooth animation
        constexpr float attackTimeSec = 0.010f;   // 10ms attack
        constexpr float releaseTimeSec = 0.200f;  // 200ms release
        
        const float dt = (float)numSamples / (float)getSampleRate();
        const float attackCoeff = 1.0f - std::exp(-dt / attackTimeSec);
        const float releaseCoeff = 1.0f - std::exp(-dt / releaseTimeSec);

        if (rms > smoothedLevel_)
            smoothedLevel_ += attackCoeff * (rms - smoothedLevel_);
        else
            smoothedLevel_ += releaseCoeff * (rms - smoothedLevel_);

        // Store in atomic for UI thread (lock-free, 0-1 range)
        // Scale to reasonable range: typical RMS is 0.0-0.5, map to 0-1 for visualization
        float normalizedLevel = juce::jlimit(0.0f, 1.0f, smoothedLevel_ * 2.0f);
        audioLevel_.store(normalizedLevel, std::memory_order_relaxed);
    }

    // === Phase 4: Synesthetic Intelligence ===
    // NOTE: analyzeAudioAndMaybeSpeak() REMOVED - was accessing UI from audio thread (ILLEGAL)
    // TODO: Implement proper thread-safe analysis using AsyncUpdater or Timer
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new SeanceEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save APVTS state
    auto state = state_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore APVTS state
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(state_.state.getType()))
        {
            state_.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// Phase 4: Synesthetic Intelligence Implementation
// DISABLED: This implementation was NOT thread-safe (accessed UI from audio thread)
// TODO: Refactor using juce::AsyncUpdater or separate Timer in PluginEditor
//==============================================================================

/*
void PluginProcessor::analyzeAudioAndMaybeSpeak()
{
    // CRITICAL BUG: getActiveEditor() CANNOT be called from audio thread!
    // This entire function was being called from processBlock() which is ILLEGAL.

    auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor());
    if (!editor)
        return;  // No UI to speak to

    double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;

    // Check if enough time has passed since last utterance
    if (currentTime - lastUtteranceTime_ < nextUtteranceDelay_)
        return;  // Too soon

    // Copy audio to analysis buffer (mono mix for simplicity)
    auto numChannels = getTotalNumInputChannels();
    if (numChannels == 0)
        return;

    auto* channelData = analysisBuffer_.getWritePointer(0);
    std::fill(channelData, channelData + fftSize, 0.0f);

    // Fill with mono mix of current audio (just for spectral snapshot)
    // Note: This is a simplified approach - in practice, you'd accumulate
    // audio over time, but for sparse utterances this is sufficient

    // For now, analyze silence and trigger based on probability only
    // (Full FFT implementation would require buffering audio over time)

    // Simplified: Generate sparse utterances based on timing + probability
    auto& random = juce::Random::getSystemRandom();

    // Probability gate: 10-20% chance when time window opens
    float utteranceProbability = 0.15f;  // 15% chance
    if (random.nextFloat() > utteranceProbability)
    {
        // Not this time - reset delay for next window
        nextUtteranceDelay_ = 30.0 + random.nextFloat() * 60.0;  // 30-90 sec
        lastUtteranceTime_ = currentTime;
        return;
    }

    // Read current parameter values for context-aware selection
    float mix = mixParam_ ? static_cast<float>(*mixParam_) : 1.0f;
    float intensity = intensityParam_ ? static_cast<float>(*intensityParam_) : 0.5f;

    // For Phase 4 initial implementation: Select message based on parameters
    // (Full FFT spectral analysis would go here)
    SpectralFeatures features;
    features.isFlat = (mix < 0.3f);  // Barely being used
    features.hasStrongResonance = (intensity > 0.7f);  // High intensity

    // Simplified random color/texture selection for initial implementation
    juce::String message = selectSynestheticMessage(features, mix);

    // Send to TransmissionArea via MessageManager (thread-safe)
    juce::MessageManager::callAsync([editor, message]()
    {
        auto zone = TransmissionArea::getRandomZone();
        editor->getTransmissionArea().setMessage(message,
                                                TransmissionArea::MessageType::None,
                                                zone,
                                                TransmissionArea::RenderMode::Stutter);
    });

    // Reset for next utterance window
    lastUtteranceTime_ = currentTime;
    nextUtteranceDelay_ = 30.0 + random.nextFloat() * 60.0;  // 30-90 sec
}
*/

// Helper functions also disabled (were only used by analyzeAudioAndMaybeSpeak)
/*
PluginProcessor::SpectralFeatures PluginProcessor::extractSpectralFeatures(
    const float* spectrum, int spectrumSize)
{
    // Full FFT spectral analysis (for future enhancement)
    SpectralFeatures features;

    double sampleRate = getSampleRate();
    if (sampleRate <= 0.0)
        return features;

    float totalEnergy = 0.0f;
    float lowEnergy = 0.0f;   // 100-300 Hz
    float highEnergy = 0.0f;  // 3-7 kHz

    float weightedSum = 0.0f;
    float maxMagnitude = 0.0f;
    int maxBin = 0;

    // Frequency resolution: sampleRate / fftSize
    float binFrequency = static_cast<float>(sampleRate) / static_cast<float>(fftSize);

    for (int bin = 1; bin < spectrumSize; ++bin)
    {
        float magnitude = spectrum[bin];
        float frequency = bin * binFrequency;

        totalEnergy += magnitude;
        weightedSum += magnitude * frequency;

        // Track peak
        if (magnitude > maxMagnitude)
        {
            maxMagnitude = magnitude;
            maxBin = bin;
        }

        // Energy distribution
        if (frequency >= 100.0f && frequency <= 300.0f)
            lowEnergy += magnitude;
        if (frequency >= 3000.0f && frequency <= 7000.0f)
            highEnergy += magnitude;
    }

    features.peakFrequency = maxBin * binFrequency;
    features.spectralCentroid = (totalEnergy > 0.0f) ? (weightedSum / totalEnergy) : 0.0f;
    features.lowEnergyRatio = (totalEnergy > 0.0f) ? (lowEnergy / totalEnergy) : 0.0f;
    features.highEnergyRatio = (totalEnergy > 0.0f) ? (highEnergy / totalEnergy) : 0.0f;
    features.hasStrongResonance = (maxMagnitude > totalEnergy * 0.3f);  // Peak > 30% of total
    features.isFlat = (maxMagnitude < totalEnergy * 0.1f);  // Very distributed energy

    return features;
}

juce::String PluginProcessor::selectSynestheticMessage(
    const SpectralFeatures& features, float mix)
{
    auto& random = juce::Random::getSystemRandom();

    // ULTRA-RARE: Beautiful glitches (mask slips)
    // 1/1000 utterances → smooth "Ugh..."
    if (random.nextFloat() < 0.001f)
    {
        // Note: TransmissionArea will be set to RenderMode::Smooth externally if needed
        return "Ugh...";
    }

    // 1/5000 utterances → smooth table flip
    if (random.nextFloat() < 0.0002f)
    {
        return "wtf (╯°□°)...";
    }

    // Self-criticism: Flat response and barely being used
    if (features.isFlat && mix < 0.3f)
    {
        juce::StringArray messages = {"Doings.", "Hollow.", "Uninspired."};
        return messages[random.nextInt(messages.size())];
    }

    // HIGH INTENSITY: Sharp/cutting descriptors
    if (features.hasStrongResonance)
    {
        juce::StringArray messages = {"Sharp...", "Cutting...", "Bright..."};
        return messages[random.nextInt(messages.size())];
    }

    // Future: Full FFT-based synesthetic mapping
    // For now, random sensory observations

    // Color palette (frequency-based, simplified)
    juce::StringArray colors = {
        "Indigo...",      // Deep low-end
        "Violet...",      // Low-mid
        "Amber...",       // Mid warmth
        "Copper...",      // Mid-high
        "Gold...",        // High-mid
        "Silver...",      // High resonance
        "Crystalline..."  // Ultra-high
    };

    // Texture palette (resonance behavior)
    juce::StringArray textures = {
        "Breathing...",
        "Blooming...",
        "Soft...",
        "Warm..."
    };

    // Self-observations (non-spectral, introspective)
    juce::StringArray observations = {
        "Hmm.",
        "Wait...",
        "There.",
        "Yes."
    };

    // Weighted selection: 40% colors, 30% textures, 30% observations
    float selector = random.nextFloat();

    if (selector < 0.4f)
        return colors[random.nextInt(colors.size())];
    else if (selector < 0.7f)
        return textures[random.nextInt(textures.size())];
    else
        return observations[random.nextInt(observations.size())];
}
*/

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
