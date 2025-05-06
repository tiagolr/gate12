/*
  ==============================================================================

    AudioDisplay.h
    Author:  tiagolr

    A wave display for audio buffer with markers on detected transients

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/Pattern.h"
#include <deque>

class GATE12AudioProcessor;

class AudioDisplay : public juce::Component, private juce::Timer
{
public:
    AudioDisplay(GATE12AudioProcessor&);
    ~AudioDisplay() override {};
    void resized() override;
    void timerCallback() override;
    void paint(Graphics& g) override;
    
    std::deque<double> audioBuffer;
    std::deque<bool> hitBuffer; 
    GATE12AudioProcessor& audioProcessor;
};