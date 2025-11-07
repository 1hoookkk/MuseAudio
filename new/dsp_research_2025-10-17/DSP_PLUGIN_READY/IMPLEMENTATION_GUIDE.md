# 🔧 Step-by-Step Implementation Guide

**How to integrate DSP_PLUGIN_READY into your audio plugin**

---

## 📋 Prerequisites

- JUCE framework installed
- CMake or Projucer project set up
- Basic C++ knowledge
- Rust installed (to build the DSP library)

---

## 🚀 Quick Implementation (15 Minutes)

### **Step 1: Build the DSP Library**

```bash
# Navigate to the DSP library
cd "C:\NEEDS CONSOLIDATING AND REFACTOR\DSP_PLUGIN_READY"

# Build the library
cargo build --release

# This creates:
# - Windows: target/release/dsp_plugin.lib
# - macOS: target/release/libdsp_plugin.a
# - Linux: target/release/libdsp_plugin.a
```

---

### **Step 2: Copy Files to Your Plugin Project**

```
YourPlugin/
├── Source/
│   └── PluginProcessor.cpp
├── DSP/                    ← Create this folder
│   ├── include/
│   │   └── dsp_plugin.h    ← Copy from DSP_PLUGIN_READY/include/
│   └── lib/
│       └── libdsp_plugin.a ← Copy from DSP_PLUGIN_READY/target/release/
└── CMakeLists.txt (or YourPlugin.jucer)
```

**Commands:**
```bash
# In your plugin project root
mkdir -p DSP/include
mkdir -p DSP/lib

# Copy header
cp "C:\NEEDS CONSOLIDATING AND REFACTOR\DSP_PLUGIN_READY\include\dsp_plugin.h" DSP/include/

# Copy library (Windows example)
cp "C:\NEEDS CONSOLIDATING AND REFACTOR\DSP_PLUGIN_READY\target\release\dsp_plugin.lib" DSP/lib/
```

---

### **Step 3: Update Your Build Configuration**

#### **Option A: Using CMake**

Add to your `CMakeLists.txt`:

```cmake
# Add DSP library
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DSP/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DSP/lib/libdsp_plugin.a
)

# On Windows, you might need:
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32
        userenv
        bcrypt
    )
endif()

# On macOS, you might need:
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Security"
        "-framework CoreFoundation"
    )
endif()
```

#### **Option B: Using Projucer**

1. Open your `.jucer` file in Projucer
2. Go to **File → Global Paths**
3. Set header search path: `../../DSP/include`
4. Go to **Exporters** → Your platform
5. Add to **Extra Linker Flags**:
   ```
   ../../DSP/lib/libdsp_plugin.a
   ```
6. Save and regenerate your IDE project

---

### **Step 4: Implement in Your PluginProcessor**

#### **Header File (`PluginProcessor.h`):**

```cpp
#pragma once

#include <JuceHeader.h>
#include "dsp_plugin.h"  // ← Add this

class YourAudioProcessor : public juce::AudioProcessor
{
public:
    YourAudioProcessor();
    ~YourAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // ... other JUCE methods ...

private:
    // DSP components
    DspFilter* filterLeft = nullptr;   // ← Add these
    DspFilter* filterRight = nullptr;  // ← Add these

    // Parameters
    float filterFrequency = 1000.0f;
    float filterResonance = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YourAudioProcessor)
};
```

#### **Implementation File (`PluginProcessor.cpp`):**

```cpp
#include "PluginProcessor.h"

YourAudioProcessor::YourAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Create filters (will be initialized in prepareToPlay)
}

YourAudioProcessor::~YourAudioProcessor()
{
    // Clean up DSP resources
    if (filterLeft != nullptr)
        dsp_filter_destroy(filterLeft);

    if (filterRight != nullptr)
        dsp_filter_destroy(filterRight);
}

void YourAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Clean up old filters
    if (filterLeft != nullptr)
        dsp_filter_destroy(filterLeft);
    if (filterRight != nullptr)
        dsp_filter_destroy(filterRight);

    // Create new filters at correct sample rate
    filterLeft = dsp_filter_create((float)sampleRate);
    filterRight = dsp_filter_create((float)sampleRate);

    // Set initial parameters
    dsp_filter_set_frequency(filterLeft, filterFrequency);
    dsp_filter_set_frequency(filterRight, filterFrequency);

    dsp_filter_set_resonance(filterLeft, filterResonance);
    dsp_filter_set_resonance(filterRight, filterResonance);

    dsp_filter_set_smoothing(filterLeft, 0.01f);
    dsp_filter_set_smoothing(filterRight, 0.01f);
}

void YourAudioProcessor::releaseResources()
{
    // Reset filters (optional)
    if (filterLeft != nullptr)
        dsp_filter_reset(filterLeft);
    if (filterRight != nullptr)
        dsp_filter_reset(filterRight);
}

void YourAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // Make sure we have filters
    if (filterLeft == nullptr || filterRight == nullptr)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Process left channel
    if (numChannels >= 1)
    {
        float* leftChannel = buffer.getWritePointer(0);
        dsp_filter_process(filterLeft, leftChannel, numSamples);
    }

    // Process right channel
    if (numChannels >= 2)
    {
        float* rightChannel = buffer.getWritePointer(1);
        dsp_filter_process(filterRight, rightChannel, numSamples);
    }
}
```

