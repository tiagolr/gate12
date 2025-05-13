#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;
    int getPopupMenuBorderSize() override;
    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override;
    void drawComboBox (Graphics&, int width, int height, bool isButtonDown,int buttonX, int buttonY, int buttonW, int buttonH, ComboBox&) override;
    void positionComboBoxText (ComboBox& box, Label& label) override;

private:
    juce::Typeface::Ptr typeface;
};