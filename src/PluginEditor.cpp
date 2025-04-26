// Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Globals.h"

GATE12AudioProcessorEditor::GATE12AudioProcessorEditor (GATE12AudioProcessor& p)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
{
    audioProcessor.params.addParameterListener("sync", this);

    setSize (640, 640);
    setScaleFactor(audioProcessor.scale);
    auto col = 10;
    auto row = 10;


    // TOP BAR
    addAndMakeVisible(logoLabel);
    logoLabel.setColour(juce::Label::ColourIds::textColourId, Colours::white);
    logoLabel.setFont(FontOptions(26.0f));
    logoLabel.setText("GATE-12", NotificationType::dontSendNotification);
    logoLabel.setBounds(col, row-3, 100, 30);
    col += 110;

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
    syncMenu.setBounds(col, row, 80, 25);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "sync", syncMenu);
    col += 90;

    // TODO pattern selector

    for (int i = 0; i < 12; ++i) {
        auto btn = std::make_unique<TextButton>(std::to_string(i + 1));

        btn->setRadioGroupId (1337);
        btn->setClickingTogglesState (false);
        btn->setColour (TextButton::textColourOffId,  Colour(globals::COLOR_BG));
        btn->setColour (TextButton::textColourOnId,   Colour(globals::COLOR_BG));
        btn->setColour (TextButton::buttonColourId,   Colour(globals::COLOR_ACTIVE).darker(0.8f));
        btn->setColour (TextButton::buttonOnColourId, Colour(globals::COLOR_ACTIVE));
        btn->setBounds (col + i * 22, row, 22, 25);
        btn->setConnectedEdges (((i != 0) ? Button::ConnectedOnLeft : 0) | ((i != 11) ? Button::ConnectedOnRight : 0));
        btn->setComponentID(i == 0 ? "leftCorner" : i == 11 ? "rightCorner" : "middle");
        btn->onClick = [i, this]() {
            patterns[i].get()->setToggleState(true, dontSendNotification);
        };
        addAndMakeVisible(*btn);

        patterns.push_back(std::move(btn));
    }
    col += 230;

    addAndMakeVisible(trigSyncLabel);
    trigSyncLabel.setColour(juce::Label::ColourIds::textColourId, Colour(globals::COLOR_NEUTRAL_LIGHT));
    trigSyncLabel.setFont(FontOptions(16.0f));
    trigSyncLabel.setText("Trig Sync", NotificationType::dontSendNotification);
    trigSyncLabel.setBounds(col, row, 70, 25);
    col += 75;

    addAndMakeVisible(trigSyncMenu);
    trigSyncMenu.addItem("Off", 1);
    trigSyncMenu.addItem("1/4 Beat", 2);
    trigSyncMenu.addItem("1/2 Beat", 3);
    trigSyncMenu.addItem("1 Beat", 4);
    trigSyncMenu.addItem("2 Beats", 5);
    trigSyncMenu.addItem("4 Beats", 6);
    trigSyncMenu.setBounds(col, row, 90, 25);
    trigSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "trigsync", trigSyncMenu);

    // KNOBS
    row += 35;
    col = 10;
    rate = std::make_unique<Rotary>(p, "rate", "Rate", LabelFormat::hzFloat1);
    addAndMakeVisible(*rate);
    rate->setBounds(col,row,80,75);
    col += 75;

    phase = std::make_unique<Rotary>(p, "phase", "Phase", LabelFormat::integerx100);
    addAndMakeVisible(*phase);
    phase->setBounds(col,row,80,75);
    col += 75;

    min = std::make_unique<Rotary>(p, "min", "Min", LabelFormat::integerx100);
    addAndMakeVisible(*min);
    min->setBounds(col,row,80,75);
    col += 75;

    max = std::make_unique<Rotary>(p, "max", "Max", LabelFormat::integerx100);
    addAndMakeVisible(*max);
    max->setBounds(col,row,80,75);
    col += 75;

    smooth = std::make_unique<Rotary>(p, "smooth", "Smooth", LabelFormat::integerx100);
    addAndMakeVisible(*smooth);
    smooth->setBounds(col,row,80,75);
    col += 75;

    attack = std::make_unique<Rotary>(p, "attack", "Attack", LabelFormat::integerx100);
    addAndMakeVisible(*attack);
    attack->setBounds(col,row,80,75);
    col += 75;

    release = std::make_unique<Rotary>(p, "release", "Release", LabelFormat::integerx100);
    addAndMakeVisible(*release);
    release->setBounds(col,row,80,75);
    col += 75;

    tension = std::make_unique<Rotary>(p, "tension", "Tension", LabelFormat::integerx100, true);
    addAndMakeVisible(*tension);
    tension->setBounds(col,row,80,75);
    col += 75;

    // ABOUT
    about = std::make_unique<About>();
    addAndMakeVisible(*about);
    about->setBounds(getBounds());
    about->setVisible(false);

    customLookAndFeel = new CustomLookAndFeel();
    setLookAndFeel(customLookAndFeel);

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
    //auto isRateSync = (int)audioProcessor.params.getRawParameterValue("sync")->load() == 0;

    //rate.get()->setVisible(isRateSync);
}

//==============================================================================

void GATE12AudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(Colour(globals::COLOR_BG));
}

void GATE12AudioProcessorEditor::resized()
{
}
