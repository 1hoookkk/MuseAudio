@echo off
REM Build script for DSP_PLUGIN_READY library
REM Run this before integrating into your plugin

echo ========================================
echo Building DSP_PLUGIN_READY Library
echo ========================================
echo.

REM Check if cargo is available
where cargo >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Cargo not found!
    echo Please install Rust from: https://rustup.rs/
    pause
    exit /b 1
)

echo [1/3] Cleaning old build...
cargo clean

echo.
echo [2/3] Building release version...
cargo build --release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Running tests...
cargo test --release

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Library location:
echo   target\release\dsp_plugin.lib
echo.
echo C API header:
echo   include\dsp_plugin.h
echo.
echo Next steps:
echo   1. Copy dsp_plugin.lib to your plugin project
echo   2. Copy include\dsp_plugin.h to your plugin project
echo   3. Link the library in your build system
echo   4. See IMPLEMENTATION_GUIDE.md for details
echo.
pause
