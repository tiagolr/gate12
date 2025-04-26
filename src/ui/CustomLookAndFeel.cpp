#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
  // setColour(ComboBox::backgroundColourId, Colour(globals::COLOR_BACKGROUND));
  // setColour(ComboBox::textColourId, Colour(globals::COLOR_ACTIVE));
  // setColour(ComboBox::arrowColourId, Colour(globals::COLOR_ACTIVE));
  // setColour(ComboBox::outlineColourId, Colour(globals::COLOR_ACTIVE));
  // setColour(TooltipWindow::backgroundColourId, Colour(globals::COLOR_ACTIVE_L).darker(0.4f));
  // setColour(PopupMenu::backgroundColourId, Colour(globals::COLOR_ACTIVE_L).darker(0.4f).withAlpha(0.99f));
  // setColour(PopupMenu::highlightedBackgroundColourId, Colour(globals::COLOR_ACTIVE_L).darker(0.8f));

  typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::RobotoBold_ttf, BinaryData::RobotoBold_ttfSize);
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