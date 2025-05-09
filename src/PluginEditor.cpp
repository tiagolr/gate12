// Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Globals.h"

GATE12AudioProcessorEditor::GATE12AudioProcessorEditor (GATE12AudioProcessor& p)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
{
    MessageManager::callAsync([this]() {
        audioProcessor.loadSettings(); // load saved paint patterns from other plugin instances
    });
    audioProcessor.addChangeListener(this);
    audioProcessor.params.addParameterListener("sync", this);
    audioProcessor.params.addParameterListener("trigger", this);
    audioProcessor.params.addParameterListener("pattern", this);

    setResizable(true, false);
    setResizeLimits(PLUG_WIDTH, PLUG_HEIGHT, MAX_PLUG_WIDTH, MAX_PLUG_HEIGHT);

    setSize (audioProcessor.plugWidth, audioProcessor.plugHeight);
    setScaleFactor(audioProcessor.scale);
    auto col = PLUG_PADDING;
    auto row = PLUG_PADDING;

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
        std::ostringstream oss;
        auto points = audioProcessor.viewPattern->points;
        for (const auto& point : points) {
            oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
        }
        DBG(oss.str() << "\n");
    };
#endif

    addAndMakeVisible(syncMenu);
    syncMenu.setTooltip("Tempo sync");
    syncMenu.addItem("Rate Hz", 1);
    syncMenu.addSectionHeading("Straight");
    syncMenu.addItem("1/16", 2);
    syncMenu.addItem("1/8", 3);
    syncMenu.addItem("1/4", 4);
    syncMenu.addItem("1/2", 5);
    syncMenu.addItem("1 Bar", 6);
    syncMenu.addItem("2 Bars", 7);
    syncMenu.addItem("4 Bars", 8);
    syncMenu.addSectionHeading("Triplet");
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
    triggerLabel.setColour(juce::Label::ColourIds::textColourId, Colour(COLOR_NEUTRAL_LIGHT));
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
    algoMenu.setColour(ComboBox::arrowColourId, Colour(COLOR_AUDIO));
    algoMenu.setColour(ComboBox::textColourId, Colour(COLOR_AUDIO));
    algoMenu.setColour(ComboBox::outlineColourId, Colour(COLOR_AUDIO));
    algoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "algo", algoMenu);
    col += 100;

    addAndMakeVisible(audioSettingsButton);
    audioSettingsButton.setBounds(col, row, 25, 25);
    audioSettingsButton.onClick = [this]() {
        audioProcessor.showAudioKnobs = !audioProcessor.showAudioKnobs;
        toggleUIComponents();
    };
    audioSettingsButton.setAlpha(0.0f);
    col += 35;

    col = getWidth() - PLUG_PADDING - 25;
    settingsButton = std::make_unique<SettingsButton>(p);
    addAndMakeVisible(*settingsButton);
    settingsButton->onScaleChange = [this]() { setScaleFactor(audioProcessor.scale); };
    settingsButton->toggleUIComponents = [this]() { toggleUIComponents(); };
    settingsButton->toggleAbout = [this]() { about.get()->setVisible(true); };
    settingsButton->setBounds(col,row,25,25);

    // SECOND ROW

    row += 35;
    col = PLUG_PADDING;
    for (int i = 0; i < 12; ++i) {
        auto btn = std::make_unique<TextButton>(std::to_string(i + 1));
        btn->setRadioGroupId (1337);
        btn->setToggleState(audioProcessor.pattern->index == i, dontSendNotification);
        btn->setClickingTogglesState (false);
        btn->setColour (TextButton::textColourOffId,  Colour(COLOR_BG));
        btn->setColour (TextButton::textColourOnId,   Colour(COLOR_BG));
        btn->setColour (TextButton::buttonColourId,   Colour(COLOR_ACTIVE).darker(0.8f));
        btn->setColour (TextButton::buttonOnColourId, Colour(COLOR_ACTIVE));
        btn->setBounds (col + i * 22, row, 22+1, 25); // width +1 makes it seamless on higher DPI
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
    patSyncLabel.setColour(juce::Label::ColourIds::textColourId, Colour(COLOR_NEUTRAL_LIGHT));
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
    col = PLUG_PADDING;
    rate = std::make_unique<Rotary>(p, "rate", "Rate", LabelFormat::hz1f);
    addAndMakeVisible(*rate);
    rate->setBounds(col,row,80,65);
    col += 75;

    phase = std::make_unique<Rotary>(p, "phase", "Phase", LabelFormat::percx100);
    addAndMakeVisible(*phase);
    phase->setBounds(col,row,80,65);
    col += 75;

    min = std::make_unique<Rotary>(p, "min", "Min", LabelFormat::percx100);
    addAndMakeVisible(*min);
    min->setBounds(col,row,80,65);
    col += 75;

    max = std::make_unique<Rotary>(p, "max", "Max", LabelFormat::percx100);
    addAndMakeVisible(*max);
    max->setBounds(col,row,80,65);
    col += 75;

    smooth = std::make_unique<Rotary>(p, "smooth", "Smooth", LabelFormat::percx100);
    addAndMakeVisible(*smooth);
    smooth->setBounds(col,row,80,65);
    col += 75;

    attack = std::make_unique<Rotary>(p, "attack", "Attack", LabelFormat::percx100);
    addAndMakeVisible(*attack);
    attack->setBounds(col,row,80,65);
    col += 75;

    release = std::make_unique<Rotary>(p, "release", "Release", LabelFormat::percx100);
    addAndMakeVisible(*release);
    release->setBounds(col,row,80,65);
    col += 75;

    tension = std::make_unique<Rotary>(p, "tension", "Tension", LabelFormat::percx100, true);
    addAndMakeVisible(*tension);
    tension->setBounds(col,row,80,65);
    //col += 75;

    tensionatk = std::make_unique<Rotary>(p, "tensionatk", "T.Attack", LabelFormat::percx100, true);
    addAndMakeVisible(*tensionatk);
    tensionatk->setBounds(col,row,80,65);
    col += 75;

    tensionrel = std::make_unique<Rotary>(p, "tensionrel", "T.Release", LabelFormat::percx100, true);
    addAndMakeVisible(*tensionrel);
    tensionrel->setBounds(col,row,80,65);
    col += 75;

    // AUDIO KNOBS
    col = PLUG_PADDING;

    threshold = std::make_unique<Rotary>(p, "threshold", "Thres", LabelFormat::gainTodB1f, false, true);
    addAndMakeVisible(*threshold);
    threshold->setBounds(col,row,80,65);
    col += 75;

    sense = std::make_unique<Rotary>(p, "sense", "Sense", LabelFormat::percx100, false, true);
    addAndMakeVisible(*sense);
    sense->setBounds(col,row,80,65);
    col += 75;

    lowcut = std::make_unique<Rotary>(p, "lowcut", "Low Cut", LabelFormat::hzHp, false, true);
    addAndMakeVisible(*lowcut);
    lowcut->setBounds(col,row,80,65);
    col += 75;

    highcut = std::make_unique<Rotary>(p, "highcut", "Hi Cut", LabelFormat::hzLp, false, true);
    addAndMakeVisible(*highcut);
    highcut->setBounds(col,row,80,65);
    col += 75;

    offset = std::make_unique<Rotary>(p, "offset", "Offset", LabelFormat::audioOffset, true, true);
    addAndMakeVisible(*offset);
    offset->setBounds(col,row,80,65);
    col += 75;

    audioDisplay = std::make_unique<AudioDisplay>(p);
    addAndMakeVisible(*audioDisplay);
    audioDisplay->setBounds(col,row,getWidth() - col - PLUG_PADDING - 80 - 10, 65);

    col = getWidth() - PLUG_PADDING - 80;
    addAndMakeVisible(useSidechain);
    useSidechain.setTooltip("Use sidechain for transient detection");
    useSidechain.setButtonText("Sidechain");
    useSidechain.setComponentID("button");
    useSidechain.setColour(TextButton::buttonColourId, Colour(COLOR_AUDIO));
    useSidechain.setColour(TextButton::buttonOnColourId, Colour(COLOR_AUDIO));
    useSidechain.setColour(TextButton::textColourOnId, Colour(COLOR_BG));
    useSidechain.setColour(TextButton::textColourOffId, Colour(COLOR_AUDIO));
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
    useMonitor.setColour(TextButton::buttonColourId, Colour(COLOR_AUDIO));
    useMonitor.setColour(TextButton::buttonOnColourId, Colour(COLOR_AUDIO));
    useMonitor.setColour(TextButton::textColourOnId, Colour(COLOR_BG));
    useMonitor.setColour(TextButton::textColourOffId, Colour(COLOR_AUDIO));
    useMonitor.setBounds(col,row+35,80,25);
    useMonitor.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.useMonitor = !audioProcessor.useMonitor;
            toggleUIComponents();
        });
    };

    // THIRD ROW
    col = PLUG_PADDING;
    row += 75;
    addAndMakeVisible(paintButton);
    paintButton.setButtonText("Paint");
    paintButton.setComponentID("button");
    paintButton.setBounds(col, row, 90, 25);
    paintButton.onClick = [this]() {
        if (audioProcessor.isPaintEdit()) {
            audioProcessor.togglePaintEdit();
        }
        audioProcessor.showPaintWidget = !audioProcessor.showPaintWidget;
        toggleUIComponents();
    };
    col += 100;

    addAndMakeVisible(pointLabel);
    pointLabel.setText("Point", dontSendNotification);
    pointLabel.setColour(Label::textColourId, Colour(COLOR_NEUTRAL));
    pointLabel.setFont(FontOptions(16.f));
    pointLabel.setBounds(col,row,45,25);
    col += 55;

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
    loopButton.setTooltip("Toggle continuous play");
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
    col = getWidth() - PLUG_PADDING - 60;

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

    col -= 10+15+5;
    addAndMakeVisible(nudgeRightButton);
    nudgeRightButton.setAlpha(0.f);
    nudgeRightButton.setBounds(col, row, 20, 25);
    nudgeRightButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            double grid = (double)audioProcessor.getCurrentGrid();
            auto snapshot = audioProcessor.viewPattern->points;
            audioProcessor.viewPattern->rotate(1.0/grid);
            audioProcessor.viewPattern->buildSegments();
            audioProcessor.createUndoPointFromSnapshot(snapshot);
        });
    };

    col -= 10+15-5;
    addAndMakeVisible(nudgeLeftButton);
    nudgeLeftButton.setAlpha(0.f);
    nudgeLeftButton.setBounds(col, row, 20, 25);
    nudgeLeftButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            double grid = (double)audioProcessor.getCurrentGrid();
            auto snapshot = audioProcessor.viewPattern->points;
            audioProcessor.viewPattern->rotate(-1.0/grid);
            audioProcessor.viewPattern->buildSegments();
            audioProcessor.createUndoPointFromSnapshot(snapshot);
        });
    };

    col -= 30;
    addAndMakeVisible(redoButton);
    redoButton.setButtonText("redo");
    redoButton.setComponentID("button");
    redoButton.setBounds(col, row, 20, 25);
    redoButton.setAlpha(0.f);
    redoButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.viewPattern->redo();
            repaint();
        });
    };

    col -= 35;
    addAndMakeVisible(undoButton);
    undoButton.setButtonText("undo");
    undoButton.setComponentID("button");
    undoButton.setBounds(col, row, 20, 25);
    undoButton.setAlpha(0.f);
    undoButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.viewPattern->undo();
            repaint();
        });
    };
    row += 35;

    // PAINT TOOL
    col = PLUG_PADDING;
    addAndMakeVisible(paintEditButton);
    paintEditButton.setButtonText("Edit");
    paintEditButton.setComponentID("button");
    paintEditButton.setBounds(col, row+40/2-25/2, 60, 25);
    paintEditButton.onClick = [this]() {
        audioProcessor.togglePaintEdit();
    };

    col += 65;
    addAndMakeVisible(paintPrevButton);
    paintPrevButton.setBounds(col+2, row+2, 20, 20);
    paintPrevButton.setAlpha(0.f);
    paintPrevButton.onClick = [this]() {
        int page = audioProcessor.paintPage - 1;
        if (page < 0) page = 3;
        audioProcessor.paintPage = page;
        toggleUIComponents();
    };

    addAndMakeVisible(paintPageLabel);
    paintPageLabel.setText("16-24", dontSendNotification);
    paintPageLabel.setJustificationType(Justification::centred);
    paintPageLabel.setColour(Label::textColourId, Colour(COLOR_NEUTRAL));
    paintPageLabel.setBounds(col, row+25-2, 45, 16);

    col += 25;
    addAndMakeVisible(paintNextButton);
    paintNextButton.setBounds(col-2, row+2, 20, 20);
    paintNextButton.setAlpha(0.f);
    paintNextButton.onClick = [this]() {
        int page = audioProcessor.paintPage + 1;
        if (page > 3) page = 0;
        audioProcessor.paintPage = page;
        toggleUIComponents();
    };

    col += 25;
    paintWidget = std::make_unique<PaintToolWidget>(p);
    addAndMakeVisible(*paintWidget);
    paintWidget->setBounds(col,row,PLUG_WIDTH - PLUG_PADDING - col, 40);

    // VIEW
    col = 0;
    row += 50;
    view = std::make_unique<View>(p);
    addAndMakeVisible(*view);
    view->setBounds(col,row,getWidth(), getHeight() - row);

    addAndMakeVisible(latencyWarning);
    latencyWarning.setText("Plugin latency has changed, restart playback", dontSendNotification);
    latencyWarning.setColour(Label::backgroundColourId, Colours::black.withAlpha(0.5f));
    latencyWarning.setJustificationType(Justification::centred);
    latencyWarning.setColour(Label::textColourId, Colour(COLOR_ACTIVE));
    latencyWarning.setBounds(view->getBounds().getCentreX() - 150, PLUG_HEIGHT - 20 - 25, 300, 25);

    // ABOUT
    about = std::make_unique<About>();
    addAndMakeVisible(*about);
    about->setBounds(getBounds());
    about->setVisible(false);

    customLookAndFeel = new CustomLookAndFeel();
    setLookAndFeel(customLookAndFeel);

    init = true;
    resized();
    toggleUIComponents();
}

