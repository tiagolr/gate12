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
    audioProcessor.params.addParameterListener("pattern", this);

    setSize (globals::PLUG_WIDTH, globals::PLUG_HEIGHT);
    setScaleFactor(audioProcessor.scale);
    auto col = globals::PAD;
    auto row = globals::PAD;

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
    syncMenu.addSectionHeading("Free");
    syncMenu.addItem("Rate Hz", 1);
    syncMenu.addSectionHeading("Straight");
    syncMenu.addItem("1/16", 2);
    syncMenu.addItem("1/8", 3);
    syncMenu.addItem("1/4", 4);
    syncMenu.addItem("1/2", 5);
    syncMenu.addItem("1 Bar", 6);
    syncMenu.addItem("2 Bars", 7);
    syncMenu.addItem("4 Bars", 8);
    syncMenu.addSectionHeading("Tripplet");
    syncMenu.addItem("1/16T", 9);
    syncMenu.addItem("1/8T", 10);
    syncMenu.addItem("1/4T", 11);
    syncMenu.addItem("1/2T", 12);
    syncMenu.addItem("1/1T", 13);
    syncMenu.addSectionHeading("Dotted");
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

    addAndMakeVisible(algoMenu);
    algoMenu.setTooltip("Algorithm used for transient detection");
    algoMenu.addItem("Simple", 1);
    algoMenu.addItem("Drums", 2);
    algoMenu.setBounds(col,row,90,25);
    algoMenu.setColour(ComboBox::arrowColourId, Colour(globals::COLOR_AUDIO));
    algoMenu.setColour(ComboBox::textColourId, Colour(globals::COLOR_AUDIO));
    algoMenu.setColour(ComboBox::outlineColourId, Colour(globals::COLOR_AUDIO));
    algoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "algo", algoMenu);
    col += 100;

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
        audioProcessor.showAudioKnobs = !audioProcessor.showAudioKnobs;
        toggleUIComponents();
    };
    col += 35;

    col = getWidth() - globals::PAD - 25;
    settingsButton = std::make_unique<SettingsButton>(p);
    addAndMakeVisible(*settingsButton);
    settingsButton->onScaleChange = [this]() { setScaleFactor(audioProcessor.scale); };
    settingsButton->toggleUIComponents = [this]() { toggleUIComponents(); };
    settingsButton->toggleAbout = [this]() { about.get()->setVisible(true); };
    settingsButton->setBounds(col,row,25,25);

    // SECOND ROW

    row += 35;
    col = globals::PAD;
    for (int i = 0; i < 12; ++i) {
        auto btn = std::make_unique<TextButton>(std::to_string(i + 1));
        btn->setRadioGroupId (1337);
        btn->setToggleState(audioProcessor.pattern->index == i, dontSendNotification);
        btn->setClickingTogglesState (false);
        btn->setColour (TextButton::textColourOffId,  Colour(globals::COLOR_BG));
        btn->setColour (TextButton::textColourOnId,   Colour(globals::COLOR_BG));
        btn->setColour (TextButton::buttonColourId,   Colour(globals::COLOR_ACTIVE).darker(0.8f));
        btn->setColour (TextButton::buttonOnColourId, Colour(globals::COLOR_ACTIVE));
        btn->setBounds (col + i * 22, row, 22, 25);
        btn->setConnectedEdges (((i != 0) ? Button::ConnectedOnLeft : 0) | ((i != 11) ? Button::ConnectedOnRight : 0));
        btn->setComponentID(i == 0 ? "leftPattern" : i == 11 ? "rightPattern" : "pattern");
        btn->onClick = [i, this]() {
            MessageManager::callAsync([i, this] {
                auto param = audioProcessor.params.getParameter("pattern");
                param->beginChangeGesture();
                param->setValueNotifyingHost(param->convertTo0to1((float)(i + 1)));
                param->endChangeGesture();
            });
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
    patSyncMenu.setTooltip("Changes pattern in sync with song position during playback");
    patSyncMenu.addItem("Off", 1);
    patSyncMenu.addItem("1/4 Beat", 2);
    patSyncMenu.addItem("1/2 Beat", 3);
    patSyncMenu.addItem("1 Beat", 4);
    patSyncMenu.addItem("2 Beats", 5);
    patSyncMenu.addItem("4 Beats", 6);
    patSyncMenu.setBounds(col, row, 90, 25);
    patSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "patsync", patSyncMenu);

    // KNOBS ROW
    row += 35;
    col = globals::PAD;
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

    addAndMakeVisible(nudgeLeftButton);
    nudgeLeftButton.setAlpha(0.f);
    nudgeLeftButton.setBounds(phase->getX(), phase->getBottom()-10, 10, 10);
    nudgeLeftButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            auto phase = audioProcessor.params.getParameter("phase");
            auto grid = audioProcessor.params.getRawParameterValue("grid")->load();
            auto gridSize = 1.f / grid;
            auto value = phase->getValue();
            value = std::ceil(value * grid) * gridSize - gridSize;
            if (value < 0.f)
                value = 1.0f;
            phase->beginChangeGesture();
            phase->setValueNotifyingHost(value);
            phase->endChangeGesture();
        });
    };

    addAndMakeVisible(nudgeRightButton);
    nudgeRightButton.setAlpha(0.f);
    nudgeRightButton.setBounds(phase->getRight()-10, phase->getBottom() - 10, 10, 10);
    nudgeRightButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            auto phase = audioProcessor.params.getParameter("phase");
            auto grid = audioProcessor.params.getRawParameterValue("grid")->load();
            auto gridSize = 1.f / grid;
            auto value = phase->getValue();
            value = std::floor(value * grid) * gridSize + gridSize;
            if (value > 1.f)
                value = 0.0f;
            phase->beginChangeGesture();
            phase->setValueNotifyingHost(value);
            phase->endChangeGesture();
        });
    };

    // AUDIO KNOBS
    col = globals::PAD;

    threshold = std::make_unique<Rotary>(p, "threshold", "Thres", LabelFormat::integerx100, false, true);
    addAndMakeVisible(*threshold);
    threshold->setBounds(col,row,80,65);
    col += 75;

    sense = std::make_unique<Rotary>(p, "sense", "Sense", LabelFormat::percent, false, true);
    addAndMakeVisible(*sense);
    sense->setBounds(col,row,80,65);
    col += 75;

    lowcut = std::make_unique<Rotary>(p, "lowcut", "LowCut", LabelFormat::percent, false, true);
    addAndMakeVisible(*lowcut);
    lowcut->setBounds(col,row,80,65);
    col += 75;

    highcut = std::make_unique<Rotary>(p, "highcut", "HiCut", LabelFormat::percent, false, true);
    addAndMakeVisible(*highcut);
    highcut->setBounds(col,row,80,65);
    col += 75;

    offset = std::make_unique<Rotary>(p, "offset", "Offset", LabelFormat::percent, true, true);
    addAndMakeVisible(*offset);
    offset->setBounds(col,row,80,65);
    col += 75;

    audioDisplay = std::make_unique<AudioDisplay>(p);
    addAndMakeVisible(*audioDisplay);
    audioDisplay->setBounds(col,row,getWidth() - col - globals::PAD - 80 - 10, 65);
    MessageManager::callAsync([this] {
        audioProcessor.monW = audioDisplay->getWidth();
    });

    col = getWidth() - globals::PAD - 80;
    addAndMakeVisible(useSidechain);
    useSidechain.setTooltip("Use sidechain for transient detection");
    useSidechain.setButtonText("Sidechain");
    useSidechain.setComponentID("button");
    useSidechain.setColour(TextButton::buttonColourId, Colour(globals::COLOR_AUDIO));
    useSidechain.setColour(TextButton::buttonOnColourId, Colour(globals::COLOR_AUDIO));
    useSidechain.setColour(TextButton::textColourOnId, Colour(globals::COLOR_BG));
    useSidechain.setColour(TextButton::textColourOffId, Colour(globals::COLOR_AUDIO));
    useSidechain.setBounds(col,row,80,25);
    useSidechain.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.useSidechain = !audioProcessor.useSidechain;
            toggleUIComponents();
        });
    };

    addAndMakeVisible(useMonitor);
    useMonitor.setTooltip("Monitor signal used for transient detection");
    useMonitor.setButtonText("Monitor");
    useMonitor.setComponentID("button");
    useMonitor.setColour(TextButton::buttonColourId, Colour(globals::COLOR_AUDIO));
    useMonitor.setColour(TextButton::buttonOnColourId, Colour(globals::COLOR_AUDIO));
    useMonitor.setColour(TextButton::textColourOnId, Colour(globals::COLOR_BG));
    useMonitor.setColour(TextButton::textColourOffId, Colour(globals::COLOR_AUDIO));
    useMonitor.setBounds(col,row+35,80,25);
    useMonitor.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.useMonitor = !audioProcessor.useMonitor;
            toggleUIComponents();
        });
    };

    // THIRD ROW
    col = globals::PAD;
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
    loopButton.setTooltip("Toggle loop play mode");
    loopButton.setColour(TextButton::buttonColourId, Colours::transparentWhite);
    loopButton.setColour(ComboBox::outlineColourId, Colours::transparentWhite);
    loopButton.setBounds(col, row, 25, 25);
    loopButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.alwaysPlaying = !audioProcessor.alwaysPlaying;
            repaint();
        });
    };
    col += 35;

    // THIRD ROW RIGHT
    col = getWidth() - globals::PAD - 60;

    addAndMakeVisible(snapButton);
    snapButton.setTooltip("Toggle snap by using ctrl key");
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
    MessageManager::callAsync([this] {
        audioProcessor.viewW = view->winw;
    });

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
    audioProcessor.params.removeParameterListener("pattern", this);
}

