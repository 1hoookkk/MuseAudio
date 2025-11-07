#pragma once

#if FIELD_USE_DSP_PLUGIN_READY

extern "C" {
#include "dsp_plugin.h"
}

#include <memory>

namespace DspReady
{

/**
 * Adapter that wraps DSP_PLUGIN_READY's simple filter to match the EMU filter interface.
 *
 * Maps the available functionality (frequency, resonance) and provides no-op implementations
 * for EMU-specific features (coefficient banks, morph position, drive, etc.) that don't exist
 * in the simple filter implementation.
 */
class DspFilterAdapter
{
public:
    DspFilterAdapter() = default;
    ~DspFilterAdapter();

    // Core audio processing interface
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    float processSample(float sample);

    // Mapped parameters (have effect)
    void setFrequency(float hz);
    void setResonance(float r);  // 0.0 - 1.0

    // EMU-specific methods - intentionally no-op as underlying DSP_PLUGIN_READY doesn't support these features
    void setFilterModel(int model) { /* adapter no-op */ }
    void setFilterType(int type) { /* adapter no-op */ }
    void enableOversampling(int factor) { /* adapter no-op */ }
    void enableNonlinearStage(bool enable) { /* adapter no-op */ }
    bool loadCoefficientBankFromJson(const void* data, size_t size) { return false; /* not supported */ }
    bool loadCoefficientBank(const char* path) { return false; /* not supported */ }
    void setActiveBank(const char* name) { /* adapter no-op */ }
    void setMorphPosition(float pos) { /* adapter no-op */ }
    void setCharacter(float c) { /* adapter no-op */ }
    void setDrive(float d) { /* adapter no-op */ }
    void setQuality(float q) { /* adapter no-op */ }

private:
    DspFilter* filter_ = nullptr;
    float sampleRate_ = 44100.0f;

    // Single-sample buffer for processing
    float sampleBuffer_[1];
};

} // namespace DspReady

#endif // FIELD_USE_DSP_PLUGIN_READY
