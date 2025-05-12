#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class GATE12AudioProcessor;

class SequencerWidget : public juce::Component {
public:
    SequencerWidget(GATE12AudioProcessor& p);
    ~SequencerWidget() override {}

    TextButton minBtn;
    TextButton maxBtn;
    TextButton tenBtn;
    TextButton tenaBtn;
    TextButton tenrBtn;
    TextButton flipXBtn;
    TextButton flipYBtn;
    TextButton gateBtn;

    TextButton silenceBtn;
    TextButton rampupBtn;
    TextButton rampdnBtn;
    TextButton lineBtn;
    TextButton triBtn;
    TextButton ptoolBtn;

    void updateButtonsState();
    void paint(Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    GATE12AudioProcessor& audioProcessor;
};