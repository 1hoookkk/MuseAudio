//! C FFI bindings for the DSP Plugin Ready library.
//!
//! These functions mirror the declarations in `include/dsp_plugin.h` so the
//! DSP can be consumed from C or C++ audio plugin hosts.

use crate::{
    filter::Filter,
    oscillator::{Oscillator, Waveform},
    utils,
};
use std::slice;

fn waveform_from_c(value: u32) -> Waveform {
    match value {
        1 => Waveform::Saw,
        2 => Waveform::Square,
        3 => Waveform::Triangle,
        _ => Waveform::Sine,
    }
}

// ---------------------------------------------------------------------------
// Filter API
// ---------------------------------------------------------------------------

/// Create a new filter for use via the C API.
#[no_mangle]
pub extern "C" fn dsp_filter_create(sample_rate: f32) -> *mut Filter {
    Box::into_raw(Box::new(Filter::new(sample_rate)))
}

/// Destroy a filter created with [`dsp_filter_create`].
#[no_mangle]
pub unsafe extern "C" fn dsp_filter_destroy(filter: *mut Filter) {
    if !filter.is_null() {
        unsafe {
            drop(Box::from_raw(filter));
        }
    }
}

/// Set the cutoff frequency for a filter handle.
#[no_mangle]
pub unsafe extern "C" fn dsp_filter_set_frequency(filter: *mut Filter, freq: f32) {
    if let Some(filter) = unsafe { filter.as_mut() } {
        filter.set_frequency(freq);
    }
}

/// Set the resonance amount for a filter handle.
#[no_mangle]
pub unsafe extern "C" fn dsp_filter_set_resonance(filter: *mut Filter, res: f32) {
    if let Some(filter) = unsafe { filter.as_mut() } {
        filter.set_resonance(res);
    }
}

/// Configure the smoothing rate for parameter interpolation.
#[no_mangle]
pub unsafe extern "C" fn dsp_filter_set_smoothing(filter: *mut Filter, rate: f32) {
    if let Some(filter) = unsafe { filter.as_mut() } {
        filter.set_smoothing(rate);
    }
}

/// Process an audio buffer through the filter in-place.
#[no_mangle]
pub unsafe extern "C" fn dsp_filter_process(filter: *mut Filter, buffer: *mut f32, length: u32) {
    if let Some(filter) = unsafe { filter.as_mut() } {
        if buffer.is_null() || length == 0 {
            return;
        }
        let buffer = unsafe { slice::from_raw_parts_mut(buffer, length as usize) };
        filter.process(buffer);
    }
}

/// Reset the internal state of the filter.
#[no_mangle]
pub unsafe extern "C" fn dsp_filter_reset(filter: *mut Filter) {
    if let Some(filter) = unsafe { filter.as_mut() } {
        filter.reset();
    }
}

// ---------------------------------------------------------------------------
// Oscillator API
// ---------------------------------------------------------------------------

/// Create a new oscillator for use via the C API.
#[no_mangle]
pub extern "C" fn dsp_osc_create(sample_rate: f32) -> *mut Oscillator {
    Box::into_raw(Box::new(Oscillator::new(sample_rate)))
}

/// Destroy an oscillator created with [`dsp_osc_create`].
#[no_mangle]
pub unsafe extern "C" fn dsp_osc_destroy(osc: *mut Oscillator) {
    if !osc.is_null() {
        unsafe {
            drop(Box::from_raw(osc));
        }
    }
}

/// Set the oscillator frequency.
#[no_mangle]
pub unsafe extern "C" fn dsp_osc_set_frequency(osc: *mut Oscillator, freq: f32) {
    if let Some(osc) = unsafe { osc.as_mut() } {
        osc.set_frequency(freq);
    }
}

/// Set the oscillator amplitude.
#[no_mangle]
pub unsafe extern "C" fn dsp_osc_set_amplitude(osc: *mut Oscillator, amp: f32) {
    if let Some(osc) = unsafe { osc.as_mut() } {
        osc.set_amplitude(amp);
    }
}

/// Select the oscillator waveform.
#[no_mangle]
pub unsafe extern "C" fn dsp_osc_set_waveform(osc: *mut Oscillator, waveform: u32) {
    if let Some(osc) = unsafe { osc.as_mut() } {
        osc.set_waveform(waveform_from_c(waveform));
    }
}

/// Generate audio from the oscillator into the supplied buffer.
#[no_mangle]
pub unsafe extern "C" fn dsp_osc_process(osc: *mut Oscillator, buffer: *mut f32, length: u32) {
    if let Some(osc) = unsafe { osc.as_mut() } {
        if buffer.is_null() || length == 0 {
            return;
        }
        let buffer = unsafe { slice::from_raw_parts_mut(buffer, length as usize) };
        osc.process(buffer);
    }
}

/// Reset the oscillator phase.
#[no_mangle]
pub unsafe extern "C" fn dsp_osc_reset(osc: *mut Oscillator) {
    if let Some(osc) = unsafe { osc.as_mut() } {
        osc.reset();
    }
}

// ---------------------------------------------------------------------------
// Utility API
// ---------------------------------------------------------------------------

/// Clamp a floating-point value between the provided minimum and maximum.
#[no_mangle]
pub extern "C" fn dsp_clamp(value: f32, min: f32, max: f32) -> f32 {
    utils::clamp(value, min, max)
}

/// Linearly interpolate between two values.
#[no_mangle]
pub extern "C" fn dsp_lerp(a: f32, b: f32, t: f32) -> f32 {
    utils::lerp(a, b, t)
}

/// Convert a decibel value to linear gain.
#[no_mangle]
pub extern "C" fn dsp_db_to_linear(db: f32) -> f32 {
    utils::db_to_linear(db)
}

/// Convert a linear gain value to decibels.
#[no_mangle]
pub extern "C" fn dsp_linear_to_db(linear: f32) -> f32 {
    utils::linear_to_db(linear.max(1e-12))
}
