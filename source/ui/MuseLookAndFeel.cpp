#include "MuseLookAndFeel.h"

MuseLookAndFeel::MuseLookAndFeel()
{
    applyTheme();
}

void MuseLookAndFeel::applyTheme()
{
    // === Slider (Knob) ===
    setColour(juce::Slider::backgroundColourId, MuseTheme::Grey800);
    setColour(juce::Slider::trackColourId, MuseTheme::Amber);
    setColour(juce::Slider::thumbColourId, MuseTheme::Grey200);
    setColour(juce::Slider::textBoxTextColourId, MuseTheme::Cream);
    setColour(juce::Slider::textBoxBackgroundColourId, MuseTheme::Obsidian);
    setColour(juce::Slider::textBoxHighlightColourId, MuseTheme::Amber.withAlpha(0.3f));
    setColour(juce::Slider::textBoxOutlineColourId, MuseTheme::Grey700);

    // === Button ===
    setColour(juce::TextButton::buttonColourId, MuseTheme::Grey800);
    setColour(juce::TextButton::buttonOnColourId, MuseTheme::Amber);
    setColour(juce::TextButton::textColourOffId, MuseTheme::Grey200);
    setColour(juce::TextButton::textColourOnId, MuseTheme::Obsidian);

    // === ComboBox ===
    setColour(juce::ComboBox::backgroundColourId, MuseTheme::Grey800);
    setColour(juce::ComboBox::textColourId, MuseTheme::Cream);
    setColour(juce::ComboBox::outlineColourId, MuseTheme::Grey700);
    setColour(juce::ComboBox::buttonColourId, MuseTheme::Grey700);
    setColour(juce::ComboBox::arrowColourId, MuseTheme::Grey400);
    setColour(juce::ComboBox::focusedOutlineColourId, MuseTheme::Amber);

    // === PopupMenu ===
    setColour(juce::PopupMenu::backgroundColourId, MuseTheme::Grey900);
    setColour(juce::PopupMenu::textColourId, MuseTheme::Cream);
    setColour(juce::PopupMenu::headerTextColourId, MuseTheme::Grey400);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, MuseTheme::Amber.withAlpha(0.2f));
    setColour(juce::PopupMenu::highlightedTextColourId, MuseTheme::Cream);

    // === Label ===
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, MuseTheme::Cream);
    setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);

    // === Tooltip ===
    setColour(juce::TooltipWindow::backgroundColourId, MuseTheme::Grey900);
    setColour(juce::TooltipWindow::textColourId, MuseTheme::Cream);
    setColour(juce::TooltipWindow::outlineColourId, MuseTheme::Amber);

    // === TextEditor ===
    setColour(juce::TextEditor::backgroundColourId, MuseTheme::Obsidian);
    setColour(juce::TextEditor::textColourId, MuseTheme::Cream);
    setColour(juce::TextEditor::highlightColourId, MuseTheme::Amber.withAlpha(0.3f));
    setColour(juce::TextEditor::highlightedTextColourId, MuseTheme::Obsidian);
    setColour(juce::TextEditor::outlineColourId, MuseTheme::Grey700);
    setColour(juce::TextEditor::focusedOutlineColourId, MuseTheme::Amber);

    // === ResizableWindow ===
    setColour(juce::ResizableWindow::backgroundColourId, MuseTheme::Grey950);
}

void MuseLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPosProportional, float rotaryStartAngle,
                                       float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height);
    auto radius = juce::jmin(width, height) * 0.5f;
    auto center = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    bool isHovered = slider.isMouseOver();

    // === Outer Shadow ===
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillEllipse(bounds.reduced(1).translated(1, 2));

    // === 3D Gradient Knob (72×72px) - Light top-left to dark bottom-right ===
    juce::ColourGradient gradient(
        MuseTheme::KNOB_LIGHT, center.x - radius * 0.7f, center.y - radius * 0.7f,
        MuseTheme::KNOB_DARK, center.x + radius * 0.7f, center.y + radius * 0.7f,
        false
    );
    g.setGradientFill(gradient);
    g.fillEllipse(bounds);

    // === White Indicator Line ===
    float pointerLength = radius * 0.7f;
    juce::Point<float> pointerStart(
        center.x + std::cos(angle - juce::MathConstants<float>::halfPi) * (radius * 0.2f),
        center.y + std::sin(angle - juce::MathConstants<float>::halfPi) * (radius * 0.2f)
    );
    juce::Point<float> pointerEnd(
        center.x + std::cos(angle - juce::MathConstants<float>::halfPi) * pointerLength,
        center.y + std::sin(angle - juce::MathConstants<float>::halfPi) * pointerLength
    );

    g.setColour(juce::Colours::white);
    g.drawLine(pointerStart.x, pointerStart.y, pointerEnd.x, pointerEnd.y, 2.0f);

    // === Mint Glow on Hover ===
    if (isHovered)
    {
        g.setColour(MuseTheme::MINT_GLOW.withAlpha(0.3f));
        g.drawEllipse(bounds.reduced(1), 2.0f);
    }

    // === Focus Ring (if has keyboard focus) ===
    if (slider.hasKeyboardFocus(false))
    {
        MuseTheme::Focus::drawFocusRing(g, bounds.toNearestInt());
    }
}

