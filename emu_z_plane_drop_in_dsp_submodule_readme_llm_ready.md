# EMU Z‑Plane DSP Submodule — LLM‑ready, Drop‑in Spec

> Purpose: enable **any engineer or code‑gen agent** to implement a **bit‑stable, RT‑safe, drop‑in** Z‑plane morphing filter submodule from this README **without other docs**.

---

## 1) Scope & Guarantees
- **Input**: mono/stereo/ multichannel 32‑bit floats.
- **Output**: same format, **0 latency** (IIR cascade—no look‑ahead).
- **Topology**: **6 biquad (SOS) cascade** = 12th‑order all‑pole filter.
- **Shapes**: each *shape* = **6 complex pole pairs** in polar coords `(r, θ)`; conjugates implied `(r, −θ)`.
- **Morph**: continuous A→B morph in **polar domain** with shortest‑path angle interpolation.
- **RT‑safety**: no heap / locks / file I/O on audio thread.
- **Determinism**: coefficient updates occur **at block boundaries only** when params change.

---

## 2) Normative C ABI (header contract)
> File name: `emu_zplane_api.h`. An implementation **must** export these symbols exactly.

```c
#ifdef __cplusplus
extern "C" {
#endif

// Opaque instance
typedef struct emu_zp_handle emu_zp_handle;

// Status codes
typedef enum {
  EMU_ZP_OK = 0,
  EMU_ZP_ERR_BAD_ARGS = 1,
  EMU_ZP_ERR_UNSUPPORTED = 2,
  EMU_ZP_ERR_STATE = 3
} emu_zp_status;

// Create/Destroy/Reset
emu_zp_handle* emu_create(double sample_rate, int block_size, int channels);
void           emu_destroy(emu_zp_handle* h);
void           emu_reset(emu_zp_handle* h); // clears filter state only

// Runtime parameters (thread‑safe, RT‑safe setters)
emu_zp_status emu_set_morph(emu_zp_handle* h, float morph_0_1);        // 0=A, 1=B
emu_zp_status emu_set_intensity(emu_zp_handle* h, float intensity_0_1); // radius scaling
emu_zp_status emu_set_drive_db(emu_zp_handle* h, float drive_db);
emu_zp_status emu_set_saturation(emu_zp_handle* h, float sat_0_1);      // optional; safe if unimpl.
void          emu_set_smoothing_ms(emu_zp_handle* h, float morph_ms, float intensity_ms);

// Shape loading (control thread only — never audio thread)
// polar_12 layout: {r0,th0, r1,th1, ... r5,th5}; r in (0,1), theta in (-PI,PI]
emu_zp_status emu_set_shapeA_polar(emu_zp_handle* h, const float* polar_12);
emu_zp_status emu_set_shapeB_polar(emu_zp_handle* h, const float* polar_12);

// Optional JSON helpers (implementation may return EMU_ZP_ERR_UNSUPPORTED)
emu_zp_status emu_set_shapeA_json(emu_zp_handle* h, const char* json_text_utf8);
emu_zp_status emu_set_shapeB_json(emu_zp_handle* h, const char* json_text_utf8);

// Processing (returns EMU_ZP_OK on success)
// Planar: arrays of channel pointers; in/out may alias if desired
emu_zp_status emu_process_planar(emu_zp_handle* h, const float** in, float** out, int frames);
// Interleaved convenience (channels as passed to emu_create)
emu_zp_status emu_process_interleaved(emu_zp_handle* h, const float* in, float* out, int frames);

// Queries
int   emu_latency_samples(const emu_zp_handle* h); // MUST be 0
float emu_get_sample_rate(const emu_zp_handle* h);

#ifdef __cplusplus
}
#endif
```

**Build/Linking**
- C99 or C++17; no RTTI/exceptions required.
- Single static lib or amalgamated `.c/.cpp` + this header.
- Optional SIMD (SSE2/NEON) behind compile‑time guards.

---

