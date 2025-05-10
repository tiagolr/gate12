/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/Rotary.h"
#include "ui/TextDial.h"
#include "ui/GridSelector.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/About.h"
#include "ui/View.h"
#include "ui/SettingsButton.h"
#include "ui/AudioDisplay.h"
#include "ui/PaintToolWidget.h"

using namespace globals;

class GATE12AudioProcessorEditor : public juce::AudioProcessorEditor, private juce::AudioProcessorValueTreeState::Listener, public juce::ChangeListener
{
public:
    GATE12AudioProcessorEditor (GATE12AudioProcessor&);
    ~GATE12AudioProcessorEditor() override;

    //==============================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void toggleUIComponents ();
    void paint (juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback(ChangeBroadcaster* source) override;
    void drawGear(Graphics&g, Rectangle<int> bounds, float radius, int segs, Colour color, Colour bg);
    void drawUndoButton(Graphics& g, juce::Rectangle<float> area, bool invertx, Colour color);

private:
    bool init = false;
    GATE12AudioProcessor& audioProcessor;
    CustomLookAndFeel* customLookAndFeel = nullptr;
    std::unique_ptr<About> about;

    std::vector<std::unique_ptr<TextButton>> patterns;

#if defined(DEBUG)
    juce::TextButton presetExport;
#endif

    Label logoLabel;
    ComboBox syncMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncAttachment;
    Label patSyncLabel;
    ComboBox patSyncMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> patSyncAttachment;
    std::unique_ptr<SettingsButton> settingsButton;
    std::unique_ptr<TextDial> mixDial;

    std::unique_ptr<Rotary> rate;
    std::unique_ptr<Rotary> phase;
    std::unique_ptr<Rotary> min;
    std::unique_ptr<Rotary> max;
    std::unique_ptr<Rotary> smooth;
    std::unique_ptr<Rotary> attack;
    std::unique_ptr<Rotary> release;
    std::unique_ptr<Rotary> tension;
    std::unique_ptr<Rotary> tensionatk;
    std::unique_ptr<Rotary> tensionrel;
    std::unique_ptr<Rotary> threshold;
    std::unique_ptr<Rotary> sense;
    std::unique_ptr<Rotary> lowcut;
    std::unique_ptr<Rotary> highcut;
    std::unique_ptr<Rotary> offset;
    
    ComboBox algoMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algoAttachment;
    TextButton useSidechain;
    TextButton useMonitor;
    TextButton nudgeRightButton;
    Label nudgeLabel;
    TextButton nudgeLeftButton;
    TextButton undoButton;
    TextButton redoButton;
    std::unique_ptr<AudioDisplay> audioDisplay;
    TextButton paintButton;
    TextButton paintEditButton;
    TextButton paintNextButton;
    TextButton paintPrevButton;
    Label paintPageLabel;
    ComboBox pointMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> pointAttachment;
    Label pointLabel;
    TextButton loopButton;
    Label triggerLabel;
    ComboBox triggerMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> triggerAttachment;
    TextButton audioSettingsButton;
    TextButton snapButton;
    std::unique_ptr<GridSelector> gridSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> snapAttachment;
    std::unique_ptr<View> view;
    Label latencyWarning;
    std::unique_ptr<PaintToolWidget> paintWidget;

    TooltipWindow tooltipWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GATE12AudioProcessorEditor)
};
