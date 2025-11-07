#include "ZPlaneShapes.hpp"
#include "EMUAuthenticTables.h"
#include <juce_core/juce_core.h>

ZPlaneShapes::ZPlaneShapes()
{
    loadFallback(); // Always start with fallback
}

bool ZPlaneShapes::loadFromJson(const juce::File& directory)
{
    // Try loading both JSON files
    auto fileA = directory.getChildFile("audity_shapes_A_48k.json");
    auto fileB = directory.getChildFile("audity_shapes_B_48k.json");

    if (!fileA.existsAsFile() || !fileB.existsAsFile())
    {
        juce::Logger::writeToLog("ZPlaneShapes: JSON files not found, using fallback");
        return false;
    }

    int shapesFoundA = 0, shapesFoundB = 0;

    if (!parseJsonFile(fileA, shapesFoundA) || !parseJsonFile(fileB, shapesFoundB))
    {
        juce::Logger::writeToLog("ZPlaneShapes: JSON parse failed, using fallback");
        loadFallback(); // Restore fallback on failure
        return false;
    }

    // Verify we found expected number of shapes
    if (shapesFoundA < 3 || shapesFoundB < 3)
    {
        juce::Logger::writeToLog("ZPlaneShapes: Incomplete JSON data, using fallback");
        loadFallback();
        return false;
    }

    hasRuntimeData_ = true;
    juce::Logger::writeToLog("ZPlaneShapes: Successfully loaded " +
                            juce::String(shapesFoundA) + " shapes from JSON");
    return true;
}

bool ZPlaneShapes::parseJsonFile(const juce::File& file, int& outShapesFound)
{
    auto jsonString = file.loadFileAsString();
    auto json = juce::JSON::parse(jsonString);

    if (!json.isObject())
        return false;

    auto* jsonObj = json.getDynamicObject();
    if (!jsonObj)
        return false;

    auto shapesVar = jsonObj->getProperty("shapes");
    if (!shapesVar.isArray())
        return false;

    auto* shapesArray = shapesVar.getArray();
    outShapesFound = 0;

    // Parse up to 4 shapes
    for (int i = 0; i < juce::jmin(4, shapesArray->size()); ++i)
    {
        auto shapeVar = (*shapesArray)[i];
        if (!shapeVar.isObject())
            continue;

        auto* shapeObj = shapeVar.getDynamicObject();
        auto polesVar = shapeObj->getProperty("poles");

        if (!polesVar.isArray())
            continue;

        auto* polesArray = polesVar.getArray();
        if (polesArray->size() != 6)
            continue;

        // Extract 6 pole pairs
        ZPlaneShapeSet::PoleArray poleData;
        bool validPoles = true;

        for (int p = 0; p < 6; ++p)
        {
            auto poleVar = (*polesArray)[p];
            if (!poleVar.isObject())
            {
                validPoles = false;
                break;
            }

            auto* poleObj = poleVar.getDynamicObject();
            auto r = (float)poleObj->getProperty("r");
            auto theta = (float)poleObj->getProperty("theta");

            poleData[p * 2] = r;
            poleData[p * 2 + 1] = theta;
        }

        if (!validPoles)
            continue;

        // Determine which shape (A or B) based on filename
        if (file.getFileNameWithoutExtension().contains("_A_"))
        {
            pairs_[i].shapeA = poleData;
        }
        else
        {
            pairs_[i].shapeB = poleData;
        }

        outShapesFound++;
    }

    return outShapesFound > 0;
}

const ZPlaneShapeSet& ZPlaneShapes::getPair(int index) const
{
    return pairs_[juce::jlimit(0, 3, index)];
}

void ZPlaneShapes::resetToFallback()
{
    loadFallback();
    hasRuntimeData_ = false;
}

void ZPlaneShapes::loadFallback()
{
    using namespace emu;

    // Load from EMUAuthenticTables.h
    for (int i = 0; i < 4; ++i)
    {
        const auto& pair = SHAPE_PAIRS[i];

        // Copy shape A
        for (size_t p = 0; p < 12; ++p)
            pairs_[i].shapeA[p] = pair.first[p];

        // Copy shape B
        for (size_t p = 0; p < 12; ++p)
            pairs_[i].shapeB[p] = pair.second[p];
    }
}
