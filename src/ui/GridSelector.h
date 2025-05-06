#pragma once

#include <JuceHeader.h>
#include <juce_gui_basics/juce_gui_basics.h>

class GATE12AudioProcessor;

class GridSelector : public juce::SettableTooltipClient, public juce::Component, private juce::AudioProcessorValueTreeState::Listener {
public:
    GridSelector(GATE12AudioProcessor&);
    ~GridSelector() override;
    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

protected:
    GATE12AudioProcessor& audioProcessor;
};