void GATE12AudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "pattern") {
        patterns[(int)newValue - 1].get()->setToggleState(true, dontSendNotification);
    }

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
    audioSettingsLogo.setVisible(trigger == Trigger::Audio);
    algoMenu.setVisible(trigger == Trigger::Audio);
    if (!audioSettingsLogo.isVisible()) {
        audioProcessor.showAudioKnobs = false;
    }
    else {
        juce::MemoryInputStream audioInputStream(
            audioProcessor.showAudioKnobs ? BinaryData::geardark_png : BinaryData::gear_png, 
            audioProcessor.showAudioKnobs ? BinaryData::geardark_pngSize : BinaryData::gear_pngSize, 
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
    loopButton.setVisible(trigger > 0);

    int sync = (int)audioProcessor.params.getRawParameterValue("sync")->load();
    bool showAudioKnobs = audioProcessor.showAudioKnobs;

    // layout knobs
    rate->setVisible(!showAudioKnobs);
    phase->setVisible(!showAudioKnobs);
    min->setVisible(!showAudioKnobs);
    max->setVisible(!showAudioKnobs);
    smooth->setVisible(!showAudioKnobs);
    attack->setVisible(!showAudioKnobs);
    release->setVisible(!showAudioKnobs);
    tension->setVisible(!showAudioKnobs);
    nudgeLeftButton.setVisible(!showAudioKnobs);
    nudgeRightButton.setVisible(!showAudioKnobs);

    threshold->setVisible(showAudioKnobs);
    sense->setVisible(showAudioKnobs);
    lowcut->setVisible(showAudioKnobs);
    highcut->setVisible(showAudioKnobs);
    offset->setVisible(showAudioKnobs);
    audioDisplay->setVisible(showAudioKnobs);

    if (!showAudioKnobs) {
        auto col = globals::PAD;
        auto row = globals::PAD + 35 + 35;
        rate->setVisible(sync == 0);
        rate->setTopLeftPosition(col, row);
        if (rate->isVisible()) 
            col += 75;
        phase->setTopLeftPosition(col, row);
        col += 75;
        min->setTopLeftPosition(col, row);
        col += 75;
        max->setTopLeftPosition(col, row);
        col += 75;
        if (audioProcessor.dualSmooth) {
            smooth->setVisible(false);
            attack->setVisible(true);
            release->setVisible(true);
            attack->setTopLeftPosition(col, row);
            col += 75;
            release->setTopLeftPosition(col, row);
            col+= 75;
        }
        else {
            smooth->setVisible(true);
            attack->setVisible(false);
            release->setVisible(false);
            smooth->setTopLeftPosition(col, row);
            col += 75;
        }
        tension->setTopLeftPosition(col, row);
        nudgeLeftButton.setTopLeftPosition(phase->getX(), phase->getBottom() - 10);
        nudgeRightButton.setTopRightPosition(phase->getX() + phase->getWidth(), phase->getBottom()-10);
    }

    useSidechain.setVisible(showAudioKnobs);
    useMonitor.setVisible(showAudioKnobs);
    useSidechain.setToggleState(audioProcessor.useSidechain, dontSendNotification);
    useMonitor.setToggleState(audioProcessor.useMonitor, dontSendNotification);

    repaint();
}

//==============================================================================

void GATE12AudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(Colour(globals::COLOR_BG));
    g.setColour(Colour(globals::COLOR_ACTIVE));
    auto trigger = (int)audioProcessor.params.getRawParameterValue("trigger")->load();

    // draw loop play button
    if (trigger != Trigger::Sync) {
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
    // draw audio settings button outline
    if (audioSettingsLogo.isVisible() && audioProcessor.showAudioKnobs) {
        g.setColour(Colour(globals::COLOR_AUDIO));
        g.fillRoundedRectangle(audioSettingsLogo.getBounds().expanded(4,4).toFloat(), 3.0f);
    }
    // draw phase nudge buttons
    if (!audioProcessor.showAudioKnobs) {
        g.setColour(Colour(globals::COLOR_ACTIVE));
        juce::Path nudgeLeftTriangle;
        nudgeLeftTriangle.startNewSubPath(0.0f, nudgeLeftButton.getHeight() / 2.f);    
        nudgeLeftTriangle.lineTo((float)nudgeLeftButton.getWidth(), 0.f);            
        nudgeLeftTriangle.lineTo((float)nudgeLeftButton.getWidth(), (float)nudgeLeftButton.getHeight());
        nudgeLeftTriangle.closeSubPath();
        g.fillPath(nudgeLeftTriangle, AffineTransform::translation((float)nudgeLeftButton.getX(), (float)nudgeLeftButton.getY()));

        juce::Path nudgeRightTriangle;
        nudgeRightTriangle.startNewSubPath(0.0f, 0.0f);    
        nudgeRightTriangle.lineTo((float)nudgeRightButton.getWidth(), nudgeRightButton.getHeight()/2.f);            
        nudgeRightTriangle.lineTo(0.0f, (float)nudgeRightButton.getHeight());
        nudgeRightTriangle.closeSubPath();
        g.fillPath(nudgeRightTriangle, AffineTransform::translation((float)nudgeRightButton.getX(), (float)nudgeRightButton.getY()));
    }
}

void GATE12AudioProcessorEditor::resized()
{
}
