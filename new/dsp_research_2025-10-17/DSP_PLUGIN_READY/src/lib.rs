//! DSP Plugin Ready Library
//!
//! Production-ready DSP code for audio plugins.
//! Simple, fast, and reliable.

#![deny(unsafe_op_in_unsafe_fn)]
#![warn(missing_docs)]

pub mod filter;
pub mod oscillator;
pub mod utils;

// Re-export main types
pub use filter::Filter;
pub use oscillator::Oscillator;
pub use oscillator::Waveform;

/// Library version
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// C FFI exports
#[cfg(feature = "c-api")]
pub mod ffi;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_library_basics() {
        let mut filter = Filter::new(44100.0);
        filter.set_frequency(1000.0);

        let mut buffer = vec![0.5f32; 128];
        filter.process(&mut buffer);

        assert!(!buffer.iter().all(|&x| x == 0.5));
    }
}
