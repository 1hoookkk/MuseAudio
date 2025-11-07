# 🎛️ Simple DSP Library

A clean, focused DSP library with just the essentials - no over-engineering.

## 📁 What's Included

```
SIMPLE_DSP_LIBRARY/
├── src/
│   ├── lib.rs              # Main library - just 3 core components
│   ├── filter.rs           # EMU-style filter (the good stuff)
│   ├── oscillator.rs       # Basic oscillators
│   └── utils.rs            # Simple math helpers
├── include/
│   └── simple_dsp.h        # C API header
├── examples/
│   ├── basic_filter.rs     # Simple filter example
│   └── simple_synth.rs     # Basic synthesis
├── Cargo.toml              # Build config
└── README.md               # This file
```

## 🚀 Super Simple Usage

### **Rust**

```rust
use simple_dsp::{EmuFilter, Oscillator};

// Create filter
let mut filter = EmuFilter::new(44100.0);
filter.set_frequency(1000.0);
filter.set_resonance(0.5);

// Process audio
let mut output = vec![0.0f32; 256];
filter.process(&mut output);
```

### **C**

```c
#include "simple_dsp.h"

emu_filter_t* filter = emu_filter_create(44100.0);
emu_filter_set_freq(filter, 1000.0f);
emu_filter_set_resonance(filter, 0.5f);

float output[256];
emu_filter_process(filter, output, 256);
```

## 🎯 Only What You Need

### **Core Components**

- **EmuFilter**: Authentic EMU-style filter with real coefficients
- **Oscillator**: Basic wavetable synthesis (sine, saw, square, triangle)
- **Utils**: Simple math helpers (interpolation, smoothing)

### **That's It.**

- No complex architecture
- No over-abstraction
- No unnecessary features
- Just working DSP code

## 🛠️ Build & Test

```bash
cargo build          # Build library
cargo test           # Run tests
cargo run --example basic_filter  # Try the examples
```

## 📊 Performance

- **Real-time safe**: No allocations in audio thread
- **Fast**: Optimized but readable code
- **Small**: Minimal dependencies
- **Cross-platform**: Works anywhere Rust works

---

**Simple. Clean. Focused.** 🎵
