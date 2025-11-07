#pragma once

#include <vector>
#include <complex>
#include <algorithm>
#include <cmath>

namespace dsp
{

class STFT
{
public:
    void prepare (int sampleRate, int fftSize, int hopSize)
    {
        fs_ = sampleRate;
        fftSize_ = fftSize;
        hopSize_ = hopSize;
        window_.resize (fftSize_);
        for (int n = 0; n < fftSize_; ++n)
            window_[n] = 0.5f - 0.5f * std::cos (2.0f * float (M_PI) * (float) n / (float) (fftSize_ - 1));
        buffer_.assign (fftSize_, 0.0f);
        mag_.assign (fftSize_ / 2 + 1, 0.0f);
        writePos_ = 0;
        readySamples_ = 0;
    }

    bool addSamples (const float* samples, int numSamples, int& framesReady)
    {
        framesReady = 0;
        for (int i = 0; i < numSamples; ++i)
        {
            buffer_[writePos_] = samples[i];
            ++writePos_;
            if (writePos_ >= fftSize_)
                writePos_ = 0;
            ++readySamples_;
            if (readySamples_ >= hopSize_)
            {
                readySamples_ -= hopSize_;
                ++framesReady;
            }
        }
        return framesReady > 0;
    }

    void computeMagnitude()
    {
        temp_.assign (fftSize_, 0.0f);
        for (int n = 0; n < fftSize_; ++n)
        {
            int idx = writePos_ + n;
            if (idx >= fftSize_) idx -= fftSize_;
            temp_[n] = buffer_[idx] * window_[n];
        }
        const int bins = fftSize_ / 2;
        for (int k = 0; k <= bins; ++k)
        {
            std::complex<double> acc { 0.0, 0.0 };
            for (int n = 0; n < fftSize_; ++n)
            {
                double angle = -2.0 * M_PI * (double) k * (double) n / (double) fftSize_;
                acc += std::polar (static_cast<double> (temp_[n]), angle);
            }
            mag_[k] = static_cast<float> (std::abs (acc));
        }
    }

    const std::vector<float>& getMagnitude() const noexcept { return mag_; }

private:
    int fs_ = 48000;
    int fftSize_ = 1024;
    int hopSize_ = 256;
    int writePos_ = 0;
    int readySamples_ = 0;

    std::vector<float> buffer_;
    std::vector<float> temp_;
    std::vector<float> window_;
    std::vector<float> mag_;
};

class BarkBands
{
public:
    void prepare (int sampleRate, int fftSize, int hopSize, int numBands = 24)
    {
        (void) hopSize;
        fs_ = sampleRate;
        fftSize_ = fftSize;
        bands_ = std::max (1, numBands);
        weights_.assign (bands_, std::vector<float> (fftSize_ / 2 + 1, 0.0f));
        energies.assign (bands_, -100.0f);
        maskingThresholds.assign (bands_, -100.0f);

        const int K = fftSize_ / 2;
        const float barkMax = hzToBark (0.5f * fs_);
        for (int b = 0; b < bands_; ++b)
        {
            const float center = (b + 0.5f) * (barkMax / (float) bands_);
            const float half   = barkMax / (float) bands_;
            for (int k = 0; k <= K; ++k)
            {
                float f = (float) k * (float) fs_ / (float) fftSize_;
                float bark = hzToBark (f);
                float dist = std::abs (bark - center);
                float w = std::max (0.0f, 1.0f - dist / half);
                weights_[b][k] = w;
            }
            const float sum = std::accumulate (weights_[b].begin(), weights_[b].end(), 0.0f);
            if (sum > 0.0f)
                for (auto& w : weights_[b]) w /= sum;
        }
    }

    void updateEnergies (const std::vector<float>& magnitude)
    {
        const int K = fftSize_ / 2;
        for (int b = 0; b < bands_; ++b)
        {
            double accum = 0.0;
            for (int k = 0; k <= K; ++k)
            {
                const double mag = static_cast<double> (magnitude[k]);
                accum += weights_[b][k] * mag * mag;
            }
            const double db = 10.0 * std::log10 (std::max (accum, 1.0e-12));
            energies[b] = static_cast<float> (db);
        }
    }

    void updateMaskingThresholds()
    {
        // Simple approximation: smooth energies with ±1 Bark spreading and subtract a margin.
        const float spread = 1.0f;
        for (int b = 0; b < bands_; ++b)
        {
            float accum = 0.0f;
            float norm  = 0.0f;
            for (int k = 0; k < bands_; ++k)
            {
                float dist = std::abs ((float) k - (float) b);
                if (dist > spread) continue;
                float w = 1.0f - (dist / spread);
                accum += w * energies[k];
                norm  += w;
            }
            const float avg = (norm > 0.0f) ? accum / norm : energies[b];
            maskingThresholds[b] = avg - 3.0f; // leave 3 dB headroom by default
        }
    }

    static float hzToBark (float f) noexcept
    {
        return 13.0f * std::atan (0.00076f * f) + 3.5f * std::atan (std::pow (f / 7500.0f, 2.0f));
    }

    int fs_ = 48000;
    int fftSize_ = 1024;
    int bands_ = 24;

    std::vector<std::vector<float>> weights_;

    std::vector<float> energies;
    std::vector<float> maskingThresholds;
};

} // namespace dsp
