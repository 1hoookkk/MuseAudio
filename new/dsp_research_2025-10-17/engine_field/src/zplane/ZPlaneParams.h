#pragma once

struct ZPlaneParams
{
    int   morphPair = 0;     // index into MORPH_PAIRS
    float morph     = 0.0f;  // [0..1]
    float intensity = 0.0f;  // 0 => null-friendly
    float driveDb   = 0.0f;  // 0 dB => 1.0
    float sat       = 0.0f;  // [0..1]
    float lfoRate   = 0.0f;  // Hz
    float lfoDepth  = 0.0f;  // [0..1]
    bool  autoMakeup= false;

    // Extended parameters for production use
    float radiusGamma     = 1.0f;  // pole radius scaling
    float postTiltDbPerOct= 0.0f;  // spectral tilt compensation
    float driveHardness   = 0.5f;  // drive characteristic

    // Formant-pitch coupling
    bool  formantLock     = true;  // true=Lock formants, false=Follow pitch
    float pitchRatio      = 1.0f;  // current pitch shift ratio for formant compensation
};

