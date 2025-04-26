#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;
    int getPopupMenuBorderSize() override;

    /*
    * Custom button look for pattern selector buttons
    */
    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override
    {
        auto tag = button.getComponentID();
        if (tag != "leftCorner" && tag != "rightCorner" && tag != "middle") {
            LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, isMouseOverButton, isButtonDown);
            return;
        }

        auto bounds = button.getLocalBounds().toFloat();

        bool roundLeft = tag == "leftCorner";
        bool roundRight = tag == "rightCorner";

        juce::Path path;

        if (roundLeft && roundRight) {
            // If somehow both, just a full rounded rect
            path.addRoundedRectangle(bounds, 0.0f);
        }
        else {
            auto cornerSize = 4.0f;
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

private:
    juce::Typeface::Ptr typeface;
};