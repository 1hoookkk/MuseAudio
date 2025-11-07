#include "RustEmuFilter.h"

#include <juce_core/juce_core.h>

namespace RustDSP
{
    namespace
    {
        void logFailure(const char* context, EmuFilterError error) noexcept
        {
            juce::Logger::writeToLog(juce::String("[RustEmuFilter] ") + context
                                     + " failed with error code " + juce::String(static_cast<int>(error)));
        }
    } // namespace

    bool RustEmuFilter::succeeded(EmuFilterError error) noexcept
    {
        return error == EmuFilterError_Success;
    }

    RustEmuFilter::RustEmuFilter() noexcept
    {
        handle_ = emu_filter_create();
        if (handle_ == nullptr)
            juce::Logger::writeToLog("[RustEmuFilter] Failed to create EMU filter handle.");
    }

    RustEmuFilter::~RustEmuFilter() noexcept
    {
        destroyHandle();
    }

    RustEmuFilter::RustEmuFilter(RustEmuFilter&& other) noexcept
        : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }

    RustEmuFilter& RustEmuFilter::operator=(RustEmuFilter&& other) noexcept
    {
        if (this != &other)
        {
            destroyHandle();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    void RustEmuFilter::destroyHandle() noexcept
    {
        if (handle_ != nullptr)
        {
            emu_filter_destroy(handle_);
            handle_ = nullptr;
        }
    }

    bool RustEmuFilter::prepare(double sampleRate, int samplesPerBlock) noexcept
    {
        if (handle_ == nullptr)
            return false;

        const auto result = emu_filter_prepare(handle_, static_cast<float>(sampleRate), samplesPerBlock);
        if (!succeeded(result))
        {
            logFailure("prepare", result);
            return false;
        }

        return true;
    }

    void RustEmuFilter::reset() noexcept
    {
        if (handle_ != nullptr)
            emu_filter_reset(handle_);
    }

    void RustEmuFilter::setFilterType(EmuFilterType type) noexcept
    {
        if (handle_ != nullptr)
        {
            const auto result = emu_filter_set_filter_type(handle_, type);
            if (!succeeded(result))
                logFailure("setFilterType", result);
        }
    }

    void RustEmuFilter::setFilterModel(EmuFilterModel model) noexcept
    {
        if (handle_ != nullptr)
        {
            const auto result = emu_filter_set_filter_model(handle_, model);
            if (!succeeded(result))
                logFailure("setFilterModel", result);
        }
    }

    void RustEmuFilter::setFrequency(float frequency) noexcept
    {
        if (handle_ != nullptr)
            emu_filter_set_frequency(handle_, frequency);
    }

    void RustEmuFilter::setResonance(float resonance) noexcept
    {
        if (handle_ != nullptr)
            emu_filter_set_resonance(handle_, resonance);
    }

    void RustEmuFilter::setGain(float gain) noexcept
    {
        if (handle_ != nullptr)
            emu_filter_set_gain(handle_, gain);
    }

    void RustEmuFilter::setMorphPosition(float morph) noexcept
    {
        if (handle_ != nullptr)
            emu_filter_set_morph_position(handle_, morph);
    }

    void RustEmuFilter::setDrive(float drive) noexcept
    {
        if (handle_ != nullptr)
            emu_filter_set_drive(handle_, drive);
    }

    void RustEmuFilter::setCharacter(float character) noexcept
    {
        if (handle_ != nullptr)
            emu_filter_set_character(handle_, character);
    }

    void RustEmuFilter::setQuality(float quality) noexcept
    {
        if (handle_ != nullptr)
            emu_filter_set_quality(handle_, quality);
    }

    void RustEmuFilter::enableNonlinearStage(bool enable) noexcept
    {
        if (handle_ != nullptr)
            emu_filter_enable_nonlinear_stage(handle_, enable);
    }

    void RustEmuFilter::enableOversampling(int factor) noexcept
    {
        if (handle_ != nullptr)
            emu_filter_enable_oversampling(handle_, factor);
    }

    bool RustEmuFilter::loadCoefficientBankFromJson(const void* data, size_t sizeInBytes) noexcept
    {
        if (handle_ == nullptr || data == nullptr || sizeInBytes == 0)
            return false;

        auto* bytes = static_cast<const unsigned char*>(data);
        auto result = emu_filter_load_bank_from_json(handle_, bytes, static_cast<unsigned long long>(sizeInBytes));
        if (!succeeded(result))
        {
            logFailure("loadCoefficientBankFromJson", result);
            return false;
        }
        return true;
    }

    bool RustEmuFilter::setActiveBank(const std::string& bankName) noexcept
    {
        if (handle_ == nullptr)
            return false;

        const auto result = emu_filter_set_active_bank(handle_, bankName.c_str());
        if (!succeeded(result))
        {
            logFailure("setActiveBank", result);
            return false;
        }
        return true;
    }

    float RustEmuFilter::processSample(float sample) noexcept
    {
        if (handle_ == nullptr)
            return sample;

        auto temp = sample;
        emu_filter_process(handle_, &temp, 1);
        return temp;
    }

    void RustEmuFilter::processBuffer(float* buffer, int numSamples) noexcept
    {
        if (handle_ == nullptr || buffer == nullptr || numSamples <= 0)
            return;

        emu_filter_process(handle_, buffer, numSamples);
    }

    void RustEmuFilter::processStereo(float* left, float* right, int numSamples) noexcept
    {
        if (handle_ == nullptr || left == nullptr || right == nullptr || numSamples <= 0)
            return;

        emu_filter_process_stereo(handle_, left, right, numSamples);
    }

    float RustEmuFilter::getCurrentFrequency() const noexcept
    {
        return handle_ != nullptr ? emu_filter_get_current_frequency(handle_) : 0.0f;
    }

    float RustEmuFilter::getCurrentResonance() const noexcept
    {
        return handle_ != nullptr ? emu_filter_get_current_resonance(handle_) : 0.0f;
    }

    float RustEmuFilter::getCurrentMorph() const noexcept
    {
        return handle_ != nullptr ? emu_filter_get_current_morph(handle_) : 0.0f;
    }
} // namespace RustDSP
