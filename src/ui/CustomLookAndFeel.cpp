#include "CustomLookAndFeel.h"

static void drawSliderBevel(Graphics& g, Rectangle<float> bounds, float corner, Colour bg)
{
    bounds = bounds.translated(0.5f, 0.5f);
    juce::ColourGradient gradient(
        Colour(0xff0F0F0F).withAlpha(0.35f), bounds.getX(), bounds.getY(),
        Colours::white.withAlpha(0.12f), bounds.getX(), bounds.getBottom(), false
    );
    gradient.addColour(0.5, Colour(0xff0F0F0F).withAlpha(0.02f));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, corner);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds.expanded(-1.f), corner);
}

CustomLookAndFeel::CustomLookAndFeel()
{
  setColour(ComboBox::backgroundColourId, Colour(COLOR_BG));
  setColour(ComboBox::textColourId, Colour(COLOR_ACTIVE));
  setColour(ComboBox::arrowColourId, Colour(COLOR_ACTIVE));
  setColour(ComboBox::outlineColourId, Colour(COLOR_ACTIVE));
  setColour(TooltipWindow::backgroundColourId, Colour(COLOR_BG).brighter(0.15f));
  setColour(PopupMenu::backgroundColourId, Colour(COLOR_ACTIVE).darker(0.5f).withAlpha(0.99f));
  setColour(PopupMenu::highlightedBackgroundColourId, Colour(COLOR_ACTIVE).darker(0.8f));
  setColour(TextButton::buttonColourId, Colour(COLOR_ACTIVE));
  setColour(TextButton::buttonOnColourId, Colour(COLOR_ACTIVE));
  setColour(TextButton::textColourOnId, Colour(COLOR_BG));
  setColour(TextButton::textColourOffId, Colour(COLOR_ACTIVE));

  typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::UbuntuMedium_ttf, BinaryData::UbuntuMedium_ttfSize);
  setDefaultSansSerifTypeface(typeface);
  this->setDefaultLookAndFeel(this);
}

// Override the getTypefaceForFont function
juce::Typeface::Ptr CustomLookAndFeel::getTypefaceForFont(const juce::Font& /*font*/)
{
    return typeface;
}

int CustomLookAndFeel::getPopupMenuBorderSize()
{
    return 5;
}

void CustomLookAndFeel::drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown)
{
    auto tag = button.getComponentID();
    auto bounds = button.getLocalBounds().toFloat();
    auto cornerSize = 3.0f;

    if (tag == "button") {
        g.setColour(backgroundColour);
        if (button.getToggleState())
            g.fillRoundedRectangle(bounds, cornerSize);
        else
            g.drawRoundedRectangle(bounds.reduced(0.5, 0.5), cornerSize, 1.0f);

        if (isMouseOverButton) {
            g.setColour(Colours::black.withAlpha(0.2f));
            g.fillRoundedRectangle(bounds.expanded(1,1), cornerSize);
        }
        if (isButtonDown) {
            g.setColour(Colours::black.withAlpha(0.4f));
            g.fillRoundedRectangle(bounds.expanded(1,1), cornerSize);
        }

        return;
    }

    if (tag != "leftPattern" && tag != "rightPattern" && tag != "pattern") {
        LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, isMouseOverButton, isButtonDown);
        return;
    }

    bool roundLeft = tag == "leftPattern";
    bool roundRight = tag == "rightPattern";

    juce::Path path;

    if (roundLeft && roundRight) {
        path.addRoundedRectangle(bounds, 0.0f);
    }
    else {
        float topLeft = roundLeft ? cornerSize : 0.0f;
        float topRight = roundRight ? cornerSize : 0.0f;
        float bottomLeft = roundLeft ? cornerSize : 0.0f;
        float bottomRight = roundRight ? cornerSize : 0.0f;

        path.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerSize, cornerSize, topLeft, topRight, bottomLeft, bottomRight);
    }

    g.setColour(backgroundColour);
    g.fillPath(path);
}

void CustomLookAndFeel::drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box)
{
    (void)buttonX;
    (void)buttonY;
    (void)buttonH;
    (void)buttonW;
    (void)isButtonDown;

    auto cornerSize = 3.0f;
    Rectangle<int> boxBounds (0, 0, width, height);

    g.setColour (box.findColour (ComboBox::backgroundColourId));
    g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);

    g.setColour (box.findColour (ComboBox::outlineColourId));
    g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

    auto arrowZone = Rectangle<int>(width - 20, 0, 20, height).translated(-3,0).toFloat();
    Path path;
    auto r = 4.0f;
    path.startNewSubPath({arrowZone.getCentreX() - r, arrowZone.getCentreY() - r/2});
    path.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + r/2);
    path.lineTo(arrowZone.getCentreX() + r, arrowZone.getCentreY() - r/2);

    g.setColour (box.findColour (ComboBox::arrowColourId).withAlpha ((box.isEnabled() ? 0.9f : 0.2f)));
    g.strokePath (path, PathStrokeType(2.f));
}

void CustomLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (1, 1,
        box.getWidth() - 20,
        box.getHeight() - 2);

    label.setFont (getComboBoxFont (box));
}

void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos,
    const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    auto tag = slider.getComponentID();
    if (tag != "stereo_slider") {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    if (tag == "stereo_slider") {
        auto bounds = Rectangle<int>(x, y, width, height).toFloat();
        drawSliderBevel(g, bounds.expanded(0.5f), 4.f, Colour(COLOR_BEVEL));
        bounds = bounds.reduced(4.f);
        bounds = bounds.withHeight(bounds.getHeight() + 1);

        g.setColour(Colour(COLOR_ACTIVE).darker(0.5f));
        Path p;
        p.addRoundedRectangle(bounds, 4.f);
        g.saveState();
        g.reduceClipRegion(p);

        const float valuePos = juce::jmap((float)slider.getValue(),
            (float)slider.getMinimum(),
            (float)slider.getMaximum(),
            bounds.getX(),
            bounds.getRight());

        const float zeroPos = juce::jmap(0.0f,
            (float)slider.getMinimum(),
            (float)slider.getMaximum(),
            bounds.getX(),
            bounds.getRight());

        const float snappedValuePos = std::round(valuePos);
        const float snappedZeroPos = std::round(zeroPos);

        if (snappedValuePos >= snappedZeroPos) {
            g.fillRect(juce::Rectangle<float>(
                snappedZeroPos,
                bounds.getY(),
                snappedValuePos - snappedZeroPos,
                bounds.getHeight()));
        }
        else {
            g.fillRect(juce::Rectangle<float>(
                snappedValuePos,
                bounds.getY(),
                snappedZeroPos - snappedValuePos,
                bounds.getHeight()));
        }

        String text = slider.isMouseOverOrDragging()
            ? String(slider.getValue())
            : "Stereo";

        g.setFont(FontOptions(16.f));
        g.setColour(Colours::white);
        g.drawText(text, bounds, Justification::centred);

        // Center line
        //g.drawLine(snappedZeroPos,
        //    bounds.getY(),
        //    snappedZeroPos,
        //    bounds.getBottom(),
        //    2.0f);

        g.restoreState();
        return;
    }
};