#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
  setColour(ComboBox::backgroundColourId, Colour(globals::COLOR_BG));
  setColour(ComboBox::textColourId, Colour(globals::COLOR_ACTIVE));
  setColour(ComboBox::arrowColourId, Colour(globals::COLOR_ACTIVE));
  setColour(ComboBox::outlineColourId, Colour(globals::COLOR_ACTIVE));
  setColour(TooltipWindow::backgroundColourId, Colour(globals::COLOR_ACTIVE).darker(0.5f));
  setColour(PopupMenu::backgroundColourId, Colour(globals::COLOR_ACTIVE).darker(0.5f).withAlpha(0.99f));
  setColour(PopupMenu::highlightedBackgroundColourId, Colour(globals::COLOR_ACTIVE).darker(0.8f));
  setColour(TextButton::buttonColourId, Colour(globals::COLOR_ACTIVE));
  setColour(TextButton::buttonOnColourId, Colour(globals::COLOR_ACTIVE));
  setColour(TextButton::textColourOnId, Colour(globals::COLOR_BG));
  setColour(TextButton::textColourOffId, Colour(globals::COLOR_ACTIVE));

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
        if (button.getToggleState()) {
            g.fillRoundedRectangle(bounds, cornerSize);
        }
        else {
            g.drawRoundedRectangle(bounds.reduced(0.5, 0.5), cornerSize, 1.0f);
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
        // If somehow both, just a full rounded rect
        path.addRoundedRectangle(bounds, 0.0f);
    }
    else {
        // Custom corner rounding
        float topLeft = roundLeft ? cornerSize : 0.0f;
        float topRight = roundRight ? cornerSize : 0.0f;
        float bottomLeft = roundLeft ? cornerSize : 0.0f;
        float bottomRight = roundRight ? cornerSize : 0.0f;

        path.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerSize, cornerSize, topLeft, topRight, bottomLeft, bottomRight);
    }

    g.setColour(backgroundColour);
    g.fillPath(path);
}