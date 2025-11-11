#include "PresetManager.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& state)
    : apvts_(state)
{
    // Ensure preset directory exists on construction
    ensurePresetDirectoryExists();
}

juce::File PresetManager::getPresetDirectory() const
{
    // Fixed preset location: C:\Muse\MuseAudio\new
    return juce::File("C:\\Muse\\MuseAudio\\new");
}

juce::File PresetManager::getPresetFile(const juce::String& presetName) const
{
    auto dir = getPresetDirectory();

    // Add .musepreset extension if not present
    juce::String filename = presetName;
    if (!filename.endsWithIgnoreCase(".musepreset"))
        filename += ".musepreset";

    return dir.getChildFile(filename);
}

bool PresetManager::ensurePresetDirectoryExists()
{
    auto dir = getPresetDirectory();

    // Create directory if it doesn't exist
    if (!dir.exists())
    {
        auto result = dir.createDirectory();
        if (result.failed())
        {
            DBG("Failed to create preset directory: " + result.getErrorMessage());
            return false;
        }
    }

    return true;
}

bool PresetManager::savePreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
    {
        DBG("Cannot save preset with empty name");
        return false;
    }

    // Ensure directory exists
    if (!ensurePresetDirectoryExists())
        return false;

    auto presetFile = getPresetFile(presetName);

    // Get current APVTS state (contains all parameter values)
    auto state = apvts_.copyState();

    // Convert ValueTree to XML
    auto xml = state.createXml();
    if (xml == nullptr)
    {
        DBG("Failed to create XML from state");
        return false;
    }

    // Add metadata to preset
    xml->setAttribute("presetName", presetName);
    xml->setAttribute("pluginName", "Muse");
    xml->setAttribute("version", "1.0");
    xml->setAttribute("timestamp", juce::Time::getCurrentTime().toISO8601(true));

    // Write XML to file
    auto result = xml->writeTo(presetFile);

    if (result)
    {
        DBG("Preset saved successfully: " + presetFile.getFullPathName());
        return true;
    }
    else
    {
        DBG("Failed to write preset file: " + presetFile.getFullPathName());
        return false;
    }
}

bool PresetManager::loadPreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
    {
        DBG("Cannot load preset with empty name");
        return false;
    }

    auto presetFile = getPresetFile(presetName);

    // Check if file exists
    if (!presetFile.existsAsFile())
    {
        DBG("Preset file does not exist: " + presetFile.getFullPathName());
        return false;
    }

    // Load XML from file
    auto xml = juce::parseXML(presetFile);
    if (xml == nullptr)
    {
        DBG("Failed to parse preset XML: " + presetFile.getFullPathName());
        return false;
    }

    // Verify it's a valid Muse preset
    if (!xml->hasAttribute("pluginName") || xml->getStringAttribute("pluginName") != "Muse")
    {
        DBG("Invalid preset file (not a Muse preset): " + presetFile.getFullPathName());
        return false;
    }

    // Convert XML to ValueTree
    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid())
    {
        DBG("Failed to create ValueTree from XML");
        return false;
    }

    // Load state into APVTS (this updates all parameters)
    apvts_.replaceState(state);

    DBG("Preset loaded successfully: " + presetFile.getFullPathName());
    return true;
}

juce::StringArray PresetManager::getAvailablePresets() const
{
    juce::StringArray presetNames;

    auto dir = getPresetDirectory();
    if (!dir.exists())
        return presetNames;

    // Find all .musepreset files
    auto presetFiles = dir.findChildFiles(juce::File::findFiles, false, "*.musepreset");

    // Extract preset names (without extension)
    for (const auto& file : presetFiles)
    {
        presetNames.add(file.getFileNameWithoutExtension());
    }

    // Sort alphabetically
    presetNames.sort(true);

    return presetNames;
}

bool PresetManager::deletePreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
    {
        DBG("Cannot delete preset with empty name");
        return false;
    }

    auto presetFile = getPresetFile(presetName);

    if (!presetFile.existsAsFile())
    {
        DBG("Preset file does not exist: " + presetFile.getFullPathName());
        return false;
    }

    // Delete the file
    bool success = presetFile.deleteFile();

    if (success)
    {
        DBG("Preset deleted successfully: " + presetFile.getFullPathName());
    }
    else
    {
        DBG("Failed to delete preset: " + presetFile.getFullPathName());
    }

    return success;
}
