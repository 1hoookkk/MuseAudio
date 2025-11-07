#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/SeanceLookAndFeel.h"
#include "ui/SeanceColors.h"
#include "ui/FloatingWord.h"

/**
 * SeanceEditor - Window Into Her Studio
 * 
 * The Definitive Vision:
 * - Her silhouette fills the center (60% height, chin on centerline)
 * - 3 knobs float over her in inverted triangle
 * - Words materialize anywhere in space (environmental ghost)
 * - Warm brutalist temple with dramatic vignette
 * - She's oblivious, just doing. User is eavesdropping.
 */
class SeanceEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    SeanceEditor(PluginProcessor& p)
        : AudioProcessorEditor(&p), processorRef(p)
    {
        using namespace Muse::Layout;
        
        // === Apply Séance Look and Feel ===
        setLookAndFeel(&seanceLookAndFeel);
        
        // === Setup Knobs (Inverted Triangle Formation) ===
        auto setupKnob = [this](juce::Slider& knob, int x, int y)
        {
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knob.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f,
                                     true);
            knob.setRange(0.0, 1.0, 0.001);
            knob.setMouseDragSensitivity(300);
            knob.setBounds(x - KnobSize/2, y - KnobSize/2, KnobSize, KnobSize);
            addAndMakeVisible(knob);
        };
        
        setupKnob(morphKnob, MorphKnobX, MorphKnobY);       // Upper right (where she gazes)
        setupKnob(intensityKnob, IntensityKnobX, IntensityKnobY);  // Lower left
        setupKnob(focusKnob, FocusKnobX, FocusKnobY);       // Lower right
        
        // === Setup Knob Labels (Minimal) ===
        auto setupLabel = [this](juce::Label& label, const juce::String& text, int x, int y)
        {
            using namespace Muse::Colors;
            label.setText(text, juce::dontSendNotification);
            label.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::plain));
            label.setColour(juce::Label::textColourId, Taupe.withAlpha(0.7f));
            label.setJustificationType(juce::Justification::centred);
            label.setBounds(x - 40, y + KnobSize/2 + 4, 80, 14);
            addAndMakeVisible(label);
        };
        
        setupLabel(morphLabel, "MORPH", MorphKnobX, MorphKnobY);
        setupLabel(intensityLabel, "INTENSITY", IntensityKnobX, IntensityKnobY);
        setupLabel(focusLabel, "FOCUS", FocusKnobX, FocusKnobY);
        
        // === Setup Shape Selector (Minimal, Bottom Center) ===
        shapeSelector.setBounds(
            (CanvasWidth - ShapeSelectorWidth) / 2,
            ShapeSelectorY,
            ShapeSelectorWidth,
            ShapeSelectorHeight
        );
        shapeSelector.addItem("Vowel", 1);
        shapeSelector.addItem("Bell", 2);
        shapeSelector.addItem("Low", 3);
        shapeSelector.addItem("Sub", 4);
        shapeSelector.setSelectedId(1);
        addAndMakeVisible(shapeSelector);
        
        // === Setup Floating Word (Environmental Ghost) ===
        addAndMakeVisible(floatingWord);
        
        // === Parameter Attachments ===
        morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.getState(), "morph", morphKnob);
        intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.getState(), "intensity", intensityKnob);
        focusAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.getState(), "mix", focusKnob);
        
        // Shape selector controls "pair" parameter
        shapeSelector.onChange = [this]()
        {
            int selectedId = shapeSelector.getSelectedId();
            if (selectedId > 0)
            {
                float pairValue = (selectedId - 1) / 3.0f;  // Map 1-4 to 0.0-1.0
                processorRef.getState().getParameter("pair")->setValueNotifyingHost(pairValue);
            }
        };
        
        // === Load Silhouette Image ===
        int dataSize = 0;
        const char* imageData = BinaryData::getNamedResource("muse_silhouette_png", dataSize);
        if (imageData != nullptr && dataSize > 0)
        {
            silhouetteImage = juce::ImageCache::getFromMemory(imageData, dataSize);
        }
        
        // === Start UI Update Timer (30fps) ===
        startTimerHz(30);
        
        // === Set Size ===
        setSize(CanvasWidth, CanvasHeight);
    }
    
    ~SeanceEditor() override
    {
        setLookAndFeel(nullptr);
    }
    
    void paint(juce::Graphics& g) override
    {
        using namespace Muse;
        
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        
        // === Layer 1: Dark Texture Base (15% opacity) ===
        g.setColour(Colors::TextureBase.withAlpha(0.15f));
        g.fillAll();
        
        // === Layer 2: Warm Linen Overlay (85% opacity) ===
        g.setColour(Colors::WarmOverlay.withAlpha(0.85f));
        g.fillAll();
        
        // === Layer 3: Dramatic Vignette (Warm Center → Dark Edges) ===
        juce::ColourGradient vignette(
            juce::Colours::transparentBlack, centre.x, centre.y,
            Colors::VignetteEdge.withAlpha(0.7f), 0.0f, 0.0f,
            true  // radial
        );
        g.setGradientFill(vignette);
        g.fillAll();
        
        // === Paint Her Silhouette (She IS The UI) ===
        if (!silhouetteImage.isNull())
        {
            // Calculate silhouette dimensions
            float silhouetteHeight = Layout::CanvasHeight * Layout::SilhouetteHeightPercent;
            float aspectRatio = silhouetteImage.getWidth() / (float)silhouetteImage.getHeight();
            float silhouetteWidth = silhouetteHeight * aspectRatio;
            
            // Position: Chin on horizontal centerline, offset to left (gazing right)
            float silhouetteX = Layout::CanvasWidth * Layout::SilhouetteOffsetX - silhouetteWidth / 2.0f;
            float silhouetteY = Layout::CanvasHeight * Layout::SilhouetteChinOnCenterLine - silhouetteHeight;
            
            // Draw with 98% opacity (condensed light, not solid)
            g.setOpacity(0.98f);
            g.drawImage(silhouetteImage,
                       juce::Rectangle<float>(silhouetteX, silhouetteY, silhouetteWidth, silhouetteHeight),
                       juce::RectanglePlacement::centred);
        }
    }
    
    void resized() override
    {
        // Knobs and labels positioned in constructor (inverted triangle)
        // Nothing to do here - layout is absolute positioned
    }
    
private:
    void timerCallback() override
    {
        // TODO: Check for sparse synesthetic utterances from PluginProcessor
        // This is where we'll poll for DSP state and trigger floating words
        
        // For now, just ensure UI is responsive
        repaint();
    }
    
    PluginProcessor& processorRef;
    
    SeanceLookAndFeel seanceLookAndFeel;
    
    // === Knobs (Inverted Triangle) ===
    juce::Slider morphKnob;
    juce::Slider intensityKnob;
    juce::Slider focusKnob;
    
    // === Labels ===
    juce::Label morphLabel;
    juce::Label intensityLabel;
    juce::Label focusLabel;
    
    // === Shape Selector (Minimal) ===
    juce::ComboBox shapeSelector;
    
    // === Her Silhouette ===
    juce::Image silhouetteImage;
    
    // === Environmental Voice ===
    FloatingWord floatingWord;
    
    // === Parameter Attachments ===
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> focusAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SeanceEditor)
};
