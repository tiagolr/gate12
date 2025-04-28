/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/Rotary.h"
#include "ui/GridSelector.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/About.h"
#include "ui/View.h"

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
    std::unique_ptr<About> about;

    std::vector<std::unique_ptr<TextButton>> patterns;
    bool showAudioKnobs = false;

#if defined(DEBUG)
    juce::TextButton presetExport;
#endif

    Label logoLabel;
    ComboBox syncMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncAttachment;
    Label patSyncLabel;
    ComboBox patSyncMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> patSyncAttachment;
    std::unique_ptr<Rotary> rate;
    std::unique_ptr<Rotary> phase;
    std::unique_ptr<Rotary> min;
    std::unique_ptr<Rotary> max;
    std::unique_ptr<Rotary> smooth;
    std::unique_ptr<Rotary> attack;
    std::unique_ptr<Rotary> release;
    std::unique_ptr<Rotary> tension;
    ImageButton paintLogo;
    ComboBox paintMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> paintAttachment;
    ImageButton pointLogo;
    ComboBox pointMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> pointAttachment;
    TextButton loopButton;
    TextButton retriggerButton;
    Label triggerLabel;
    ComboBox triggerMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> triggerAttachment;
    ImageButton audioSettingsLogo;
    Label gridLabel;
    TextButton snapButton;
    std::unique_ptr<GridSelector> gridSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> snapAttachment;
    std::unique_ptr<View> view;

    TooltipWindow tooltipWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GATE12AudioProcessorEditor)
};