GATE12AudioProcessorEditor::~GATE12AudioProcessorEditor()
{
    audioProcessor.saveSettings(); // save paint patterns to disk
    setLookAndFeel(nullptr);
    delete customLookAndFeel;
    audioProcessor.params.removeParameterListener("sync", this);
    audioProcessor.params.removeParameterListener("trigger", this);
    audioProcessor.params.removeParameterListener("pattern", this);
    audioProcessor.removeChangeListener(this);
}

void GATE12AudioProcessorEditor::changeListenerCallback(ChangeBroadcaster* source)
{
    (void)source;
    toggleUIComponents();
}

void GATE12AudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "pattern") {
        patterns[(int)newValue - 1].get()->setToggleState(true, dontSendNotification);
    }

    toggleUIComponents();
};

void GATE12AudioProcessorEditor::toggleUIComponents()
{
    auto trigger = (int)audioProcessor.params.getRawParameterValue("trigger")->load();
    auto triggerColor = trigger == 0 ? COLOR_ACTIVE : trigger == 1 ? COLOR_MIDI : COLOR_AUDIO;
    triggerMenu.setColour(ComboBox::arrowColourId, Colour(triggerColor));
    triggerMenu.setColour(ComboBox::textColourId, Colour(triggerColor));
    triggerMenu.setColour(ComboBox::outlineColourId, Colour(triggerColor));
    algoMenu.setVisible(trigger == Trigger::audio);
    audioSettingsButton.setVisible(trigger == Trigger::audio);
    if (!audioSettingsButton.isVisible()) {
        audioProcessor.showAudioKnobs = false;
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
    tension->setVisible(!showAudioKnobs && !audioProcessor.dualTension);
    tensionatk->setVisible(!showAudioKnobs && audioProcessor.dualTension);
    tensionrel->setVisible(!showAudioKnobs && audioProcessor.dualTension);

    threshold->setVisible(showAudioKnobs);
    sense->setVisible(showAudioKnobs);
    lowcut->setVisible(showAudioKnobs);
    highcut->setVisible(showAudioKnobs);
    offset->setVisible(showAudioKnobs);
    audioDisplay->setVisible(showAudioKnobs);

    if (!showAudioKnobs) {
        auto col = PLUG_PADDING;
        auto row = PLUG_PADDING + 35 + 35;
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
        tensionatk->setTopLeftPosition(col, row);
        col += 75;
        tensionrel->setTopLeftPosition(col, row);
    }

    useSidechain.setVisible(showAudioKnobs);
    useMonitor.setVisible(showAudioKnobs);
    useSidechain.setToggleState(audioProcessor.useSidechain, dontSendNotification);
    useMonitor.setToggleState(audioProcessor.useMonitor, dontSendNotification);

    latencyWarning.setVisible(audioProcessor.showLatencyWarning);

    paintWidget->setVisible(audioProcessor.showPaintWidget);
    view->setBounds(view->getBounds().withTop(paintWidget->isVisible() 
        ? paintWidget->getBounds().getBottom()
        : paintWidget->getBounds().getY() - 10)
    );

    paintButton.setToggleState(audioProcessor.showPaintWidget, dontSendNotification);
    paintEditButton.setVisible(audioProcessor.showPaintWidget);
    paintEditButton.setToggleState(audioProcessor.isPaintEdit(), dontSendNotification);
    int firstPaintPat = audioProcessor.paintPage * 8 + 1;
    paintPageLabel.setText(String(firstPaintPat) + "-" + String(firstPaintPat+7), dontSendNotification);
    paintPageLabel.setVisible(audioProcessor.showPaintWidget);

    repaint();
}

//==============================================================================

void GATE12AudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(Colour(COLOR_BG));
    auto trigger = (int)audioProcessor.params.getRawParameterValue("trigger")->load();

    // draw loop play button
    if (trigger != Trigger::sync) {
        if (audioProcessor.alwaysPlaying) {
            g.setColour(Colours::yellow);
            auto loopBounds = loopButton.getBounds().expanded(-5);
            g.fillRect(loopBounds.removeFromLeft(5));
            loopBounds = loopButton.getBounds().expanded(-5);
            g.fillRect(loopBounds.removeFromRight(5));
        }
        else {
            g.setColour(Colour(0xff00ff00));
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
    g.setColour(Colour(COLOR_ACTIVE));
    if (audioSettingsButton.isVisible() && audioProcessor.showAudioKnobs) {
        g.setColour(Colour(COLOR_AUDIO));
        g.fillRoundedRectangle(audioSettingsButton.getBounds().toFloat(), 3.0f);
        drawGear(g, audioSettingsButton.getBounds(), 10, 6, Colour(COLOR_BG), Colour(COLOR_AUDIO));
    }
    else if (audioSettingsButton.isVisible()) {
        drawGear(g, audioSettingsButton.getBounds(), 10, 6, Colour(COLOR_AUDIO), Colour(COLOR_BG));
    }

    // draw rotate pat triangles
    g.setColour(Colour(COLOR_ACTIVE));
    auto triCenter = nudgeLeftButton.getBounds().toFloat().getCentre();
    auto triRadius = 5.f;
    juce::Path nudgeLeftTriangle;
    nudgeLeftTriangle.addTriangle(
        triCenter.translated(-triRadius, 0),
        triCenter.translated(triRadius, -triRadius),
        triCenter.translated(triRadius, triRadius)
    );
    g.fillPath(nudgeLeftTriangle);

    triCenter = nudgeRightButton.getBounds().toFloat().getCentre();
    juce::Path nudgeRightTriangle;
    nudgeRightTriangle.addTriangle(
        triCenter.translated(-triRadius, -triRadius),
        triCenter.translated(-triRadius, triRadius),
        triCenter.translated(triRadius, 0)
    );
    g.fillPath(nudgeRightTriangle);

    // draw rotate paint page triangles
    if (audioProcessor.showPaintWidget) {
        g.setColour(Colour(COLOR_ACTIVE));
        triCenter = paintNextButton.getBounds().toFloat().getCentre();
        triRadius = 5.f;
        juce::Path paintNextTriangle;
        paintNextTriangle.addTriangle(
            triCenter.translated(-triRadius, -triRadius),
            triCenter.translated(-triRadius, triRadius),
            triCenter.translated(triRadius, 0)
        );
        g.fillPath(paintNextTriangle);

        triCenter = paintPrevButton.getBounds().toFloat().getCentre();
        juce::Path paintPrevTriangle;
        paintPrevTriangle.addTriangle(
            triCenter.translated(-triRadius, 0),
            triCenter.translated(triRadius, -triRadius),
            triCenter.translated(triRadius, triRadius)
        );
        g.fillPath(paintPrevTriangle);
    }   

    // draw undo redo buttons
    auto canUndo = !audioProcessor.viewPattern->undoStack.empty();
    auto canRedo = !audioProcessor.viewPattern->redoStack.empty();
    drawUndoButton(g, undoButton.getBounds().toFloat(), true, Colour(canUndo ? COLOR_ACTIVE : COLOR_NEUTRAL));
    drawUndoButton(g, redoButton.getBounds().toFloat(), false, Colour(canRedo ? COLOR_ACTIVE : COLOR_NEUTRAL));
}

void GATE12AudioProcessorEditor::drawGear(Graphics& g, Rectangle<int> bounds, float radius, int segs, Colour color, Colour background)
{
    float x = bounds.toFloat().getCentreX();
    float y = bounds.toFloat().getCentreY();
    float oradius = radius;
    float iradius = radius / 3.f;
    float cradius = iradius / 1.5f; 
    float coffset = MathConstants<float>::twoPi;
    float inc = MathConstants<float>::twoPi / segs;

    g.setColour(color);
    g.fillEllipse(x-oradius,y-oradius,oradius*2.f,oradius*2.f);

    g.setColour(background);
    for (int i = 0; i < segs; i++) {
        float angle = coffset + i * inc;
        float cx = x + std::cos(angle) * oradius;
        float cy = y + std::sin(angle) * oradius;
        g.fillEllipse(cx - cradius, cy - cradius, cradius * 2, cradius * 2);
    }
    g.fillEllipse(x-iradius, y-iradius, iradius*2.f, iradius*2.f);
}

void GATE12AudioProcessorEditor::drawUndoButton(Graphics& g, juce::Rectangle<float> area, bool invertx, Colour color)
{
        auto bounds = area;
        auto thickness = 2.f;
        float left = bounds.getX();
        float right = bounds.getRight();
        float top = bounds.getCentreY() - 4;
        float bottom = bounds.getCentreY() + 4;
        float centerY = bounds.getCentreY();
        float shaftStart = right - 7;

        Path arrowPath;
        // arrow head
        arrowPath.startNewSubPath(right, centerY);
        arrowPath.lineTo(shaftStart, top);
        arrowPath.startNewSubPath(right, centerY);
        arrowPath.lineTo(shaftStart, bottom);

        // shaft
        float radius = (bottom - centerY);
        arrowPath.startNewSubPath(right, centerY);
        arrowPath.lineTo(left + radius - 1, centerY);

        // semi circle
        arrowPath.startNewSubPath(left + radius, centerY);
        arrowPath.addArc(left, centerY, radius, radius, 2.f * float_Pi, float_Pi);

        if (invertx) {
            AffineTransform flipTransform = AffineTransform::scale(-1.0f, 1.0f)
                .translated(bounds.getWidth(), 0);

            // First move the path to origin, apply transform, then move back
            arrowPath.applyTransform(AffineTransform::translation(-bounds.getPosition()));
            arrowPath.applyTransform(flipTransform);
            arrowPath.applyTransform(AffineTransform::translation(bounds.getPosition()));
        }

        g.setColour(color);
        g.strokePath(arrowPath, PathStrokeType(thickness));
}

void GATE12AudioProcessorEditor::resized()
{
    if (!init) return; // defer resized() call during constructor

    // layout right aligned components and view
    // first row
    auto col = getWidth() - PLUG_PADDING;
    auto bounds = settingsButton->getBounds();
    settingsButton->setBounds(bounds.withX(col - bounds.getWidth()));

    // knobs row
    bounds = audioDisplay->getBounds();
    audioDisplay->setBounds(bounds.withRight(col - useSidechain.getBounds().getWidth() - 10));
    bounds = useSidechain.getBounds();
    useSidechain.setBounds(bounds.withX(col - bounds.getWidth()));
    bounds = useMonitor.getBounds();
    useMonitor.setBounds(bounds.withX(col - bounds.getWidth()));

    // third row
    bounds = snapButton.getBounds();
    auto dx = (col - bounds.getWidth()) - bounds.getX();
    snapButton.setBounds(snapButton.getBounds().translated(dx, 0));
    gridSelector->setBounds(gridSelector->getBounds().translated(dx, 0));
    nudgeLeftButton.setBounds(nudgeLeftButton.getBounds().translated(dx, 0));
    nudgeRightButton.setBounds(nudgeRightButton.getBounds().translated(dx, 0));
    redoButton.setBounds(redoButton.getBounds().translated(dx, 0));
    undoButton.setBounds(undoButton.getBounds().translated(dx, 0));

    // view
    bounds = view->getBounds();
    view->setBounds(bounds.withWidth(getWidth()).withHeight(getHeight() - bounds.getY()));

    bounds = latencyWarning.getBounds();
    latencyWarning.setBounds(bounds
        .withX(view->getBounds().getCentreX() - bounds.getWidth() / 2)
        .withY(getHeight() - 20 - bounds.getHeight())
    );

    audioProcessor.plugWidth = getWidth();
    audioProcessor.plugHeight = getHeight();
}
