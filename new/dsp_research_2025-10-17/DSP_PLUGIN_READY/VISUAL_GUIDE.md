# 📸 Visual Implementation Guide

**See exactly where everything goes**

---

## 📁 Step 1: Project Structure

```
YourPluginProject/
│
├── JUCE/                          ← JUCE framework
│
├── Source/                        ← Your plugin code
│   ├── PluginProcessor.h          ← ADD: #include "dsp_plugin.h"
│   ├── PluginProcessor.cpp        ← ADD: DSP filter code
│   └── PluginEditor.cpp
│
├── DSP/                           ← CREATE THIS FOLDER
│   ├── include/
│   │   └── dsp_plugin.h           ← COPY FROM: DSP_PLUGIN_READY/include/
│   └── lib/
│       └── libdsp_plugin.a        ← COPY FROM: DSP_PLUGIN_READY/target/release/
│
└── CMakeLists.txt                 ← MODIFY: Add DSP paths
```

---

## 🔧 Step 2: Header File Changes

**File: `Source/PluginProcessor.h`**

```cpp
// AT TOP OF FILE (add these):
#include <JuceHeader.h>
#include "dsp_plugin.h"  // ← ADD THIS LINE

class YourProcessor : public juce::AudioProcessor
{
    // ... existing code ...

private:
    // ↓↓↓ ADD THESE MEMBERS ↓↓↓
    DspFilter* filterLeft = nullptr;
    DspFilter* filterRight = nullptr;
    // ↑↑↑ ADD THESE MEMBERS ↑↑↑

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YourProcessor)
};
```

---

## 💻 Step 3: Implementation Changes

**File: `Source/PluginProcessor.cpp`**

### **In Constructor:**

```cpp
YourProcessor::YourProcessor()
{
    // Your existing code stays here
    // No DSP initialization needed yet
}
```

### **In Destructor:**

```cpp
YourProcessor::~YourProcessor()
{
    // ↓↓↓ ADD THESE LINES ↓↓↓
    if (filterLeft != nullptr)
        dsp_filter_destroy(filterLeft);

    if (filterRight != nullptr)
        dsp_filter_destroy(filterRight);
    // ↑↑↑ ADD THESE LINES ↑↑↑
}
```

### **In prepareToPlay():**

```cpp
void YourProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // ↓↓↓ ADD THESE LINES ↓↓↓

    // Clean up old filters
    if (filterLeft != nullptr)
        dsp_filter_destroy(filterLeft);
    if (filterRight != nullptr)
        dsp_filter_destroy(filterRight);

    // Create new filters
    filterLeft = dsp_filter_create((float)sampleRate);
    filterRight = dsp_filter_create((float)sampleRate);

    // Set parameters
    dsp_filter_set_frequency(filterLeft, 1000.0f);
    dsp_filter_set_frequency(filterRight, 1000.0f);

    dsp_filter_set_resonance(filterLeft, 0.5f);
    dsp_filter_set_resonance(filterRight, 0.5f);

    // ↑↑↑ ADD THESE LINES ↑↑↑
}
```

### **In processBlock():**

```cpp
void YourProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                  juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // ↓↓↓ ADD THESE LINES ↓↓↓

    // Safety check
    if (filterLeft == nullptr || filterRight == nullptr)
        return;

    const int numSamples = buffer.getNumSamples();

    // Process left channel
    float* leftChannel = buffer.getWritePointer(0);
    dsp_filter_process(filterLeft, leftChannel, numSamples);

    // Process right channel
    if (buffer.getNumChannels() >= 2)
    {
        float* rightChannel = buffer.getWritePointer(1);
        dsp_filter_process(filterRight, rightChannel, numSamples);
    }

    // ↑↑↑ ADD THESE LINES ↑↑↑
}
```

---

## 🔨 Step 4: CMakeLists.txt Changes

**File: `CMakeLists.txt`**

```cmake
# Your existing CMake setup
project(YourPlugin)
juce_add_plugin(YourPlugin ...)

# ↓↓↓ ADD THESE SECTIONS ↓↓↓

# Add DSP include path
target_include_directories(YourPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DSP/include
)

# Link DSP library
if(WIN32)
    target_link_libraries(YourPlugin PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/DSP/lib/dsp_plugin.lib
        ws2_32 userenv bcrypt
    )
elseif(APPLE)
    target_link_libraries(YourPlugin PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/DSP/lib/libdsp_plugin.a
        "-framework Security"
        "-framework CoreFoundation"
    )
else()
    target_link_libraries(YourPlugin PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/DSP/lib/libdsp_plugin.a
        pthread dl
    )
endif()

# ↑↑↑ ADD THESE SECTIONS ↑↑↑
```