## 3) Math (normative)
### 3.1 Biquad from a conjugate pole pair
For each pair \(p = r e^{\pm j\theta}\) with \(0<r<1\):
- Denominator: \(A(z) = 1 - 2r\cos\theta\, z^{-1} + r^2 z^{-2}\)  
  → `a0=1`, `a1 = -2*r*cos(theta)`, `a2 = r*r`
- Numerator (default): **unity** → `b0=1, b1=0, b2=0`  
  (Implementations MAY add a *scalar cascade gain* later; see §3.4.)

### 3.2 Morph interpolation (shape A → B)
- **Radius**: linear lerp in \(r\): \(r_m = (1-\alpha) r_A + \alpha r_B\).
- **Angle**: unwrap to **shortest path** before lerp, then wrap to (−π,π].  
  \(\theta_m = (1-\alpha)\,\operatorname{unwrap}(\theta_A,\theta_B) + \alpha\,\cdot\ldots\) → wrap.
- **Intensity** (radius scaling): `r' = 1.0f - (1.0f - r_m) * intensity`.  
  `intensity=0` → bypass toward all‑pass; `1` → original sharpness.

### 3.3 Sample‑rate remapping (exact, no re‑extraction)
Given a pole \(z=r e^{j\theta}\) measured at \(F_s^{(src)}\) and target \(F_s^{(dst)}\):
- \(s = F_s^{(src)} (\ln r + j\theta)\)
- \(z' = e^{s / F_s^{(dst)}}\) → \(r' = r^{F_s^{(dst)}/F_s^{(src)}},\ \theta' = \theta \cdot F_s^{(dst)}/F_s^{(src)}\)  
Wrap \(\theta'\) to (−π,π], clamp \(r'\) to `< 1`.

### 3.4 Cascade gain (optional)
Default is **no normalization** (`b0=1`). If desired, apply a single scalar gain \(g\) **after** SOS cascade to normalize peak or DC. If implemented, expose `emu_set_output_gain_db()` (optional).

### 3.5 Stability & clamps
- Enforce `r <= 0.9995f` after all operations.
- Reject any NaN/Inf inputs in setters with `EMU_ZP_ERR_BAD_ARGS`.

---

## 4) Real‑Time & Threading Rules (normative)
- **No** dynamic allocation, locks, or file I/O on the audio thread.
- Parameter setters are **lock‑free** (atomics or double‑buffered control block).
- Coefficients recomputed **at most once per block** and only when any of `(morph, intensity, shapes, sampleRate)` changed since last call.
- Provide denormal protection (FTZ/DAZ or add tiny DC offset) within the implementation.

**Smoothing**  
Default: one‑pole smoothing for `morph` and `intensity` inside the control block.
- Given time constant `τ_ms` and sample rate `Fs`, per‑sample coefficient `a = exp(-1/(τ_ms*1e-3*Fs))`.
- Apply smoothing to the **control value**; recompute coefficients from the smoothed value at block boundaries.
- `emu_set_smoothing_ms()` MUST accept `0` to disable smoothing for tests.

---

## 5) Data Formats
### 5.1 Polar arrays (mandatory)
`polar_12 = { r0, th0, r1, th1, ... r5, th5 }` with `th` in (−π,π]. If shapes are authored in [0,2π), normalize on load.

### 5.2 JSON shape schema (optional helper)
```json
{
  "name": "Vowel_Ae",
  "family": "Vowel Formants",
  "sample_rate": 48000,
  "sections": 6,
  "pairs": [
    {"r": 0.9951, "theta": 0.41},
    {"r": 0.9937, "theta": 1.08},
    {"r": 0.9902, "theta": 2.31},
    {"r": 0.9890, "theta": 2.92},
    {"r": 0.9870, "theta": 3.77},
    {"r": 0.9855, "theta": 4.21}
  ]
}
```
**Note**: JSON parsing must occur off the audio thread; parsed values are passed via `emu_set_shape*_polar()`.

---

## 6) Processing Contract
- The submodule **must not** modify input buffers.
- Interleaved and planar entry points must yield identical results.
- `emu_latency_samples()` **must return 0**.
- `emu_reset()` clears all SOS states to zero and takes effect before the next `emu_process_*` call.

---

## 7) Reference Implementation Outline (for code‑gen agents)
1. **State**: store sample rate, channels, block size; control block (atomics), smoothed controls, and two 6×(r,θ) shapes.
2. **Control path**: compare‑and‑swap dirty flags; if changed, compute `r_m, θ_m` (with unwrap), apply intensity rule, clamp, then build 6 SOS.
3. **Audio path**: for each channel, run 6 SOS in cascade over `frames`, applying optional drive/saturation pre/post as implemented.
4. **Denormals**: enable FTZ/DAZ or add `1e-20f` bias to state updates.
5. **SIMD (optional)**: process frames in blocks; keep scalar fallback.

---

## 8) Tests & Acceptance (must pass)
**T0 – API sanity**
- Create → set A/B (valid polar_12) → set morph=0/1/0.5 → process 256 frames of zeros; output must remain zeros.

**T1 – Impulse response invariants**
- With `morph=0`, `intensity=1`, feed unit impulse; result equals cascade IR for A within `1e-6` RMS of a direct SOS reference.
- With `morph=1`, same for B.

**T2 – Morph continuity**
- Sweep morph linearly 0→1 over 1s of pink noise; no NaNs/Infs; spectral energy varies smoothly (no >6 dB frame‑to‑frame jumps in 1/3‑oct bands).

**T3 – Stability**
- Randomized shapes where all `r<0.9995` → no runaway; max |sample| < 20 dBFS for unit Gaussian input over 10 s.

**T4 – SR remap parity**
- Author shapes at 48 kHz; remap to 44.1 kHz via §3.3; validate that a 1 kHz tone shifts less than 1 cent relative to expected mapping.

**T5 – Reentrancy**
- Call setters concurrently from a control thread while processing; no data races (TSAN clean) and no glitches beyond smoothing window.

---

## 9) Error Handling
- Setters validate ranges: `morph,intensity,saturation ∈ [0,1]`; otherwise `EMU_ZP_ERR_BAD_ARGS`.
- Null pointers → `EMU_ZP_ERR_BAD_ARGS`.
- Unsupported JSON helpers → `EMU_ZP_ERR_UNSUPPORTED`.

---

## 10) Build Options (recommended macros)
- `EMU_ZP_ENABLE_SIMD` (0/1) – enable SSE2/NEON paths.
- `EMU_ZP_DENORMAL_PROTECT` (0/1) – enable FTZ/DAZ or bias.
- `EMU_ZP_VALIDATE_ARGS` (0/1) – runtime guards on in/out pointers.

---

## 11) Usage Examples
**C (planar)**
```c
emu_zp_handle* h = emu_create(48000.0, 256, 2);
emu_set_shapeA_polar(h, shapeA);
emu_set_shapeB_polar(h, shapeB);
emu_set_morph(h, 0.25f);
emu_set_intensity(h, 1.0f);
const float* in[2]  = {inL, inR};
float*       out[2] = {outL, outR};
emu_process_planar(h, in, out, frames);
```

**C (interleaved)**
```c
emu_zp_handle* h = emu_create(44100.0, 512, 2);
emu_process_interleaved(h, inLR, outLR, frames);
```

---

## 12) LLM Implementer Checklist (copy/paste into your prompt)
- [ ] Create `emu_zplane_api.h` exactly as in §2.
- [ ] Implement `.c/.cpp` with 6‑SOS cascade from §3.1.
- [ ] Polar morph per §3.2 with unwrap+wrap.
- [ ] Intensity radius rule per §3.2.
- [ ] SR remap per §3.3.
- [ ] RT rules from §4 (no alloc/locks on audio thread).
- [ ] Pass tests in §8.

---

## 13) Notes on Banks & Presets (non‑normative)
- Preset banks (LFO/env routing, etc.) are **out of scope** for this submodule. Drive those via host parameters; the submodule exposes only DSP core controls.

---

## 14) Attribution & License
- This README specifies **mathematical filters** in generic terms. Ensure you have rights to any extracted data you ship. Use responsibly.

---

**Version**: 1.0 (LLM‑ready)
**Maintainer**: you
**Contact**: (fill in)
