/*
 * DSP Plugin Ready - C API
 *
 * Simple C API for audio plugin integration
 */

#ifndef DSP_PLUGIN_H
#define DSP_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Opaque filter handle */
typedef struct DspFilter DspFilter;

/* Opaque oscillator handle */
typedef struct DspOscillator DspOscillator;

/* Waveform types */
typedef enum {
    DSP_WAVE_SINE = 0,
    DSP_WAVE_SAW = 1,
    DSP_WAVE_SQUARE = 2,
    DSP_WAVE_TRIANGLE = 3
} DspWaveform;

/*
 * Filter API
 */

/* Create new filter at given sample rate */
DspFilter* dsp_filter_create(float sample_rate);

/* Destroy filter */
void dsp_filter_destroy(DspFilter* filter);

/* Set cutoff frequency (20Hz - 20kHz) */
void dsp_filter_set_frequency(DspFilter* filter, float freq);

/* Set resonance (0.0 - 1.0) */
void dsp_filter_set_resonance(DspFilter* filter, float res);

/* Set smoothing rate (0.0 - 0.1) */
void dsp_filter_set_smoothing(DspFilter* filter, float rate);

/* Process audio buffer in-place */
void dsp_filter_process(DspFilter* filter, float* buffer, uint32_t length);

/* Reset filter state */
void dsp_filter_reset(DspFilter* filter);

/*
 * Oscillator API
 */

/* Create new oscillator at given sample rate */
DspOscillator* dsp_osc_create(float sample_rate);

/* Destroy oscillator */
void dsp_osc_destroy(DspOscillator* osc);

/* Set frequency in Hz (20Hz - 20kHz) */
void dsp_osc_set_frequency(DspOscillator* osc, float freq);

/* Set amplitude (0.0 - 1.0) */
void dsp_osc_set_amplitude(DspOscillator* osc, float amp);

/* Set waveform type */
void dsp_osc_set_waveform(DspOscillator* osc, DspWaveform waveform);

/* Generate audio into buffer */
void dsp_osc_process(DspOscillator* osc, float* buffer, uint32_t length);

/* Reset oscillator phase */
void dsp_osc_reset(DspOscillator* osc);

/*
 * Utility functions
 */

/* Clamp value between min and max */
float dsp_clamp(float value, float min, float max);

/* Linear interpolation */
float dsp_lerp(float a, float b, float t);

/* Convert dB to linear gain */
float dsp_db_to_linear(float db);

/* Convert linear gain to dB */
float dsp_linear_to_db(float linear);

#ifdef __cplusplus
}
#endif

#endif /* DSP_PLUGIN_H */