---

## 📊 Step 5: Build & Test

### **Build:**

```bash
# In your plugin project
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### **Test:**

1. Copy plugin to:
   - **Windows:** `C:\Program Files\Common Files\VST3\`
   - **macOS:** `~/Library/Audio/Plug-Ins/VST3/`

2. Open DAW and scan for plugins

3. Load plugin on audio track

4. **It works!** ✅

---

## 🎨 Visual Signal Flow

```
Audio Input
    ↓
[processBlock called]
    ↓
Left Channel → dsp_filter_process(filterLeft, ...) → Filtered Left
    ↓
Right Channel → dsp_filter_process(filterRight, ...) → Filtered Right
    ↓
Audio Output
```

---

## 📋 Complete File Diff

### **Before (no DSP):**

```cpp
// PluginProcessor.h
class YourProcessor {
private:
    // Empty - no DSP members
};

// PluginProcessor.cpp
void processBlock(...) {
    // No DSP processing
}
```

### **After (with DSP):**

```cpp
// PluginProcessor.h
#include "dsp_plugin.h"  // ← Added

class YourProcessor {
private:
    DspFilter* filterLeft = nullptr;   // ← Added
    DspFilter* filterRight = nullptr;  // ← Added
};

// PluginProcessor.cpp
void processBlock(...) {
    dsp_filter_process(filterLeft, ...);   // ← Added
    dsp_filter_process(filterRight, ...);  // ← Added
}
```

---

## ✅ Verification Checklist

After each step, verify:

**After Step 1:**
- [ ] `DSP/include/dsp_plugin.h` exists
- [ ] `DSP/lib/libdsp_plugin.a` exists

**After Step 2:**
- [ ] PluginProcessor.h includes `"dsp_plugin.h"`
- [ ] No compile errors on header

**After Step 3:**
- [ ] PluginProcessor.cpp has filter code
- [ ] Code compiles

**After Step 4:**
- [ ] CMakeLists.txt has DSP paths
- [ ] Project builds successfully

**After Step 5:**
- [ ] Plugin loads in DAW
- [ ] Audio is filtered
- [ ] No crashes

---

## 🎯 Quick Test

Add debug output to verify it's working:

```cpp
void YourProcessor::prepareToPlay(double sr, int bs)
{
    // ... create filters ...

    DBG("Filters created at " << sr << " Hz");
    DBG("Filter left: " << (filterLeft != nullptr));
    DBG("Filter right: " << (filterRight != nullptr));
}

void YourProcessor::processBlock(...)
{
    // ... process audio ...

    // Print first few samples occasionally
    if (buffer.getNumSamples() > 0)
        DBG("First sample: " << buffer.getSample(0, 0));
}
```

Look for these messages in your IDE console!

---

## 🚨 Common Issues - Visual Troubleshooting

### **Issue: "Cannot find dsp_plugin.h"**

**Fix:**
```cmake
# ❌ Wrong:
target_include_directories(... "DSP/include")

# ✅ Correct:
target_include_directories(... "${CMAKE_CURRENT_SOURCE_DIR}/DSP/include")
```

### **Issue: "Undefined reference to dsp_filter_create"**

**Fix:**
```cmake
# ❌ Missing:
target_link_libraries(YourPlugin PRIVATE ...)

# ✅ Add system libs (Windows):
target_link_libraries(YourPlugin PRIVATE
    DSP/lib/dsp_plugin.lib
    ws2_32 userenv bcrypt  # ← These are required!
)
```

### **Issue: "No audio output"**

**Fix:**
```cpp
// ❌ Filters not created:
void processBlock(...) {
    dsp_filter_process(filterLeft, ...);  // filterLeft is null!
}

// ✅ Check first:
void processBlock(...) {
    if (filterLeft == nullptr) return;  // ← Add this!
    dsp_filter_process(filterLeft, ...);
}
```

---

**Follow this visual guide and you'll have a working plugin in minutes!** 📸
