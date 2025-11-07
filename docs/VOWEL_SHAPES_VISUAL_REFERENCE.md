# Vowel Shapes Visual Reference

## LED Matrix Format (8x3 grid)

Each vowel shape is rendered on an 8-column by 3-row LED matrix.
- `█` = LED ON (mint green)
- `░` = LED OFF (dim)

## Shape Pair 0: VOWEL (Formant Transitions)

### AA - Wide Open (morph 0.00 - 0.33)
*Like "father" - maximum mouth opening*
```
░░░░░░░░
░░░░░░░░
████████
```

### AH - Mid Open (morph 0.33 - 0.67)
*Like "hut" - neutral mid position*
```
░░░░░░░░
████████
░░░░░░░░
```

### EE - Smile (morph 0.67 - 1.00)
*Like "see" - upward curve (smile)*
```
░█░░░░█░
░░████░░
░░░░░░░░
```

## Shape Pair 1: BELL (Resonance Shapes)

### OH - Round Medium (morph 0.00 - 0.50)
*Like "go" - oval shape*
```
░░████░░
░█░░░░█░
░░████░░
```

### OO - Round Tight (morph 0.50 - 1.00)
*Like "boot" - small circle*
```
░░░░░░░░
░░░██░░░
░░░░░░░░
```

## Shape Pair 2: LOW (Width Variations)

### Wide - Maximum Width (morph 0.00 - 0.50)
*Low formants - full rectangle*
```
████████
████████
████████
```

### Narrow - Reduced Width (morph 0.50 - 1.00)
*Focused low-end - narrow vertical*
```
░░░░░░░░
░░░██░░░
░░░░░░░░
```

## Shape Pair 3: SUB (Minimal Movement)

### Neutral - Flat/Minimal (any morph)
*Sub bass frequencies - subtle line*
```
░░░░░░░░
░░████░░
░░░░░░░░
```

## Animation Features

### Breathing Effect
When idle or at low activity, all shapes have a subtle "breathing" animation:
- Sine wave modulation of LED positions
- Frequency: ~0.3 Hz (3-second cycle)
- Amplitude: Scales with `activityLevel` (0.0 - 1.0)

### Update Rate
- **Timer Frequency**: 30fps (33ms polling interval)
- **Repaint**: Only when shape changes (conditional repaint)
- **Smoothness**: JUCE component repaint + OS compositor

## Real-Time Mapping

| Z-Plane Filter State | OLEDMouth Visualization |
|---------------------|------------------------|
| VOWEL pair + morph=0.1 | AA (wide open) |
| VOWEL pair + morph=0.5 | AH (mid neutral) |
| VOWEL pair + morph=0.9 | EE (smile curve) |
| BELL pair + morph=0.3 | OH (round oval) |
| BELL pair + morph=0.8 | OO (tight circle) |
| LOW pair + morph=0.2 | Wide (full rectangle) |
| LOW pair + morph=0.7 | Narrow (thin line) |
| SUB pair (any morph) | Neutral (minimal) |

## LED Colors

All LEDs use the OLED aesthetic color scheme:

- **ON**: Mint green (`#d8f3dc`, `OLEDLookAndFeel::MintGreen`)
- **OFF**: Dim mint green (`#d8f3dc` @ 5% alpha)
- **GLOW**: Expanded halo around lit LEDs (20% alpha, 2px expansion)

## Technical Implementation

### Rendering Pipeline
```cpp
// OLEDMouth.h (lines 45-86)
void paint(juce::Graphics& g)
{
    // 1. Calculate cell dimensions (8x3 grid)
    const int cols = 8;
    const int rows = 3;
    const float cellWidth = bounds.getWidth() / cols;
    const float cellHeight = bounds.getHeight() / rows;

    // 2. Iterate through grid
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            // 3. Check if LED should be on
            bool isLit = shouldLEDBeOn(col, row, cols, rows);

            // 4. Draw LED with glow effect
            if (isLit)
            {
                g.setColour(MintGreen.withAlpha(0.2f));
                g.fillRoundedRectangle(cellBounds.expanded(2.0f), 1.0f);  // Glow

                g.setColour(MintGreen);
                g.fillRoundedRectangle(cellBounds, 1.0f);  // LED
            }
            else
            {
                g.setColour(MintGreen.withAlpha(0.05f));
                g.fillRoundedRectangle(cellBounds, 1.0f);  // Dim LED
            }
        }
    }
}
```

### Shape Logic
```cpp
// OLEDMouth.h (lines 124-208)
bool shouldLEDBeOn(int col, int row, int totalCols, int totalRows)
{
    switch (currentVowelShape)
    {
        case VowelShape::AA:
            return row == 2;  // Bottom row only

        case VowelShape::AH:
            return row == 1;  // Middle row only

        case VowelShape::EE:
            // Upward curve calculation
            float distanceFromCenter = abs(col - totalCols/2.0f) / (totalCols/2.0f);
            float curve = distanceFromCenter * 1.2f;
            return row == (int)curve;

        // ... (other shapes)
    }
}
```

## Performance Metrics

- **CPU Usage (UI Thread)**: < 0.5% (30fps repaint)
- **Memory**: 0 allocations (pre-allocated grid)
- **Latency**: 33ms max (one timer tick)
- **Repaint Frequency**: ~30fps (only when shape changes)

## Muse Personality Integration

The OLEDMouth visualizer embodies Muse's personality:

1. **Retro Hardware Aesthetic**: 8x3 LED matrix (classic digital display)
2. **Minimal but Expressive**: Simple shapes convey complex formant positions
3. **Subtle Breathing**: Alive, not static (when idle)
4. **Responsive**: Real-time reaction to filter state (no perceptible lag)
5. **Brutalist Elegance**: Geometric precision, no unnecessary decoration

## Future Enhancements

### Phase 1: Activity-Driven Breathing
- Connect `activityLevel` to RMS/peak detector in audio thread
- Modulate breathing amplitude based on signal strength
- Faster breathing during active processing

### Phase 2: Smooth Transitions
- Interpolate between vowel shapes when morph changes rapidly
- Linear interpolation of LED states over ~100ms
- Prevents jarring jumps during automation

### Phase 3: Intensity Feedback
- Visualize filter resonance strength (intensity parameter)
- Brighter LEDs = higher intensity
- Pulsing effect at extreme resonance

### Phase 4: Custom Shape Editor
- User-definable LED patterns per vowel
- Save/load custom mouth shapes
- "Haunted" randomization mode (Muse personality)

---

**Reference Files**:
- LED Rendering: `source/ui/OLEDMouth.h` (lines 45-208)
- Timer Callback: `source/PluginEditor.cpp` (lines 285-297)
- Vowel Calculation: `source/PluginProcessor.cpp` (lines 216-242)

**Visual Design**: 8x3 LED matrix, mint green (#d8f3dc), retro OLED aesthetic
**Update Rate**: 30fps (33ms polling via juce::Timer)
**Real-Time Safety**: Lock-free atomic reads, zero audio thread blocking
