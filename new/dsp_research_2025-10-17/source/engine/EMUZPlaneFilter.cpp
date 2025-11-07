#include "EMUZPlaneFilter.h"
#include "../../emu_data/EMUAuthenticTables.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace ConsolidatedDSP
{
    namespace
    {
        constexpr float kMinFrequency = 20.0f;
        constexpr float kMaxCharacter = 1.0f;
        constexpr float kMinCharacter = 0.0f;

        // Cascade makeup gain to compensate for 4-stage attenuation (~9dB)
        constexpr float kCascadeMakeupGain = 2.8f;

        // Stability constants for pole radius clamping
        constexpr float kMaxPoleRadius = 0.995f;  // MUST be < 1.0 for stability
        constexpr float kMinPoleRadius = 0.10f;

        /// Stabilizes biquad denominator coefficients by clamping pole radii
        /// Ensures poles stay inside unit circle (|pole| < kMaxPoleRadius < 1.0)
        /// Denominator: 1 + b1*z^-1 + b2*z^-2  =>  poles are roots of z^2 + b1*z + b2 = 0
        inline void stabilizeDenominator(float& b1, float& b2) noexcept
        {
            if (!std::isfinite(b1) || !std::isfinite(b2)) { b1 = 0.0f; b2 = 0.0f; return; }

            // Work in double precision for numerical headroom
            double b1d = static_cast<double>(b1);
            double b2d = juce::jlimit(-0.999, 0.999, static_cast<double>(b2));  // Pre-bound a2

            const double disc = b1d * b1d - 4.0 * b2d;

            if (disc >= 0.0)
            {
                // Real poles
                const double sqrtDisc = std::sqrt(disc);
                double r1 = 0.5 * (-b1d + sqrtDisc);
                double r2 = 0.5 * (-b1d - sqrtDisc);

                // Clamp each root magnitude
                auto clampRoot = [](double p) -> double {
                    const double sign = (p < 0.0) ? -1.0 : 1.0;
                    const double mag = std::abs(p);
                    const double rmax = static_cast<double>(kMaxPoleRadius);
                    const double rmin = static_cast<double>(kMinPoleRadius);
                    if (mag > rmax) return sign * rmax;
                    if (mag < rmin) return sign * rmin;
                    return p;
                };

                r1 = clampRoot(r1);
                r2 = clampRoot(r2);

                // Reconstruct: b1 = -(r1 + r2), b2 = r1*r2
                b1d = -(r1 + r2);
                b2d = r1 * r2;
            }
            else
            {
                // Complex conjugate poles: r * e^{±jθ}
                // For z^2 + b1*z + b2 = 0, |poles| = sqrt(|b2|)
                double r = std::sqrt(std::abs(b2d));

                // Clamp radius
                r = juce::jlimit(static_cast<double>(kMinPoleRadius),
                                static_cast<double>(kMaxPoleRadius), r);

                // Preserve angle via b1 (b1 = -2*r*cos(theta))
                double cosTheta = (r > 1e-12) ? (-b1d * 0.5) / r : 0.0;
                if (!std::isfinite(cosTheta)) cosTheta = 0.0;
                cosTheta = juce::jlimit(-1.0, 1.0, cosTheta);

                // Reconstruct
                b2d = r * r;
                b1d = -2.0 * r * cosTheta;
            }

            // Final "rails" clamps (mirrors C++ reference BiquadCascade)
            // a1 ∈ [-1.999, 1.999], a2 ∈ [-0.999, 0.999]
            b1 = static_cast<float>(juce::jlimit(-1.999, 1.999, b1d));
            b2 = static_cast<float>(juce::jlimit(-0.999, 0.999, b2d));
        }

        // BLT prewarp for sample rate independence
        inline float prewarpOmega(float omegaRef, float k /*= Fref/Fs*/) noexcept
        {
            const float t = std::tan(0.5f * omegaRef);
            return 2.0f * std::atan(t * k);
        }

        // Remap z-plane poles from reference SR to target SR
        inline void remapPolarFromRef(float rRef, float thRef, float kExp,
                                     float& rNow, float& thNow) noexcept
        {
            const float rSafe = juce::jlimit(1.0e-8f, 0.999999f, rRef);
            rNow  = std::pow(rSafe, kExp);
            thNow = thRef * kExp;
        }
    }

    //==============================================================================
    void EMUZPlaneFilter::SmoothParameter::setSampleRate(double sampleRate, float timeMs) noexcept
    {
        const double safeSampleRate = std::max(sampleRate, 1.0);
        const float timeSeconds = std::max(timeMs * 0.001f, 0.0001f);
        coefficient = 1.0f - std::exp(-1.0f / static_cast<float>(safeSampleRate * timeSeconds));
    }

    void EMUZPlaneFilter::SmoothParameter::setTarget(float value) noexcept
    {
        target = value;
    }

    float EMUZPlaneFilter::SmoothParameter::getNextValue() noexcept
    {
        current += (target - current) * coefficient;
        return current;
    }

    void EMUZPlaneFilter::SmoothParameter::reset(float value) noexcept
    {
        current = target = value;
    }

    //==============================================================================
    void EMUZPlaneFilter::BiquadSection::reset() noexcept
    {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    float EMUZPlaneFilter::BiquadSection::process(float input) noexcept
    {
        float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;

        // Denormal suppression
        static constexpr float denormalThreshold = 1.0e-20f;
        if (std::abs(output) < denormalThreshold)
            output = 0.0f;

        // State sanitization - clamp to prevent numerical windup
        static constexpr float maxState = 100.0f;
        auto clampState = [](float& val) {
            if (!std::isfinite(val)) val = 0.0f;
            else val = juce::jlimit(-maxState, maxState, val);
        };

        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;

        // Sanitize state variables periodically (every call is safe, minimal cost)
        clampState(x1);
        clampState(x2);
        clampState(y1);
        clampState(y2);

        return output;
    }

    //==============================================================================
    EMUZPlaneFilter::EMUZPlaneFilter()
    {
        frequency_.setSampleRate(sampleRate_);
        resonance_.setSampleRate(sampleRate_);
        gain_.setSampleRate(sampleRate_);
        morph_.setSampleRate(sampleRate_);
        drive_.setSampleRate(sampleRate_);
        character_.setSampleRate(sampleRate_);

        frequency_.reset(1000.0f);
        resonance_.reset(0.35f);
        gain_.reset(0.0f);
        morph_.reset(0.5f);
        drive_.reset(0.15f);
        character_.reset(0.5f);

        effectiveParams_ = FilterParameters{};
        updateFilterState();
    }

    EMUZPlaneFilter::~EMUZPlaneFilter() = default;

    //==============================================================================
    void EMUZPlaneFilter::prepare(double sampleRate, int samplesPerBlock)
    {
        sampleRate_ = std::max(1.0, sampleRate);
        samplesPerBlock_ = std::max(1, samplesPerBlock);

        // NEW: Calculate sample rate remapping for invariance
        refSampleRate_ = 44100.0;
        remapExp_ = static_cast<float>(refSampleRate_ / sampleRate_);

        frequency_.setSampleRate(sampleRate_);
        resonance_.setSampleRate(sampleRate_);
        gain_.setSampleRate(sampleRate_);
        morph_.setSampleRate(sampleRate_);
        drive_.setSampleRate(sampleRate_);
        character_.setSampleRate(sampleRate_);

        reset();
    }

    void EMUZPlaneFilter::reset()
    {
        // No lock needed - operates only on cascade_ state, safe for audio thread
        for (auto& section : cascade_)
            section.reset();
    }

    //==============================================================================
    void EMUZPlaneFilter::setFilterType(FilterType type) noexcept
    {
        filterType_ = type;
        updateFilterState();
    }

    void EMUZPlaneFilter::setFilterModel(FilterModel model) noexcept
    {
        filterModel_ = model;
        updateFilterState();
    }

    void EMUZPlaneFilter::setFrequency(float frequency) noexcept
    {
        frequency_.setTarget(juce::jlimit(kMinFrequency, static_cast<float>(sampleRate_ * 0.49), frequency));
    }

    void EMUZPlaneFilter::setResonance(float resonance) noexcept
    {
        resonance_.setTarget(juce::jlimit(0.05f, 0.98f, resonance));
    }

    void EMUZPlaneFilter::setGain(float gain) noexcept
    {
        gain_.setTarget(gain);
    }

    void EMUZPlaneFilter::setMorphPosition(float morph) noexcept
    {
        morph_.setTarget(juce::jlimit(0.0f, 1.0f, morph));
    }

    void EMUZPlaneFilter::setParameters(const FilterParameters& params) noexcept
    {
        setFilterType(params.type);
        setFilterModel(params.model);
        setFrequency(params.frequency);
        setResonance(params.resonance);
        setGain(params.gain);
        setMorphPosition(params.morphPosition);
        setDrive(params.drive);
        setCharacter(params.character);
        setQuality(params.quality);
    }

    void EMUZPlaneFilter::setDrive(float drive) noexcept
    {
        drive_.setTarget(juce::jlimit(0.0f, 1.0f, drive));
    }

    void EMUZPlaneFilter::setCharacter(float character) noexcept
    {
        character_.setTarget(juce::jlimit(kMinCharacter, kMaxCharacter, character));
    }

    void EMUZPlaneFilter::setQuality(float quality) noexcept
    {
        quality_ = juce::jlimit(0.25f, 2.0f, quality);
        updateFilterState();
    }

    void EMUZPlaneFilter::enableNonlinearStage(bool enable) noexcept
    {
        nonlinearEnabled_ = enable;
    }

    void EMUZPlaneFilter::enableOversampling(int factor)
    {
        oversamplingFactor_ = juce::jlimit(1, 4, factor);
    }

    //==============================================================================
    float EMUZPlaneFilter::processSample(float input) noexcept
    {
        FilterParameters params = effectiveParams_;
        params.frequency = frequency_.getNextValue();
        params.resonance = resonance_.getNextValue();
        params.gain = gain_.getNextValue();
        params.morphPosition = morph_.getNextValue();
        params.drive = drive_.getNextValue();
        params.character = character_.getNextValue();
        params.type = filterType_;
        params.model = filterModel_;

        if (const auto* bank = getCurrentBank())
        {
            const auto bankParams = interpolateMorphTargets(params.morphPosition);
            // Blend the bank data with user-controlled parameters.
            params.frequency = params.frequency * 0.4f + bankParams.frequency * 0.6f;
            params.resonance = params.resonance * 0.5f + bankParams.resonance * 0.5f;
            params.drive = std::max(params.drive, bankParams.drive);
            params.character = std::max(params.character, bankParams.character);
        }

        params.frequency = juce::jlimit(kMinFrequency, static_cast<float>(sampleRate_ * 0.48), params.frequency);
        params.resonance = juce::jlimit(0.05f, 0.98f, params.resonance);
        params.morphPosition = juce::jlimit(0.0f, 1.0f, params.morphPosition);
        params.drive = juce::jlimit(0.0f, 1.0f, params.drive);
        params.character = juce::jlimit(0.0f, 1.0f, params.character);

        const auto differences = [](const FilterParameters& a, const FilterParameters& b)
        {
            return std::abs(a.frequency - b.frequency) > 1.0f
                || std::abs(a.resonance - b.resonance) > 0.005f
                || std::abs(a.morphPosition - b.morphPosition) > 0.002f
                || std::abs(a.drive - b.drive) > 0.005f
                || std::abs(a.character - b.character) > 0.005f
                || a.type != b.type
                || a.model != b.model;
        };

        if (differences(params, effectiveParams_))
        {
            effectiveParams_ = params;
            updateFilterState();
        }

        float sample = input;
        const int iterations = std::max(1, oversamplingFactor_);
        for (int i = 0; i < iterations; ++i)
        {
            float internal = sample;
            if (nonlinearEnabled_)
                internal = applyNonlinearStage(internal * (1.0f + params.drive * 3.5f));

            for (auto& section : cascade_)
                internal = section.process(internal);

            sample = internal;
        }

        // Apply makeup gain to compensate for cascade attenuation
        sample *= kCascadeMakeupGain;

        const float postGain = juce::Decibels::decibelsToGain(params.gain);
        sample *= postGain;
        sample = fastTanh(sample * quality_);

        return juce::jlimit(-1.2f, 1.2f, sample);
    }

    void EMUZPlaneFilter::processBlock(juce::AudioBuffer<float>& buffer) noexcept
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* data = buffer.getWritePointer(channel);
            for (int i = 0; i < numSamples; ++i)
                data[i] = processSample(data[i]);
        }
    }

    void EMUZPlaneFilter::processStereo(float& left, float& right) noexcept
    {
        left = processSample(left);
        right = processSample(right);
    }

    //==============================================================================
    bool EMUZPlaneFilter::loadCoefficientBank(const juce::String& bankPath)
    {
        const juce::File file(bankPath);
        if (!file.existsAsFile())
            return false;

        const juce::String jsonText = file.loadFileAsString();
        if (jsonText.isEmpty())
            return false;

        juce::var parsed = juce::JSON::parse(jsonText);
        if (parsed.isVoid())
            return false;

        CoefficientBank bank;
        bank.bankName = file.getFileNameWithoutExtension();
        bank.description = file.getFullPathName();

        if (auto* meta = parsed.getProperty("meta", juce::var()).getDynamicObject())
        {
            if (meta->hasProperty("bank"))
                bank.bankName = meta->getProperty("bank").toString();
            if (meta->hasProperty("source"))
                bank.description = meta->getProperty("source").toString();
        }

        if (auto presetsVar = parsed.getProperty("presets", juce::var());
            presetsVar.isArray())
        {
            const auto& array = *presetsVar.getArray();
            const int morphCount = juce::jlimit(2, 12, array.size());
            bank.morphTargets.reserve(morphCount);

            for (int i = 0; i < morphCount; ++i)
            {
                const float morph = morphCount > 1 ? static_cast<float>(i) / static_cast<float>(morphCount - 1) : 0.0f;
                FilterParameters params;
                params.type = FilterType::Morphing;
                params.model = FilterModel::AuthenticEmu;
                params.morphPosition = morph;
                params.frequency = juce::jlimit(120.0f, 16000.0f, 400.0f + 5000.0f * morph);
                params.resonance = juce::jlimit(0.15f, 0.95f, 0.25f + 0.6f * morph);
                params.drive = juce::jlimit(0.0f, 1.0f, 0.2f + 0.7f * morph);
                params.character = juce::jlimit(0.0f, 1.0f, morph);
                bank.morphTargets.push_back(params);
            }
        }

        if (bank.morphTargets.empty())
        {
            FilterParameters classic;
            classic.type = FilterType::VintageEMU;
            classic.model = FilterModel::AuthenticEmu;
            classic.frequency = 500.0f;
            classic.resonance = 0.35f;
            classic.drive = 0.15f;
            classic.character = 0.2f;
            classic.morphPosition = 0.0f;

            FilterParameters morph;
            morph.type = FilterType::Morphing;
            morph.model = FilterModel::ZPlaneMorph;
            morph.frequency = 1200.0f;
            morph.resonance = 0.55f;
            morph.drive = 0.45f;
            morph.character = 0.55f;
            morph.morphPosition = 0.5f;

            FilterParameters peak;
            peak.type = FilterType::Peak;
            peak.model = FilterModel::EmuModern;
            peak.frequency = 3200.0f;
            peak.resonance = 0.85f;
            peak.drive = 0.7f;
            peak.character = 0.9f;
            peak.morphPosition = 1.0f;

            bank.morphTargets = { classic, morph, peak };
        }

        return loadCoefficientBankData(bank);
    }

    bool EMUZPlaneFilter::loadCoefficientBankData(const CoefficientBank& bank)
    {
        if (bank.bankName.isEmpty())
            return false;

        const juce::ScopedLock lock(stateLock_);

        auto existing = std::find_if(coefficientBanks_.begin(), coefficientBanks_.end(),
            [&bank](const CoefficientBank& candidate) { return candidate.bankName == bank.bankName; });

        if (existing != coefficientBanks_.end())
            *existing = bank;
        else
            coefficientBanks_.push_back(bank);

        int newIndex = static_cast<int>(std::distance(coefficientBanks_.begin(),
                                                      std::find_if(coefficientBanks_.begin(), coefficientBanks_.end(),
                                                          [&bank](const CoefficientBank& candidate)
                                                          {
                                                              return candidate.bankName == bank.bankName;
                                                          })));

        // Update cached bank for lock-free audio thread access
        cachedBank_ = bank;
        cachedBankValid_.store(true, std::memory_order_release);
        currentBankIndex_.store(newIndex, std::memory_order_release);

        updateMorphTargets();
        return true;
    }

    void EMUZPlaneFilter::setActiveBank(const juce::String& bankName)
    {
        const juce::ScopedLock lock(stateLock_);
        for (size_t i = 0; i < coefficientBanks_.size(); ++i)
        {
            if (coefficientBanks_[i].bankName == bankName)
            {
                // Update cached bank for lock-free audio thread access
                cachedBank_ = coefficientBanks_[i];
                cachedBankValid_.store(true, std::memory_order_release);
                currentBankIndex_.store(static_cast<int>(i), std::memory_order_release);
                updateMorphTargets();
                return;
            }
        }
    }

    const EMUZPlaneFilter::CoefficientBank* EMUZPlaneFilter::getCurrentBank() const noexcept
    {
        // Lock-free audio thread access via cached bank
        if (!cachedBankValid_.load(std::memory_order_acquire))
            return nullptr;
        return &cachedBank_;
    }

    bool EMUZPlaneFilter::loadCoefficientBankFromJson(const void* data, size_t sizeInBytes)
    {
        if (data == nullptr || sizeInBytes == 0)
            return false;

        const juce::String jsonText = juce::String::createStringFromData(data, static_cast<int>(sizeInBytes));
        if (jsonText.isEmpty())
            return false;

        return loadCoefficientBankFromJsonInternal(jsonText, "embedded", "Proteus1");
    }

    bool EMUZPlaneFilter::loadCoefficientBankFromJsonInternal(const juce::String& jsonText,
                                                             const juce::String& sourceLabel,
                                                             const juce::String& suggestedBankName)
    {
        juce::var parsed = juce::JSON::parse(jsonText);
        if (parsed.isVoid())
            return false;

        CoefficientBank bank;
        bank.bankName = suggestedBankName;
        bank.description = sourceLabel;

        if (auto* meta = parsed.getProperty("meta", juce::var()).getDynamicObject())
        {
            if (meta->hasProperty("bank"))
                bank.bankName = meta->getProperty("bank").toString();
            if (meta->hasProperty("source"))
                bank.description = meta->getProperty("source").toString();
        }

        if (auto presetsVar = parsed.getProperty("presets", juce::var());
            presetsVar.isArray())
        {
            const auto& array = *presetsVar.getArray();
            const int morphCount = juce::jlimit(2, 12, array.size());
            bank.morphTargets.reserve(morphCount);

            for (int i = 0; i < morphCount; ++i)
            {
                const float morph = morphCount > 1 ? static_cast<float>(i) / static_cast<float>(morphCount - 1) : 0.0f;
                FilterParameters params;
                params.type = FilterType::Morphing;
                params.model = FilterModel::AuthenticEmu;
                params.morphPosition = morph;
                params.frequency = juce::jlimit(120.0f, 16000.0f, 400.0f + 5000.0f * morph);
                params.resonance = juce::jlimit(0.15f, 0.95f, 0.25f + 0.6f * morph);
                params.drive = juce::jlimit(0.0f, 1.0f, 0.2f + 0.7f * morph);
                params.character = juce::jlimit(0.0f, 1.0f, morph);
                bank.morphTargets.push_back(params);
            }
        }

        if (bank.morphTargets.empty())
        {
            FilterParameters classic;
            classic.type = FilterType::VintageEMU;
            classic.model = FilterModel::AuthenticEmu;
            classic.frequency = 500.0f;
            classic.resonance = 0.35f;
            classic.drive = 0.15f;
            classic.character = 0.2f;
            classic.morphPosition = 0.0f;

            FilterParameters morph;
            morph.type = FilterType::Morphing;
            morph.model = FilterModel::ZPlaneMorph;
            morph.frequency = 1200.0f;
            morph.resonance = 0.55f;
            morph.drive = 0.45f;
            morph.character = 0.55f;
            morph.morphPosition = 0.5f;

            FilterParameters peak;
            peak.type = FilterType::Peak;
            peak.model = FilterModel::EmuModern;
            peak.frequency = 3200.0f;
            peak.resonance = 0.85f;
            peak.drive = 0.7f;
            peak.character = 0.9f;
            peak.morphPosition = 1.0f;

            bank.morphTargets = { classic, morph, peak };
        }

        return loadCoefficientBankData(bank);
    }

    //==============================================================================
    EMUZPlaneFilter::FilterResponse EMUZPlaneFilter::getFrequencyResponse(int numPoints) const
    {
        FilterResponse response;
        numPoints = std::max(8, numPoints);
        response.frequencies.resize(numPoints);
        response.magnitudes.resize(numPoints);
        response.phases.resize(numPoints);
        response.currentFrequency = effectiveParams_.frequency;
        response.currentResonance = effectiveParams_.resonance;
        response.currentMorph = effectiveParams_.morphPosition;

        const double samplePeriod = 1.0 / sampleRate_;
        for (int i = 0; i < numPoints; ++i)
        {
            const double freq = std::exp(std::log(kMinFrequency) +
                                         (std::log(sampleRate_ * 0.5) - std::log(kMinFrequency)) *
                                             (static_cast<double>(i) / (numPoints - 1)));
            const double omega = 2.0 * juce::double_Pi * freq * samplePeriod;
            std::complex<double> numerator(1.0, 0.0);
            std::complex<double> denominator(1.0, 0.0);
            const std::complex<double> jw = std::complex<double>(std::cos(omega), -std::sin(omega));
            const std::complex<double> jw2 = std::complex<double>(std::cos(2.0 * omega), -std::sin(2.0 * omega));

            for (const auto& section : cascade_)
            {
                std::complex<double> num(static_cast<double>(section.a0), 0.0);
                num += static_cast<double>(section.a1) * jw;
                num += static_cast<double>(section.a2) * jw2;

                std::complex<double> den(1.0, 0.0);
                den += static_cast<double>(section.b1) * jw;
                den += static_cast<double>(section.b2) * jw2;

                numerator *= num;
                denominator *= den;
            }

            const auto H = numerator / denominator;
            response.frequencies[i] = static_cast<float>(freq);
            response.magnitudes[i] = static_cast<float>(20.0 * std::log10(std::max(1.0e-12, std::abs(H))));
            response.phases[i] = static_cast<float>(std::arg(H));
        }

        return response;
    }

    float EMUZPlaneFilter::getCurrentFrequency() const noexcept
    {
        return effectiveParams_.frequency;
    }

    float EMUZPlaneFilter::getCurrentResonance() const noexcept
    {
        return effectiveParams_.resonance;
    }

    float EMUZPlaneFilter::getCurrentMorph() const noexcept
    {
        return effectiveParams_.morphPosition;
    }

    //==============================================================================
    void EMUZPlaneFilter::updateFilterState()
    {
        switch (filterModel_)
        {
            case FilterModel::EmuClassic:
            case FilterModel::AuthenticEmu:
                rebuildVintageModel(effectiveParams_);
                break;

            case FilterModel::EmuModern:
                rebuildModernModel(effectiveParams_);
                break;

            case FilterModel::ZPlaneMorph:
            case FilterModel::Hybrid:
                rebuildMorphModel(effectiveParams_);
                break;
        }
    }

    void EMUZPlaneFilter::rebuildFromParameters(const FilterParameters& params)
    {
        juce::ignoreUnused(params);
        updateFilterState();
    }

    void EMUZPlaneFilter::rebuildVintageModel(const FilterParameters& params)
    {
        // AUTHENTIC EMU Z-PLANE FILTER using real pole data from EMUAuthenticTables.h
        const float character = juce::jlimit(0.0f, 1.0f, params.character);
        const float resonance = juce::jlimit(0.05f, 0.98f, params.resonance);

        // Character parameter selects between 8 authentic shapes
        // 0.0 = vowel_pair_A, 0.125 = bell_pair_A, 0.25 = low_pair_A, 0.375 = vowel_pair_B, etc.
        const float shapePosition = character * (AUTHENTIC_EMU_NUM_SHAPES - 1);
        const int shapeIndex0 = juce::jlimit(0, AUTHENTIC_EMU_NUM_SHAPES - 1, static_cast<int>(std::floor(shapePosition)));
        const int shapeIndex1 = juce::jlimit(0, AUTHENTIC_EMU_NUM_SHAPES - 1, shapeIndex0 + 1);
        const float shapeFraction = shapePosition - static_cast<float>(shapeIndex0);

        // Calculate sample rate remapping factor
        const float kExp = static_cast<float>(AUTHENTIC_EMU_SAMPLE_RATE_REF) / static_cast<float>(sampleRate_);

        // Resonance modulates pole radius (closer to 1.0 = more resonant)
        const float resScale = 0.85f + resonance * 0.12f; // Range: 0.85 to 0.97

        // Process each of the 6 pole pairs -> 6 biquad sections
        constexpr int numPolePairs = 6;  // Each EMU shape has 6 pole pairs (12 floats)
        for (int pairIdx = 0; pairIdx < numPolePairs; ++pairIdx)
        {
            // Get authentic poles from both shapes (format: r, θ for each pair)
            const float r0_ref = AUTHENTIC_EMU_SHAPES[shapeIndex0][pairIdx * 2 + 0];
            const float th0_ref = AUTHENTIC_EMU_SHAPES[shapeIndex0][pairIdx * 2 + 1];
            const float r1_ref = AUTHENTIC_EMU_SHAPES[shapeIndex1][pairIdx * 2 + 0];
            const float th1_ref = AUTHENTIC_EMU_SHAPES[shapeIndex1][pairIdx * 2 + 1];

            // Interpolate between shapes
            float r_ref = r0_ref * (1.0f - shapeFraction) + r1_ref * shapeFraction;
            float th_ref = th0_ref * (1.0f - shapeFraction) + th1_ref * shapeFraction;

            // Apply resonance modulation
            r_ref = juce::jlimit(0.1f, 0.999f, r_ref * resScale);

            // Remap poles from reference sample rate to current sample rate
            float r_now, th_now;
            remapPolarFromRef(r_ref, th_ref, kExp, r_now, th_now);

            // Convert complex conjugate pole pair (r*e^±jθ) to biquad coefficients
            // For poles at r*e^±jθ, denominator: 1 + b1*z^-1 + b2*z^-2
            // where b1 = -2*r*cos(θ), b2 = r²
            const float b1 = -2.0f * r_now * std::cos(th_now);
            const float b2 = r_now * r_now;

            // Numerator: lowpass response (1 + 2*z^-1 + z^-2) scaled for unity DC gain
            // DC gain = (1 + a1 + a2) / (1 + b1 + b2)
            // For unity gain: a0 + a1 + a2 = 1 + b1 + b2
            const float dcGain = 1.0f + b1 + b2;
            const float a0 = dcGain / 4.0f;
            const float a1 = dcGain / 2.0f;
            const float a2 = dcGain / 4.0f;

            // Write to biquad section
            auto& section = cascade_[static_cast<size_t>(pairIdx)];
            section.a0 = a0;
            section.a1 = a1;
            section.a2 = a2;
            section.b1 = b1;
            section.b2 = b2;

            // Final safety stabilization
            stabilizeDenominator(section.b1, section.b2);
        }
    }

    void EMUZPlaneFilter::rebuildModernModel(const FilterParameters& params)
    {
        // No lock needed - called from audio thread, operates only on cascade_ member
        const float nyquist = static_cast<float>(sampleRate_ * 0.5);
        const float cutoff = juce::jlimit(80.0f, nyquist * 0.9f, params.frequency);
        const float resonance = juce::jlimit(0.1f, 1.4f, params.resonance * 1.4f + 0.15f);
        const float drive = juce::jlimit(0.0f, 1.0f, params.drive);
        const float morph = juce::jlimit(0.0f, 1.0f, params.morphPosition);

        for (std::size_t i = 0; i < cascade_.size(); ++i)
        {
            const float stageMorph = juce::jlimit(0.0f, 1.0f, morph + static_cast<float>(i) * 0.12f);
            const float freqScale = 0.6f + stageMorph * 1.4f;
            const float stageCutoff = juce::jlimit(kMinFrequency, nyquist * 0.99f, cutoff * freqScale);
            const float stageRes = juce::jlimit(0.2f, 2.0f, resonance + stageMorph * 0.8f + drive * 0.5f);

            const float omegaRef = 2.0f * juce::MathConstants<float>::pi * stageCutoff / static_cast<float>(refSampleRate_);
            const float omega = prewarpOmega(omegaRef, static_cast<float>(refSampleRate_ / sampleRate_));
            const float sinOmega = std::sin(omega);
            const float cosOmega = std::cos(omega);
            const float alpha = sinOmega / (2.0f * stageRes);

            auto& section = cascade_[i];
            if (i == 0)
            {
                section.a0 = (1.0f - cosOmega) / 2.0f;
                section.a1 = 1.0f - cosOmega;
                section.a2 = (1.0f - cosOmega) / 2.0f;
            }
            else if (i == 1)
            {
                section.a0 = alpha;
                section.a1 = 0.0f;
                section.a2 = -alpha;
            }
            else
            {
                section.a0 = 1.0f;
                section.a1 = -2.0f * cosOmega;
                section.a2 = 1.0f;
            }

            section.b1 = -2.0f * cosOmega;
            section.b2 = 1.0f - alpha;

            const float norm = 1.0f / (1.0f + alpha);
            section.a0 *= norm;
            section.a1 *= norm;
            section.a2 *= norm;
            section.b1 *= norm;
            section.b2 *= norm;

            // Stabilize denominator to ensure poles inside unit circle
            stabilizeDenominator(section.b1, section.b2);
        }
    }

    void EMUZPlaneFilter::rebuildMorphModel(const FilterParameters& params)
    {
        // No lock needed - called from audio thread, uses lock-free getCurrentBank()
        const auto morphParams = interpolateMorphTargets(params.morphPosition);
        FilterParameters combined = params;
        combined.frequency = combined.frequency * 0.5f + morphParams.frequency * 0.5f;
        combined.resonance = combined.resonance * 0.5f + morphParams.resonance * 0.5f;
        combined.drive = std::max(combined.drive, morphParams.drive);
        combined.character = std::max(combined.character, morphParams.character);

        rebuildVintageModel(combined);
    }

    void EMUZPlaneFilter::updateMorphTargets() noexcept
    {
        if (const auto* bank = getCurrentBank())
        {
            if (!bank->morphTargets.empty())
            {
                FilterParameters params = effectiveParams_;
                const auto target = interpolateMorphTargets(morph_.current);
                params.frequency = params.frequency * 0.4f + target.frequency * 0.6f;
                params.resonance = params.resonance * 0.5f + target.resonance * 0.5f;
                effectiveParams_ = params;
            }
        }
    }

    EMUZPlaneFilter::FilterParameters EMUZPlaneFilter::interpolateMorphTargets(float morph) const noexcept
    {
        // Lock-free audio thread access via cached bank
        if (!cachedBankValid_.load(std::memory_order_acquire))
            return FilterParameters{};

        const auto& targets = cachedBank_.morphTargets;
        if (targets.empty())
            return FilterParameters{};
        if (targets.size() == 1)
            return targets.front();

        morph = juce::jlimit(0.0f, 1.0f, morph);
        const float position = morph * static_cast<float>(targets.size() - 1);
        const int index = static_cast<int>(std::floor(position));
        const int nextIndex = std::min(index + 1, static_cast<int>(targets.size() - 1));
        const float fraction = position - static_cast<float>(index);

        const auto& a = targets[static_cast<size_t>(index)];
        const auto& b = targets[static_cast<size_t>(nextIndex)];

        FilterParameters result;
        result.type = filterType_;
        result.model = filterModel_;
        result.morphPosition = morph;
        result.frequency = juce::jmap(fraction, a.frequency, b.frequency);
        result.resonance = juce::jmap(fraction, a.resonance, b.resonance);
        result.gain = juce::jmap(fraction, a.gain, b.gain);
        result.drive = juce::jmap(fraction, a.drive, b.drive);
        result.character = juce::jmap(fraction, a.character, b.character);
        result.quality = quality_;
        return result;
    }

    float EMUZPlaneFilter::applyNonlinearStage(float input) noexcept
    {
        const float character = juce::jlimit(0.0f, 1.0f, character_.current);
        const float saturation = 0.6f + character * 0.8f;
        return fastTanh(input * saturation) / saturation;
    }

    float EMUZPlaneFilter::fastTanh(float x) noexcept
    {
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

} // namespace ConsolidatedDSP
