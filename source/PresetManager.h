#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

/**
 * PresetManager - Simple, RT-safe preset save/load system
 *
 * Stores actual parameter state (APVTS ValueTree) as XML files
 * Preset location: C:\Muse\MuseAudio\new\*.musepreset
 *
 * Thread-safety: All file I/O happens on message thread only
 * RT-safety: No file operations in audio callback
 */
class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& state);

    // Save current parameter state to preset file
    // Returns true on success, false on error
    // Thread: Message thread only (not RT-safe)
    bool savePreset(const juce::String& presetName);

    // Load preset file into parameter state
    // Returns true on success, false on error
    // Thread: Message thread only (not RT-safe)
    bool loadPreset(const juce::String& presetName);

    // Get list of all available presets in the preset directory
    // Thread: Message thread only (not RT-safe)
    juce::StringArray getAvailablePresets() const;

    // Get the preset directory path (C:\Muse\MuseAudio\new)
    juce::File getPresetDirectory() const;

    // Get the full path for a preset name
    juce::File getPresetFile(const juce::String& presetName) const;

    // Delete a preset file
    // Returns true on success, false on error
    // Thread: Message thread only (not RT-safe)
    bool deletePreset(const juce::String& presetName);

    // Check if preset directory exists, create if needed
    bool ensurePresetDirectoryExists();

private:
    juce::AudioProcessorValueTreeState& apvts_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
