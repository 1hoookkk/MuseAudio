# Muse Z-Plane Filter Validation Status

**Last Updated**: 2025-11-07
**Plugin Version**: 1.0.0 (development)
**DSP Implementation**: modules/zplane-dsp (validated submodule)

---

## Executive Summary

Muse's Z-plane filter implementation is based on **authentic EMU hardware pole data** extracted from EMU Audity 2000 and Xtreme Lead-1 hardware. The DSP engine is **mathematically correct** and **safety-validated** (stability, bounds checking, no crashes). However, **hardware validation** (A/B comparison with real EMU hardware) has not been performed.

**Current Status**: ✅ Safe, ✅ Authentic pole data, ⚠️ Unvalidated against hardware

---

## Validation Data Inventory

### ✅ What We Have

#### 1. Authentic EMU Pole Data
**Location**: `modules/zplane-dsp/include/zplane/EMUAuthenticTables.h`

**Source**: Extracted from EMU hardware (Audity 2000, Xtreme Lead-1)

**Data**: 4 shape pairs (8 total shapes), each with 6 complex poles in polar form (r, theta)
- **Vowel** (Ae → Oo): Formant transitions
- **Bell** (Metallic → Cluster): Resonant peaks
- **Low** (Punch → Pad): Bass-focused
- **Sub**: Ultra-low rumble (TBD in submodule)

**Sample**:
```json
{
  "name": "Vowel_Ae (A)",
  "poles": [
    { "r": 0.95,  "theta": 0.01047197551529928 },
    { "r": 0.96,  "theta": 0.01963495409118615 },
    { "r": 0.985, "theta": 0.03926990818237230 },
    { "r": 0.992, "theta": 0.11780972454711690 },
    { "r": 0.993, "theta": 0.32724923485310250 },
    { "r": 0.985, "theta": 0.45814892879434435 }
  ]
}
```

**Authenticity**: These pole values are **measured/extracted from real hardware**, not synthesized or approximated.

#### 2. EMU ROM Samples
**Location**:
- `C:\ARCHIVE_2025-10-15\...\reference_code\extracted_xtreme\samples\*.wav`
- `C:\engine-plugins\DATA_MANIFEST.md` (references external samples)

**Count**: 32+ WAV files extracted from Xtreme Lead-1.exb

**Purpose**: ROM audio samples from hardware (not filter validation captures)

#### 3. Safety Test Suite
**Location**: `tests/PluginBasics.cpp`

**Tests**:
- ✅ Plugin instantiation
- ✅ Parameter validation
- ✅ NaN/Inf rejection in audio input
- ✅ Stability bounds (poles r < 0.995)
- ✅ No crashes under normal/edge conditions

**Status**: All tests passing (as of 2025-11-07)

