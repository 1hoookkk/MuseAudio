#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

//==============================================================================
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // PHASE 2: Simplified UX - Remove category selector, add Auto mode
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "auto",
        "Auto",
        false));  // Default OFF - manual control

    // PHASE 5: Shape pair selector (overridden by auto mode when enabled)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "pair",
        "Pair",
        0, 3,  // 0=Vowel, 1=Bell, 2=Low, 3=Sub
        0));   // Default: Vowel

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "morph",
        "Morph",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f));  // Neutral position

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "intensity",
        "Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f));

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
    , presetManager_(state_)  // PHASE 4.1: Initialize preset manager with APVTS
{
    // Cache parameter pointers for RT-safe audio thread access (JUCE 8 best practice)
    pairParam_ = state_.getRawParameterValue("pair");
    morphParam_ = state_.getRawParameterValue("morph");
    intensityParam_ = state_.getRawParameterValue("intensity");
    mixParam_ = state_.getRawParameterValue("mix");
    autoMakeupParam_ = state_.getRawParameterValue("autoMakeup");
    autoParam_ = state_.getRawParameterValue("auto");  // PHASE 5: Content-aware intelligence
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

    // Randomize first utterance delay (30-90 seconds) using instance RNG (thread-safe)
    nextUtteranceDelay_ = 30.0 + instanceRandom_.nextFloat() * 60.0;  // 30-90 sec

    // PHASE 3.1: Initialize CPU load monitoring (JUCE best practice)
    loadMeasurer_.reset(sampleRate, samplesPerBlock);
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
    // Branchless loop allows compiler auto-vectorization (SSE2/NEON with -O2/-O3)
    int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);

        // Branchless NaN/Inf sanitization (compiler vectorizes to SIMD blend on x86/ARM)
        // Note: FloatVectorOperations doesn't have isfinite; manual loop optimizes well
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = channelData[i];
            // Ternary → cmov (scalar) or vblendps (SSE4.1) or vcmpps+vblendps (AVX)
            channelData[i] = std::isfinite(sample) ? sample : 0.0f;
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

    // === PHASE 5: Content-Aware Intelligence ===
    // Psychoacoustic analysis for automatic shape selection (10 Hz rate)
    if (autoParam_ && *autoParam_ > 0.5f)  // Auto mode enabled
    {
        const double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001;

        if (currentTime - lastPsychoAnalysisTime_ >= psychoAnalysisInterval_)
        {
            lastPsychoAnalysisTime_ = currentTime;

            // Convert filter poles to psychoacoustic analyzer format
            const auto& poleArray = filter_.getLastPoles();
            std::vector<std::pair<float, float>> poles;
            poles.reserve(poleArray.size());
            for (const auto& p : poleArray)
                poles.emplace_back(p.r, p.theta);

            // Analyze content characteristics (vowelness, metallicity, warmth, punch)
            const auto analysis = psycho::analyzeCharacter(poles, static_cast<float>(getSampleRate()));

            // Store results for UI feedback (thread-safe atomic writes)
            detectedVowelness_.store(analysis.vowelness, std::memory_order_relaxed);
            detectedMetallicity_.store(analysis.metallicity, std::memory_order_relaxed);
            detectedWarmth_.store(analysis.warmth, std::memory_order_relaxed);
            detectedPunch_.store(analysis.punch, std::memory_order_relaxed);

            // Select best matching shape pair (highest score wins)
            const float scores[4] = {analysis.vowelness, analysis.metallicity, analysis.warmth, analysis.punch};
            int bestIndex = 0;
            for (int i = 1; i < 4; ++i)
                if (scores[i] > scores[bestIndex])
                    bestIndex = i;

            // Smooth transition (200ms time constant at 10 Hz = 0.05 alpha)
            smoothedPairTarget_ += 0.05f * (static_cast<float>(bestIndex) - smoothedPairTarget_);
            const int finalPair = juce::jlimit(0, 3, static_cast<int>(std::round(smoothedPairTarget_)));

            suggestedPairIndex_.store(finalPair, std::memory_order_relaxed);

            // Override pair parameter for auto mode (will take effect next block)
            if (pairParam_)
                pairParam_->store(static_cast<float>(finalPair), std::memory_order_relaxed);
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
    // PHASE 2.1: RMS already calculated in unified output sanitization loop above
    // This avoids a third buffer traversal (previously: input NaN, output NaN, RMS = 3 passes)
    // Now: input sanitize+preRMS, filter process, output sanitize+RMS = 2 passes + filter
    {
        // Calculate RMS from accumulated sum (already computed above)
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
    // Provide the Brutalist Temple PluginEditor UI
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
