#pragma once

#include <JuceHeader.h>
#include <functional>

class GATE12AudioProcessor;

class SettingsButton : public juce::Component {
public:
    SettingsButton(GATE12AudioProcessor& p) : audioProcessor(p) {}
    ~SettingsButton() override {}

    void mouseDown(const juce::MouseEvent& e) override;
    void paint(Graphics& g) override;

    std::function<void()> onScaleChange;
    std::function<void()> toggleUIComponents;
    std::function<void()> toggleAbout;

private:
    GATE12AudioProcessor& audioProcessor;
};