//! Simple oscillator implementation

use crate::utils::{clamp, lerp};

/// Waveform types
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum OscillatorWaveform {
    Sine,
    Sawtooth,
    Square,
    Triangle,
}

/// Simple oscillator
pub struct Oscillator {
    sample_rate: f32,
    phase: f32,
    phase_increment: f32,
    frequency: f32,
    waveform: OscillatorWaveform,
    last_output: f32,
}

impl Oscillator {
    /// Create new oscillator
    pub fn new(sample_rate: f32) -> Self {
        Self {
            sample_rate,
            phase: 0.0,
            phase_increment: 0.0,
            frequency: 440.0,
            waveform: OscillatorWaveform::Sine,
            last_output: 0.0,
        }
    }

    /// Set frequency
    pub fn set_frequency(&mut self, freq: f32) {
        self.frequency = clamp(freq, 0.1, 20000.0);
        self.phase_increment = self.frequency / self.sample_rate;
    }

    /// Set waveform
    pub fn set_waveform(&mut self, waveform: OscillatorWaveform) {
        self.waveform = waveform;
    }

    /// Generate samples
    pub fn process(&mut self, output: &mut [f32]) {
        for sample in output.iter_mut() {
            *sample = self.generate_sample();
            self.advance_phase();
        }
    }

    /// Generate single sample
    fn generate_sample(&self) -> f32 {
        match self.waveform {
            OscillatorWaveform::Sine => (self.phase * 2.0 * std::f32::consts::PI).sin(),
            OscillatorWaveform::Sawtooth => {
                if self.phase < 0.5 {
                    self.phase * 4.0 - 1.0
                } else {
                    self.phase * 4.0 - 3.0
                }
            }
            OscillatorWaveform::Square => {
                if self.phase < 0.5 {
                    1.0
                } else {
                    -1.0
                }
            }
            OscillatorWaveform::Triangle => {
                if self.phase < 0.25 {
                    self.phase * 4.0
                } else if self.phase < 0.75 {
                    2.0 - self.phase * 4.0
                } else {
                    self.phase * 4.0 - 4.0
                }
            }
        }
    }

    /// Advance phase
    fn advance_phase(&mut self) {
        self.phase += self.phase_increment;
        if self.phase >= 1.0 {
            self.phase -= 1.0;
        }
    }

    /// Reset phase
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
        assert_eq!(osc.waveform, OscillatorWaveform::Sine);
    }

    #[test]
    fn test_frequency_setting() {
        let mut osc = Oscillator::new(44100.0);
        osc.set_frequency(880.0);
        assert_eq!(osc.frequency, 880.0);

        // Test clamping
        osc.set_frequency(0.0);
        assert_eq!(osc.frequency, 0.1);

        osc.set_frequency(30000.0);
        assert_eq!(osc.frequency, 20000.0);
    }

    #[test]
    fn test_waveforms() {
        let mut osc = Oscillator::new(44100.0);

        let waveforms = [
            OscillatorWaveform::Sine,
            OscillatorWaveform::Sawtooth,
            OscillatorWaveform::Square,
            OscillatorWaveform::Triangle,
        ];

        for waveform in waveforms.iter() {
            osc.set_waveform(*waveform);
            let mut output = vec![0.0f32; 10];
            osc.process(&mut output);

            // Should generate some output
            assert!(!output.iter().all(|&x| x == 0.0));
        }
    }

    #[test]
    fn test_phase_reset() {
        let mut osc = Oscillator::new(44100.0);

        // Process some samples to advance phase
        let mut output = vec![0.0f32; 100];
        osc.process(&mut output);

        // Reset and check phase is back to 0
        osc.reset();
        assert_eq!(osc.phase, 0.0);
    }
}
