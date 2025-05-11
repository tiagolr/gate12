#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class GATE12AudioProcessor;

class SequencerWidget : public juce::Component {
public:
    SequencerWidget(GATE12AudioProcessor& p);
    ~SequencerWidget() override {}

    TextButton minButton;
    TextButton maxButton;
    TextButton tenButton;
    TextButton tenaButton;
    TextButton tenrButton;
    TextButton flipXButton;
    TextButton flipYButton;
    TextButton gateButton;

    void updateButtonsState();
    void paint(Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    GATE12AudioProcessor& audioProcessor;
};