---

## 🎛️ Advanced: Adding Parameters

### **Step 5: Add APVTS Parameters**

In `PluginProcessor.h`:

```cpp
class YourAudioProcessor : public juce::AudioProcessor
{
public:
    // ... existing code ...

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    // Add APVTS
    juce::AudioProcessorValueTreeState parameters;

    // Parameter atomic pointers
    std::atomic<float>* freqParameter = nullptr;
    std::atomic<float>* resParameter = nullptr;

    // ... existing members ...
};
```

In `PluginProcessor.cpp`:

```cpp
YourAudioProcessor::YourAudioProcessor()
    : AudioProcessor(BusesProperties()...),
      parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Get parameter pointers
    freqParameter = parameters.getRawParameterValue("frequency");
    resParameter = parameters.getRawParameterValue("resonance");
}

juce::AudioProcessorValueTreeState::ParameterLayout
YourAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Frequency parameter (20Hz - 20kHz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "frequency",
        "Frequency",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        1000.0f,
        "Hz"
    ));

    // Resonance parameter (0.0 - 1.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "resonance",
        "Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    return { params.begin(), params.end() };
}

void YourAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    if (filterLeft == nullptr || filterRight == nullptr)
        return;

    // Update parameters from APVTS
    float freq = freqParameter->load();
    float res = resParameter->load();

    dsp_filter_set_frequency(filterLeft, freq);
    dsp_filter_set_frequency(filterRight, freq);

    dsp_filter_set_resonance(filterLeft, res);
    dsp_filter_set_resonance(filterRight, res);

    // Process audio
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels >= 1)
    {
        float* leftChannel = buffer.getWritePointer(0);
        dsp_filter_process(filterLeft, leftChannel, numSamples);
    }

    if (numChannels >= 2)
    {
        float* rightChannel = buffer.getWritePointer(1);
        dsp_filter_process(filterRight, rightChannel, numSamples);
    }
}
```

---

## 🎨 Step 6: Add a Simple UI (Optional)

In `PluginEditor.h`:

```cpp
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class YourAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    YourAudioProcessorEditor(YourAudioProcessor&);
    ~YourAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    YourAudioProcessor& audioProcessor;

    juce::Slider freqSlider;
    juce::Label freqLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;

    juce::Slider resSlider;
    juce::Label resLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YourAudioProcessorEditor)
};
```

In `PluginEditor.cpp`:

```cpp
#include "PluginEditor.h"

YourAudioProcessorEditor::YourAudioProcessorEditor(YourAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Frequency slider
    freqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    addAndMakeVisible(freqSlider);

    freqLabel.setText("Frequency", juce::dontSendNotification);
    freqLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(freqLabel);

    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "frequency", freqSlider);

    // Resonance slider
    resSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    resSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    addAndMakeVisible(resSlider);

    resLabel.setText("Resonance", juce::dontSendNotification);
    resLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(resLabel);

    resAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "resonance", resSlider);

    setSize(400, 300);
}

YourAudioProcessorEditor::~YourAudioProcessorEditor()
{
}

void YourAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawText("DSP Filter Plugin", getLocalBounds().removeFromTop(50),
               juce::Justification::centred, true);
}

void YourAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(50); // Title space

    auto sliderArea = bounds.removeFromTop(200);

    // Frequency controls
    auto freqArea = sliderArea.removeFromLeft(getWidth() / 2).reduced(10);
    freqLabel.setBounds(freqArea.removeFromTop(20));
    freqSlider.setBounds(freqArea);

    // Resonance controls
    auto resArea = sliderArea.reduced(10);
    resLabel.setBounds(resArea.removeFromTop(20));
    resSlider.setBounds(resArea);
}
```

---

## 🔨 Step 7: Build and Test

