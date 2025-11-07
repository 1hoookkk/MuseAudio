//! Simple DSP Library
//!
//! Clean, focused DSP with just the essentials.

pub mod filter;
pub mod oscillator;
pub mod utils;

pub use filter::EmuFilter;
pub use oscillator::Oscillator;
pub use utils::{lerp, smooth};

/// Library version
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_basic_filter() {
        let mut filter = EmuFilter::new(44100.0);
        filter.set_frequency(1000.0);
        filter.set_resonance(0.5);

        let mut output = vec![0.0f32; 64];
        filter.process(&mut output);

        // Should have processed the buffer
        assert!(!output.iter().all(|&x| x == 0.0));
    }

    #[test]
    fn test_basic_oscillator() {
        let mut osc = Oscillator::new(44100.0);
        osc.set_frequency(440.0);
        osc.set_waveform(OscillatorWaveform::Sine);

        let mut output = vec![0.0f32; 64];
        osc.process(&mut output);

        // Should generate non-zero output
        assert!(!output.iter().all(|&x| x == 0.0));
    }
}
