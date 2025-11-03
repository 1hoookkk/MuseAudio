#include "PluginProcessor.h"
#include "PluginEditor.h"

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
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "intensity",
        "Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f));

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

    // Read parameters from cached atomic pointers (RT-safe)
    int pairIndex = pairParam_ ? static_cast<int>(*pairParam_) : 0;
    float morph = morphParam_ ? static_cast<float>(*morphParam_) : 0.5f;
    float intensity = intensityParam_ ? static_cast<float>(*intensityParam_) : 0.5f;
    float mix = mixParam_ ? static_cast<float>(*mixParam_) : 1.0f;
    // Note: autoMakeup not used in validated filter (always on by design)

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
    filter_.setDrive(emu::AUTHENTIC_DRIVE);  // Use authentic EMU drive amount

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
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
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
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
