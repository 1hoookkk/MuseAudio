#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EngineFieldAudioProcessor::EngineFieldAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters (*this, nullptr, juce::Identifier("EngineField"),
                  {
                      // Single CHARACTER control (0-100%)
                      std::make_unique<juce::AudioParameterFloat>(
                          "character", "Character",
                          juce::NormalisableRange<float>(0.0f, 1.0f), 0.65f),

                      // Output gain
                      std::make_unique<juce::AudioParameterFloat>(
                          "outputGain", "Output Gain",
                          juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f),

                      // Bypass
                      std::make_unique<juce::AudioParameterBool>(
                          "bypass", "Bypass", false)
                  })
{
    initializeParameters();
    initializeDSP(); // Initialize DSP early to avoid load failures
}

EngineFieldAudioProcessor::~EngineFieldAudioProcessor()
{
}

//==============================================================================
void EngineFieldAudioProcessor::initializeParameters()
{
    // Get atomic parameter pointers
    characterParam = parameters.getRawParameterValue("character");
    outputGainParam = parameters.getRawParameterValue("outputGain");
    bypassParam = parameters.getRawParameterValue("bypass");
}

void EngineFieldAudioProcessor::initializeDSP()
{
    try
    {
        // Initialize shape bank (no parameters needed - it's static data)
        shapeBank = std::make_unique<StaticShapeBank>();

        // Create Z-plane engine
        zplaneEngine = std::make_unique<AuthenticEMUEngine>(*shapeBank);

        // Create oversampler
        oversampler = std::make_unique<OversampledEngine>();
    }
    catch (const std::exception& e)
    {
        DBG("DSP initialization failed: " + juce::String(e.what()));
    }
    catch (...)
    {
        DBG("DSP initialization failed: unknown error");
    }
}

const juce::String EngineFieldAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EngineFieldAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EngineFieldAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EngineFieldAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EngineFieldAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EngineFieldAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EngineFieldAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EngineFieldAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EngineFieldAudioProcessor::getProgramName (int index)
{
    return {};
}

void EngineFieldAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EngineFieldAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialize DSP if not already done
    if (!shapeBank)
        initializeDSP();

    // Prepare audio buffers
    const int numChannels = getTotalNumOutputChannels();
    dryBuffer.setSize(numChannels, samplesPerBlock);
    wetBuffer.setSize(numChannels, samplesPerBlock);

    // Clear buffers
    dryBuffer.clear();
    wetBuffer.clear();

    // Prepare Z-plane engine (always 12th order = 6 sections)
    if (zplaneEngine)
    {
        zplaneEngine->setSectionsActive(6);
        zplaneEngine->prepare(sampleRate, samplesPerBlock, numChannels);
    }

    // Prepare oversampler (start with Off, will be set dynamically)
    if (oversampler)
    {
        oversampler->prepare(sampleRate, numChannels, OversampledEngine::Mode::Off_1x);
        oversampler->setMaxBlock(samplesPerBlock);
    }
}

void EngineFieldAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    dryBuffer.setSize(0, 0);
    wetBuffer.setSize(0, 0);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EngineFieldAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This checks if the input layout matches the output layout
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    // Only support stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
  #endif
}
#endif

void EngineFieldAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (totalNumInputChannels == 0 || totalNumOutputChannels == 0)
        return;

    // Safety check - make sure DSP is initialized
    if (!zplaneEngine)
    {
        // Just pass through with gain
        const float outputGainDb = outputGainParam ? outputGainParam->load() : 0.0f;
        const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);
        buffer.applyGain(outputGain);
        return;
    }

    // Check bypass state
    const bool isBypassed = (bypassParam && bypassParam->load() > 0.5f);

    if (isBypassed)
    {
        const float outputGainDb = outputGainParam ? outputGainParam->load() : 0.0f;
        const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);
        buffer.applyGain(outputGain);
        return;
    }

    try
    {
        updateDSPParameters();

        // Simple direct processing without oversampling for now
        if (!zplaneEngine->isEffectivelyBypassed())
        {
            zplaneEngine->processLinear(buffer);
            zplaneEngine->processNonlinear(buffer);
        }

        // Apply output gain
        const float outputGainDb = outputGainParam ? outputGainParam->load() : 0.0f;
        const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);
        buffer.applyGain(outputGain);
    }
    catch (...)
    {
        // On error, just pass through
    }
}

void EngineFieldAudioProcessor::updateDSPParameters()
{
    if (!zplaneEngine)
        return;

    // Get CHARACTER knob value
    const float character = characterParam ? characterParam->load() : 0.65f;

    // Map to internal DSP parameters
    ZPlaneParams params;

    // Intensity scales linearly
    params.intensity = character;

    // Morph follows a curve (more movement in middle range)
    params.morph = std::sin(character * juce::MathConstants<float>::pi * 0.5f);

    // Drive kicks in after 30%
    if (character > 0.3f)
        params.driveDb = (character - 0.3f) * 12.0f; // 0-8.4dB
    else
        params.driveDb = 0.0f;

    // Saturation adds warmth at high settings
    if (character > 0.5f)
        params.sat = (character - 0.5f) * 2.0f; // 0-1
    else
        params.sat = 0.0f;

    // Use bell_pair (most versatile for drums) - morphPair 1
    params.morphPair = 1;

    // LFO parameters (disabled for now)
    params.lfoRate = 0.0f;
    params.lfoDepth = 0.0f;
    params.autoMakeup = false;

    // Calibration parameters
    params.radiusGamma = 1.0f;
    params.postTiltDbPerOct = 0.0f;
    params.driveHardness = 0.5f;

    // Formant-pitch coupling
    params.formantLock = true;
    params.pitchRatio = 1.0f;

    // Apply to engine
    zplaneEngine->setParams(params);

    // Auto-engage 2x oversampling above 50%
    if (oversampler)
    {
        auto mode = character > 0.5f
            ? OversampledEngine::Mode::OS2_IIR
            : OversampledEngine::Mode::Off_1x;

        // Note: Mode changes should ideally be done in prepareToPlay
        // For now we'll keep the mode set during prepare
        // A production version would need to re-prepare when switching modes
    }
}


//==============================================================================
juce::AudioProcessorEditor* EngineFieldAudioProcessor::createEditor()
{
    return new EngineFieldAudioProcessorEditor (*this, parameters);
}

bool EngineFieldAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
void EngineFieldAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    parameters.state.setProperty("version", JucePlugin_VersionString, nullptr);
    std::unique_ptr<juce::XmlElement> xml (parameters.state.createXml());
    copyXmlToBinary (*xml, destData);
}

void EngineFieldAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EngineFieldAudioProcessor();
}