#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include "AlienColors.h"
#include <array>
#include <complex>

namespace engine::ui
{
    /**
     * Alien Z-Plane Visualizer - Complete implementation
     *
     * Displays poles/zeros on the complex unit circle with:
     * - Frequency response curve (computed from H(z))
     * - Energy field visualization
     * - Smooth spring physics animation
     * - Deep space aesthetic
     *
     * Ported from archive/alien/ZPlaneVisualizer (viral temple aesthetic)
     */
    class ZPlaneVisualizer : public juce::Component,
                            private juce::Timer,
                            private juce::OpenGLRenderer
    {
    public:
        ZPlaneVisualizer(bool useOpenGL = false);
        ~ZPlaneVisualizer() override;

        // Component overrides
        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseMove(const juce::MouseEvent& event) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

        // OpenGL overrides (stubbed - software rendering only for Phase 1)
        void newOpenGLContextCreated() override;
        void renderOpenGL() override;
        void openGLContextClosing() override;

        // Filter coefficient updates
        struct FilterCoefficients
        {
            std::array<std::complex<float>, 6> a{};  // Denominator (poles)
            std::array<std::complex<float>, 6> b{};  // Numerator (zeros)
            int numPoles = 0;
            int numZeros = 0;
        };

        void updateCoefficients(const FilterCoefficients& coeffs);
        void setMorphPosition(float position);
        void setResonance(float resonance);

        // Visualization options
        void setShowGrid(bool show) { showGrid = show; repaint(); }
        void setShowFrequencyResponse(bool show) { showFreqResponse = show; repaint(); }
        void setInteractive(bool interactive) { isInteractive = interactive; }

        // Colour IDs for hot-reload binding (Phase 2)
        enum ColourIds
        {
            colourIdBackground = 0x6f040001,
            colourIdAxes       = 0x6f040002,
            colourIdGrid       = 0x6f040003,
            colourIdResponse   = 0x6f040004,
            colourIdPole       = 0x6f040005,
            colourIdZero       = 0x6f040006
        };

        // Geometry setters for hot-reload (Phase 3)
        void setPointRadius(float px) { pointRadius = px; repaint(); }
        void setGlowRadius(float px) { glowRadius = px; repaint(); }
        void setUnitCircleScale(float scale) { unitCircleRadius = scale; repaint(); }

        // Callbacks (optional - for interactive mode)
        std::function<void(float freq, float res)> onPoleChanged;
        std::function<void(float freq, float res)> onZeroChanged;

    private:
        void timerCallback() override;
        void renderBackground(juce::Graphics& g);
        void renderGrid(juce::Graphics& g);
        void renderUnitCircle(juce::Graphics& g);
        void renderCoefficients(juce::Graphics& g);
        void renderFrequencyResponse(juce::Graphics& g);
        void renderAlienEffects(juce::Graphics& g);
        void renderConstellationConnections(juce::Graphics& g);

        // Coordinate conversion
        juce::Point<float> complexToScreen(const std::complex<float>& c) const;
        std::complex<float> screenToComplex(juce::Point<float> p) const;
        float getFrequencyAtPoint(juce::Point<float> p) const;

        // Animation helpers
        void animateCoefficients();
        void updateEnergyField();

        // OpenGL context (disabled by default)
        juce::OpenGLContext openGLContext;
        bool useOpenGL = false;

        // Filter state
        FilterCoefficients currentCoeffs;
        FilterCoefficients targetCoeffs;
        float morphPosition = 0.0f;
        float resonanceAmount = 0.5f;

        // Visualization state
        bool showGrid = true;
        bool showFreqResponse = true;
        bool isInteractive = false;  // Disabled by default

        // Animation state
        struct AnimatedPoint
        {
            juce::Point<float> current;
            juce::Point<float> target;
            float velocity = 0.0f;
            float energy = 0.0f;
            float pulsePhase = 0.0f;
        };

        std::vector<AnimatedPoint> polePositions;
        std::vector<AnimatedPoint> zeroPositions;

        // Interaction state
        int selectedPoleIndex = -1;
        int selectedZeroIndex = -1;
        bool isDragging = false;
        juce::Point<float> dragOffset;

        // Energy field visualization
        std::array<std::array<float, 32>, 32> energyField{};
        float fieldPhase = 0.0f;

        // Visual parameters
        float unitCircleRadius = 0.4f;  // Relative to component size
        float pointRadius = 8.0f;
        float glowRadius = 20.0f;

        // Performance
        juce::Image cachedBackground;
        bool needsBackgroundRedraw = true;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZPlaneVisualizer)
    };
}
