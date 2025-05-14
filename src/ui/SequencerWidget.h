#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class GATE12AudioProcessor;

class SequencerWidget : public juce::Component {
public:
    SequencerWidget(GATE12AudioProcessor& p);
    ~SequencerWidget() override {}
    void resized() override;

    TextButton minBtn;
    TextButton maxBtn;
    TextButton tenBtn;
    TextButton tenaBtn;
    TextButton tenrBtn;
    TextButton flipXBtn;

    TextButton silenceBtn;
    TextButton rampupBtn;
    TextButton rampdnBtn;
    TextButton lineBtn;
    TextButton triBtn;
    TextButton linkBtn;
    TextButton ptoolBtn;

    TextButton randomBtn;
    TextButton randomMenuBtn;
    Slider randomRange;
    TextButton clearBtn;

    TextButton applyBtn;
    TextButton resetBtn;

    double randomMin = 0.0;
    double randomMax = 1.0;

    void updateButtonsState();
    void paint(Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    GATE12AudioProcessor& audioProcessor;
};