cmake_minimum_required(VERSION 3.25)

project(zplane-dsp VERSION 1.0.0 LANGUAGES CXX)

# Header-only library for Z-Plane DSP
# Extracted from validated EngineField implementation
add_library(zplane-dsp INTERFACE)

target_include_directories(zplane-dsp
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Requires JUCE (for juce::LinearSmoothedValue and juce::MathConstants)
# JUCE must be available in parent project
target_link_libraries(zplane-dsp
    INTERFACE
    juce::juce_dsp
)

# C++17 minimum (for constexpr if)
target_compile_features(zplane-dsp INTERFACE cxx_std_17)
