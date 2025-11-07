#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include <vector>
#include <array>

// Include all consolidated components
#include "SpectralEngine.h"
#include "AtomicOscillatorBank.h"
#include "GrainPool.h"
#include "EMUZPlaneFilter.h"

namespace ConsolidatedDSP {

/**
 * @brief Unified DSP engine consolidating all advanced audio processing capabilities
 * 
 * This is the main consolidation point that brings together all the DSP components
 * from the various directories into a single, cohesive system:
 * 
 * Core Components:
 * - SpectralEngine: Real-time STFT processing with paint-to-audio
 * - AtomicOscillatorBank: Lock-free additive synthesis
 * - GrainPool: Advanced granular synthesis with paint control
 * - EMUZPlaneFilter: Authentic EMU filtering with Z-plane morphing
 * 
 * Features:
 * - Unified parameter management across all components
 * - Cross-component modulation and routing
 * - Real-time performance monitoring
 * - GPU acceleration when available
 * - Comprehensive preset system
 * - Thread-safe operation throughout
 */
class ConsolidatedEngine
{
public:
    // Engine modes for different use cases
    enum class EngineMode
    {
        Creative,        // Full creative capabilities with all features
        Performance,     // Optimized for live performance
        Production,      // High-quality processing for production
        Experimental,    // Experimental features and research
        Vintage,         // Vintage EMU character focus
        Modern           // Modern digital processing focus
    };
    
    // Processing chains
    enum class ProcessingChain
    {
        Sequential,      // Process components in sequence
        Parallel,        // Process components in parallel
        Hybrid,          // Mix of sequential and parallel
        Adaptive,        // Automatically optimize routing
        Custom           // User-defined routing
    };
    
    struct EngineParameters
    {
        // Global engine settings
        EngineMode mode = EngineMode::Creative;
        ProcessingChain chain = ProcessingChain::Hybrid;
        double sampleRate = 44100.0;
        int samplesPerBlock = 512;
        int numChannels = 2;
        
        // Component enable/disable
        bool enableSpectralEngine = true;
        bool enableOscillatorBank = true;
        bool enableGrainPool = true;
        bool enableEMUFilter = true;
        bool enableGPUAcceleration = false;
        
        // Global modulation
        float masterVolume = 1.0f;
        float masterPitch = 0.0f;        // Semitones
        float masterFilter = 1.0f;       // Filter frequency multiplier
        float masterDrive = 0.0f;        // Global drive/saturation
        
        // Performance settings
        float cpuLimit = 80.0f;          // CPU usage limit percentage
        bool enableAdaptiveQuality = true;
        int maxVoices = 256;
        bool enableVoiceStealing = true;
        
        // Paint-to-audio settings
        bool enablePaintControl = true;
        float paintSensitivity = 1.0f;
        float paintSmoothing = 0.1f;
        int paintResolution = 128;       // Paint grid resolution
        
        // Cross-component modulation
        struct ModulationRouting
        {
            bool spectralToFilter = true;     // Spectral data to filter cutoff
            bool grainToOscillator = true;    // Grain density to oscillator pitch
            bool oscillatorToGrain = true;    // Oscillator pitch to grain position
            bool paintToAll = true;           // Paint controls all components
            float modulationDepth = 0.5f;
        } modulation;
    };
    
    // Paint stroke data for unified control
    struct UnifiedPaintStroke
    {
        float x, y;                        // Paint coordinates
        float pressure;                    // Paint pressure/intensity
        juce::Colour color;                // Paint color
        float time;                        // Stroke timing
        float radius;                      // Influence radius
        bool active;                       // Stroke is active
        
        // Component-specific mappings
        struct ComponentMappings
        {
            float spectralIntensity = 1.0f;     // Spectral processing intensity
            float oscillatorPitch = 0.0f;       // Oscillator pitch offset
            float grainDensity = 1.0f;          // Grain density multiplier
            float filterCutoff = 1.0f;          // Filter frequency multiplier
            float drive = 0.0f;                 // Drive amount
        } mappings;
    };
    
