#pragma once

#include <JuceHeader.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Globals.h"

using namespace globals;
class GATE12AudioProcessor;

enum RotaryLabel {
    hz,
    hzLp,
    hzHp,
    gainTodB1f,
    hz1f,
    percx100,
    intx100,
    float1,
    float2,
    float2x100,
    audioOffset,
};

class Rotary : public juce::SettableTooltipClient, public juce::Component, private juce::AudioProcessorValueTreeState::Listener {
public:
    Rotary(GATE12AudioProcessor& p, juce::String paramId, juce::String name, RotaryLabel format, bool isSymmetric = false, bool isAudioKnob = false);
    ~Rotary() override;
    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

protected:
    juce::String paramId;
    juce::String name;
    RotaryLabel format;
    GATE12AudioProcessor& audioProcessor;

private:
    bool isSymmetric;
    bool isAudioKnob;
    float deg130 = 130.0f * juce::MathConstants<float>::pi / 180.0f;
    void draw_rotary_slider(juce::Graphics& g, float slider_pos);
    void draw_label_value(juce::Graphics& g, float slider_val);

    float pixels_per_percent{100.0f};
    float cur_normed_value{0.0f};
    juce::Point<int> last_mouse_position;
    juce::Point<int> start_mouse_pos;
    bool mouse_down = false;
};