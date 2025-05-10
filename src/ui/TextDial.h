#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class GATE12AudioProcessor;

enum TextDialLabel {
    tdPercx100,
};

class TextDial : public juce::SettableTooltipClient, public juce::Component, private juce::AudioProcessorValueTreeState::Listener {
public:
    TextDial(GATE12AudioProcessor& p, String paramId, String prefix, String suffix, TextDialLabel format, float fontSize, unsigned int color);
    ~TextDial() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;


private:
    String paramId;
    String prefix;
    String suffix;
    TextDialLabel format;
    float fontSize = 16.0f;
    unsigned int fontColor;
    GATE12AudioProcessor& audioProcessor;

    float pixels_per_percent{100.0f};
    float cur_normed_value{0.0f};
    juce::Point<int> last_mouse_position;
    bool mouse_down = false;
};