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
    syncMenu.setBounds(col, row, 90, 25);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "sync", syncMenu);
    col += 100;

    // TODO pattern selector

    for (int i = 0; i < 12; ++i) {
        auto btn = std::make_unique<TextButton>(std::to_string(i + 1));

        btn->setRadioGroupId (1337);
        btn->setTooltip("Pattern selector");
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

    col = 10;
    row += 95;

    juce::MemoryInputStream paintInputStream(BinaryData::paint_png, BinaryData::paint_pngSize, false);
    juce::Image paintImage = juce::ImageFileFormat::loadFrom(paintInputStream);
    if (paintImage.isValid()) {
        paintLogo.setImages(false, true, true,
            paintImage, 1.0f, juce::Colours::transparentBlack,
            paintImage, 1.0f, juce::Colours::transparentBlack,
            paintImage, 1.0f, juce::Colours::transparentBlack
        );
    }
    addAndMakeVisible(paintLogo);
    paintLogo.setBounds(col, row, 25, 25);
    col += 25+10;

    addAndMakeVisible(paintMenu);
    paintMenu.setTooltip("Paint mode\nRight click on view to paint\nAlt + right click to erase points");
    paintMenu.addItem("Erase", 1);
    paintMenu.addItem("Line", 2);
    paintMenu.addItem("Saw Up", 3);
    paintMenu.addItem("Saw Down", 4);
    paintMenu.addItem("Triangle", 5);
    paintMenu.setBounds(col, row, 90, 25);
    paintAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "paint", paintMenu);
    col += 100;

    juce::MemoryInputStream pointInputStream(BinaryData::point_png, BinaryData::point_pngSize, false);
    juce::Image pointImage = juce::ImageFileFormat::loadFrom(pointInputStream);
    if (pointImage.isValid()) {
        pointLogo.setImages(false, true, true,
            pointImage, 1.0f, juce::Colours::transparentBlack,
            pointImage, 1.0f, juce::Colours::transparentBlack,
            pointImage, 1.0f, juce::Colours::transparentBlack
        );
    }
    addAndMakeVisible(pointLogo);
    pointLogo.setBounds(col, row, 25, 25);
    col += 25+10;

    addAndMakeVisible(pointMenu);
    pointMenu.setTooltip("Point mode\nRight click points to change mode");
    pointMenu.addItem("Hold", 1);
    pointMenu.addItem("Curve", 2);
    pointMenu.addItem("S-Curve", 3);
    pointMenu.addItem("Pulse", 4);
    pointMenu.addItem("Wave", 5);
    pointMenu.addItem("Triangle", 6);
    pointMenu.addItem("Stairs", 7);
    pointMenu.addItem("Smooth St", 8);
    pointMenu.setBounds(col, row, 90, 25);
    pointAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "point", pointMenu);
    col += 100;

    addAndMakeVisible(loopButton);
    loopButton.setTooltip("Toggle always playing");
    loopButton.setColour(TextButton::buttonColourId, Colours::transparentWhite);
    loopButton.setColour(ComboBox::outlineColourId, Colours::transparentWhite);
    loopButton.setBounds(col, row, 25, 25);
    loopButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.alwaysPlaying = !audioProcessor.alwaysPlaying;
            retriggerButton.setVisible(audioProcessor.alwaysPlaying);
            repaint();
        });
    };
    col += 35;

    addAndMakeVisible(retriggerButton);
    retriggerButton.setTooltip("Restart envelope");
    retriggerButton.setButtonText("R");
    retriggerButton.setColour(TextButton::buttonColourId, Colours::transparentWhite);
    retriggerButton.setColour(TextButton::textColourOnId, Colour(globals::COLOR_ACTIVE));
    retriggerButton.setColour(TextButton::textColourOffId, Colour(globals::COLOR_ACTIVE));
    retriggerButton.setVisible(audioProcessor.alwaysPlaying);
    retriggerButton.setBounds(col, row, 25, 25);

    // FOOTER
    row = getHeight() - 35;
    col = 10;
    addAndMakeVisible(trigSyncLabel);
    trigSyncLabel.setColour(juce::Label::ColourIds::textColourId, Colour(globals::COLOR_NEUTRAL_LIGHT));
    trigSyncLabel.setFont(FontOptions(16.0f));
    trigSyncLabel.setText("Trig Sync", NotificationType::dontSendNotification);
    trigSyncLabel.setBounds(col, row, 70, 25);
    col += 75;

    addAndMakeVisible(trigSyncMenu);
    trigSyncMenu.setTooltip("Synchronize pattern changes to song position during playback");
    trigSyncMenu.addItem("Off", 1);
    trigSyncMenu.addItem("1/4 Beat", 2);
    trigSyncMenu.addItem("1/2 Beat", 3);
    trigSyncMenu.addItem("1 Beat", 4);
    trigSyncMenu.addItem("2 Beats", 5);
    trigSyncMenu.addItem("4 Beats", 6);
    trigSyncMenu.setBounds(col, row, 90, 25);
    trigSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "trigsync", trigSyncMenu);

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

    // draw loop play button
    g.setColour(Colour(globals::COLOR_ACTIVE));
    if (audioProcessor.alwaysPlaying) {
        auto loopBounds = loopButton.getBounds().expanded(-5);
        g.fillRect(loopBounds);
    }
    else {
        juce::Path triangle;
        auto loopBounds = loopButton.getBounds().expanded(-5);
        triangle.startNewSubPath(0.0f, 0.0f);    
        triangle.lineTo(0.0f, (float)loopBounds.getHeight());            
        triangle.lineTo((float)loopBounds.getWidth(), loopBounds.getHeight() / 2.f);
        triangle.closeSubPath();
        g.fillPath(triangle, AffineTransform::translation((float)loopBounds.getX(), (float)loopBounds.getY()));

    }
}

void GATE12AudioProcessorEditor::resized()
{
}
