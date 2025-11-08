#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    auto setupKnob = [&](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(s);
    };
    setupKnob(morphKnob);
    setupKnob(intensityKnob);
    setupKnob(mixKnob);

    auto setupLabel = [&](juce::Label& l, const juce::String& t)
    {
        l.setText(t, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centredBottom);
        l.setColour(juce::Label::textColourId, Muse::Colors::TextPrimary.withAlpha(0.7f));
        addAndMakeVisible(l);
    };
    setupLabel(morphLabel, "MORPH");
    setupLabel(intensityLabel, "INTENSITY");
    setupLabel(mixLabel, "MIX");

    auto setupValue = [&](juce::Label& l)
    {
        l.setJustificationType(juce::Justification::centredTop);
        l.setColour(juce::Label::textColourId, Muse::Colors::TextPrimary.withAlpha(0.5f));
        addAndMakeVisible(l);
    };
    setupValue(morphValue);
    setupValue(intensityValue);
    setupValue(mixValue);

    headerLabel.setText("MUSE", juce::dontSendNotification);
    headerLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(headerLabel);

    footerLabel.setText("AUDIOFABRICA V 1.0", juce::dontSendNotification);
    footerLabel.setJustificationType(juce::Justification::centred);
    footerLabel.setColour(juce::Label::textColourId, Muse::Colors::TextPrimary.withAlpha(0.4f));
    addAndMakeVisible(footerLabel);

    addAndMakeVisible(halftoneMouth);

    auto& apvts = processorRef.getState();
    morphAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "morph",     morphKnob);
    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "intensity", intensityKnob);
    mixAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mix",       mixKnob);

    morphKnob.onValueChange     = [this]{ morphValue.setText(juce::String(morphKnob.getValue(), 2), juce::dontSendNotification); };
    intensityKnob.onValueChange = [this]{ intensityValue.setText(juce::String(intensityKnob.getValue(), 2), juce::dontSendNotification); };
    mixKnob.onValueChange       = [this]{ mixValue.setText(juce::String(mixKnob.getValue(), 2), juce::dontSendNotification); };

    startTimerHz(30);
    setSize(400, 600);
}

PluginEditor::~PluginEditor() { stopTimer(); }

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(Muse::Colors::WarmOverlay.darker(0.1f));
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds();
    headerLabel.setBounds(bounds.removeFromTop(60));
    footerLabel.setBounds(bounds.removeFromBottom(40));

    auto visual = bounds.removeFromTop(180).reduced(20, 0);
    halftoneMouth.setBounds(visual);

    bounds.removeFromTop(40); // spacer
    auto knobRow = bounds.removeFromTop(120);
    auto left = knobRow.removeFromLeft(knobRow.getWidth() / 2);

    auto place = [](juce::Slider& s, juce::Label& name, juce::Label& val, juce::Rectangle<int> r)
    {
        name.setBounds(r.removeFromTop(30));
        val.setBounds(r.removeFromBottom(30));
        s.setBounds(r);
    };

    place(morphKnob, morphLabel, morphValue, left);
    place(intensityKnob, intensityLabel, intensityValue, knobRow);
    place(mixKnob, mixLabel, mixValue, bounds);
}

void PluginEditor::timerCallback()
{
    const auto audioLevel = processorRef.getAudioLevel();
    const auto vowel      = processorRef.getCurrentVowelShape();

    halftoneMouth.setAudioLevel(audioLevel);
    halftoneMouth.setMorph((float)morphKnob.getValue());

    switch (vowel)
    {
        case PluginProcessor::VowelShape::AA: halftoneMouth.setVowel(HalftoneMouth::Vowel::AA); break;
        case PluginProcessor::VowelShape::AH: halftoneMouth.setVowel(HalftoneMouth::Vowel::AH); break;
        case PluginProcessor::VowelShape::EE: halftoneMouth.setVowel(HalftoneMouth::Vowel::EE); break;
        case PluginProcessor::VowelShape::OH: halftoneMouth.setVowel(HalftoneMouth::Vowel::OH); break;
        case PluginProcessor::VowelShape::OO: halftoneMouth.setVowel(HalftoneMouth::Vowel::OO); break;
        default: break;
    }
}

