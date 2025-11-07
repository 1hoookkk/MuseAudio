#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"

// Terminal UI system
#include "shared/ui/terminal/TerminalFieldView.h"

//==============================================================================
// Field Plugin Editor - Terminal style UI
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor& p);
    ~PluginEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PluginProcessor& audioProcessor;
    terminal::TerminalFieldView terminalView;

    // Melatonin inspector integration
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton{ "Inspect UI" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
