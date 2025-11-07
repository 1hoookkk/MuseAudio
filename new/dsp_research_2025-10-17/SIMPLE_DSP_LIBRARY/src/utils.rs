//! Simple utility functions

/// Linear interpolation
pub fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + (b - a) * t
}

/// Simple exponential smoothing
pub fn smooth(current: f32, target: f32, smoothing: f32) -> f32 {
    current + (target - current) * smoothing
}

/// Clamp value between min and max
pub fn clamp(value: f32, min: f32, max: f32) -> f32 {
    value.max(min).min(max)
}

/// Convert frequency to MIDI note number
pub fn freq_to_midi(freq: f32) -> f32 {
    69.0 + 12.0 * (freq / 440.0).log2()
}

/// Convert MIDI note number to frequency
pub fn midi_to_freq(note: f32) -> f32 {
    440.0 * 2.0_f32.powf((note - 69.0) / 12.0)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_lerp() {
        assert_eq!(lerp(0.0, 10.0, 0.5), 5.0);
        assert_eq!(lerp(0.0, 10.0, 0.0), 0.0);
        assert_eq!(lerp(0.0, 10.0, 1.0), 10.0);
    }

    #[test]
    fn test_smooth() {
        assert_eq!(smooth(0.0, 10.0, 0.5), 5.0);
        assert_eq!(smooth(5.0, 15.0, 0.0), 5.0);
        assert_eq!(smooth(5.0, 15.0, 1.0), 15.0);
    }

    #[test]
    fn test_clamp() {
        assert_eq!(clamp(5.0, 0.0, 10.0), 5.0);
        assert_eq!(clamp(-5.0, 0.0, 10.0), 0.0);
        assert_eq!(clamp(15.0, 0.0, 10.0), 10.0);
    }

    #[test]
    fn test_midi_conversion() {
        assert!((midi_to_freq(69.0) - 440.0).abs() < 0.001);
        assert!((freq_to_midi(440.0) - 69.0).abs() < 0.001);
    }
}
