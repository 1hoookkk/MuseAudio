#pragma once

#include <cstddef>
#include <string>

#include "emu_filter_ffi.h"

namespace RustDSP
{
    class RustEmuFilter
    {
    public:
        RustEmuFilter() noexcept;
        ~RustEmuFilter() noexcept;

        RustEmuFilter(RustEmuFilter&& other) noexcept;
        RustEmuFilter& operator=(RustEmuFilter&& other) noexcept;

        RustEmuFilter(const RustEmuFilter&) = delete;
        RustEmuFilter& operator=(const RustEmuFilter&) = delete;

        bool prepare(double sampleRate, int samplesPerBlock) noexcept;
        void reset() noexcept;

        void setFilterType(EmuFilterType type) noexcept;
        void setFilterModel(EmuFilterModel model) noexcept;
        void setFrequency(float frequency) noexcept;
        void setResonance(float resonance) noexcept;
        void setGain(float gain) noexcept;
        void setMorphPosition(float morph) noexcept;
        void setDrive(float drive) noexcept;
        void setCharacter(float character) noexcept;
        void setQuality(float quality) noexcept;
        void enableNonlinearStage(bool enable) noexcept;
        void enableOversampling(int factor) noexcept;

        bool loadCoefficientBankFromJson(const void* data, size_t sizeInBytes) noexcept;
        bool setActiveBank(const std::string& bankName) noexcept;

        float processSample(float sample) noexcept;
        void processBuffer(float* buffer, int numSamples) noexcept;
        void processStereo(float* left, float* right, int numSamples) noexcept;

        float getCurrentFrequency() const noexcept;
        float getCurrentResonance() const noexcept;
        float getCurrentMorph() const noexcept;

        bool isValid() const noexcept { return handle_ != nullptr; }

    private:
        static bool succeeded(EmuFilterError error) noexcept;
        void destroyHandle() noexcept;

        EmuFilterHandle* handle_{nullptr};
    };

    inline EmuFilterType toRustFilterType(EmuFilterType type) noexcept { return type; }
    inline EmuFilterModel toRustFilterModel(EmuFilterModel model) noexcept { return model; }
} // namespace RustDSP