juce::Label* MuseLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox(slider);
    label->setFont(MuseTheme::Typography::mono(11.0f));
    label->setJustificationType(juce::Justification::centred);
    label->setColour(juce::Label::textColourId, MuseTheme::Cream);
    label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return label;
}

void MuseLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float minSliderPos, float maxSliderPos,
                                       juce::Slider::SliderStyle style, juce::Slider& slider)
{
    bool isHorizontal = (style == juce::Slider::LinearHorizontal ||
                        style == juce::Slider::LinearBar);

    auto trackBounds = isHorizontal
        ? juce::Rectangle<float>(x, y + height * 0.4f, width, height * 0.2f)
        : juce::Rectangle<float>(x + width * 0.4f, y, width * 0.2f, height);

    // === Track Background ===
    g.setColour(MuseTheme::Grey800);
    g.fillRoundedRectangle(trackBounds, MuseTheme::Radii::sm);

    // === Active Track ===
    auto activeBounds = isHorizontal
        ? trackBounds.withRight(sliderPos)
        : trackBounds.withTop(sliderPos);

    g.setColour(MuseTheme::Amber);
    g.fillRoundedRectangle(activeBounds, MuseTheme::Radii::sm);

    // === Thumb ===
    float thumbSize = 16.0f;
    juce::Rectangle<float> thumbBounds;
    if (isHorizontal)
        thumbBounds = {sliderPos - thumbSize * 0.5f, y + height * 0.5f - thumbSize * 0.5f,
                       thumbSize, thumbSize};
    else
        thumbBounds = {x + width * 0.5f - thumbSize * 0.5f, sliderPos - thumbSize * 0.5f,
                       thumbSize, thumbSize};

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillEllipse(thumbBounds.translated(0, 1));

    // Body
    juce::ColourGradient thumbGrad(
        MuseTheme::Grey200, thumbBounds.getCentreX() - 4, thumbBounds.getCentreY() - 4,
        MuseTheme::Grey400, thumbBounds.getCentreX() + 4, thumbBounds.getCentreY() + 4,
        false
    );
    g.setGradientFill(thumbGrad);
    g.fillEllipse(thumbBounds);

    // Focus ring
    if (slider.hasKeyboardFocus(false))
    {
        MuseTheme::Focus::drawFocusRing(g, thumbBounds.toNearestInt());
    }
}

void MuseLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto baseColour = button.getToggleState() ? MuseTheme::Amber : MuseTheme::Grey800;

    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker(0.2f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.1f);

    // Shadow
    if (!shouldDrawButtonAsDown)
    {
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.translated(0, 1), MuseTheme::Radii::sm);
    }

    // Body
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, MuseTheme::Radii::sm);

    // Border
    g.setColour(MuseTheme::Grey700);
    g.drawRoundedRectangle(bounds, MuseTheme::Radii::sm, 1.0f);

    // Focus ring
    if (button.hasKeyboardFocus(false))
    {
        MuseTheme::Focus::drawFocusRing(g, bounds.toNearestInt());
    }
}

void MuseLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                     bool shouldDrawButtonAsHighlighted,
                                     bool shouldDrawButtonAsDown)
{
    auto textColour = button.getToggleState() ? MuseTheme::Obsidian : MuseTheme::Cream;

    g.setFont(MuseTheme::Typography::small());
    g.setColour(textColour);
    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred, true);
}

void MuseLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto checkboxSize = 20.0f;
    auto checkboxBounds = bounds.removeFromLeft(checkboxSize).reduced(2);

    // Checkbox background
    auto bgColour = button.getToggleState() ? MuseTheme::Amber : MuseTheme::Grey800;
    if (shouldDrawButtonAsHighlighted)
        bgColour = bgColour.brighter(0.1f);

    g.setColour(bgColour);
    g.fillRoundedRectangle(checkboxBounds, MuseTheme::Radii::sm);

    // Border
    g.setColour(MuseTheme::Grey700);
    g.drawRoundedRectangle(checkboxBounds, MuseTheme::Radii::sm, 1.0f);

    // Checkmark
    if (button.getToggleState())
    {
        juce::Path tick;
        tick.startNewSubPath(checkboxBounds.getX() + 4, checkboxBounds.getCentreY());
        tick.lineTo(checkboxBounds.getCentreX(), checkboxBounds.getBottom() - 4);
        tick.lineTo(checkboxBounds.getRight() - 4, checkboxBounds.getY() + 4);

        g.setColour(MuseTheme::Obsidian);
        g.strokePath(tick, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Text label
    g.setFont(MuseTheme::Typography::body());
    g.setColour(MuseTheme::Cream);
    g.drawText(button.getButtonText(), bounds.reduced(4, 0),
               juce::Justification::centredLeft, true);

    // Focus ring
    if (button.hasKeyboardFocus(false))
    {
        MuseTheme::Focus::drawFocusRing(g, button.getLocalBounds());
    }
}

void MuseLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                   bool isButtonDown, int buttonX, int buttonY,
                                   int buttonW, int buttonH, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);

    // Background
    auto bgColour = isButtonDown ? MuseTheme::Grey700 : MuseTheme::Grey800;
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, MuseTheme::Radii::sm);

    // Border
    g.setColour(MuseTheme::Grey700);
    g.drawRoundedRectangle(bounds, MuseTheme::Radii::sm, 1.0f);

    // Dropdown arrow
    juce::Path arrow;
    auto arrowBounds = juce::Rectangle<float>(buttonX, buttonY, buttonW, buttonH).reduced(6);
    arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                     arrowBounds.getRight(), arrowBounds.getY(),
                     arrowBounds.getCentreX(), arrowBounds.getBottom());

    g.setColour(MuseTheme::Grey400);
    g.fillPath(arrow);

    // Focus ring
    if (box.hasKeyboardFocus(false))
    {
        MuseTheme::Focus::drawFocusRing(g, bounds.toNearestInt());
    }
}

void MuseLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(8, 0, box.getWidth() - 32, box.getHeight());
    label.setFont(MuseTheme::Typography::body());
    label.setJustificationType(juce::Justification::centredLeft);
}

void MuseLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);

    // Background
    g.setColour(MuseTheme::Grey900);
    g.fillRoundedRectangle(bounds, MuseTheme::Radii::md);

    // Border + shadow
    MuseTheme::Shadows::medium().drawForRectangle(g, bounds.toNearestInt());
    g.setColour(MuseTheme::Amber.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, MuseTheme::Radii::md, 1.0f);
}

void MuseLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                        bool isSeparator, bool isActive, bool isHighlighted,
                                        bool isTicked, bool hasSubMenu, const juce::String& text,
                                        const juce::String& shortcutKeyText,
                                        const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour(MuseTheme::Grey700);
        g.fillRect(area.withHeight(1).withY(area.getCentreY()));
        return;
    }

    // Highlight background
    if (isHighlighted && isActive)
    {
        g.setColour(MuseTheme::Amber.withAlpha(0.15f));
        g.fillRoundedRectangle(area.reduced(4, 2).toFloat(), MuseTheme::Radii::sm);
    }

    // Text
    auto textBounds = area.reduced(16, 0);
    g.setFont(MuseTheme::Typography::body());
    g.setColour(isActive ? MuseTheme::Cream : MuseTheme::Grey600);
    g.drawText(text, textBounds, juce::Justification::centredLeft, true);

    // Tick mark
    if (isTicked)
    {
        juce::Path tick;
        auto tickArea = area;
        auto tickBounds = tickArea.removeFromLeft(16).toFloat().reduced(4);
        tick.addTriangle(tickBounds.getX(), tickBounds.getCentreY(),
                        tickBounds.getCentreX(), tickBounds.getBottom() - 2,
                        tickBounds.getRight(), tickBounds.getY() + 2);
        g.setColour(MuseTheme::Amber);
        g.fillPath(tick);
    }

    // Submenu arrow
    if (hasSubMenu)
    {
        juce::Path arrow;
        auto arrowArea = area;
        auto arrowBounds = arrowArea.removeFromRight(16).toFloat().reduced(6);
        arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                         arrowBounds.getX(), arrowBounds.getBottom(),
                         arrowBounds.getRight(), arrowBounds.getCentreY());
        g.setColour(MuseTheme::Grey400);
        g.fillPath(arrow);
    }
}

void MuseLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text,
                                   int width, int height)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);

    // Background + shadow
    MuseTheme::Shadows::medium().drawForRectangle(g, bounds.toNearestInt());
    g.setColour(MuseTheme::Grey900);
    g.fillRoundedRectangle(bounds, MuseTheme::Radii::sm);

    // Border
    g.setColour(MuseTheme::Amber.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, MuseTheme::Radii::sm, 1.0f);

    // Text
    g.setFont(MuseTheme::Typography::small());
    g.setColour(MuseTheme::Cream);
    g.drawText(text, bounds.reduced(8, 4), juce::Justification::centredLeft, true);
}

void MuseLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox& box, juce::Graphics& g,
                                               const juce::Path& path, juce::Image& cachedImage)
{
    // Background + shadow
    MuseTheme::Shadows::strong().drawForPath(g, path);
    g.setColour(MuseTheme::Grey900);
    g.fillPath(path);

    // Border
    g.setColour(MuseTheme::Amber.withAlpha(0.5f));
    g.strokePath(path, juce::PathStrokeType(1.5f));
}

// Focus ring now handled per-component (JUCE 8 removed global drawFocusRect)
