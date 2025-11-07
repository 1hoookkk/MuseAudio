# ⚡ Quick Start - 5 Minutes to Working Plugin

**Get your DSP library running in a plugin in 5 minutes.**

---

## 🎯 Option 1: Copy-Paste Implementation (Fastest)

### **Step 1: Build DSP Library (30 seconds)**

```bash
cd DSP_PLUGIN_READY
cargo build --release
```

Or double-click: **`build_library.bat`** (Windows)

### **Step 2: Copy Files to Plugin (30 seconds)**

```bash
# In your JUCE plugin project:
mkdir DSP
mkdir DSP/include
mkdir DSP/lib

# Copy header
copy "DSP_PLUGIN_READY\include\dsp_plugin.h" "YourPlugin\DSP\include\"

# Copy library
copy "DSP_PLUGIN_READY\target\release\dsp_plugin.lib" "YourPlugin\DSP\lib\"
```

### **Step 3: Use Template Code (2 minutes)**

**Option A:** Use complete template

1. Open `DSP_PLUGIN_READY/examples/juce_plugin_template.cpp`
2. Copy entire file to your `Source/PluginProcessor.cpp`
3. Done! It's a complete working plugin.

**Option B:** Add to existing plugin

Copy just the key parts from template:

```cpp
// In your header:
#include "dsp_plugin.h"
DspFilter* filterLeft = nullptr;
DspFilter* filterRight = nullptr;

// In constructor:
// (nothing needed)

// In prepareToPlay():
filterLeft = dsp_filter_create((float)sampleRate);
filterRight = dsp_filter_create((float)sampleRate);

// In processBlock():
float* left = buffer.getWritePointer(0);
float* right = buffer.getWritePointer(1);
dsp_filter_process(filterLeft, left, buffer.getNumSamples());
dsp_filter_process(filterRight, right, buffer.getNumSamples());

// In destructor:
dsp_filter_destroy(filterLeft);
dsp_filter_destroy(filterRight);
```

### **Step 4: Link Library (1 minute)**

Add to `CMakeLists.txt`:

```cmake
target_include_directories(YourPlugin PRIVATE DSP/include)
target_link_libraries(YourPlugin PRIVATE DSP/lib/dsp_plugin.lib)
```

### **Step 5: Build & Test (1 minute)**

```bash
cmake --build . --config Release
```

**Done!** 🎉

---

## 🎯 Option 2: Use Full Template Project

We provide a complete CMakeLists.txt template:

1. Copy `CMakeLists_TEMPLATE.txt` to your project
2. Rename to `CMakeLists.txt`
3. Adjust paths
4. Build!

See: **`CMakeLists_TEMPLATE.txt`** in this folder

---

## 🎯 Option 3: Read Full Guide

For detailed explanation of every step:

See: **`IMPLEMENTATION_GUIDE.md`**

---

## 🎛️ What You Get

After following any option above, you have:

✅ Working filter plugin
✅ Stereo processing
✅ Smooth parameter changes
✅ RT-safe audio processing
✅ Ready for customization

---

## 📊 File Checklist

Before building, make sure you have:

- [ ] `DSP/include/dsp_plugin.h` - C API header
- [ ] `DSP/lib/libdsp_plugin.a` (or .lib) - Compiled library
- [ ] `Source/PluginProcessor.cpp` - Using DSP functions
- [ ] `CMakeLists.txt` - Links DSP library

---

## 🐛 Quick Troubleshooting

**Build fails?**
- Make sure you ran `cargo build --release` first
- Check library path in CMakeLists.txt

**No audio?**
- Add `DBG()` statements to verify `prepareToPlay()` is called
- Check filters aren't null: `DBG(filterLeft != nullptr);`

**Pops/clicks?**
- Add smoothing: `dsp_filter_set_smoothing(filter, 0.001f);`

---

## 🎯 Next Steps

1. ✅ Get basic filter working (5 minutes)
2. ✅ Add parameter controls (use template)
3. ✅ Customize UI
4. ✅ Try oscillator (see `dsp_plugin.h`)
5. ✅ Ship your plugin! 🚀

---

## 📚 More Help

- **Complete guide:** `IMPLEMENTATION_GUIDE.md`
- **Template code:** `examples/juce_plugin_template.cpp`
- **Template build:** `CMakeLists_TEMPLATE.txt`
- **API reference:** `include/dsp_plugin.h`

---

**You're 5 minutes away from a working filter plugin!** ⚡
