// Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Globals.h"

GATE12AudioProcessorEditor::GATE12AudioProcessorEditor (GATE12AudioProcessor& p)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
{
    audioProcessor.params.addParameterListener("sync", this);
    audioProcessor.params.addParameterListener("trigger", this);

    setSize (660, 640);
    setScaleFactor(audioProcessor.scale);
    auto col = 10;
    auto row = 10;

    // FIRST ROW

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

    addAndMakeVisible(triggerLabel);
    triggerLabel.setColour(juce::Label::ColourIds::textColourId, Colour(globals::COLOR_NEUTRAL_LIGHT));
    triggerLabel.setFont(FontOptions(16.0f));
    triggerLabel.setText("Trigger", NotificationType::dontSendNotification);
    triggerLabel.setBounds(col, row, 60, 25);
    col += 70-5;

    addAndMakeVisible(triggerMenu);
    triggerMenu.setTooltip("Envelope trigger:\nSync - song playback\nMIDI - midi notes\nAudio - audio input");
    triggerMenu.addItem("Sync", 1);
    triggerMenu.addItem("MIDI", 2);
    triggerMenu.addItem("Audio", 3);
    triggerMenu.setBounds(col, row, 80, 25);
    triggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "trigger", triggerMenu);
    col += 90;

    juce::MemoryInputStream audioInputStream(BinaryData::gear_png, BinaryData::gear_pngSize, false);
    juce::Image audioImage = juce::ImageFileFormat::loadFrom(audioInputStream);
    if (audioImage.isValid()) {
        audioSettingsLogo.setImages(false, true, true,
            audioImage, 1.0f, juce::Colours::transparentBlack,
            audioImage, 1.0f, juce::Colours::transparentBlack,
            audioImage, 1.0f, juce::Colours::transparentBlack
        );
    }
    addAndMakeVisible(audioSettingsLogo);
    audioSettingsLogo.setBounds(col+4, row+4, 25-8, 25-8);
    audioSettingsLogo.onClick = [this]() {
        showAudioKnobs = !showAudioKnobs;
        toggleUIComponents();
    };

    // SECOND ROW

    row += 35;
    col = 10;
    for (int i = 0; i < 12; ++i) {
        auto btn = std::make_unique<TextButton>(std::to_string(i + 1));

        btn->setRadioGroupId (1337);
        btn->setToggleState(audioProcessor.pattern->index == i, dontSendNotification);
        btn->setTooltip("Pattern selector");
        btn->setClickingTogglesState (false);
        btn->setColour (TextButton::textColourOffId,  Colour(globals::COLOR_BG));
        btn->setColour (TextButton::textColourOnId,   Colour(globals::COLOR_BG));
        btn->setColour (TextButton::buttonColourId,   Colour(globals::COLOR_ACTIVE).darker(0.8f));
        btn->setColour (TextButton::buttonOnColourId, Colour(globals::COLOR_ACTIVE));
        btn->setBounds (col + i * 22, row, 22, 25);
        btn->setConnectedEdges (((i != 0) ? Button::ConnectedOnLeft : 0) | ((i != 11) ? Button::ConnectedOnRight : 0));
        btn->setComponentID(i == 0 ? "leftPattern" : i == 11 ? "rightPattern" : "pattern");
        btn->onClick = [i, this]() {
            patterns[i].get()->setToggleState(true, dontSendNotification);
            };
        addAndMakeVisible(*btn);

        patterns.push_back(std::move(btn));
    }
    col += 274;

    addAndMakeVisible(patSyncLabel);
    patSyncLabel.setColour(juce::Label::ColourIds::textColourId, Colour(globals::COLOR_NEUTRAL_LIGHT));
    patSyncLabel.setFont(FontOptions(16.0f));
    patSyncLabel.setText("Pat. Sync", NotificationType::dontSendNotification);
    patSyncLabel.setBounds(col, row, 70, 25);
    col += 75;

    addAndMakeVisible(patSyncMenu);
    patSyncMenu.setTooltip("Pattern sync - changes pattern in sync with song position during playback");
    patSyncMenu.addItem("Off", 1);
    patSyncMenu.addItem("1/4 Beat", 2);
    patSyncMenu.addItem("1/2 Beat", 3);
    patSyncMenu.addItem("1 Beat", 4);
    patSyncMenu.addItem("2 Beats", 5);
    patSyncMenu.addItem("4 Beats", 6);
    patSyncMenu.setBounds(col, row, 90, 25);
    patSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "patsync", patSyncMenu);

    // KNOBS

    row += 35;
    col = 10;
    rate = std::make_unique<Rotary>(p, "rate", "Rate", LabelFormat::hzFloat1);
    addAndMakeVisible(*rate);
    rate->setBounds(col,row,80,65);
    col += 75;

    phase = std::make_unique<Rotary>(p, "phase", "Phase", LabelFormat::integerx100);
    addAndMakeVisible(*phase);
    phase->setBounds(col,row,80,65);
    col += 75;

    min = std::make_unique<Rotary>(p, "min", "Min", LabelFormat::integerx100);
    addAndMakeVisible(*min);
    min->setBounds(col,row,80,65);
    col += 75;

    max = std::make_unique<Rotary>(p, "max", "Max", LabelFormat::integerx100);
    addAndMakeVisible(*max);
    max->setBounds(col,row,80,65);
    col += 75;

    smooth = std::make_unique<Rotary>(p, "smooth", "Smooth", LabelFormat::integerx100);
    addAndMakeVisible(*smooth);
    smooth->setBounds(col,row,80,65);
    col += 75;

    attack = std::make_unique<Rotary>(p, "attack", "Attack", LabelFormat::integerx100);
    addAndMakeVisible(*attack);
    attack->setBounds(col,row,80,65);
    col += 75;

    release = std::make_unique<Rotary>(p, "release", "Release", LabelFormat::integerx100);
    addAndMakeVisible(*release);
    release->setBounds(col,row,80,65);
    col += 75;

    tension = std::make_unique<Rotary>(p, "tension", "Tension", LabelFormat::integerx100, true);
    addAndMakeVisible(*tension);
    tension->setBounds(col,row,80,65);
    col += 75;

    

    // THIRD ROW
    col = 10;
    row += 75;
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
    retriggerButton.setComponentID("button");
    retriggerButton.setColour(TextButton::buttonColourId, Colour(globals::COLOR_ACTIVE));
    retriggerButton.setVisible(audioProcessor.alwaysPlaying);
    retriggerButton.setBounds(col, row, 25, 25);

    // THIRD ROW RIGHT
    col = getWidth() - 10 - 60;

    addAndMakeVisible(snapButton);
    snapButton.setTooltip("Snap to grid on/off");
    snapButton.setButtonText("Snap");
    snapButton.setComponentID("button");
    snapButton.setBounds(col, row, 60, 25);
    snapButton.setClickingTogglesState(true);
    snapAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.params, "snap", snapButton);

    col -= 60;
    gridSelector = std::make_unique<GridSelector>(p);
    gridSelector.get()->setTooltip("Grid size can also be set using mouse wheel on view");
    addAndMakeVisible(*gridSelector);
    gridSelector->setBounds(col,row,50,25);

    // VIEW
    row += 25;
    col = 0;
    view = std::make_unique<View>(p);
    addAndMakeVisible(*view);
    view->setBounds(col,row,getWidth(), getHeight() - row);
    view->init();

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
    audioProcessor.params.removeParameterListener("trigger", this);
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
    auto trigger = (int)audioProcessor.params.getRawParameterValue("trigger")->load();
    auto triggerColor = trigger == 0 ? globals::COLOR_ACTIVE : trigger == 1 ? globals::COLOR_MIDI : globals::COLOR_AUDIO;
    triggerMenu.setColour(ComboBox::arrowColourId, Colour(triggerColor));
    triggerMenu.setColour(ComboBox::textColourId, Colour(triggerColor));
    triggerMenu.setColour(ComboBox::outlineColourId, Colour(triggerColor));
    audioSettingsLogo.setVisible(trigger == 2);
    if (!audioSettingsLogo.isVisible()) {
        showAudioKnobs = false;
    }
    else {
        juce::MemoryInputStream audioInputStream(
            showAudioKnobs ? BinaryData::geardark_png : BinaryData::gear_png, 
            showAudioKnobs ? BinaryData::geardark_pngSize : BinaryData::gear_pngSize, 
            false
        );
        juce::Image audioImage = juce::ImageFileFormat::loadFrom(audioInputStream);
        if (audioImage.isValid()) {
            audioSettingsLogo.setImages(false, true, true,
                audioImage, 1.0f, juce::Colours::transparentBlack,
                audioImage, 1.0f, juce::Colours::transparentBlack,
                audioImage, 1.0f, juce::Colours::transparentBlack
            );
        }
    }

    // layout knobs

    repaint();
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
    if (audioSettingsLogo.isVisible() && showAudioKnobs) {
        g.setColour(Colour(globals::COLOR_AUDIO));
        g.fillRoundedRectangle(audioSettingsLogo.getBounds().expanded(4,4).toFloat(), 3.0f);
    }
}

void GATE12AudioProcessorEditor::resized()
{
}
