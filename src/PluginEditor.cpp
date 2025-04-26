// Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Globals.h"

GATE12AudioProcessorEditor::GATE12AudioProcessorEditor (GATE12AudioProcessor& p)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
{
    audioProcessor.params.addParameterListener("sync", this);

    setSize (540, 540);
    setScaleFactor(audioProcessor.scale);
    auto col = 10;
    auto row = 10;

    // TOP BAR
#if defined(DEBUG)
    addAndMakeVisible(presetExport);
    presetExport.setAlpha(0.f);
    presetExport.setTooltip("DEBUG ONLY - Exports preset to debug console");
    presetExport.setButtonText("Export");
    presetExport.setBounds(10, 10, 100, 25);
    presetExport.onClick = [this] {
        auto state = audioProcessor.params.copyState();
        std::unique_ptr<juce::XmlElement>xml(state.createXml());
        juce::String xmlString = xml->toString();
        DBG(xmlString.toStdString());
    };
#endif

    col += 160;
    addAndMakeVisible(sizeLabel);
    sizeLabel.setColour(juce::Label::ColourIds::textColourId, Colour(globals::COLOR_NEUTRAL_LIGHT));
    sizeLabel.setFont(FontOptions(16.0f));
    sizeLabel.setText("UI", NotificationType::dontSendNotification);
    sizeLabel.setBounds(col, row, 30, 25);

    addAndMakeVisible(sizeMenu);
    sizeMenu.addItem("100%", 1);
    sizeMenu.addItem("125%", 2);
    sizeMenu.addItem("150%", 3);
    sizeMenu.addItem("175%", 4);
    sizeMenu.addItem("200%", 5);
    sizeMenu.setSelectedId(audioProcessor.scale == 1.0f ? 1
        : audioProcessor.scale == 1.25f ? 2
        : audioProcessor.scale == 1.5f ? 3
        : audioProcessor.scale == 1.75f ? 4
        : 5);
    sizeMenu.onChange = [this]()
        {
            const int value = sizeMenu.getSelectedId();
            auto scale = value == 1 ? 1.0f : value == 2 ? 1.25f : value == 3 ? 1.5f : value == 4 ? 1.75f : 2.0f;
            audioProcessor.setScale(scale);
            setScaleFactor(audioProcessor.scale);
        };
    sizeMenu.setBounds(col+25,row,80,25);

    addAndMakeVisible(syncMenu);
    syncMenu.setTooltip("Tempo sync");
    syncMenu.addItem("Rate Hz", 1);
    syncMenu.addItem("1/16", 2);
    syncMenu.addItem("1/8", 3);
    syncMenu.addItem("1/4", 4);
    syncMenu.addItem("1/2", 5);
    syncMenu.addItem("1/1", 6);
    syncMenu.addItem("2/1", 7);
    syncMenu.addItem("4/1", 8);
    syncMenu.addItem("1/16t", 9);
    syncMenu.addItem("1/8t", 10);
    syncMenu.addItem("1/4t", 11);
    syncMenu.addItem("1/2t", 12);
    syncMenu.addItem("1/1t", 13);
    syncMenu.addItem("1/16.", 14);
    syncMenu.addItem("1/8.", 15);
    syncMenu.addItem("1/4.", 16);
    syncMenu.addItem("1/2.", 17);
    syncMenu.addItem("1/1.", 18);
    syncMenu.setBounds(col+75, row, 60, 25);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "sync", syncMenu);

    // TODO pattern selector
    addAndMakeVisible(trigSyncLabel);
    trigSyncLabel.setColour(juce::Label::ColourIds::textColourId, Colour(globals::COLOR_NEUTRAL_LIGHT));
    trigSyncLabel.setFont(FontOptions(16.0f));
    trigSyncLabel.setText("Trig Sync", NotificationType::dontSendNotification);
    trigSyncLabel.setBounds(col+100, row, 70, 25);

    addAndMakeVisible(trigSyncMenu);
    trigSyncMenu.addItem("Off", 1);
    trigSyncMenu.addItem("1/4 Beat", 2);
    trigSyncMenu.addItem("1/2 Beat", 3);
    trigSyncMenu.addItem("1 Beat", 4);
    trigSyncMenu.addItem("2 Beats", 5);
    trigSyncMenu.addItem("4 Beats", 6);
    trigSyncMenu.setBounds(col + 200, row, 60, 25);
    trigSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "trigsync", trigSyncMenu);

    // KNOBS
    row += 40;
    col = 10;
    rate = std::make_unique<Rotary>(p, "rate", "Rate", LabelFormat::hzFloat1);
    addAndMakeVisible(*rate);
    rate->setBounds(col,row,70,75);
    col += 70;

    phase = std::make_unique<Rotary>(p, "phase", "Phase", LabelFormat::percent);
    addAndMakeVisible(*phase);
    phase->setBounds(col,row,70,75);
    col += 70;

    min = std::make_unique<Rotary>(p, "min", "Min", LabelFormat::percent);
    addAndMakeVisible(*min);
    min->setBounds(col,row,70,75);
    col += 70;

    max = std::make_unique<Rotary>(p, "max", "Max", LabelFormat::percent);
    addAndMakeVisible(*max);
    max->setBounds(col,row,70,75);
    col += 70;

    smooth = std::make_unique<Rotary>(p, "smooth", "Smooth", LabelFormat::percent);
    addAndMakeVisible(*smooth);
    smooth->setBounds(col,row,70,75);
    col += 70;

    attack = std::make_unique<Rotary>(p, "attack", "Attack", LabelFormat::percent);
    addAndMakeVisible(*attack);
    attack->setBounds(col,row,70,75);
    col += 70;

    release = std::make_unique<Rotary>(p, "release", "Release", LabelFormat::percent);
    addAndMakeVisible(*release);
    release->setBounds(col,row,70,75);
    col += 70;

    tension = std::make_unique<Rotary>(p, "tension", "Tension", LabelFormat::percent);
    addAndMakeVisible(*tension);
    tension->setBounds(col,row,70,75);
    col += 70;

    // ABOUT
    about = std::make_unique<About>();
    addAndMakeVisible(*about);
    about->setBounds(getBounds());
    about->setVisible(false);

    toggleUIComponents();
}

GATE12AudioProcessorEditor::~GATE12AudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    delete customLookAndFeel;
    audioProcessor.params.removeParameterListener("sync", this);
}

void GATE12AudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    (void)parameterID;
    (void)newValue;
    juce::MessageManager::callAsync([this] {
        toggleUIComponents();
    });
};

void GATE12AudioProcessorEditor::toggleUIComponents()
{
    auto isRateSync = (int)audioProcessor.params.getRawParameterValue("sync")->load() == 0;

    rate.get()->setVisible(isRateSync);
}

//==============================================================================

void GATE12AudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(Colour(globals::COLOR_BG));
}

void GATE12AudioProcessorEditor::resized()
{
}
