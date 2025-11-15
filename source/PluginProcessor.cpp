#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

//==============================================================================
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Shape pair selector: 0=Vowel, 1=Bell, 2=Low, 3=Sub
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "pair",
        "Pair",
        0, 3,
        0));   // Default: Vowel pair

    // Morph: Continuously interpolates between pair shapes (0.0 = A, 1.0 = B)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "morph",
        "Morph",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.25f));  // Slightly off pure A - more interesting, immediate character

    // Intensity: Formant resonance strength (0.0 = bypass, 1.0 = maximum)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "intensity",
        "Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.33f));  // Enhanced not changed - immediate "whoa" moment (r ≈ 0.87, totally safe)

    // Mix: Wet/dry blend (0.0 = 100% dry, 1.0 = 100% wet)
    // NOTE: INTENSITY controls "how much effect", MIX is only for parallel processing
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        1.0f));  // Users insert effect to HEAR it - full wet by default

    // PHASE 3: Auto mode - content-aware pair selection
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "auto",
        "Auto",
        false));  // Off by default - manual control

    // Danger mode: bypass adaptive gain and add +3 dB boost
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "danger",
        "Danger Mode",
        false));

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
    , presetManager_(state_)  // PHASE 4.1: Initialize preset manager with APVTS
{
    // Cache parameter pointers for RT-safe audio thread access (JUCE 8 best practice)
    pairParam_ = state_.getRawParameterValue("pair");
    morphParam_ = state_.getRawParameterValue("morph");
    intensityParam_ = state_.getRawParameterValue("intensity");
    mixParam_ = state_.getRawParameterValue("mix");
    autoParam_ = state_.getRawParameterValue("auto");      // PHASE 3
    dangerParam_ = state_.getRawParameterValue("danger");  // PHASE 4
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

std::vector<MuseZPlaneEngine::PoleData> PluginProcessor::getLastPoles() const
{
    juce::SpinLock::ScopedLockType lock(poleLock_);
    return cachedPoleFrame_;
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // PHASE 3: Switch to Authentic EMU mode (true hardware emulation)
    engine_.setMode(MuseZPlaneEngine::Mode::Authentic);

    // Initialize unified Z-plane engine with authentic EMU filter
    engine_.prepare(sampleRate, samplesPerBlock);
    engine_.reset();

    // Configure engine for transparent quality
    engine_.setPerformanceMode(emu::PerformanceMode::Authentic);  // Best quality (no-op for Authentic mode)
    engine_.setSectionSaturation(0.0f);  // Start with no saturation

    // Load initial shape pair (Vowel = 0)
    int pairIndex = pairParam_ ? static_cast<int>(*pairParam_) : 0;
    engine_.setShapePair(pairIndex);

    // Phase 4: Prepare FFT analysis buffer (pre-allocate, RT-safe)
    analysisBuffer_.setSize(1, fftSize, false, true, false);
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
    lastAnalysisTime_ = 0.0;
    lastUtteranceTime_ = 0.0;

    // Randomize first utterance delay (30-90 seconds) using instance RNG (thread-safe)
    nextUtteranceDelay_ = 30.0 + instanceRandom_.nextFloat() * 60.0;  // 30-90 sec

    // PHASE 3.1: Initialize CPU load monitoring (JUCE best practice)
    loadMeasurer_.reset(sampleRate, samplesPerBlock);

    parameterState_.prepare(sampleRate);
    float morphInit = morphParam_ ? static_cast<float>(*morphParam_) : 0.5f;
    float intensityInit = intensityParam_ ? static_cast<float>(*intensityParam_) : 0.0f;
    float mixInit = mixParam_ ? static_cast<float>(*mixParam_) : 1.0f;
    parameterState_.setTargets(pairIndex, morphInit, intensityInit, mixInit, 0.0f);
    parameterState_.consume(0);

    {
        juce::SpinLock::ScopedLockType lock(poleLock_);
        cachedPoleFrame_.clear();
    }
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

    // PHASE 3.1: CPU load measurement (automatically tracks block render time)
    const juce::AudioProcessLoadMeasurer::ScopedTimer loadTimer(loadMeasurer_, buffer.getNumSamples());

    // JUCE 8 best practice: prevent denormal CPU spikes
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear extra output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (totalNumInputChannels == 0)
        return;

    // PHASE 2.2: Optimized input sanitization (NaN/Inf → 0.0f)
    // + INPUT metering for visualizer (FabFilter/UAD standard)
    // Branchless loop allows compiler auto-vectorization (SSE2/NEON with -O2/-O3)
    int numSamples = buffer.getNumSamples();
    float inputRmsSum = 0.0f;  // Accumulate INPUT level for visualizer

    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);

        // Branchless NaN/Inf sanitization (compiler vectorizes to SIMD blend on x86/ARM)
        // Note: FloatVectorOperations doesn't have isfinite; manual loop optimizes well
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = channelData[i];
            // Ternary → cmov (scalar) or vblendps (SSE4.1) or vcmpps+vblendps (AVX)
            sample = std::isfinite(sample) ? sample : 0.0f;
            channelData[i] = sample;

            // Accumulate INPUT RMS (industry standard - visualizer shows what you're feeding processor)
            inputRmsSum += sample * sample;
        }
    }

    // Read parameters from cached atomic pointers (RT-safe)
    int pairIndex = pairParam_ ? static_cast<int>(*pairParam_) : 0;
    float morph = morphParam_ ? static_cast<float>(*morphParam_) : 0.5f;
    float intensity = intensityParam_ ? static_cast<float>(*intensityParam_) : 0.0f;
    float mix = mixParam_ ? static_cast<float>(*mixParam_) : 1.0f;

    // Visualizer gets raw morph value for continuous blending (no discrete quantization)

    // Update engine parameters (setShapePair handles caching internally)
    parameterState_.setTargets(pairIndex, morph, intensity, mix, 0.0f);
    auto parameterSnapshot = parameterState_.consume(numSamples);

    bool dangerModeEnabled = dangerParam_ ? (dangerParam_->load(std::memory_order_relaxed) > 0.5f) : false;
    engine_.setDangerMode(dangerModeEnabled);
    engine_.setShapePair(parameterSnapshot.pair);
    engine_.setMorph(parameterSnapshot.morph);
    engine_.setIntensity(parameterSnapshot.intensity);
    engine_.setMix(parameterSnapshot.mix);
    engine_.setDrive(parameterSnapshot.drive);  // Placeholder: currently zero

    // CRITICAL: Update coefficients ONCE per block (prevents zipper noise)
    engine_.updateCoeffsBlock(buffer.getNumSamples());

    // Process audio (handles stereo or mono, includes wet/dry mix internally)
    if (totalNumInputChannels >= 2)
    {
        // Stereo processing
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        engine_.process(left, right, buffer.getNumSamples());
    }
    else
    {
        // Mono processing - use same buffer for both L+R
        float* mono = buffer.getWritePointer(0);
        engine_.process(mono, mono, buffer.getNumSamples());
    }

    {
        auto poles = engine_.getLastPoles();
        juce::SpinLock::ScopedLockType lock(poleLock_);
        cachedPoleFrame_ = std::move(poles);
    }

    // === PHASE 3: Content-Aware Intelligence (AUTO mode) ===
    // Psychoacoustic analysis runs at 10 Hz when AUTO is enabled
    bool autoMode = autoParam_ ? (*autoParam_ > 0.5f) : false;
    if (autoMode)
    {
        double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        if (currentTime - lastPsychoAnalysisTime_ >= 0.1)  // 10 Hz analysis
        {
            lastPsychoAnalysisTime_ = currentTime;

            // Simple content detection based on input RMS energy
            float currentRMS = std::sqrt(inputRmsSum / (numSamples * totalNumInputChannels));

            // Basic heuristics for pair selection:
            // VOWEL (0): speech/vocals (mid energy 0.1-0.3)
            // BELL (1): metallic/bright/percussive (high energy > 0.3)
            // LOW (2): bass/drums (low-mid energy 0.05-0.15)
            // SUB (3): sub-bass/rumble (very low < 0.05)

            int suggestedPair = 0;  // Default to VOWEL
            if (currentRMS > 0.3f)
                suggestedPair = 1;  // BELL - high energy
            else if (currentRMS < 0.05f)
                suggestedPair = 3;  // SUB - very low energy
            else if (currentRMS < 0.15f)
                suggestedPair = 2;  // LOW - bass range

            // Update atomic for UI feedback
            suggestedPairIndex_.store(suggestedPair, std::memory_order_relaxed);

            // Auto-switch pair parameter (APVTS will notify UI)
            if (pairParam_)
                pairParam_->store(static_cast<float>(suggestedPair), std::memory_order_relaxed);
        }
    }

    // PHASE 2.1: Unified output sanitization + RMS calculation (single pass, better cache)
    // Combines: NaN detection, clamping, and RMS accumulation
    bool nanDetected = false;
    float rmsSum = 0.0f;

    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float& sample = channelData[i];

            // Sanitize NaN/Inf
            if (!std::isfinite(sample))
            {
                sample = 0.0f;
                nanDetected = true;  // MUSE MELTDOWN: NaN/Inf detected!
            }
            // Clamp extreme but finite values
            else if (std::abs(sample) > 10.0f)
            {
                sample = (sample > 0.0f) ? 10.0f : -10.0f;
            }

            // Accumulate RMS in same loop (avoids second buffer traversal)
            rmsSum += sample * sample;
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
    // FabFilter/UAD standard: visualizer shows INPUT level (what you're feeding the processor)
    // Calculated above during input sanitization to avoid extra buffer pass
    {
        // Calculate RMS from INPUT accumulated sum (industry standard metering)
        float rms = std::sqrt(inputRmsSum / (numSamples * std::max(1, totalNumInputChannels)));

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
        normalizedLevel = std::pow(normalizedLevel, 0.7f);  // Lift low levels for better visibility
        audioLevel_.store(normalizedLevel, std::memory_order_relaxed);
    }

    // === Phase 4: Synesthetic Intelligence (Safe Trigger) ===
    // Sparse utterance trigger logic moved here; heavy / UI work deferred to handleAsyncUpdate().
    {
        const double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        const double timeSinceLast = currentTime - lastUtteranceTime_;

        // Only evaluate window when not already pending delivery
        if (! pendingUtterance_.load(std::memory_order_relaxed) && timeSinceLast >= nextUtteranceDelay_)
        {
            // Probability gate (15% in valid window)
            if (instanceRandom_.nextFloat() < 0.15f)
            {
                // Lightweight feature placeholders (full FFT pipeline future)
                // Use current normalized audio level as crude brightness proxy.
                latestFeatures_.spectralCentroid = audioLevel_.load(std::memory_order_relaxed);
                latestFeatures_.hasStrongResonance = (intensity > 0.7f);
                latestFeatures_.isFlat = (mix < 0.3f);

                pendingUtterance_.store(true, std::memory_order_relaxed);
                triggerAsyncUpdate(); // Will call handleAsyncUpdate() on message thread.

                // PHASE 2.2: FIX - Reset window ONLY on successful trigger (was outside if block)
                // This ensures utterance intervals are accurate (30-90s) without drift
                lastUtteranceTime_ = currentTime;
                nextUtteranceDelay_ = 30.0 + instanceRandom_.nextFloat() * 60.0; // 30-90 sec
            }
        }
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
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
// AsyncUpdater delivery for sparse synesthetic utterances (message thread)
//==============================================================================
void PluginProcessor::handleAsyncUpdate()
{
    // TODO: Re-enable synesthetic utterances when TransmissionArea is integrated
    // Currently stubbed - PluginEditor doesn't have TransmissionArea component

    /* DISABLED - TransmissionArea not in current PluginEditor
    if (! pendingUtterance_.exchange(false, std::memory_order_relaxed))
        return; // Spurious

    // Snapshot parameter context (message thread safe – atomics)
    float mix = mixParam_ ? static_cast<float>(*mixParam_) : 1.0f;
    float intensity = intensityParam_ ? static_cast<float>(*intensityParam_) : 0.5f;

    pendingMessage_ = selectSynestheticMessage(latestFeatures_, mix, intensity);

    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor()))
    {
        juce::MessageManager::callAsync([editor, msg = pendingMessage_]()
        {
            auto zone = TransmissionArea::getRandomZone();
            editor->getTransmissionArea().setMessage(msg,
                                                     TransmissionArea::MessageType::None,
                                                     zone,
                                                     TransmissionArea::RenderMode::Stutter);
        });
    }
    */
}

juce::String PluginProcessor::selectSynestheticMessage(const SpectralFeatures& features,
                                                       float mix,
                                                       float intensity)
{
    auto& random = juce::Random::getSystemRandom();

    // Rare mask slips ---------------------------------------------------------
    if (random.nextFloat() < 0.001f) return "Ugh...";
    if (random.nextFloat() < 0.0002f) return "wtf (╯°□°)...";

    // Self-criticism when flat & unused
    if (features.isFlat && mix < 0.3f)
    {
        juce::StringArray msgs = {"Doings.", "Hollow.", "Uninspired."};
        return msgs[random.nextInt(msgs.size())];
    }

    // Intensity driven sharp descriptors
    if (features.hasStrongResonance || intensity > 0.75f)
    {
        juce::StringArray msgs = {"Sharp...", "Cutting...", "Bright..."};
        return msgs[random.nextInt(msgs.size())];
    }

    // Color / texture / observation pools (fallback)
    juce::StringArray colors = {"Indigo...", "Violet...", "Amber...", "Copper...", "Gold...", "Silver...", "Crystalline..."};
    juce::StringArray textures = {"Breathing...", "Blooming...", "Soft...", "Warm..."};
    juce::StringArray observations = {"Hmm.", "Wait...", "There.", "Yes."};

    float selector = random.nextFloat();
    if (selector < 0.4f) return colors[random.nextInt(colors.size())];
    if (selector < 0.7f) return textures[random.nextInt(textures.size())];
    return observations[random.nextInt(observations.size())];
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
