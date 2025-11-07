cmake_minimum_required(VERSION 3.22)

# Determine plugin formats based on top-level options
set(engine_formats "")
if (BUILD_VST3)
    list(APPEND engine_formats VST3)
endif()
if (BUILD_STANDALONE)
    list(APPEND engine_formats Standalone)
endif()

if (NOT engine_formats)
    message(FATAL_ERROR "No plugin formats enabled. Enable BUILD_VST3 and/or BUILD_STANDALONE.")
endif()

juce_add_plugin(EngineField
    COMPANY_NAME "YourCompany"
    COMPANY_WEBSITE "https://example.com"
    BUNDLE_ID com.example.enginefield
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS ${engine_formats}
    PRODUCT_NAME "Engine Field"
)

target_sources(EngineField PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/zplane/AuthenticEMUEngine.h
    src/zplane/BiquadCascade.h
    src/zplane/IShapeBank.h
    src/zplane/IZPlaneEngine.h
    src/zplane/NonlinearStage.h
    src/zplane/OversampledEngine.h
    src/zplane/StaticShapeBank.h
    src/zplane/ZPlaneParams.h
    src/zplane/ZPoleMath.h
)

target_compile_definitions(EngineField
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(EngineField
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
)

target_compile_options(EngineField PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

