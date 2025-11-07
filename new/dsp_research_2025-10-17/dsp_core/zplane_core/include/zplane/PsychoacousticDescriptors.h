#pragma once
#include "ZPoleMath.h"
#include "BiquadCascade.h"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace psycho
{
    // Convert frequency to ERB (Equivalent Rectangular Bandwidth) scale
    // More perceptually relevant than linear frequency or Bark scale
    inline float freqToERB(float freqHz)
    {
        // ERB = 24.7 * (4.37 * f/1000 + 1)
        return 24.7f * (4.37f * (freqHz / 1000.0f) + 1.0f);
    }

    // Convert frequency to Bark scale
    inline float freqToBark(float freqHz)
    {
        // Zwicker's Bark scale
        return 13.0f * std::atan(0.00076f * freqHz) + 3.5f * std::atan(std::pow(freqHz / 7500.0f, 2.0f));
    }

    // Calculate group delay from pole locations
    // Group delay indicates the time delay of different frequency components
    inline float calculateGroupDelay(float r, float theta, float freqNormalized)
    {
        // For a pole at z = r*e^{jθ}, the group delay is:
        // τ(ω) = -d/dω arg(H(e^{jω}))
        // For a single pole pair, this simplifies to:

        const float omega = 2.0f * zpm::kPi * freqNormalized;
        const float cos_omega = std::cos(omega);
        const float sin_omega = std::sin(omega);

        // Distance from pole to evaluation point
        const float denom_r = 1.0f + r * r - 2.0f * r * std::cos(theta - omega);
        const float denom_l = 1.0f + r * r - 2.0f * r * std::cos(theta + omega);

        // Group delay contribution from this pole pair
        const float tau_r = r * std::sin(theta - omega) / denom_r;
        const float tau_l = r * std::sin(theta + omega) / denom_l;

        return (tau_r + tau_l) / zpm::kTwoPi;
    }

    // Pole Angular Density (PAD) - identifies formant-like structures
    // Uses weighted Gaussian kernels to find regions of high pole density
    class PoleAngularDensity
    {
    public:
        explicit PoleAngularDensity(const std::vector<std::pair<float, float>>& poles,
                                   float sigma = 0.1f)
            : sigma_(sigma)
        {
            calculatePAD(poles);
        }

        // Get PAD value at a specific angle
        float getPAD(float theta) const
        {
            // Find nearest bin
            const int bin = static_cast<int>((theta + zpm::kPi) / (2.0f * zpm::kPi) * resolution_);
            if (bin >= 0 && bin < resolution_)
                return pad_[bin];
            return 0.0f;
        }

        // Find formant peaks (local maxima in PAD)
        std::vector<std::pair<float, float>> findFormantPeaks(float minThreshold = 0.1f) const
        {
            std::vector<std::pair<float, float>> peaks;

            for (int i = 1; i < resolution_ - 1; ++i)
            {
                if (pad_[i] > pad_[i-1] && pad_[i] > pad_[i+1] && pad_[i] > minThreshold)
                {
                    const float theta = -zpm::kPi + (2.0f * zpm::kPi) * i / resolution_;
                    peaks.emplace_back(theta, pad_[i]);
                }
            }

            // Sort by strength (descending)
            std::sort(peaks.begin(), peaks.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });

            return peaks;
        }

    private:
        static constexpr int resolution_ = 512; // Angular resolution
        std::array<float, resolution_> pad_{};
        float sigma_;

        void calculatePAD(const std::vector<std::pair<float, float>>& poles)
        {
            // Each pole contributes a weighted Gaussian to PAD
            // Weight = r/(1-r) - ringing poles contribute more

            for (int i = 0; i < resolution_; ++i)
            {
                const float theta = -zpm::kPi + (2.0f * zpm::kPi) * i / resolution_;
                float sum = 0.0f;

                for (const auto& pole : poles)
                {
                    const float r = pole.first;
                    const float poleTheta = pole.second;

                    // Weight by resonance (Q factor)
                    const float weight = r / (1.0f - r);

                    // Gaussian kernel
                    const float diff = zpm::wrapAngle(theta - poleTheta);
                    const float gaussian = std::exp(-(diff * diff) / (2.0f * sigma_ * sigma_));

                    sum += weight * gaussian;
                }

                pad_[i] = sum;
            }
        }
    };

    // Vowelness Index - measures similarity to vowel formant structures
    class VowelnessAnalyzer
    {
    public:
        struct VowelTemplate {
            const char* name;
            float F1, F2, F3; // Formant frequencies in Hz
        };

        static constexpr std::array<VowelTemplate, 5> vowelTemplates{{
            {"ah", {730, 1090, 2440}}, // Open back vowel
            {"ee", {270, 2290, 3010}}, // Close front vowel
            {"oh", {500, 870, 2250}},  // Open rounded vowel
            {"eh", {530, 1840, 2480}}, // Mid front vowel
            {"oo", {300, 870, 2250}}  // Close rounded vowel
        }};

        static float calculateVowelness(const std::vector<std::pair<float, float>>& poles, float sampleRate)
        {
            PoleAngularDensity pad(poles);
            auto peaks = pad.findFormantPeaks(0.1f);

            if (peaks.size() < 2) return 0.0f;

            // Convert top 3 peaks to frequencies
            std::vector<float> formants;
            for (int i = 0; i < std::min(3, static_cast<int>(peaks.size())); ++i)
            {
                const float theta = peaks[i].first;
                const float freq = theta * sampleRate / (2.0f * zpm::kPi);
                formants.push_back(freq);
            }

            if (formants.size() < 2) return 0.0f;

            // Compare against vowel templates using Bark distance
            float maxVowelness = 0.0f;
            for (const auto& vowel : vowelTemplates)
            {
                float distance = 0.0f;
                const int nFormants = std::min(3, static_cast<int>(formants.size()));

                if (nFormants >= 2)
                {
                    distance += std::abs(freqToBark(formants[0]) - freqToBark(vowel.F1));
                    distance += std::abs(freqToBark(formants[1]) - freqToBark(vowel.F2));

                    if (nFormants >= 3)
                        distance += std::abs(freqToBark(formants[2]) - freqToBark(vowel.F3));
                }

                // Convert distance to vowelness (inverse relationship)
                const float vowelness = 1.0f / (1.0f + distance * 0.1f);
                maxVowelness = std::max(maxVowelness, vowelness);
            }

            return maxVowelness;
        }
    };

    // Metallicity Index - measures comb-like regularity in pole spacing
    inline float calculateMetallicity(const std::vector<std::pair<float, float>>& poles, float sampleRate)
    {
        if (poles.size() < 3) return 0.0f;

        // Convert pole angles to frequencies
        std::vector<float> frequencies;
        for (const auto& pole : poles)
        {
            const float theta = pole.second;
            const float freq = theta * sampleRate / (2.0f * zpm::kPi);
            if (freq > 0.0f) // Only consider positive frequencies
                frequencies.push_back(freq);
        }

        if (frequencies.size() < 3) return 0.0f;

        // Find best regular spacing (fundamental frequency)
        float minVariance = 1e6f;
        float bestSpacing = 0.0f;

        for (float testSpacing = 50.0f; testSpacing <= 1000.0f; testSpacing += 10.0f)
        {
            float variance = 0.0f;
            int count = 0;

            for (float freq : frequencies)
            {
                const float n = std::round(freq / testSpacing);
                if (n > 0)
                {
                    const float expected = n * testSpacing;
                    const float error = std::abs(freq - expected);
                    variance += error * error;
                    ++count;
                }
            }

            if (count > 0)
            {
                variance /= count;
                if (variance < minVariance)
                {
                    minVariance = variance;
                    bestSpacing = testSpacing;
                }
            }
        }

        // Convert variance to metallicity (lower variance = higher metallicity)
        return std::exp(-minVariance / 100.0f);
    }

    // Warmth Index - measures low-frequency energy density
    inline float calculateWarmth(const std::vector<std::pair<float, float>>& poles, float sampleRate)
    {
        float lowFreqWeight = 0.0f;
        float totalWeight = 0.0f;

        for (const auto& pole : poles)
        {
            const float r = pole.first;
            const float theta = pole.second;
            const float freq = theta * sampleRate / (2.0f * zpm::kPi);

            const float weight = r / (1.0f - r); // Resonance weight

            if (freq < 400.0f)
                lowFreqWeight += weight;

            if (freq < 5000.0f)
                totalWeight += weight;
        }

        if (totalWeight == 0.0f) return 0.0f;
        return lowFreqWeight / totalWeight;
    }

    // Aggression Index - measures high-mid vs low-mid energy ratio
    inline float calculateAggression(const std::vector<std::pair<float, float>>& poles, float sampleRate)
    {
        float midHighWeight = 0.0f;  // 2-5 kHz
        float lowMidWeight = 0.0f;   // 200-800 Hz

        for (const auto& pole : poles)
        {
            const float r = pole.first;
            const float theta = pole.second;
            const float freq = theta * sampleRate / (2.0f * zpm::kPi);

            const float weight = (r / (1.0f - r)) * r; // Weight by Q and resonance

            if (freq >= 2000.0f && freq <= 5000.0f)
                midHighWeight += weight;
            else if (freq >= 200.0f && freq <= 800.0f)
                lowMidWeight += weight;
        }

        if (lowMidWeight == 0.0f) return 0.0f;
        return midHighWeight / lowMidWeight;
    }

    // Punch Index - measures low-frequency group delay characteristics
    inline float calculatePunch(const std::vector<std::pair<float, float>>& poles, float sampleRate)
    {
        float lowFreqDelay = 0.0f;   // 60-150 Hz
        float midFreqDelay = 0.0f;   // 2-5 kHz

        for (const auto& pole : poles)
        {
            const float r = pole.first;
            const float theta = pole.second;

            // Calculate group delay at key frequencies
            const float lowDelay = calculateGroupDelay(r, theta, 100.0f / sampleRate);
            const float midDelay = calculateGroupDelay(r, theta, 3000.0f / sampleRate);

            const float weight = r / (1.0f - r); // Resonance weight

            lowFreqDelay += lowDelay * weight;
            midFreqDelay += midDelay * weight;
        }

        const float delayDiff = lowFreqDelay - midFreqDelay;
        return std::tanh(delayDiff * 10.0f); // Normalize to [0,1]
    }

    // Comprehensive psychoacoustic analysis
    struct CharacterAnalysis
    {
        float vowelness;     // Vowel-like quality [0,1]
        float metallicity;   // Comb-like regularity [0,1]
        float warmth;        // Low-frequency density [0,1]
        float aggression;    // High-mid vs low-mid ratio [0,1]
        float punch;         // Low-frequency group delay [0,1]
    };

    inline CharacterAnalysis analyzeCharacter(const std::vector<std::pair<float, float>>& poles, float sampleRate)
    {
        return {
            VowelnessAnalyzer::calculateVowelness(poles, sampleRate),
            calculateMetallicity(poles, sampleRate),
            calculateWarmth(poles, sampleRate),
            calculateAggression(poles, sampleRate),
            calculatePunch(poles, sampleRate)
        };
    }
}