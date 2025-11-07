#pragma once
#include "IShapeBank.h"
#include "EMUAuthenticTables.h"
#include <array>

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
};
