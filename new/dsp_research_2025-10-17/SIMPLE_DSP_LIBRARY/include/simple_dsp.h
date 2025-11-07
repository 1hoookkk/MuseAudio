/* Simple DSP Library - C API */

#ifndef SIMPLE_DSP_H
#define SIMPLE_DSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Opaque handle types
typedef struct emu_filter emu_filter_t;
typedef struct oscillator oscillator_t;

// Waveform types
typedef enum {
    WAVEFORM_SINE = 0,
    WAVEFORM_SAWTOOTH,
    WAVEFORM_SQUARE,
    WAVEFORM_TRIANGLE
} waveform_t;

// Filter functions
emu_filter_t* emu_filter_create(float sample_rate);
void emu_filter_destroy(emu_filter_t* filter);
void emu_filter_set_frequency(emu_filter_t* filter, float frequency);
void emu_filter_set_resonance(emu_filter_t* filter, float resonance);
void emu_filter_set_smoothing(emu_filter_t* filter, float smoothing);
void emu_filter_process(emu_filter_t* filter, float* buffer, size_t length);
void emu_filter_reset(emu_filter_t* filter);

// Oscillator functions
oscillator_t* oscillator_create(float sample_rate);
void oscillator_destroy(oscillator_t* osc);
void oscillator_set_frequency(oscillator_t* osc, float frequency);
void oscillator_set_waveform(oscillator_t* osc, waveform_t waveform);
void oscillator_process(oscillator_t* osc, float* buffer, size_t length);
void oscillator_reset(oscillator_t* osc);

// Utility functions
float lerp(float a, float b, float t);
float smooth(float current, float target, float smoothing);
float clamp(float value, float min, float max);
float freq_to_midi(float freq);
float midi_to_freq(float note);

#ifdef __cplusplus
}
#endif

#endif // SIMPLE_DSP_H
