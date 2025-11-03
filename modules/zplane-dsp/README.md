# ZPlane DSP - Authentic E-mu Z-Plane Filter

Validated, production-ready Z-plane filter implementation extracted from EngineField.

## Features

- **Block-rate coefficient updates** - Prevents low-frequency zipper noise
- **Smart optimization** - `isSmoothing()` fast-path skips expensive pole math when stable
- **Authentic EMU behavior** - Correct saturation placement, original pole data
- **Sample-rate independent** - Bilinear transform for proper frequency warping
- **Real-time safe** - No allocations, header-only

## Usage

```cpp
#include <zplane/ZPlaneFilter.h>
#include <zplane/EMUAuthenticTables.h>

// In your audio processor
emu::ZPlaneFilter filter;

// In prepareToPlay
filter.prepare(sampleRate, samplesPerBlock);
filter.setShapePair(emu::VOWEL_A, emu::VOWEL_B);

// In processBlock (BEFORE process loop)
filter.updateCoeffsBlock(buffer.getNumSamples());

// Set parameters (RT-safe, uses atomics internally)
filter.setMorph(0.5f);      // 0-1
filter.setIntensity(0.4f);  // 0-1
filter.setDrive(0.2f);      // 0-1
filter.setMix(1.0f);        // 0-1

// In process loop
float* left = buffer.getWritePointer(0);
float* right = buffer.getWritePointer(1);
filter.process(left, right, buffer.getNumSamples());
```

## Shape Pairs

- `VOWEL_A` / `VOWEL_B` - Formant-style (default)
- `BELL_A` / `BELL_B` - Bright metallic resonance
- `LOW_A` / `LOW_B` - Punchy bass
- `SUB_A` / `SUB_B` - Ultra-low rumble

## Integration

Add to your CMakeLists.txt:

```cmake
add_subdirectory(modules/zplane-dsp)

target_link_libraries(YourPlugin
    PRIVATE
    zplane-dsp
)
```

## License

Proprietary - Muse Audio internal use only.

## Origin

Extracted from EngineField plugin (validated in production).
Based on E-mu Audity 2000 hardware measurements.
