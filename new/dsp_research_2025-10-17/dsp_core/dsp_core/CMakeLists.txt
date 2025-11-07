# Header-only DSP core utilities
add_library(dsp_core INTERFACE)

# Public include directory exposing libs/ so consumers can #include <dsp_core/file.hpp>
# Headers are at libs/dsp_core/*.hpp
target_include_directories(dsp_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)
