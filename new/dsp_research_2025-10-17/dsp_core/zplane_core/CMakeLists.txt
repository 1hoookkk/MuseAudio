cmake_minimum_required(VERSION 3.16)

# Header-only library - all implementations are inline/template
add_library(zplane_core INTERFACE)

target_include_directories(zplane_core
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_features(zplane_core INTERFACE cxx_std_20)

# Link JUCE dependencies (interface - zplane_core consumers will link these)
target_link_libraries(zplane_core
    INTERFACE
        juce::juce_dsp
        juce::juce_audio_basics
)

