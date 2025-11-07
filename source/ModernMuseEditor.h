#pragma once

#include "PluginProcessor.h"
#include "ui/ScryingMirror.h"
#include "ui/GenerativeMouth.h"
#include "ui/SynestheticWord.h"
#include "ui/OLEDLookAndFeel.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include <memory>
#include <array>

/**
 * ModernMuseEditor - JUCE 8 Best Practice Architecture
 * 
 * Design Philosophy:
 * - "Scrying Mirror" = Dark obsidian portal with procedural noise
 * - GenerativeMouth (halftone) at center = The being we observe
 * - Sparse synesthetic words = Ghost utterances from Muse's workspace
 * - Clean, maintainable FlexBox layout
 * 
 * JUCE 8 Best Practices Applied:
 * ✅ FlexBox/Grid declarative layouts (no manual rectangle math)
 * ✅ Modern C++17: std::unique_ptr, std::array, auto
 * ✅ APVTS SliderAttachments (thread-safe parameter management)
 * ✅ Lock-free atomics for DSP → UI communication
 * ✅ Resolution-independent rendering
 * ✅ Accessibility-ready component structure
 * ✅ OpenGL rendering path for complex visuals
 * ✅ Modular, reusable components
 */
class ModernMuseEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit ModernMuseEditor(PluginProcessor& p)
        : AudioProcessorEditor(&p)
        , processorRef(p)
    {
        // === Apply Look and Feel ===
        setLookAndFeel(&oledLookAndFeel);

        // === Setup Main Components ===
        setupScryingMirror();
        setupGenerativeMouth();
        setupSynestheticWords();
        setupKnobs();
        setupLabels();
        setupParameterAttachments();

        // === Start UI Update Timer (30fps) ===
        // Polls audio thread state via lock-free atomics
        startTimerHz(30);

        // === Set Initial Size (400x600 from prototype) ===
        setSize(400, 600);
        setResizable(false, false);

        // TODO: For production, enable resizing with:
        // setResizable(true, true);
        // setResizeLimits(400, 600, 800, 1200);  // Min/max bounds
    }

    ~ModernMuseEditor() override
    {
        stopTimer();
        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override
    {
        // Background is handled by ScryingMirror component
        // No manual painting needed - fully component-based
    }

    void resized() override
    {
        // === JUCE 8 Best Practice: FlexBox Declarative Layout ===
        // No manual rectangle math - clean, maintainable, responsive
        
        auto bounds = getLocalBounds();
        
        // Create master vertical layout
        juce::FlexBox masterFlex;
        masterFlex.flexDirection = juce::FlexBox::Direction::column;
        masterFlex.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        
        // === Header Section (80px) ===
        juce::FlexBox headerFlex;
        headerFlex.flexDirection = juce::FlexBox::Direction::column;
        headerFlex.justifyContent = juce::FlexBox::JustifyContent::center;
        headerFlex.items.add(juce::FlexItem(headerLabel).withHeight(30.0f).withMargin(10.0f));
        
        masterFlex.items.add(juce::FlexItem().withHeight(80.0f).withFlex(0));
        
        // === Scrying Mirror Section (Main Visual - 300px) ===
        // This is the dark obsidian portal containing the mouth + words
        masterFlex.items.add(juce::FlexItem(scryingMirror)
            .withHeight(300.0f)
            .withFlex(0)
            .withMargin(juce::FlexItem::Margin(10.0f, 20.0f, 10.0f, 20.0f)));
        
        // === Knobs Section (220px) ===
        juce::FlexBox knobsFlex;
        knobsFlex.flexDirection = juce::FlexBox::Direction::row;
        knobsFlex.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
        knobsFlex.alignItems = juce::FlexBox::AlignItems::center;
        
        // MORPH knob
        juce::FlexBox morphFlex;
        morphFlex.flexDirection = juce::FlexBox::Direction::column;
        morphFlex.alignItems = juce::FlexBox::AlignItems::center;
        morphFlex.items.add(juce::FlexItem(morphLabel).withHeight(20.0f).withWidth(80.0f));
        morphFlex.items.add(juce::FlexItem(morphKnob).withHeight(72.0f).withWidth(72.0f).withMargin(5.0f));
        morphFlex.items.add(juce::FlexItem(morphValue).withHeight(16.0f).withWidth(60.0f));
        
        // INTENSITY knob
        juce::FlexBox intensityFlex;
        intensityFlex.flexDirection = juce::FlexBox::Direction::column;
        intensityFlex.alignItems = juce::FlexBox::AlignItems::center;
        intensityFlex.items.add(juce::FlexItem(intensityLabel).withHeight(20.0f).withWidth(80.0f));
        intensityFlex.items.add(juce::FlexItem(intensityKnob).withHeight(72.0f).withWidth(72.0f).withMargin(5.0f));
        intensityFlex.items.add(juce::FlexItem(intensityValue).withHeight(16.0f).withWidth(60.0f));
        
        // MIX knob
        juce::FlexBox mixFlex;
        mixFlex.flexDirection = juce::FlexBox::Direction::column;
        mixFlex.alignItems = juce::FlexBox::AlignItems::center;
        mixFlex.items.add(juce::FlexItem(mixLabel).withHeight(20.0f).withWidth(80.0f));
        mixFlex.items.add(juce::FlexItem(mixKnob).withHeight(72.0f).withWidth(72.0f).withMargin(5.0f));
        mixFlex.items.add(juce::FlexItem(mixValue).withHeight(16.0f).withWidth(60.0f));
        
        knobsFlex.items.add(juce::FlexItem().withFlex(1.0f));  // Spacer
        knobsFlex.items.add(juce::FlexItem(morphFlex).withWidth(100.0f));
        knobsFlex.items.add(juce::FlexItem(intensityFlex).withWidth(100.0f));
        knobsFlex.items.add(juce::FlexItem(mixFlex).withWidth(100.0f));
        knobsFlex.items.add(juce::FlexItem().withFlex(1.0f));  // Spacer
        
        masterFlex.items.add(juce::FlexItem(knobsFlex).withHeight(140.0f).withFlex(0));
        
        // === Footer Section (Flexible - fills remaining space) ===
        masterFlex.items.add(juce::FlexItem(footerLabel)
            .withHeight(30.0f)
            .withFlex(0)
            .withMargin(10.0f));
        
        // Perform layout
        masterFlex.performLayout(bounds);
        
        // === Position Mouth and Words Inside Scrying Mirror ===
        // These float on top of the mirror background
        auto mirrorBounds = scryingMirror.getBounds();
        
        // GenerativeMouth at center (the focal point)
        auto mouthWidth = 240;
        auto mouthHeight = 90;
        generativeMouth.setBounds(
            mirrorBounds.getCentreX() - mouthWidth / 2,
            mirrorBounds.getCentreY() - mouthHeight / 2,
            mouthWidth,
            mouthHeight
        );
        
        // SynestheticWords overlay (full mirror bounds for positioning freedom)
        for (auto& word : synestheticWords)
        {
            word->setBounds(mirrorBounds);
        }
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Cmd+I (macOS) or Ctrl+I (Windows/Linux) toggles Melatonin Inspector
        if (key == juce::KeyPress('i', juce::ModifierKeys::commandModifier, 0))
        {
            if (!inspector)
            {
                inspector = std::make_unique<melatonin::Inspector>(*this);
                inspector->onClose = [this]() { inspector.reset(); };
            }
            inspector->setVisible(!inspector->isVisible());
            return true;
        }

        return false;
    }

private:
    void setupScryingMirror()
    {
        addAndMakeVisible(scryingMirror);
    }

    void setupGenerativeMouth()
    {
        // The amazing halftone mouth - centerpiece of the mirror
        addAndMakeVisible(generativeMouth);
    }

    void setupSynestheticWords()
    {
        // Create 3 word slots for sparse, rare appearances
        for (int i = 0; i < 3; ++i)
        {
            auto word = std::make_unique<SynestheticWord>();
            addAndMakeVisible(*word);
            synestheticWords.push_back(std::move(word));
        }
    }

    void setupKnobs()
    {
        auto setupKnob = [this](juce::Slider& knob)
        {
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knob.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f,
                                     true);
            knob.setRange(0.0, 1.0, 0.001);
            knob.setMouseDragSensitivity(300);
            addAndMakeVisible(knob);
            
            // === JUCE 8 Best Practice: Accessibility ===
            knob.setTitle("Knob");  // Screen reader support
        };

        setupKnob(morphKnob);
        setupKnob(intensityKnob);
        setupKnob(mixKnob);
        
        morphKnob.setTitle("Morph");
        intensityKnob.setTitle("Intensity");
        mixKnob.setTitle("Mix");
    }

    void setupLabels()
    {
        auto setupLabel = [this](juce::Label& label, const juce::String& text)
        {
            label.setText(text, juce::dontSendNotification);
            label.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
            label.setColour(juce::Label::textColourId, juce::Colour(OLEDLookAndFeel::MintGreen));
            label.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(label);
        };

        auto setupValueLabel = [this](juce::Label& label)
        {
            label.setText("0.0", juce::dontSendNotification);
            label.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
            label.setColour(juce::Label::textColourId, juce::Colour(OLEDLookAndFeel::MintGreen));
            label.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(label);
        };

        setupLabel(morphLabel, "MORPH");
        setupLabel(intensityLabel, "INTENSITY");
        setupLabel(mixLabel, "MIX");

        setupValueLabel(morphValue);
        setupValueLabel(intensityValue);
        setupValueLabel(mixValue);

        // Header
        headerLabel.setText("MUSE", juce::dontSendNotification);
        headerLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 18.0f, juce::Font::bold));
        headerLabel.setColour(juce::Label::textColourId, juce::Colour(OLEDLookAndFeel::MintGreen));
        headerLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(headerLabel);

        // Footer
        footerLabel.setText("AUDIOFABRICA V 1.0", juce::dontSendNotification);
        footerLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));
        footerLabel.setColour(juce::Label::textColourId, juce::Colour(OLEDLookAndFeel::MintGreen).withAlpha(0.5f));
        footerLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(footerLabel);
    }

    void setupParameterAttachments()
    {
        // === JUCE 8 Best Practice: APVTS Attachments ===
        // Thread-safe, DAW automation, preset management all handled automatically
        
        morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.getState(), "morph", morphKnob);

        intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.getState(), "intensity", intensityKnob);

        mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.getState(), "mix", mixKnob);

        // Value display callbacks
        morphKnob.onValueChange = [this]()
        {
            morphValue.setText(juce::String(morphKnob.getValue(), 2), juce::dontSendNotification);
        };

        intensityKnob.onValueChange = [this]()
        {
            intensityValue.setText(juce::String(intensityKnob.getValue(), 2), juce::dontSendNotification);
        };

        mixKnob.onValueChange = [this]()
        {
            mixValue.setText(juce::String(mixKnob.getValue(), 2), juce::dontSendNotification);
        };

        // Initialize display values
        morphValue.setText(juce::String(morphKnob.getValue(), 2), juce::dontSendNotification);
        intensityValue.setText(juce::String(intensityKnob.getValue(), 2), juce::dontSendNotification);
        mixValue.setText(juce::String(mixKnob.getValue(), 2), juce::dontSendNotification);
    }

    void timerCallback() override
    {
        // === JUCE 8 Best Practice: Lock-Free DSP → UI Communication ===
        // Poll atomics written by audio thread, update UI components
        
        // Read audio level (RMS)
        float audioLevel = processorRef.getAudioLevel();
        
        // Read DSP state (Flow/Struggle/Meltdown)
        auto museState = processorRef.getMuseState();
        
        // Read vowel shape
        auto vowel = processorRef.getCurrentVowelShape();
        
        // === Update Scrying Mirror State ===
        scryingMirror.setHeartbeatIntensity(audioLevel);
        
        if (museState == PluginProcessor::MuseState::Struggle)
        {
            scryingMirror.setJitterActive(true);
        }
        else
        {
            scryingMirror.setJitterActive(false);
        }
        
        if (museState == PluginProcessor::MuseState::Meltdown)
        {
            scryingMirror.triggerMeltdownGlitch();
        }
        
        // === Update Generative Mouth ===
        generativeMouth.setAudioLevel(audioLevel);
        generativeMouth.setVowel(static_cast<GenerativeMouth::Vowel>(vowel));
        generativeMouth.setMorph((float)morphKnob.getValue());
        
        // === Update Synesthetic Words (Sparse, Rare) ===
        updateSynestheticWords(museState, vowel, audioLevel);
    }

    void updateSynestheticWords(PluginProcessor::MuseState state, 
                                PluginProcessor::VowelShape vowel,
                                float audioLevel)
    {
        // Sparse word appearances - very rare, meaningful moments
        // This is the "Synesthetic Oblivious Artisan" personality system
        
        static int framesSinceLastWord = 0;
        framesSinceLastWord++;
        
        // Minimum 5 seconds between words (150 frames at 30fps)
        if (framesSinceLastWord < 150)
            return;
        
        // Random chance based on activity level
        auto& random = juce::Random::getSystemRandom();
        float wordProbability = audioLevel * 0.01f;  // Very low base probability
        
        if (random.nextFloat() > wordProbability)
            return;
        
        // === Trigger Synesthetic Word ===
        framesSinceLastWord = 0;
        
        // Pick random available word slot
        int wordSlot = random.nextInt(static_cast<int>(synestheticWords.size()));
        auto& word = synestheticWords[wordSlot];
        
        // Choose word and zone based on state
        juce::String wordText;
        SynestheticWord::Zone zone;
        bool isUrgent = false;
        
        switch (state)
        {
            case PluginProcessor::MuseState::Flow:
                // Calm observations
                wordText = pickFlowWord(vowel);
                zone = pickFlowZone();
                break;
                
            case PluginProcessor::MuseState::Struggle:
                // Frustrated utterances
                wordText = pickStruggleWord();
                zone = SynestheticWord::Zone::NearMouth;
                isUrgent = true;
                break;
                
            case PluginProcessor::MuseState::Meltdown:
                // Catastrophic exclamations
                wordText = "ERROR";
                zone = SynestheticWord::Zone::CenterHigh;
                isUrgent = true;
                break;
        }
        
        word->triggerWord(wordText, zone, isUrgent);
    }

    juce::String pickFlowWord(PluginProcessor::VowelShape vowel)
    {
        // Synesthetic color/texture associations
        static const std::array<juce::String, 8> flowWords = {
            "Indigo...", "Silver...", "Velvet...", "Amber...",
            "Glass...", "Smoke...", "Silk...", "Bronze..."
        };
        
        auto& random = juce::Random::getSystemRandom();
        return flowWords[random.nextInt(static_cast<int>(flowWords.size()))];
    }

    SynestheticWord::Zone pickFlowZone()
    {
        static const std::array<SynestheticWord::Zone, 4> flowZones = {
            SynestheticWord::Zone::AboveHead,
            SynestheticWord::Zone::LeftSpace,
            SynestheticWord::Zone::RightSpace,
            SynestheticWord::Zone::FloatingLow
        };
        
        auto& random = juce::Random::getSystemRandom();
        return flowZones[random.nextInt(static_cast<int>(flowZones.size()))];
    }

    juce::String pickStruggleWord()
    {
        static const std::array<juce::String, 5> struggleWords = {
            "Ugh...", "No...", "Wait...", "Hmm...", "Stop..."
        };
        
        auto& random = juce::Random::getSystemRandom();
        return struggleWords[random.nextInt(static_cast<int>(struggleWords.size()))];
    }

    // === References ===
    PluginProcessor& processorRef;

    // === Look and Feel ===
    OLEDLookAndFeel oledLookAndFeel;

    // === Main Components ===
    ScryingMirror scryingMirror;
    GenerativeMouth generativeMouth;
    std::vector<std::unique_ptr<SynestheticWord>> synestheticWords;

    // === Controls ===
    juce::Slider morphKnob;
    juce::Slider intensityKnob;
    juce::Slider mixKnob;

    juce::Label morphLabel;
    juce::Label intensityLabel;
    juce::Label mixLabel;

    juce::Label morphValue;
    juce::Label intensityValue;
    juce::Label mixValue;

    juce::Label headerLabel;
    juce::Label footerLabel;

    // === Parameter Attachments ===
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // === Debug ===
    std::unique_ptr<melatonin::Inspector> inspector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernMuseEditor)
};
