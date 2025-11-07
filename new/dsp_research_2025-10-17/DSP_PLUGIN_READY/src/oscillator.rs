//! Simple oscillator for synthesis

use crate::utils::clamp;
use std::f32::consts::PI;

/// Oscillator waveform types
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Waveform {
    /// Sine wave
    Sine,
    /// Sawtooth wave
    Saw,
    /// Square wave
    Square,
    /// Triangle wave
    Triangle,
}

/// Simple oscillator
pub struct Oscillator {
    sample_rate: f32,
    phase: f32,
    frequency: f32,
    amplitude: f32,
    waveform: Waveform,
}

impl Oscillator {
    /// Create new oscillator
    pub fn new(sample_rate: f32) -> Self {
        Self {
            sample_rate,
            phase: 0.0,
            frequency: 440.0,
            amplitude: 1.0,
            waveform: Waveform::Sine,
        }
    }

    /// Set frequency in Hz
    #[inline]
    pub fn set_frequency(&mut self, freq: f32) {
        self.frequency = clamp(freq, 20.0, 20000.0);
    }

    /// Set amplitude (0.0 - 1.0)
    #[inline]
    pub fn set_amplitude(&mut self, amp: f32) {
        self.amplitude = clamp(amp, 0.0, 1.0);
    }

    /// Set waveform type
    #[inline]
    pub fn set_waveform(&mut self, waveform: Waveform) {
        self.waveform = waveform;
    }

    /// Process audio buffer
    pub fn process(&mut self, buffer: &mut [f32]) {
        for sample in buffer.iter_mut() {
            *sample = self.generate_sample();
        }
    }

    /// Generate single sample
    #[inline]
    fn generate_sample(&mut self) -> f32 {
        let output = match self.waveform {
            Waveform::Sine => self.phase.sin(),
            Waveform::Saw => {
                let normalized = self.phase / (2.0 * PI);
                2.0 * (normalized - 0.5)
            }
            Waveform::Square => {
                if self.phase < PI {
                    1.0
                } else {
                    -1.0
                }
            }
            Waveform::Triangle => {
                let normalized = self.phase / (2.0 * PI);
                if normalized < 0.5 {
                    4.0 * normalized - 1.0
                } else {
                    3.0 - 4.0 * normalized
                }
            }
        };

        // Update phase
        let phase_increment = 2.0 * PI * self.frequency / self.sample_rate;
        self.phase += phase_increment;

        // Wrap phase
        if self.phase >= 2.0 * PI {
            self.phase -= 2.0 * PI;
        }

        output * self.amplitude
    }

    /// Reset oscillator phase
    pub fn reset(&mut self) {
        self.phase = 0.0;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_oscillator_creation() {
        let osc = Oscillator::new(44100.0);
        assert_eq!(osc.frequency, 440.0);
    }

    #[test]
    fn test_oscillator_output() {
        let mut osc = Oscillator::new(44100.0);
        osc.set_frequency(440.0);
        osc.set_amplitude(0.5);

        let mut buffer = vec![0.0f32; 64];
        osc.process(&mut buffer);

        // Should generate audio
        assert!(buffer.iter().any(|&x| x != 0.0));
    }
}
