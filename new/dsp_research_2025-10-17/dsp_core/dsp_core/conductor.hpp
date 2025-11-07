#pragma once
// Minimal Conductor stub. Provides extension point for future intent generation.

#include "psycho_bus.hpp"

namespace dsp_core {

class Conductor
{
public:
    explicit Conductor(PsychoBus& bus) : bus_(bus) {}

    void prepare() {}
    void tick(const FeatureFrame&) {}

private:
    PsychoBus& bus_;
};

} // namespace dsp_core
