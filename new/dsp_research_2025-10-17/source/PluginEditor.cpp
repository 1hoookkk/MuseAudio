#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
    , terminalView(audioProcessor, audioProcessor.getParameters())
{
    // Set initial size
    setSize(600, 900);
    setResizable(true, true);

    addAndMakeVisible(terminalView);

    // 60 Hz updates for telemetry
    startTimerHz(60);

    // Setup inspector button
    addAndMakeVisible(inspectButton);
    inspectButton.onClick = [this]
    {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector>(*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }
        inspector->setVisible(true);
    };
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

//==============================================================================
void PluginEditor::paint(juce::Graphics& g)
{
    // Terminal view handles all painting
    g.fillAll(juce::Colours::black);
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds();

    // Inspector button (bottom-right, overlay)
    inspectButton.setBounds(bounds.getRight() - 80, bounds.getBottom() - 35, 70, 25);

    // Terminal view fills the rest
    terminalView.setBounds(bounds);
}

void PluginEditor::timerCallback()
{
    // Push processor telemetry into terminal UI
    terminalView.setImpactValue(
        audioProcessor.getDeltaTiltDb(),
        audioProcessor.getDeltaRmsDb(),
        audioProcessor.getInputRmsDb()
    );
}
