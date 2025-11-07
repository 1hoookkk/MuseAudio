#pragma once
#include "IShapeBank.h"
#include <array>

// AUTHENTIC EMU SHAPES - self-contained
static const int AUTHENTIC_EMU_SAMPLE_RATE_REF = 48000;
static const int AUTHENTIC_EMU_NUM_SHAPES = 8;
static const int AUTHENTIC_EMU_NUM_PAIRS = 4;

static const float AUTHENTIC_EMU_SHAPES[8][12] = {
	{ 0.95f, 0.0104719755f, 0.96f, 0.0196349541f, 0.985f, 0.0392699082f, 0.992f, 0.117809725f, 0.993f, 0.327249235f, 0.985f, 0.458148929f }, //   0 A:vowel_pair
	{ 0.996f, 0.143989663f, 0.995f, 0.183259572f, 0.994f, 0.287979327f, 0.993f, 0.392699082f, 0.992f, 0.549778714f, 0.99f, 0.785398164f }, //   1 A:bell_pair
	{ 0.88f, 0.00392699082f, 0.9f, 0.00785398164f, 0.92f, 0.0157079633f, 0.94f, 0.0327249235f, 0.96f, 0.065449847f, 0.97f, 0.130899694f }, //   2 A:low_pair
	{ 0.96f, 0.00785398164f, 0.98f, 0.0314159261f, 0.985f, 0.044505896f, 0.992f, 0.130899694f, 0.99f, 0.287979327f, 0.985f, 0.392699082f }, //   3 B:vowel_pair
	{ 0.997f, 0.523598776f, 0.996f, 0.628318531f, 0.995f, 0.706858347f, 0.993f, 0.942477796f, 0.991f, 1.09955743f, 0.989f, 1.25663706f }, //   4 B:bell_pair
	{ 0.97f, 0.0261799388f, 0.985f, 0.065449847f, 0.99f, 0.157079633f, 0.992f, 0.235619449f, 0.99f, 0.366519143f, 0.988f, 0.471238898f }, //   5 B:low_pair
	{ 0.85f, 0.00130899694f, 0.87f, 0.00261799388f, 0.89f, 0.00523598776f, 0.91f, 0.0104719755f, 0.93f, 0.020943951f, 0.95f, 0.041887902f }, //   6 A:sub_bass_pair
	{ 0.92f, 0.00872664626f, 0.94f, 0.0174532925f, 0.96f, 0.034906585f, 0.97f, 0.0698131701f, 0.98f, 0.104719755f, 0.97f, 0.13962634f }, //   7 B:sub_bass_pair
};

static const int MORPH_PAIRS[4][2] = {
	{ 0, 3 }, // vowel_pair
	{ 1, 4 }, // bell_pair
	{ 2, 5 }, // low_pair
	{ 6, 7 }, // sub_bass_pair
};

static const char* AUTHENTIC_EMU_IDS[4] = {
	"vowel_pair",
	"bell_pair",
	"low_pair",
	"sub_bass_pair",
};

struct StaticShapeBank final : IShapeBank
{
    std::pair<int,int> morphPairIndices (int pairIndex) const override
    {
        const int num = (int) (sizeof(MORPH_PAIRS) / sizeof(MORPH_PAIRS[0]));
        const int idx = (pairIndex < 0 ? 0 : (pairIndex >= num ? num - 1 : pairIndex));
        return { MORPH_PAIRS[idx][0], MORPH_PAIRS[idx][1] };
    }
    const std::array<float,12>& shape (int idx) const override
    {
        const int N = (int) (sizeof(AUTHENTIC_EMU_SHAPES) / sizeof(AUTHENTIC_EMU_SHAPES[0]));
        const int i = (idx < 0 ? 0 : (idx >= N ? N - 1 : idx));
        return *reinterpret_cast<const std::array<float,12>*>(AUTHENTIC_EMU_SHAPES[i]);
    }
    int numPairs()  const override { return (int) (sizeof(MORPH_PAIRS) / sizeof(MORPH_PAIRS[0])); }
    int numShapes() const override { return (int) (sizeof(AUTHENTIC_EMU_SHAPES) / sizeof(AUTHENTIC_EMU_SHAPES[0])); }
    double getReferenceSampleRate() const override { return AUTHENTIC_EMU_SAMPLE_RATE_REF; }
};