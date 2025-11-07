#pragma once
#include <array>
#include <juce_core/juce_core.h>

/**
 * ZPlaneShapeSet - One shape pair (A and B) for morphing
 * 6 pole pairs stored as flat array: [r0, θ0, r1, θ1, ..., r5, θ5]
 */
struct ZPlaneShapeSet
{
    using PoleArray = std::array<float, 12>; // 6 poles × 2 (r, theta)
    PoleArray shapeA;
    PoleArray shapeB;
};

/**
 * ZPlaneShapes - Runtime JSON shape loader with hardcoded fallback
 *
 * Loads from:
 *   - source/shapes/audity_shapes_A_48k.json
 *   - source/shapes/audity_shapes_B_48k.json
 *
 * Falls back to EMUAuthenticTables.h if files missing/invalid.
 * All loading happens on message thread (constructor/init).
 */
class ZPlaneShapes
{
public:
    ZPlaneShapes();

    /**
     * Load shapes from JSON files in given directory
     * Returns true if successful, false if fallback used
     */
    bool loadFromJson(const juce::File& directory);

    /**
     * Get shape pair by index (0=Vowel, 1=Bell, 2=Low, 3=Sub)
     */
    const ZPlaneShapeSet& getPair(int index) const;

    /**
     * Reset to hardcoded fallback shapes (EMUAuthenticTables.h)
     */
    void resetToFallback();

    /**
     * Check if runtime JSON data was successfully loaded
     */
    bool hasRuntimeData() const { return hasRuntimeData_; }

private:
    void loadFallback();
    bool parseJsonFile(const juce::File& file, int& outShapesFound);

    std::array<ZPlaneShapeSet, 4> pairs_;
    bool hasRuntimeData_ { false };
};
