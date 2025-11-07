#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace terminal
{
    /**
     * @brief ASCII terminal aesthetic with off-white mockup colors
     * No technical jargon - just visual controls
     * Compact 400x600
     */
    class TerminalFieldView : public juce::Component,
                              private juce::Timer
    {
    public:
        TerminalFieldView(juce::AudioProcessor& proc, juce::AudioProcessorValueTreeState& state)
            : apvts(state)
        {
            startTimerHz(60);

            characterParam = apvts.getRawParameterValue("character");
            resonanceParam = apvts.getRawParameterValue("resonance");
            outputParam = apvts.getRawParameterValue("output");
            bypassParam = apvts.getRawParameterValue("bypass");

            // Pre-build static separator line (never changes)
            cachedSeparator = repeatChar('─', 42);

            // Pre-allocate mutable strings to avoid reallocations in paint()
            cachedImpactBar.preallocateBytes(90);      // 30 chars × 3 bytes per UTF-8 char
            cachedOutputBar.preallocateBytes(75);      // 25 chars × 3 bytes per UTF-8 char
            cachedCharValue.preallocateBytes(10);
            cachedImpactValue.preallocateBytes(10);
            cachedOutputValue.preallocateBytes(15);

            setSize(400, 600);
        }

        ~TerminalFieldView() override
        {
            stopTimer();
        }

        void setImpactValue(float deltaTilt, float deltaRms, float inputRms)
        {
            impactDeltaTilt = deltaTilt;
            impactDeltaRms = deltaRms;
            impactInputRms = inputRms;
        }

        void paint(juce::Graphics& g) override
        {
            // Colors: off-white beige + black + orange accent
            const juce::Colour bgColor(0xFFE5E0D8);
            const juce::Colour textColor(0xFF000000);
            const juce::Colour accentColor(0xFFC75D3C);

            // Background
            g.fillAll(bgColor);

            // Monospace font for ASCII aesthetic
            juce::Font monoFont(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain);
            g.setFont(monoFont);

            // Get parameters
            const float character = characterParam ? characterParam->load() : 0.5f;
            const float output = outputParam ? outputParam->load() : 0.0f;
            const bool bypassed = bypassParam ? (bypassParam->load() > 0.5f) : false;

            const int padding = 25;
            int yPos = padding;
            const int lineHeight = 18;

            // Outer border
            g.setColour(textColor);
            g.drawRect(getLocalBounds().reduced(8), 2);

            // Header: "engine: Field" + [bypass]
            juce::Font headerFont(juce::Font::getDefaultMonospacedFontName(), 20.0f, juce::Font::bold);
            g.setFont(headerFont);
            g.drawText("engine: Field", padding, yPos, getWidth() - padding * 2 - 90, 30, juce::Justification::left);

            // Bypass button - store rect for hit testing and show state
            g.setFont(monoFont);
            bypassRect.setBounds(getWidth() - padding - 80, yPos, 70, 25);

            if (bypassed)
            {
                g.setColour(accentColor);
                g.fillRect(bypassRect);
                g.setColour(bgColor);
                g.drawText("[bypass]", bypassRect, juce::Justification::centred);
            }
            else
            {
                g.setColour(textColor);
                g.drawRect(bypassRect, 2);
                g.drawText("[bypass]", bypassRect, juce::Justification::centred);
            }

            yPos += 45;

            // Separator line (ASCII) - use cached
            g.setFont(monoFont);
            g.drawText(cachedSeparator, 0, yPos, getWidth(), lineHeight, juce::Justification::centred);
            yPos += lineHeight + 8;

            // Impact meter (ASCII segments)
            g.drawText("impact", padding, yPos, 80, lineHeight, juce::Justification::left);
            yPos += lineHeight + 2;

            const float impactNorm = juce::jlimit(0.0f, 1.0f, (impactDeltaRms + 12.0f) / 24.0f);

            // Rebuild impact bar into cached string
            buildSegmentedBar(cachedImpactBar, impactNorm, 30);

            g.setColour(accentColor);
            g.drawText(cachedImpactBar, padding, yPos, getWidth() - padding * 2, lineHeight, juce::Justification::left);

            g.setColour(textColor);
            yPos += lineHeight + 2;

            // Build impact value into cached string
            cachedImpactValue.clear();
            cachedImpactValue << "[" << static_cast<int>(impactNorm * 100) << "%]";
            g.drawText(cachedImpactValue, padding, yPos, 80, lineHeight, juce::Justification::left);
            yPos += lineHeight + 15;

            // Separator - use cached
            g.drawText(cachedSeparator, 0, yPos, getWidth(), lineHeight, juce::Justification::centred);
            yPos += lineHeight + 20;

            // CHARACTER - hero control (ASCII circle)
            g.drawText("character", 0, yPos, getWidth(), lineHeight, juce::Justification::centred);
            yPos += lineHeight + 10;

            // Draw ASCII circle/dial
            drawASCIIDial(g, getWidth() / 2, yPos + 60, 55, character, textColor, accentColor);
            yPos += 130;

            // Character percentage - use cached
            cachedCharValue.clear();
            cachedCharValue << "[" << static_cast<int>(character * 100) << "%]";
            g.drawText(cachedCharValue, 0, yPos, getWidth(), lineHeight, juce::Justification::centred);
            yPos += lineHeight + 5;

            // Shape name (no technical jargon, just vibes)
            juce::String shapeName = getShapeVibe(character);
            juce::Font smallFont(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain);
            g.setFont(smallFont);
            g.setColour(juce::Colour(0xFF666666));
            g.drawText(shapeName, 0, yPos, getWidth(), lineHeight, juce::Justification::centred);
            yPos += lineHeight + 20;

            // Separator - use cached
            g.setColour(textColor);
            g.setFont(monoFont);
            g.drawText(cachedSeparator, 0, yPos, getWidth(), lineHeight, juce::Justification::centred);
            yPos += lineHeight + 12;

            // OUTPUT slider (ASCII style)
            g.setColour(textColor);
            g.drawText("output", padding, yPos, 80, lineHeight, juce::Justification::left);
            yPos += lineHeight + 4;

            const float outputNorm = (output + 12.0f) / 24.0f;

            // Store slider rect for hit testing
            outputSliderRect.setBounds(padding, yPos - 5, getWidth() - padding - 90, 25);

            // Build output bar into cached string
            buildSliderBar(cachedOutputBar, outputNorm, 25);
            g.drawText(cachedOutputBar, padding, yPos, getWidth() - padding - 90, lineHeight, juce::Justification::left);

            // Output value box - use cached string
            cachedOutputValue.clear();
            cachedOutputValue << (output >= 0.0f ? "+" : "") << juce::String(output, 1) << "db";
            juce::Rectangle<int> outRect(getWidth() - padding - 70, yPos - 2, 65, 20);
            g.drawRect(outRect, 1);
            g.drawText(cachedOutputValue, outRect, juce::Justification::centred);
        }

        void resized() override {}

        void mouseDown(const juce::MouseEvent& e) override
        {
            // Check bypass button click
            if (bypassRect.contains(e.position.toInt()))
            {
                if (auto* param = apvts.getParameter("bypass"))
                {
                    float currentValue = param->getValue();
                    param->setValueNotifyingHost(currentValue > 0.5f ? 0.0f : 1.0f);
                }
                return;
            }

            // Check output slider click
            if (outputSliderRect.contains(e.position.toInt()))
            {
                isDraggingSlider = true;
                updateSliderFromMouse(e);
                return;
            }

            // Check if clicking on dial area
            const int dialCenterX = getWidth() / 2;
            const int dialCenterY = 280;  // Approximate Y position
            const int dialRadius = 60;

            const float dx = e.position.x - dialCenterX;
            const float dy = e.position.y - dialCenterY;
            const float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < dialRadius)
            {
                isDraggingDial = true;
                updateDialFromMouse(e);
            }
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (isDraggingDial)
                updateDialFromMouse(e);
            else if (isDraggingSlider)
                updateSliderFromMouse(e);
        }

        void mouseUp(const juce::MouseEvent&) override
        {
            isDraggingDial = false;
            isDraggingSlider = false;
        }

        void mouseDoubleClick(const juce::MouseEvent& e) override
        {
            const int dialCenterX = getWidth() / 2;
            const int dialCenterY = 280;
            const int dialRadius = 60;

            const float dx = e.position.x - dialCenterX;
            const float dy = e.position.y - dialCenterY;
            const float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < dialRadius)
            {
                if (auto* param = apvts.getParameter("character"))
                    param->setValueNotifyingHost(0.5f);
            }
        }

    private:
        void updateDialFromMouse(const juce::MouseEvent& e)
        {
            if (!characterParam) return;

            const int dialCenterX = getWidth() / 2;
            const int dialCenterY = 280;

            const float dx = e.position.x - dialCenterX;
            const float dy = e.position.y - dialCenterY;
            float angle = std::atan2(dy, dx) + juce::MathConstants<float>::halfPi;

            if (angle < 0.0f)
                angle += juce::MathConstants<float>::twoPi;

            float newValue = angle / juce::MathConstants<float>::twoPi;

            if (auto* param = apvts.getParameter("character"))
                param->setValueNotifyingHost(newValue);
        }

        void updateSliderFromMouse(const juce::MouseEvent& e)
        {
            if (!outputParam) return;

            // Convert mouse X position to normalized value (0.0 to 1.0)
            float normalizedValue = (e.position.x - outputSliderRect.getX()) / static_cast<float>(outputSliderRect.getWidth());
            normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);

            // Convert to output range: -12dB to +12dB
            float outputValue = normalizedValue * 24.0f - 12.0f;

            if (auto* param = apvts.getParameter("output"))
                param->setValueNotifyingHost((outputValue + 12.0f) / 24.0f);
        }

        void drawASCIIDial(juce::Graphics& g, int centerX, int centerY, int radius,
                          float value, juce::Colour mainColor, juce::Colour accentColor)
        {
            // Draw circle outline using dots
            const int numDots = 24;
            g.setColour(mainColor);

            for (int i = 0; i < numDots; ++i)
            {
                const float angle = (static_cast<float>(i) / static_cast<float>(numDots)) * juce::MathConstants<float>::twoPi;
                const int x = centerX + static_cast<int>(std::cos(angle) * static_cast<float>(radius));
                const int y = centerY + static_cast<int>(std::sin(angle) * static_cast<float>(radius));

                g.fillEllipse(static_cast<float>(x - 2), static_cast<float>(y - 2), 4.0f, 4.0f);
            }

            // Draw indicator line
            const float indicatorAngle = value * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi;
            const int indX = centerX + static_cast<int>(std::cos(indicatorAngle) * static_cast<float>(radius - 10));
            const int indY = centerY + static_cast<int>(std::sin(indicatorAngle) * static_cast<float>(radius - 10));

            g.setColour(accentColor);
            g.drawLine(static_cast<float>(centerX), static_cast<float>(centerY),
                      static_cast<float>(indX), static_cast<float>(indY), 3.0f);
        }

        void buildSegmentedBar(juce::String& target, float value, int length)
        {
            target.clear();
            const int filled = static_cast<int>(value * static_cast<float>(length));

            for (int i = 0; i < length; ++i)
            {
                if (i < filled)
                    target += juce::String(juce::CharPointer_UTF8("\xe2\x96\x88"));  // █
                else
                    target += juce::String(juce::CharPointer_UTF8("\xe2\x96\x91"));  // ░
            }
        }

        void buildSliderBar(juce::String& target, float value, int length)
        {
            target.clear();
            const int thumbPos = static_cast<int>(value * static_cast<float>(length));

            for (int i = 0; i < length; ++i)
            {
                if (i == thumbPos)
                    target += juce::String(juce::CharPointer_UTF8("\xe2\x96\xa0"));  // ■
                else if (i < length - 1)
                    target += juce::String(juce::CharPointer_UTF8("\xe2\x94\x80"));  // ─
                else
                    target += " ";
            }
        }

        juce::String repeatChar(juce::juce_wchar ch, int count)
        {
            juce::String result;
            result.preallocateBytes(count);
            for (int i = 0; i < count; ++i)
                result += ch;
            return result;
        }

        juce::String getShapeVibe(float value) const
        {
            // No technical jargon - just vibes
            if (value < 0.125f) return "warm vocal";
            if (value < 0.25f) return "bright metallic";
            if (value < 0.375f) return "smooth low";
            if (value < 0.5f) return "rich vocal";
            if (value < 0.625f) return "cutting edge";
            if (value < 0.75f) return "deep low";
            if (value < 0.875f) return "sub rumble";
            return "ultra low";
        }

        void timerCallback() override
        {
            repaint();
        }

        juce::AudioProcessorValueTreeState& apvts;

        std::atomic<float>* characterParam = nullptr;
        std::atomic<float>* resonanceParam = nullptr;
        std::atomic<float>* outputParam = nullptr;
        std::atomic<float>* bypassParam = nullptr;

        float impactDeltaTilt = 0.0f;
        float impactDeltaRms = 0.0f;
        float impactInputRms = -100.0f;

        bool isDraggingDial = false;
        bool isDraggingSlider = false;

        // UI element rectangles for hit testing
        juce::Rectangle<int> bypassRect;
        juce::Rectangle<int> outputSliderRect;

        // Cached strings to avoid allocations in paint() - Phase 1 optimization
        juce::String cachedSeparator;
        juce::String cachedImpactBar;
        juce::String cachedOutputBar;
        juce::String cachedCharValue;
        juce::String cachedImpactValue;
        juce::String cachedOutputValue;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TerminalFieldView)
    };

} // namespace terminal