    // Performance monitoring
    struct PerformanceMetrics
    {
        float cpuUsage = 0.0f;
        float memoryUsage = 0.0f;
        float audioLatency = 0.0f;
        float paintLatency = 0.0f;
        int activeVoices = 0;
        int activeGrains = 0;
        int activeOscillators = 0;
        bool gpuAccelerated = false;
        float gpuUsage = 0.0f;
        
        // Component-specific metrics
        struct ComponentMetrics
        {
            float spectralCpu = 0.0f;
            float oscillatorCpu = 0.0f;
            float grainCpu = 0.0f;
            float filterCpu = 0.0f;
        } components;
    };
    
    ConsolidatedEngine();
    ~ConsolidatedEngine();
    
    // Core engine management
    bool initialize(const EngineParameters& params);
    void shutdown();
    void reset();
    void setParameters(const EngineParameters& params) noexcept;
    const EngineParameters& getParameters() const noexcept;
    
    // Component access
    SpectralEngine* getSpectralEngine() noexcept { return spectralEngine_.get(); }
    AtomicOscillatorBank* getOscillatorBank() noexcept { return oscillatorBank_.get(); }
    GrainPool* getGrainPool() noexcept { return grainPool_.get(); }
    EMUZPlaneFilter* getEMUFilter() noexcept { return emuFilter_.get(); }
    
    // Audio processing
    void processBlock(juce::AudioBuffer<float>& buffer) noexcept;
    void processMidi(const juce::MidiBuffer& midiMessages) noexcept;
    
    // Paint-to-audio interface
    void addPaintStroke(const UnifiedPaintStroke& stroke);
    void clearPaintStrokes();
    void updatePaintStrokes();  // Called from UI thread
    
    // Preset management
    struct EnginePreset
    {
        juce::String name;
        juce::String description;
        juce::String category;
        EngineParameters engineParams;
        SpectralEngine::SpectralEffect spectralEffect = SpectralEngine::SpectralEffect::None;
        AtomicOscillator::Waveform oscillatorWaveform = AtomicOscillator::Waveform::Sine;
        GrainPool::GrainMode grainMode = GrainPool::GrainMode::Asynchronous;
        EMUZPlaneFilter::FilterType filterType = EMUZPlaneFilter::FilterType::Lowpass;
    };
    
    static std::vector<EnginePreset> getFactoryPresets();
    void loadPreset(const EnginePreset& preset);
    void savePreset(const juce::String& name, const juce::String& category = "User");
    std::vector<EnginePreset> getUserPresets() const;
    
    // Performance monitoring
    PerformanceMetrics getPerformanceMetrics() const noexcept;
    void enablePerformanceMonitoring(bool enable) noexcept;
    
    // GPU acceleration
    bool isGPUAccelerationAvailable() const noexcept;
    void enableGPUAcceleration(bool enable) noexcept;
    bool isGPUAccelerationEnabled() const noexcept;
    
    // Advanced features
    void enableAdaptiveProcessing(bool enable) noexcept;
    void setVoiceLimit(int limit) noexcept;
    void enableVoiceStealing(bool enable) noexcept;
    
    // Analysis and diagnostics
    struct DiagnosticInfo
    {
        bool allComponentsHealthy = true;
        std::vector<juce::String> warnings;
        std::vector<juce::String> errors;
        juce::String engineStatus;
        float averageLatency = 0.0f;
        int xrunCount = 0;
    };
    
    DiagnosticInfo getDiagnosticInfo() const noexcept;
    void runDiagnostics();
    
private:
    // Core components
    std::unique_ptr<SpectralEngine> spectralEngine_;
    std::unique_ptr<AtomicOscillatorBank> oscillatorBank_;
    std::unique_ptr<GrainPool> grainPool_;
    std::unique_ptr<EMUZPlaneFilter> emuFilter_;
    
    // GPU acceleration
    class GPUImp
