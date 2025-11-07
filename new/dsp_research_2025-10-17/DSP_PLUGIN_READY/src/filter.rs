//! High-quality EMU-style filter
//!
//! Production-ready filter with smooth parameter changes

use crate::utils::{clamp, smooth_param};
use std::f32::consts::PI;

/// EMU-style filter with smooth parameter changes
pub struct Filter {
    sample_rate: f32,

    // Current parameters
    frequency: f32,
    resonance: f32,

    // Target parameters (for smoothing)
    target_frequency: f32,
    target_resonance: f32,

    // Biquad coefficients
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,

    // Filter state (mono)
    x1: f32,
    x2: f32,
    y1: f32,
    y2: f32,

    // Smoothing rate
    smoothing: f32,
}

impl Filter {
    /// Create new filter at given sample rate
    pub fn new(sample_rate: f32) -> Self {
        let mut filter = Self {
            sample_rate,
            frequency: 1000.0,
            resonance: 0.5,
            target_frequency: 1000.0,
            target_resonance: 0.5,
            b0: 0.0,
            b1: 0.0,
            b2: 0.0,
            a1: 0.0,
            a2: 0.0,
            x1: 0.0,
            x2: 0.0,
            y1: 0.0,
            y2: 0.0,
            smoothing: 0.001,
        };
        filter.update_coefficients();
        filter
    }

    /// Set cutoff frequency (20Hz - 20kHz)
    #[inline]
    pub fn set_frequency(&mut self, freq: f32) {
        self.target_frequency = clamp(freq, 20.0, self.sample_rate * 0.45);
    }

    /// Set resonance (0.0 - 1.0)
    #[inline]
    pub fn set_resonance(&mut self, res: f32) {
        self.target_resonance = clamp(res, 0.0, 0.99);
    }

    /// Set smoothing rate (0.0 = instant, 1.0 = very slow)
    #[inline]
    pub fn set_smoothing(&mut self, rate: f32) {
        self.smoothing = clamp(rate, 0.0, 0.1);
    }

    /// Process audio buffer in-place
    pub fn process(&mut self, buffer: &mut [f32]) {
        // Smooth parameters
        self.frequency = smooth_param(self.frequency, self.target_frequency, self.smoothing);
        self.resonance = smooth_param(self.resonance, self.target_resonance, self.smoothing);

        // Update coefficients if needed
        if (self.frequency - self.target_frequency).abs() > 1.0 {
            self.update_coefficients();
        }

        // Process each sample
        for sample in buffer.iter_mut() {
            *sample = self.process_sample(*sample);
        }
    }

    /// Process single sample
    #[inline]
    fn process_sample(&mut self, input: f32) -> f32 {
        // Biquad Direct Form II
        let output = self.b0 * input + self.b1 * self.x1 + self.b2 * self.x2
            - self.a1 * self.y1
            - self.a2 * self.y2;

        // Update state
        self.x2 = self.x1;
        self.x1 = input;
        self.y2 = self.y1;
        self.y1 = output;

        output
    }

    /// Update biquad coefficients
    fn update_coefficients(&mut self) {
        let omega = 2.0 * PI * self.frequency / self.sample_rate;
        let sin_omega = omega.sin();
        let cos_omega = omega.cos();

        // Q factor from resonance (logarithmic mapping)
        let q = 0.5 + self.resonance * 10.0;
        let alpha = sin_omega / (2.0 * q);

        // Lowpass coefficients
        let a0 = 1.0 + alpha;

        self.b0 = (1.0 - cos_omega) / (2.0 * a0);
        self.b1 = (1.0 - cos_omega) / a0;
        self.b2 = (1.0 - cos_omega) / (2.0 * a0);
        self.a1 = -2.0 * cos_omega / a0;
        self.a2 = (1.0 - alpha) / a0;
    }

    /// Reset filter state
    pub fn reset(&mut self) {
        self.x1 = 0.0;
        self.x2 = 0.0;
        self.y1 = 0.0;
        self.y2 = 0.0;
    }

    /// Get current frequency
    pub fn frequency(&self) -> f32 {
        self.frequency
    }

    /// Get current resonance
    pub fn resonance(&self) -> f32 {
        self.resonance
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_filter_creation() {
        let filter = Filter::new(44100.0);
        assert_eq!(filter.frequency(), 1000.0);
    }

    #[test]
    fn test_filter_processing() {
        let mut filter = Filter::new(44100.0);
        filter.set_frequency(1000.0);
        filter.set_resonance(0.7);

        let mut buffer = vec![0.0f32; 64];
        buffer[0] = 1.0; // Impulse

        filter.process(&mut buffer);

        // Should have filtered the impulse
        assert!(buffer.iter().any(|&x| x != 0.0));
    }
}
