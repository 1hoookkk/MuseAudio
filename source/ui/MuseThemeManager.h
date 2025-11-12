#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MuseLookAndFeel.h"

/**
 * MuseThemeManager - Global theme switching (light/dark/custom).
 *
 * Usage:
 *   MuseThemeManager::getInstance().setTheme(MuseTheme::Theme::Dark);
 *   MuseThemeManager::getInstance().applyToComponent(editor);
 *
 * Note: Theme switching UI not implemented yet (Phase 2).
 * Currently defaults to Dark theme.
 */
namespace MuseTheme {
    enum class Theme { Dark, Light };

    struct Palette {
        juce::Colour background;
        juce::Colour surface;
        juce::Colour border;
        juce::Colour textPrimary;
        juce::Colour textSecondary;
        juce::Colour accent;
        juce::Colour accentHover;

        static Palette dark() {
            return {
                .background = Grey950,
                .surface = Grey800,
                .border = Grey700,
                .textPrimary = Cream,
                .textSecondary = Grey400,
                .accent = Amber,
                .accentHover = Amber.brighter(0.2f)
            };
        }

        static Palette light() {
            return {
                .background = Grey50,
                .surface = juce::Colours::white,
                .border = Grey200,
                .textPrimary = Grey900,
                .textSecondary = Grey600,
                .accent = Amber.darker(0.1f),
                .accentHover = Amber
            };
        }
    };
}

class MuseThemeManager
{
public:
    static MuseThemeManager& getInstance()
    {
        static MuseThemeManager instance;
        return instance;
    }

    void setTheme(MuseTheme::Theme theme)
    {
        currentTheme_ = theme;
        currentPalette_ = (theme == MuseTheme::Theme::Dark)
            ? MuseTheme::Palette::dark()
            : MuseTheme::Palette::light();

        // Broadcast theme change to all registered components
        broadcastThemeChange();
    }

    MuseTheme::Theme getCurrentTheme() const { return currentTheme_; }
    MuseTheme::Palette getCurrentPalette() const { return currentPalette_; }

    void registerComponent(juce::Component* comp)
    {
        registeredComponents_.add(comp);
    }

    void unregisterComponent(juce::Component* comp)
    {
        registeredComponents_.removeAllInstancesOf(comp);
    }

    // Persistence (save/load from ValueTree)
    void saveThemeToState(juce::ValueTree& state)
    {
        state.setProperty("theme", currentTheme_ == MuseTheme::Theme::Dark ? "dark" : "light",
                         nullptr);
    }

    void loadThemeFromState(const juce::ValueTree& state)
    {
        juce::String themeName = state.getProperty("theme", "dark");
        setTheme(themeName == "light" ? MuseTheme::Theme::Light : MuseTheme::Theme::Dark);
    }

private:
    MuseThemeManager() : currentTheme_(MuseTheme::Theme::Dark),
                         currentPalette_(MuseTheme::Palette::dark()) {}

    void broadcastThemeChange()
    {
        for (auto* comp : registeredComponents_)
        {
            if (comp != nullptr)
                comp->repaint();
        }
    }

    MuseTheme::Theme currentTheme_;
    MuseTheme::Palette currentPalette_;
    juce::Array<juce::Component*> registeredComponents_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseThemeManager)
};
