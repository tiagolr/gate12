/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/Rotary.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/About.h"

using namespace globals;

class GATE12AudioProcessorEditor : public juce::AudioProcessorEditor, private juce::AudioProcessorValueTreeState::Listener
{
public:
    GATE12AudioProcessorEditor (GATE12AudioProcessor&);
    ~GATE12AudioProcessorEditor() override;

    //==============================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void toggleUIComponents ();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    GATE12AudioProcessor& audioProcessor;
    CustomLookAndFeel* customLookAndFeel = nullptr;

    Label noiseLabel;
    Label envelopeLabel;
    Label malletLabel;
    std::unique_ptr<About> about;

    std::vector<std::unique_ptr<TextButton>> patterns;

#if defined(DEBUG)
    juce::TextButton presetExport;
#endif

    Label logoLabel;
    ComboBox syncMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncAttachment;
    Label trigSyncLabel;
    ComboBox trigSyncMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> trigSyncAttachment;
    std::unique_ptr<Rotary> rate;
    std::unique_ptr<Rotary> phase;
    std::unique_ptr<Rotary> min;
    std::unique_ptr<Rotary> max;
    std::unique_ptr<Rotary> smooth;
    std::unique_ptr<Rotary> attack;
    std::unique_ptr<Rotary> release;
    std::unique_ptr<Rotary> tension;
    Label paintModeLabel;
    ComboBox paintMode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> paintModeAttachment;
    Label pointModeLabel;
    ComboBox pointMode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> pointModeAttachment;
    TextButton snap;
    // gridSelector
    // retrigger button
    // play button

    TooltipWindow tooltipWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GATE12AudioProcessorEditor)
};