### **Build the Plugin:**

```bash
# If using CMake
cd your-plugin-project/build
cmake ..
cmake --build . --config Release

# If using Projucer
# Open in Xcode/Visual Studio and build
```

### **Test in DAW:**

1. Copy built plugin to:
   - **Windows:** `C:\Program Files\Common Files\VST3\`
   - **macOS:** `~/Library/Audio/Plug-Ins/VST3/`

2. Open your DAW (Ableton, Logic, Reaper, etc.)

3. Scan for new plugins

4. Load your plugin on a track

5. Test:
   - Adjust frequency knob → should hear filter cutoff change
   - Adjust resonance knob → should hear resonance increase
   - Play audio through plugin → should be filtered

---

## 🐛 Troubleshooting

### **Problem: "Cannot find dsp_plugin.h"**

**Solution:**
- Check include path in CMakeLists.txt
- Make sure header is in `DSP/include/`
- Try absolute path: `target_include_directories(... "${CMAKE_SOURCE_DIR}/DSP/include")`

### **Problem: Linker errors with Rust library**

**Solution:**
- On Windows, add system libraries:
  ```cmake
  target_link_libraries(YourPlugin PRIVATE ws2_32 userenv bcrypt)
  ```
- On macOS, add frameworks:
  ```cmake
  target_link_libraries(YourPlugin PRIVATE "-framework Security")
  ```

### **Problem: Plugin loads but no audio**

**Solution:**
- Check `prepareToPlay()` is called
- Add debug prints:
  ```cpp
  DBG("Filter created: " << (filterLeft != nullptr));
  DBG("Processing " << numSamples << " samples");
  ```
- Verify sample rate is valid (not 0)

### **Problem: Audio pops/clicks**

**Solution:**
- Increase smoothing:
  ```cpp
  dsp_filter_set_smoothing(filterLeft, 0.001f); // Lower = faster
  ```
- Don't change parameters every sample
- Update parameters only when they change

---

## 📊 Performance Tips

### **1. Update Parameters Efficiently**

**Bad:**
```cpp
void processBlock(...)
{
    // DON'T do this - sets params every block even if unchanged
    dsp_filter_set_frequency(filter, someValue);
    dsp_filter_process(filter, buffer, numSamples);
}
```

**Good:**
```cpp
void processBlock(...)
{
    // Only update when changed
    float newFreq = freqParameter->load();
    if (std::abs(newFreq - lastFreq) > 0.1f)
    {
        dsp_filter_set_frequency(filter, newFreq);
        lastFreq = newFreq;
    }
    dsp_filter_process(filter, buffer, numSamples);
}
```

### **2. Use Separate Filters for Stereo**

**Bad:**
```cpp
// Processing both channels with one filter = mono!
for (int ch = 0; ch < numChannels; ++ch)
    dsp_filter_process(filter, buffer.getWritePointer(ch), numSamples);
```

**Good:**
```cpp
// Separate filters = true stereo
dsp_filter_process(filterLeft, buffer.getWritePointer(0), numSamples);
dsp_filter_process(filterRight, buffer.getWritePointer(1), numSamples);
```

### **3. Benchmark Your Plugin**

Add timing code:
```cpp
void processBlock(...)
{
    auto start = std::chrono::high_resolution_clock::now();

    // ... processing ...

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    DBG("Process time: " << duration.count() << " µs");
}
```

---

## ✅ Complete Example Project Structure

```
MyFilterPlugin/
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   └── PluginEditor.cpp
├── DSP/
│   ├── include/
│   │   └── dsp_plugin.h
│   └── lib/
│       └── libdsp_plugin.a
├── CMakeLists.txt
└── MyFilterPlugin.jucer
```

---

## 🎯 Next Steps

1. ✅ Follow steps 1-7 above
2. ✅ Build and test basic filter
3. ✅ Add more parameters (try oscillator too!)
4. ✅ Customize UI
5. ✅ Profile performance
6. ✅ Ship your plugin! 🚀

---

## 📚 Additional Resources

- **DSP Code:** `DSP_PLUGIN_READY/src/`
- **C API Reference:** `DSP_PLUGIN_READY/include/dsp_plugin.h`
- **Examples:** `DSP_PLUGIN_READY/examples/`
- **JUCE Docs:** https://docs.juce.com
- **Rust FFI:** `DSP_PLUGIN_READY/README.md`

---

**You now have everything to implement the DSP library in your plugin!** 🎵

Just follow the steps, copy-paste the code, and you'll have a working filter plugin in 15 minutes.
