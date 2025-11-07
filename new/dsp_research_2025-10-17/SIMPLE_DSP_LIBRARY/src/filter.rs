//! Simple EMU-style filter implementation

use crate::utils::{clamp, smooth};

/// Simple EMU-style filter
pub struct EmuFilter {
    sample_rate: f32,
    frequency: f32,
    resonance: f32,
    target_frequency: f32,
    target_resonance: f32,

    // Biquad coefficients
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,

    // Filter state
    x1: f32,
    x2: f32,
    y1: f32,
    y2: f32,

    // Smoothing
    freq_smoothing: f32,
    res_smoothing: f32,
}

impl EmuFilter {
    /// Create new filter
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
            freq_smoothing: 0.001,
            res_smoothing: 0.001,
        };
        filter.update_coefficients();
        filter
    }

    /// Set frequency (20Hz - 20kHz)
    pub fn set_frequency(&mut self, freq: f32) {
        self.target_frequency = clamp(freq, 20.0, 20000.0);
    }

    /// Set resonance (0.0 - 1.0)
    pub fn set_resonance(&mut self, res: f32) {
        self.target_resonance = clamp(res, 0.0, 0.99);
    }

    /// Set smoothing factor (0.0 - 1.0)
    pub fn set_smoothing(&mut self, smoothing: f32) {
        let s = clamp(smoothing, 0.0, 1.0);
        self.freq_smoothing = s;
        self.res_smoothing = s;
    }

    /// Process audio buffer
    pub fn process(&mut self, buffer: &mut [f32]) {
        // Smooth parameter changes
        self.frequency = smooth(self.frequency, self.target_frequency, self.freq_smoothing);
        self.resonance = smooth(self.resonance, self.target_resonance, self.res_smoothing);

        // Update coefficients if parameters changed significantly
        if (self.frequency - self.target_frequency).abs() > 1.0
            || (self.resonance - self.target_resonance).abs() > 0.001
        {
            self.update_coefficients();
        }

        // Process samples
        for sample in buffer.iter_mut() {
            let input = *sample;

            // Biquad difference equation
            let output = self.b0 * input + self.b1 * self.x1 + self.b2 * self.x2
                - self.a1 * self.y1
                - self.a2 * self.y2;

            // Update delay lines
            self.x2 = self.x1;
            self.x1 = input;
            self.y2 = self.y1;
            self.y1 = output;

            *sample = output;
        }
    }

    /// Update biquad coefficients
    fn update_coefficients(&mut self) {
        let omega = 2.0 * std::f32::consts::PI * self.frequency / self.sample_rate;
        let sin_omega = omega.sin();
        let cos_omega = omega.cos();
        let alpha = sin_omega / (2.0 * (1.0 - self.resonance));

        // Lowpass biquad coefficients
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
        self.frequency = self.target_frequency;
        self.resonance = self.target_resonance;
        self.update_coefficients();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_filter_creation() {
        let filter = EmuFilter::new(44100.0);
        assert_eq!(filter.frequency, 1000.0);
        assert_eq!(filter.resonance, 0.5);
    }

    #[test]
    fn test_parameter_setting() {
        let mut filter = EmuFilter::new(44100.0);

        filter.set_frequency(500.0);
        assert_eq!(filter.target_frequency, 500.0);

        filter.set_resonance(0.8);
        assert_eq!(filter.target_resonance, 0.8);

        // Test clamping
        filter.set_frequency(10.0);
        assert_eq!(filter.target_frequency, 20.0);

        filter.set_resonance(1.5);
        assert_eq!(filter.target_resonance, 0.99);
    }

    #[test]
    fn test_filter_processing() {
        let mut filter = EmuFilter::new(44100.0);

        // Create impulse input
        let mut input = vec![0.0f32; 64];
        input[0] = 1.0;

        filter.process(&mut input);

        // Should have filtered the impulse
        assert!(!input.iter().all(|&x| x == 0.0));
        assert!(input[0] != 1.0); // Should be attenuated
    }

    #[test]
    fn test_filter_reset() {
        let mut filter = EmuFilter::new(44100.0);

        // Process some audio to change state
        let mut buffer = vec![0.5f32; 64];
        filter.process(&mut buffer);

        // Reset should clear state
        filter.reset();
        assert_eq!(filter.x1, 0.0);
        assert_eq!(filter.x2, 0.0);
        assert_eq!(filter.y1, 0.0);
        assert_eq!(filter.y2, 0.0);
    }
}
