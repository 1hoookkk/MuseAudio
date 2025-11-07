#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EmuFilterHandle EmuFilterHandle;

typedef enum EmuFilterError
{
    EmuFilterError_Success = 0,
    EmuFilterError_NullPointer = 1,
    EmuFilterError_InvalidArgument = 2,
    EmuFilterError_ParseError = 3
} EmuFilterError;

typedef enum EmuFilterType
{
    EmuFilterType_Lowpass = 0,
    EmuFilterType_Highpass = 1,
    EmuFilterType_Bandpass = 2,
    EmuFilterType_Notch = 3,
    EmuFilterType_Peak = 4,
    EmuFilterType_Allpass = 5,
    EmuFilterType_VintageEMU = 6,
    EmuFilterType_Morphing = 7
} EmuFilterType;

typedef enum EmuFilterModel
{
    EmuFilterModel_EmuClassic = 0,
    EmuFilterModel_EmuModern = 1,
    EmuFilterModel_ZPlaneMorph = 2,
    EmuFilterModel_AuthenticEmu = 3,
    EmuFilterModel_Hybrid = 4
} EmuFilterModel;

EmuFilterHandle* emu_filter_create(void);
void emu_filter_destroy(EmuFilterHandle* handle);

EmuFilterError emu_filter_prepare(EmuFilterHandle* handle, float sampleRate, int blockSize);
EmuFilterError emu_filter_reset(EmuFilterHandle* handle);

EmuFilterError emu_filter_set_filter_type(EmuFilterHandle* handle, EmuFilterType type);
EmuFilterError emu_filter_set_filter_model(EmuFilterHandle* handle, EmuFilterModel model);
EmuFilterError emu_filter_set_frequency(EmuFilterHandle* handle, float frequency);
EmuFilterError emu_filter_set_resonance(EmuFilterHandle* handle, float resonance);
EmuFilterError emu_filter_set_gain(EmuFilterHandle* handle, float gain);
EmuFilterError emu_filter_set_morph_position(EmuFilterHandle* handle, float morph);
EmuFilterError emu_filter_set_drive(EmuFilterHandle* handle, float drive);
EmuFilterError emu_filter_set_character(EmuFilterHandle* handle, float character);
EmuFilterError emu_filter_set_quality(EmuFilterHandle* handle, float quality);
EmuFilterError emu_filter_enable_nonlinear_stage(EmuFilterHandle* handle, bool enable);
EmuFilterError emu_filter_enable_oversampling(EmuFilterHandle* handle, int factor);

EmuFilterError emu_filter_load_bank_from_json(EmuFilterHandle* handle, const unsigned char* data, unsigned long long length);
EmuFilterError emu_filter_set_active_bank(EmuFilterHandle* handle, const char* name);

EmuFilterError emu_filter_process(EmuFilterHandle* handle, float* buffer, int length);
EmuFilterError emu_filter_process_stereo(EmuFilterHandle* handle, float* left, float* right, int length);

float emu_filter_get_current_frequency(EmuFilterHandle* handle);
float emu_filter_get_current_resonance(EmuFilterHandle* handle);
float emu_filter_get_current_morph(EmuFilterHandle* handle);

#ifdef __cplusplus
}
#endif