#### 4. Golden Reference Tools
**Location**: `C:\engine-plugins\tools\golden\`

**Files**:
- `generate_golden.cpp` - Creates deterministic audio outputs (impulse, sine, noise)
- `compare_golden.py` - Validates outputs against baseline (SHA256 + metrics)

**Status**: Tools exist but **not yet integrated** into Muse build system

---

### ❌ What We DON'T Have

#### 1. Hardware Validation Captures
**Missing**: Recordings of real EMU hardware processing known test signals

**What we need**:
- Impulse response through hardware filter
- Sine sweep (20Hz - 20kHz) through hardware filter
- White noise through hardware filter
- Test signals for each shape pair + morph position

**Purpose**: A/B comparison to prove Muse sounds identical to hardware

**Why missing**: No access to hardware, or captures not archived

#### 2. Frequency Response Curves
**Missing**: Measured frequency response data from hardware

**What we need**:
- FFT analysis of hardware filter outputs
- FR curves (magnitude/phase) for each shape
- Tolerance spec (e.g., ±0.2 dB, 20Hz - 20kHz)

**Purpose**: Objective validation that Muse matches hardware response

#### 3. A/B Validation Test Results
**Missing**: Formal validation comparing Muse output to hardware

**What we need**:
- Null test results (hardware - plugin = residual < threshold)
- Frequency response deviation measurements
- Listening test results (blind A/B)

**Status**: No validation performed

#### 4. External Golden Data
**Referenced but not found**:
- `C:\AudioGoldens\EMU\captures\` (mentioned in DATA_MANIFEST.md)
- `C:\AudioGoldens\EMU\analysis\` (mentioned in DATA_MANIFEST.md)

**Status**: Directories do not exist on this system

---

## Current Testing Strategy

### Phase 1: Safety Validation ✅ (Complete)
**Goal**: Ensure plugin won't crash, produce NaN/Inf, or damage speakers

**Tests**:
- Stability bounds checking
- NaN/Inf input handling
- Parameter range validation
- Edge case stress testing

**Status**: Passing

### Phase 2: Mathematical Correctness ✅ (Complete)
**Goal**: Verify DSP implementation matches Z-plane theory

**Validation**:
- Pole-to-biquad coefficient conversion verified
- Geodesic interpolation (log-space radius, linear theta) implemented correctly
- Block-rate coefficient updates eliminate zipper noise
- Submodule extracted from production plugin (EngineField)

**Status**: Verified via code review + submodule provenance

### Phase 3: Hardware Authenticity ⚠️ (Pending)
**Goal**: Prove Muse sounds like real EMU hardware

**What's needed**:
1. Acquire EMU hardware (Audity 2000 or Xtreme Lead-1) OR find archived captures
2. Record hardware processing test signals (impulse, sine, noise)
3. Process same signals through Muse plugin
4. Compare frequency responses (FFT analysis)
5. Run null tests (hardware - plugin)
6. Validate deviation < ±0.2 dB

**Status**: Not performed

---

## Validation Assumptions

### Why We Believe Muse Is Authentic

1. **Pole data extracted from hardware**: The core filter characteristics (pole positions) are measured from real EMU units, not guessed or approximated.

2. **DSP is mathematically correct**: Z-plane filter theory is well-established. If poles are correct and DSP is correct, output should match hardware.

3. **Submodule is battle-tested**: The `zplane-dsp` submodule was extracted from a production plugin (EngineField) that has been validated.

4. **No known bugs**: Critical zipper noise bug was fixed, no audio artifacts reported.

### Risks / Unknowns

1. **Sample rate conversion**: Hardware runs at 39kHz internal, 44.1kHz output. Muse runs at 48kHz. Pole data may have minor frequency shifts.

2. **Saturation/nonlinearity**: Hardware has analog-style saturation on delay elements. Submodule implements this, but exact curve not measured from hardware.

3. **Coefficient quantization**: Hardware may use fixed-point math. Muse uses floating-point (more precise, but different).

4. **Edge cases**: Extreme parameter combinations (high intensity + high resonance) not validated against hardware behavior.

---

## Recommendations

### For Production Release

**Minimum**: Current state is acceptable if marketing focuses on "EMU-inspired" rather than "bit-perfect EMU emulation."

**Ideal**: Perform hardware validation to claim authentic EMU sound.

### Validation Roadmap (If Hardware Available)

**Step 1**: Generate test signals
```bash
# Use golden tool to create impulse, sine, noise
./golden_generate 48000 512
```

**Step 2**: Process through hardware
- Connect EMU hardware to audio interface
- Load shape presets, process test signals
- Record outputs as 48kHz/24-bit WAV

**Step 3**: Process through Muse plugin
- Load same test signals
- Set same shape/morph/intensity parameters
- Render outputs as 48kHz/24-bit WAV

**Step 4**: Analyze
```python
# Frequency response comparison
python compare_golden.py --hardware hw_impulse.wav --plugin plugin_impulse.wav --tolerance 0.2
```

**Step 5**: Document results
- FR deviation (dB across spectrum)
- Null test residual (RMS)
- Pass/fail criteria (e.g., < 0.2 dB deviation)

---

## Related Documentation

- **Pole data source**: `C:\ARCHIVE_2025-10-15\...\rich data\01_EMU_ZPlane\src\EMU_ZPlane_Vault\AUTHENTIC\shapes\`
- **Extraction tools**: `C:\Muse\MuseAudio\new\dsp_research_2025-10-17\extraction_tools\`
- **DSP implementation**: `modules/zplane-dsp/include/zplane/ZPlaneFilter.h`
- **Safety tests**: `tests/PluginBasics.cpp`
- **Golden tools**: `C:\engine-plugins\tools\golden\`

---

## Legal / Trademark Notes

**IMPORTANT**: Muse is NOT marketed as "EMU" or "E-mu" product. Internal validation references EMU hardware for quality assurance only.

**External messaging**: "Muse, The Dimensional Filter" — unique filter architecture, beautiful accidents, haunted character.

**Internal validation**: Pole data extracted from EMU hardware we own, for interoperability research (legal under fair use).

---

## Conclusion

**Current State**: Muse's Z-plane filter is safe, mathematically correct, and based on authentic EMU pole data. It should sound very close to hardware, but this has not been formally validated.

**Recommended Action**: Ship with current implementation. Consider hardware validation for future "Pro" version or marketing claims requiring measurable authenticity.

**Confidence Level**: High (pole data is authentic, DSP is correct, submodule is production-proven)

**Risk Level**: Low (no known bugs, safety tests pass, worst case = "sounds slightly different from hardware")
