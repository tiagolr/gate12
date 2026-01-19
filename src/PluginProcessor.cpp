 // Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <ctime>

GATE12AudioProcessor::GATE12AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    )
    , settings{}
    , params(*this, &undoManager, "PARAMETERS", {
        std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0f, 1.0f, 1.0f),
        std::make_unique<juce::AudioParameterInt>("pattern", "Pattern", 1, 12, 1),
        std::make_unique<juce::AudioParameterChoice>("patsync", "Pattern Sync", StringArray { "Off", "1/4 Beat", "1/2 Beat", "1 Beat", "2 Beats", "4 Beats"}, 0),
        std::make_unique<juce::AudioParameterChoice>("trigger", "Trigger", StringArray { "Sync", "MIDI", "Audio", "Free"}, 0),
        std::make_unique<juce::AudioParameterChoice>("sync", "Sync", StringArray { "Rate Hz", "1/256", "1/128", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "1/16t", "1/8t", "1/4t", "1/2t", "1/1t", "1/16.", "1/8.", "1/4.", "1/2.", "1/1." }, 9),
        std::make_unique<juce::AudioParameterFloat>("rate", "Rate Hz", juce::NormalisableRange<float>(0.01f, 5000.0f, 0.00001f, 0.2f), 1.0f),
        std::make_unique<juce::AudioParameterFloat>("phase", "Phase", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("min", "Min", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("max", "Max", 0.0f, 1.0f, 1.0f),
        std::make_unique<juce::AudioParameterFloat>("smooth", "Smooth", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("attack", "Attack", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("release", "Release", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tension", "Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tensionatk", "Attack Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tensionrel", "Release Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("stereo", "Stereo Offset", juce::NormalisableRange<float>(-180.f, 180.f, 1.f), 0.f),
        std::make_unique<juce::AudioParameterFloat>("split_low", "Split Low", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.35f), 20.f),
        std::make_unique<juce::AudioParameterFloat>("split_high", "Split High", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.35f), 20000.f),
        std::make_unique<juce::AudioParameterChoice>("split_slope", "Split Slope", StringArray{"6dB", "12dB"}, 0),
        std::make_unique<juce::AudioParameterBool>("snap", "Snap", false),
        std::make_unique<juce::AudioParameterInt>("grid", "Grid", 0, (int)std::size(GRID_SIZES)-1, 2),
        std::make_unique<juce::AudioParameterInt>("seqstep", "Sequencer Step", 0, (int)std::size(GRID_SIZES)-1, 2),
        // audio trigger params
        std::make_unique<juce::AudioParameterChoice>("algo", "Audio Algorithm", StringArray { "Simple", "Drums" }, 0),
        std::make_unique<juce::AudioParameterFloat>("threshold", "Audio Threshold", NormalisableRange<float>(0.0f, 1.0f), 0.5f),
        std::make_unique<juce::AudioParameterFloat>("sense", "Audio Sensitivity", 0.0f, 1.0f, 0.5f),
        std::make_unique<juce::AudioParameterFloat>("lowcut", "Audio LowCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.3f) , 20.f),
        std::make_unique<juce::AudioParameterFloat>("highcut", "Audio HighCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.3f) , 20000.f),
        std::make_unique<juce::AudioParameterFloat>("offset", "Audio Offset", -1.0f, 1.0f, 0.0f)
    })
#endif
{
    srand(static_cast<unsigned int>(time(nullptr))); // seed random generator
    juce::PropertiesFile::Options options{};
    options.applicationName = ProjectInfo::projectName;
    options.filenameSuffix = ".settings";
#if defined(JUCE_LINUX) || defined(JUCE_BSD)
    options.folderName = "~/.config/gate12";
#endif
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = PropertiesFile::storeAsXML;
    settings.setStorageParameters(options);

    for (auto* param : getParameters()) {
        param->addListener(this);
    }

    params.addParameterListener("pattern", this);

    // init patterns
    for (int i = 0; i < 12; ++i) {
        patterns[i] = new Pattern(i);
        patterns[i]->insertPoint(0, 1, 0, 1);
        patterns[i]->insertPoint(0.5, 0, 0, 1);
        patterns[i]->insertPoint(1, 1, 0, 1);
        patterns[i]->buildSegments();
    }

    // init paintMode Patterns
    for (int i = 0; i < PAINT_PATS; ++i) {
        paintPatterns[i] = new Pattern(i + PAINT_PATS_IDX);
        if (i < 8) {
            auto preset = Presets::getPaintPreset(i);
            for (auto& point : preset) {
                paintPatterns[i]->insertPoint(point.x, point.y, point.tension, point.type);
            }
        }
        else {
            paintPatterns[i]->insertPoint(0.0, 1.0, 0.0, 1);
            paintPatterns[i]->insertPoint(1.0, 0.0, 0.0, 1);
        }
        paintPatterns[i]->buildSegments();
    }

    sequencer = new Sequencer(*this);
    pattern = patterns[0];
    viewPattern = pattern;
    preSamples.resize(MAX_PLUG_WIDTH, 0); // samples array size must be >= viewport width
    postSamples.resize(MAX_PLUG_WIDTH, 0);
    sideSamples.resize(MAX_PLUG_WIDTH, 0);
    monSamples.resize(MAX_PLUG_WIDTH, 0); // samples array size must be >= audio monitor width
    value = new RCSmoother();
    value2 = new RCSmoother();

    loadSettings();
}

GATE12AudioProcessor::~GATE12AudioProcessor()
{
    params.removeParameterListener("pattern", this);
}

void GATE12AudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "pattern") {
        int pat = (int)newValue;
        if (pat != pattern->index + 1 && pat != queuedPattern) {
            queuePattern(pat);
        }
    }
}

void GATE12AudioProcessor::parameterValueChanged (int parameterIndex, float newValue)
{
    (void)newValue;
    (void)parameterIndex;
    paramChanged = true;
}

void GATE12AudioProcessor::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    (void)parameterIndex;
    (void)gestureIsStarting;
}

void GATE12AudioProcessor::loadSettings ()
{
    settings.closeFiles(); // FIX files changed by other plugin instances not loading
    if (auto* file = settings.getUserSettings()) {
        scale = (float)file->getDoubleValue("scale", 1.0f);
        plugWidth = file->getIntValue("width", PLUG_WIDTH);
        plugHeight = file->getIntValue("height", PLUG_HEIGHT);
        auto tensionparam = (double)params.getRawParameterValue("tension")->load();
        auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
        auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();

        for (int i = 0; i < PAINT_PATS; ++i) {
            auto str = file->getValue("paintpat" + String(i),"").toStdString();
            if (!str.empty()) {
                paintPatterns[i]->clear();
                paintPatterns[i]->clearUndo();
                double x, y, tension;
                int type;
                std::istringstream iss(str);
                while (iss >> x >> y >> tension >> type) {
                    paintPatterns[i]->insertPoint(x,y,tension,type);
                }
                paintPatterns[i]->setTension(tensionparam, tensionatk, tensionrel, dualTension);
                paintPatterns[i]->buildSegments();
            }
        }
    }
}

void GATE12AudioProcessor::saveSettings ()
{
    settings.closeFiles(); // FIX files changed by other plugin instances not loading
    if (auto* file = settings.getUserSettings()) {
        file->setValue("scale", scale);
        file->setValue("width", plugWidth);
        file->setValue("height", plugHeight);
        for (int i = 0; i < PAINT_PATS; ++i) {
            std::ostringstream oss;
            auto points = paintPatterns[i]->points;
            for (const auto& point : points) {
                oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
            }
            file->setValue("paintpat"+juce::String(i), var(oss.str()));
        }
    }
    settings.saveIfNeeded();
}

void GATE12AudioProcessor::setScale(float s)
{
    scale = s;
    saveSettings();
}

int GATE12AudioProcessor::getCurrentGrid()
{
    auto gridIndex = (int)params.getRawParameterValue("grid")->load();
    return GRID_SIZES[gridIndex];
}

int GATE12AudioProcessor::getCurrentSeqStep()
{
    auto gridIndex = (int)params.getRawParameterValue("seqstep")->load();
    return GRID_SIZES[gridIndex];
}

void GATE12AudioProcessor::createUndoPoint(int patindex)
{
    if (patindex == -1) {
        viewPattern->createUndo();
    }
    else {
        if (patindex < 12) {
            patterns[patindex]->createUndo();
        }
        else {
            paintPatterns[patindex - 100]->createUndo();
        }
    }
    sendChangeMessage(); // UI repaint
}

/*
    Used to create an undo point from a previously saved state
    Assigns the snapshot points to the pattern temporarily
    Creates an undo point and finally replaces back the points
*/
void GATE12AudioProcessor::createUndoPointFromSnapshot(std::vector<PPoint> snapshot)
{
    if (!Pattern::comparePoints(snapshot, viewPattern->points)) {
        auto points = viewPattern->points;
        viewPattern->points = snapshot;
        createUndoPoint();
        viewPattern->points = points;
    }
}

void GATE12AudioProcessor::setUIMode(UIMode mode)
{
    MessageManager::callAsync([this, mode]() {
        if ((mode != Seq && mode != PaintEdit) && sequencer->isOpen) {
            sequencer->close();
        }

        if (mode == UIMode::Normal) {
            viewPattern = pattern;
            showSequencer = false;
            showPaintWidget = false;
        }
        else if (mode == UIMode::Paint) {
            viewPattern = pattern;
            showPaintWidget = true;
            showSequencer = false;
        }
        else if (mode == UIMode::PaintEdit) {
            viewPattern = paintPatterns[paintTool];
            showPaintWidget = true;
            showSequencer = false;
        }
        else if (mode == UIMode::Seq) {
            if (sequencer->isOpen) {
                sequencer->close(); // just in case its changing from PaintEdit back to sequencer
            }
            sequencer->open();
            viewPattern = pattern;
            showPaintWidget = sequencer->selectedShape == CellShape::SPTool;
            showSequencer = true;
        }
        luimode = uimode;
        uimode = mode;
        sendChangeMessage();
    });
}

void GATE12AudioProcessor::togglePaintMode()
{
    setUIMode(uimode == UIMode::Paint
        ? UIMode::Normal
        : UIMode::Paint
    );
}

void GATE12AudioProcessor::togglePaintEditMode()
{
    setUIMode(uimode == UIMode::PaintEdit
        ? luimode
        : UIMode::PaintEdit
    );
}

void GATE12AudioProcessor::toggleSequencerMode()
{
    setUIMode(uimode == UIMode::Seq
        ? UIMode::Normal
        : UIMode::Seq
    );
}

Pattern* GATE12AudioProcessor::getPaintPatern(int index)
{
    return paintPatterns[index];
}

void GATE12AudioProcessor::setViewPattern(int index)
{
    if (index >= 0 && index < 12) {
        viewPattern = patterns[index];
    }
    else if (index >= PAINT_PATS && index < PAINT_PATS_IDX + PAINT_PATS) {
        viewPattern = paintPatterns[index - PAINT_PATS_IDX];
    }
    sendChangeMessage();
}

void GATE12AudioProcessor::setPaintTool(int index)
{
    paintTool = index;
    if (uimode == UIMode::PaintEdit) {
        viewPattern = paintPatterns[index];
        sendChangeMessage();
    }
}

void GATE12AudioProcessor::restorePaintPatterns()
{
    for (int i = 0; i < 8; ++i) {
        paintPatterns[i]->clear();
        paintPatterns[i]->clearUndo();
        auto preset = Presets::getPaintPreset(i);
        for (auto& point : preset) {
            paintPatterns[i]->insertPoint(point.x, point.y, point.tension, point.type);
        }
        paintPatterns[i]->buildSegments();
    }
    sendChangeMessage();
}

//==============================================================================
const juce::String GATE12AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GATE12AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GATE12AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GATE12AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GATE12AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GATE12AudioProcessor::getNumPrograms()
{
    return 40;
}

int GATE12AudioProcessor::getCurrentProgram()
{
    return currentProgram == -1 ? 0 : currentProgram;
}

void GATE12AudioProcessor::setCurrentProgram (int index)
{
    if (currentProgram == index) return;
    loadProgram(index);
}

void GATE12AudioProcessor::loadProgram (int index)
{
    if (sequencer->isOpen)
        sequencer->close();

    currentProgram = index;
    auto loadPreset = [](Pattern& pat, int idx) {
        auto preset = Presets::getPreset(idx);
        pat.clear();
        for (auto p = preset.begin(); p < preset.end(); ++p) {
            pat.insertPoint(p->x, p->y, p->tension, p->type);
        }
        pat.buildSegments();
        pat.clearUndo();
    };

    if (index == 0) { // Init
        for (int i = 0; i < 12; ++i) {
            patterns[i]->loadTriangle();
            patterns[i]->buildSegments();
            patterns[i]->clearUndo();
        }
    }
    else if (index == 1 || index == 14 || index == 27) {
        for (int i = 0; i < 12; ++i) {
            loadPreset(*patterns[i], index + i);
        }
    }
    else {
        loadPreset(*pattern, index - 1);
    }

    setUIMode(UIMode::Normal);
    sendChangeMessage(); // UI Repaint
}

const juce::String GATE12AudioProcessor::getProgramName (int index)
{
    static const std::array<juce::String, 40> progNames = {
        "Init",
        "Load Patterns 01-12", "Empty", "Gate 2", "Gate 4", "Gate 8", "Gate 12", "Gate 16", "Gate 24", "Gate 32", "Trance 1", "Trance 2", "Trance 3", "Trance 4",
        "Load Patterns 13-25", "Saw 1", "Saw 2", "Step 1", "Step 1 FadeIn", "Step 4 Gate", "Off Beat", "Dynamic 1/4", "Swing", "Gate Out", "Gate In", "Speed up", "Speed Down",
        "Load Patterns 26-38", "End Fade", "End Gate", "Tremolo Slow", "Tremolo Fast", "Sidechain", "Drum Loop", "Copter", "AM", "Fade In", "Fade Out", "Fade OutIn", "Mute"
    };
    return progNames.at(index);
}

void GATE12AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    (void)index;
    (void)newName;
}

//==============================================================================
void GATE12AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    srate = (float)sampleRate;
    rawBuffer.setSize(2, samplesPerBlock);
    lowBuffer.setSize(2, samplesPerBlock);
    highBuffer.setSize(2, samplesPerBlock);
    lpFilterL.clear(0.0);
    lpFilterR.clear(0.0);
    hpFilterL.clear(0.0);
    hpFilterR.clear(0.0);
    transDetectorL.clear(sampleRate);
    transDetectorR.clear(sampleRate);
    std::fill(monSamples.begin(), monSamples.end(), 0.0);
    onSlider(); // sets latency on first run
}

void GATE12AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GATE12AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void GATE12AudioProcessor::onSlider()
{
    setSmooth();
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    if (trigger != ltrigger || antiClick != lantiClick) {
        auto latency = getLatencySamples();
        int antiClickLatency = getAntiClickLatency();
        setLatencySamples(trigger == Trigger::Audio
            ? int(srate * AUDIO_LATENCY_MILLIS / 1000.0) + antiClickLatency
            : trigger == MIDI ? antiClickLatency
            : 0
        );
        if (getLatencySamples() != latency && playing) {
            showLatencyWarning = true;
            MessageManager::callAsync([this]() { sendChangeMessage(); });
        }
        clearLatencyBuffers();
        ltrigger = trigger;
        lantiClick = antiClick;
    }
    if (trigger == Trigger::Sync && alwaysPlaying)
        alwaysPlaying = false; // force alwaysPlaying off when trigger is not MIDI or Audio

    if (trigger != Trigger::MIDI && midiTrigger)
        midiTrigger = false;

    if (trigger != Trigger::Audio && audioTrigger)
        audioTrigger = false;

    auto tension = (double)params.getRawParameterValue("tension")->load();
    auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
    auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
    if (tension != ltension || tensionatk != ltensionatk || tensionrel != ltensionrel) {
        onTensionChange();
        ltensionatk = tensionatk;
        ltensionrel = tensionrel;
        ltension = tension;
    }

    auto sync = (int)params.getRawParameterValue("sync")->load();
    if (sync == 0) syncQN = 1.; // not used
    else if (sync == 1) syncQN = 1. / 64.; // 1/256
    else if (sync == 2) syncQN = 1. / 32.; // 1/128
    else if (sync == 3) syncQN = 1. / 16.; // 1/64
    else if (sync == 4) syncQN = 1. / 8.; // 1/32
    else if (sync == 5) syncQN = 1. / 4.; // 1/16
    else if (sync == 6) syncQN = 1. / 2.; // 1/8
    else if (sync == 7) syncQN = 1. / 1.; // 1/4
    else if (sync == 8) syncQN = 1. * 2.; // 1/2
    else if (sync == 9) syncQN = 1. * 4.; // 1bar
    else if (sync == 10) syncQN = 1. * 8.; // 2bar
    else if (sync == 11) syncQN = 1. * 16.; // 4bar
    else if (sync == 12) syncQN = 1. / 6.; // 1/16t
    else if (sync == 13) syncQN = 1. / 3.; // 1/8t
    else if (sync == 14) syncQN = 2. / 3.; // 1/4t
    else if (sync == 15) syncQN = 4. / 3.; // 1/2t
    else if (sync == 16) syncQN = 8. / 3.; // 1/1t
    else if (sync == 17) syncQN = 1. / 4. * 1.5; // 1/16.
    else if (sync == 18) syncQN = 1. / 2. * 1.5; // 1/8.
    else if (sync == 19) syncQN = 1. / 1. * 1.5; // 1/4.
    else if (sync == 20) syncQN = 2. / 1. * 1.5; // 1/2.
    else if (sync == 21) syncQN = 4. / 1. * 1.5; // 1/1.

    auto highcut = (double)params.getRawParameterValue("highcut")->load();
    auto lowcut = (double)params.getRawParameterValue("lowcut")->load();
    lpFilterL.lp(srate, highcut, 0.707);
    lpFilterR.lp(srate, highcut, 0.707);
    hpFilterL.hp(srate, lowcut, 0.707);
    hpFilterR.hp(srate, lowcut, 0.707);

    float splitLow = params.getRawParameterValue("split_low")->load();
    float splitHigh = params.getRawParameterValue("split_high")->load();
    splitter.setFreqs((float)srate, splitLow, splitHigh);
}

void GATE12AudioProcessor::onTensionChange()
{
    auto tension = (double)params.getRawParameterValue("tension")->load();
    auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
    auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
    pattern->setTension(tension, tensionatk, tensionrel, dualTension);
    pattern->buildSegments();
    for (int i = 0; i < PAINT_PATS; ++i) {
        paintPatterns[i]->setTension(tension, tensionatk, tensionrel, dualTension);
        paintPatterns[i]->buildSegments();
    }
}

void GATE12AudioProcessor::onPlay()
{
    clearDrawBuffers();
    clearLatencyBuffers();
    splitter.clear();
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    double ratehz = (double)params.getRawParameterValue("rate")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();

    if (trigger == Trigger::Free)
        return;

    midiTrigger = false;
    audioTrigger = false;

    beatPos = ppqPosition;
    ratePos = beatPos * secondsPerBeat * ratehz;
    trigpos = 0.0;
    trigposSinceHit = 1.0;
    trigphase = phase;

    audioTriggerCountdown = -1;
    transDetectorL.clear((double)srate);
    transDetectorR.clear((double)srate);

    if (trigger == Trigger::Sync || alwaysPlaying) {
        restartEnv(false);
    }
}

void GATE12AudioProcessor::restartEnv(bool fromZero)
{
    int sync = (int)params.getRawParameterValue("sync")->load();
    double min = (double)params.getRawParameterValue("min")->load();
    double max = (double)params.getRawParameterValue("max")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();
    double stereo = (double)params.getRawParameterValue("stereo")->load() / 360.0;

    if (fromZero) { // restart from phase
        xpos = phase;
    }
    else { // restart from beat pos
        xpos = sync > 0
            ? beatPos / syncQN + phase
            : ratePos + phase;
        xpos -= std::floor(xpos);
    }

    xpos2 = xpos + stereo;
    if (xpos2 < 0.0) xpos2 += 1;
    if (xpos2 > 1.0) xpos2 -= std::floor(xpos2);

    value->reset(getY(xpos, min, max)); // reset smooth
    value2->reset(getY(xpos2, min, max)); // reset smooth
}

void GATE12AudioProcessor::onStop()
{
    if (showLatencyWarning) {
        showLatencyWarning = false;
        MessageManager::callAsync([this]() { sendChangeMessage(); });
    }
}

void GATE12AudioProcessor::clearDrawBuffers()
{
    std::fill(preSamples.begin(), preSamples.end(), 0.0);
    std::fill(postSamples.begin(), postSamples.end(), 0.0);
    std::fill(sideSamples.begin(), sideSamples.end(), 0.0);
}

void GATE12AudioProcessor::clearLatencyBuffers()
{
    auto latency = getLatencySamples();
    latBufferL.resize(latency, 0.0);
    latBufferR.resize(latency, 0.0);
    latMonitorBufferL.resize(latency, 0.0);
    latMonitorBufferR.resize(latency, 0.0);
    latpos = 0;
}

void GATE12AudioProcessor::toggleUseSidechain()
{
    useSidechain = !useSidechain;
    hpFilterL.clear(0.0);
    hpFilterR.clear(0.0);
    lpFilterL.clear(0.0);
    lpFilterR.clear(0.0);
}

void GATE12AudioProcessor::toggleMonitorSidechain()
{
    useMonitor = !useMonitor;
    hpFilterL.clear(0.0);
    hpFilterR.clear(0.0);
    lpFilterL.clear(0.0);
    lpFilterR.clear(0.0);
}

double inline GATE12AudioProcessor::getY(double x, double min, double max)
{
    return min + (max - min) * (1 - pattern->get_y_at(x));
}

void GATE12AudioProcessor::setSmooth()
{
    float attack = 0;
    float release = 0;

    if (dualSmooth) {
        attack = params.getRawParameterValue("attack")->load();
        release = params.getRawParameterValue("release")->load();
    }
    else {
        float smooth = params.getRawParameterValue("smooth")->load();
        attack = smooth;
        release = smooth;
    }

    attack *= attack;
    release *= release;
    value->setup(attack * 0.25, release * 0.25, (double)srate);
    value2->setup(attack * 0.25, release * 0.25, (double)srate);
}

void GATE12AudioProcessor::startMidiTrigger()
{
    double phase = (double)params.getRawParameterValue("phase")->load();
    double min = (double)params.getRawParameterValue("min")->load();
    double max = (double)params.getRawParameterValue("max")->load();
    antiClickCooldown = getAntiClickLatency();
    antiClickSamples = antiClickCooldown;
    antiClickStart = ypos;
    antiClickTarget = getY(phase, min, max);
}

void GATE12AudioProcessor::queuePattern(int patidx)
{
    queuedPattern = patidx;
    queuedPatternCountdown = 0;
    int patsync = (int)params.getRawParameterValue("patsync")->load();

    if (playing && patsync != PatSync::Off) {
        int interval = samplesPerBeat;
        if (patsync == PatSync::QuarterBeat)
            interval = interval / 4;
        else if (patsync == PatSync::HalfBeat)
            interval = interval / 2;
        else if (patsync == PatSync::Beat_x2)
            interval = interval * 2;
        else if (patsync == PatSync::Beat_x4)
            interval = interval * 4;
        queuedPatternCountdown = (interval - timeInSamples % interval) % interval;
    }
}

void GATE12AudioProcessor::setAntiClick(int ac)
{
    jassert(ac >= 0 && ac <= 2);
    antiClick = ac;
    paramChanged = true;
}

int GATE12AudioProcessor::getAntiClickLatency()
{
    return antiClick == 1
        ? (int)(ANTICLICK_LOW_MILLIS / 1000.0)
        : antiClick == 2
        ? (int)(ANTICLICK_HIGH_MILLIS / 1000.0)
        : 0;
}

bool GATE12AudioProcessor::supportsDoublePrecisionProcessing() const
{
    return false;
}

void GATE12AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals disableDenormals;
    int sblock = getBlockSize();
    bool looping = false;
    double loopStart = 0.0;
    double loopEnd = 0.0;

    // Get playhead info
    if (auto* phead = getPlayHead()) {
        if (auto pos = phead->getPosition()) {
            if (auto ppq = pos->getPpqPosition())
                ppqPosition = *ppq;
            if (auto tempo = pos->getBpm()) {
                beatsPerSecond = *tempo / 60.0;
                beatsPerSample = *tempo / (60.0 * srate);
                samplesPerBeat = (int)((60.0 / *tempo) * srate);
                secondsPerBeat = 60.0 / *tempo;
            }
            looping = pos->getIsLooping();
            if (auto loopPoints = pos->getLoopPoints()) {
                loopStart = loopPoints->ppqStart;
                loopEnd = loopPoints->ppqEnd;
            }
            auto play = pos->getIsPlaying();
            if (!playing && play) // playback started
                onPlay();
            else if (playing && !play) // playback stopped
                onStop();

            playing = play;
            if (playing) {
                if (auto samples = pos->getTimeInSamples()) {
                    timeInSamples = *samples;
                }
            }
        }
    }

    int inputBusCount = getBusCount(true);
    int audioOutputs = getTotalNumOutputChannels();
    int audioInputs = inputBusCount > 0 ? getChannelCountOfBus(true, 0) : 0;
    int sideInputs = inputBusCount > 1 ? getChannelCountOfBus(true, 1) : 0;

    if (!audioInputs || !audioOutputs)
        return;

    double mix = (double)params.getRawParameterValue("mix")->load();
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    int sync = (int)params.getRawParameterValue("sync")->load();
    double min = (double)params.getRawParameterValue("min")->load();
    double max = (double)params.getRawParameterValue("max")->load();
    double ratehz = (double)params.getRawParameterValue("rate")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();
    double lowcut = (double)params.getRawParameterValue("lowcut")->load();
    double highcut = (double)params.getRawParameterValue("highcut")->load();
    int algo = (int)params.getRawParameterValue("algo")->load();
    double threshold = (double)params.getRawParameterValue("threshold")->load();
    double sense = 1.0 - (double)params.getRawParameterValue("sense")->load();
    double stereo = (double)params.getRawParameterValue("stereo")->load() / 360.0;
    int splitSlope = (int)params.getRawParameterValue("split_slope")->load();
    sense = std::pow(sense, 2); // make sensitivity more responsive
    int numSamples = buffer.getNumSamples();

    // processes draw wave samples
    auto processDisplaySample = [&](double pos, double env, double env2, double lsamp, double rsamp) {
        auto preamp = std::max(std::fabs(lsamp), std::fabs(rsamp));
        auto postamp = std::max(std::fabs(lsamp * env), std::fabs(rsamp * env2));
        winpos = (int)std::floor(pos * viewW);
        if (lwinpos != winpos) {
            preSamples[winpos] = 0.0;
            postSamples[winpos] = 0.0;
        }
        lwinpos = winpos;
        if (preSamples[winpos] < preamp)
            preSamples[winpos] = preamp;
        if (postSamples[winpos] < postamp)
            postSamples[winpos] = postamp;
    };

    // sidechain display
    auto processSideDisplaySample = [&](double pos, int samp) {
        if (!sideInputs) return;
        auto lsamp = buffer.getSample(audioInputs, samp);
        auto rsamp = buffer.getSample(sideInputs > 1 ? audioInputs + 1 : audioInputs, samp);
        auto ampvalue = std::max(std::fabs(lsamp), std::fabs(rsamp));

        sidewinpos = (int)std::floor(pos * viewW);
        if (lsidewinpos != sidewinpos) {
            sideSamples[sidewinpos] = 0.0;
        }
        lsidewinpos = sidewinpos;
        if (sideSamples[sidewinpos] < ampvalue) {
            sideSamples[sidewinpos] = ampvalue;
        }
    };

    double monIncrementPerSample = 1.0 / ((srate * 2) / monW); // 2 seconds of audio displayed on monitor
    auto processMonitorSample = [&](double lsamp, double rsamp, bool hit) {
        double indexd = monpos.load();
        indexd += monIncrementPerSample;

        if (indexd >= monW)
            indexd -= monW;

        int index = (int)indexd;
        if (lmonpos != index)
            monSamples[index] = 0.0;
        lmonpos = index;

        double maxamp = std::max(std::fabs(lsamp), std::fabs(rsamp));
        if (hit || monSamples[index] >= 10.0)
            maxamp = std::max(maxamp + 10.0, hitamp + 10.0); // encode hits by adding +10 to amp

        monSamples[index] = std::max(monSamples[index], maxamp);
        monpos.store(indexd);
    };

    // applies envelope to a sample index
    auto applyGain = [&](int sampIdx, double env, double env2, double lsample, double rsample) {
        for (int channel = 0; channel < audioOutputs; ++channel) {
            auto wet = (channel == 0 ? lsample * env : rsample * env2);
            auto dry = (double)buffer.getSample(channel, sampIdx);
            if (outputCV) {
                buffer.setSample(channel, sampIdx, (float)env);
            }
            else {
                buffer.setSample(channel, sampIdx, (float)(wet * mix + dry * (1.0 - mix)));
            }
        }
    };

    if (paramChanged) {
        onSlider();
        paramChanged = false;
    }

    // Process new MIDI messages
    for (const auto metadata : midiMessages) {
        juce::MidiMessage message = metadata.getMessage();
        if (message.isNoteOn() || message.isNoteOff()) {
            midiIn.push_back({ // queue midi message
                metadata.samplePosition,
                message.isNoteOn(),
                message.getNoteNumber(),
                message.getVelocity(),
                message.getChannel() - 1
            });
        }
    }

    // Process midi out queue
    for (auto it = midiOut.begin(); it != midiOut.end();) {
        auto& [msg, offset] = *it;

        if (offset < sblock) {
            midiMessages.addEvent(msg, offset);
            it = midiOut.erase(it);
        }
        else {
            offset -= sblock;
            ++it;
        }
    }

    // remove midi in messages that have been processed
    midiIn.erase(std::remove_if(midiIn.begin(), midiIn.end(), [](const MidiInMsg& msg) {
        return msg.offset < 0;
    }), midiIn.end());

    // update outputs with last block information at the start of the new block
    if (outputCC > 0) {
        auto val = (int)std::round(ypos*127.0);
        if (bipolarCC) val -= 64;
        auto cc = MidiMessage::controllerEvent(outputCCChan + 1, outputCC-1, val);
        midiMessages.addEvent(cc, 0);
    }

    if (trigger == Trigger::Free) {
        ratePos += ratehz / srate;
        beatPos += beatsPerSample;
    }
    // keep beatPos in sync with playhead so plugin can be bypassed and return to its sync pos
    else if (playing) {
        beatPos = ppqPosition;
        ratePos = beatPos * secondsPerBeat * ratehz;
    }

    // frequency splitting
    // rawBuffer is used for audioTrigger
    // buffer will contain the mid frequency (splitted)
    // lowBuffer and highBuffer will contain the excluded frequencies to be summed at the end
    rawBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
    rawBuffer.copyFrom(1, 0, buffer, audioInputs > 1 ? 1 : 0, 0, numSamples);
    if (splitter.freqLP > 20.0 || splitter.freqHP < 20000.0) {
        splitter.processBlock(
            splitSlope,
            buffer.getReadPointer(0),
            buffer.getReadPointer(audioInputs > 1 ? 1 : 0),
            lowBuffer.getWritePointer(0),
            lowBuffer.getWritePointer(1),
            buffer.getWritePointer(0),
            buffer.getWritePointer(audioInputs > 1 ? 1 : 0),
            highBuffer.getWritePointer(0),
            highBuffer.getWritePointer(1),
            numSamples
        );
    }

    for (int sample = 0; sample < numSamples; ++sample) {
        if (playing && looping && beatPos >= loopEnd && trigger != Trigger::Free) {
            beatPos = loopStart + (beatPos - loopEnd);
            ratePos = beatPos * secondsPerBeat * ratehz;
        }

        // process midi in queue
        for (auto& msg : midiIn) {
            if (msg.offset == 0) {
                if (msg.isNoteon) {
                    if (msg.channel == triggerChn || triggerChn == 16) {
                        auto patidx = msg.note % 12;
                        queuePattern(patidx + 1);
                    }
                    if (trigger == Trigger::MIDI && (msg.channel == midiTriggerChn || midiTriggerChn == 16)) {
                        if (queuedPattern) {
                            queuedMidiTrigger = true;
                        }
                        else {
                            startMidiTrigger();
                        }
                    }
                }
            }
            msg.offset -= 1;
        }

        // process queued pattern
        if (queuedPattern) {
            if (!playing || queuedPatternCountdown == 0) {
                if (sequencer->isOpen) {
                    sequencer->close();
                    setUIMode(UIMode::Normal);
                }
                pattern = patterns[queuedPattern - 1];
                viewPattern = pattern;
                auto tension = (double)params.getRawParameterValue("tension")->load();
                auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
                auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
                pattern->setTension(tension, tensionatk, tensionrel, dualTension);
                pattern->buildSegments();
                MessageManager::callAsync([this]() {
                    sendChangeMessage();
                    });
                queuedPattern = 0;
                if (queuedMidiTrigger) {
                    startMidiTrigger();
                    queuedMidiTrigger = false;
                }
            }
            if (queuedPatternCountdown > 0) {
                queuedPatternCountdown -= 1;
            }
        }

        // Sync mode
        if (trigger == Trigger::Sync || trigger == Trigger::Free) {
            xpos = sync > 0
                ? beatPos / syncQN + phase
                : ratePos + phase;

            xpos -= std::floor(xpos);
            double newypos = getY(xpos, min, max);
            ypos = value->process(newypos, newypos > ypos);

            ypos2 = ypos;
            xpos2 = xpos;
            if (std::fabs(stereo) > 1e-4) {
                xpos2 = xpos + stereo;
                if (xpos2 < 0.0) xpos2 += 1;
                xpos2 -= std::floor(xpos2);
                double newypos2 = getY(xpos2, min, max);
                ypos2 = value2->process(newypos2, newypos2 > ypos2);
            }

            auto lsample = (double)buffer.getSample(0, sample);
            auto rsample = (double)buffer.getSample(1 % audioInputs, sample);
            applyGain(sample, ypos, ypos2, lsample, rsample);
            processDisplaySample(xpos, ypos, ypos2, lsample, rsample);
            processSideDisplaySample(xpos, sample);
        }

        // MIDI mode
        else if (trigger == Trigger::MIDI) {
            int latency = (int)latBufferL.size(); // latency is used in MIDI mode for anti-click only
            int readPos = latency > 0 ? (latpos + 1) % latency : 0;

            // read audio samples into latency buffer
            double lsample, rsample;
            if (latency > 0) {
                latBufferL[latpos] = (double)buffer.getSample(0, sample);
                latBufferR[latpos] = (double)buffer.getSample(audioInputs > 1 ? 1 : 0, sample);

                lsample = latBufferL[readPos];
                rsample = latBufferR[readPos];

                // write delayed samples to buffer to later apply dry/wet mix
                for (int channel = 0; channel < audioOutputs; ++channel) {
                    buffer.setSample(channel, sample, (float)(channel == 0 ? lsample : rsample));
                }
            }
            else {
                lsample = (double)buffer.getSample(0, sample);
                rsample = (double)buffer.getSample(audioInputs > 1 ? 1 : 0, sample);
            }

            auto inc = sync > 0
                ? beatsPerSample / syncQN
                : 1 / srate * ratehz;
            xpos += inc;

            trigpos += inc;
            xpos -= std::floor(xpos);

            if (!alwaysPlaying) {
                if (midiTrigger) {
                    if (trigpos >= 1.0) { // envelope finished, stop midiTrigger
                        midiTrigger = false;
                        xpos = phase ? phase : 1.0;
                    }
                }
                else {
                    xpos = phase ? phase : 1.0; // midiTrigger is stopped, hold last position
                }
            }

            double newypos = antiClickCooldown >= 0
                ? newypos = tween_ease_inout((double)(antiClickSamples - antiClickCooldown), antiClickStart, antiClickTarget, (double)antiClickSamples)
                : newypos = getY(xpos, min, max); // otherwise get the normal xposition value

            ypos = value->process(newypos, newypos > ypos);

            // stereo processing
            xpos2 = xpos;
            ypos2 = ypos;
            if (std::fabs(stereo) > 1e-4) {
                xpos2 = xpos + stereo;
                if (xpos2 < 0.0) xpos2 += 1;
                xpos2 -= std::floor(xpos2);
                double newypos2 = getY(xpos2, min, max);
                ypos2 = value2->process(newypos2, newypos2 > ypos2);
            }

            applyGain(sample, ypos, ypos2, lsample, rsample);
            double viewx = (alwaysPlaying || midiTrigger) ? xpos : (trigpos + trigphase) - std::floor(trigpos + trigphase);
            processDisplaySample(viewx, ypos, ypos2, lsample, rsample);
            processSideDisplaySample(viewx, sample);

            if (latency > 0) {
                latpos = (latpos + 1) % latency;
            }
        }

        // Audio mode
        else if (trigger == Trigger::Audio) {
            int latency = (int)latBufferL.size();

            // read audio samples
            double lsample = (double)buffer.getSample(0, sample);
            double rsample = (double)buffer.getSample(audioInputs > 1 ? 1 : 0, sample);
            latBufferL[latpos] = lsample;
            latBufferR[latpos] = rsample;

            // read sidechain samples
            double lsidesample = 0.0;
            double rsidesample = 0.0;
            if (useSidechain && sideInputs) {
                lsidesample = (double)buffer.getSample(audioInputs, sample);
                rsidesample = (double)buffer.getSample(sideInputs > 1 ? audioInputs + 1 : audioInputs, sample);
            }

            // Detect audio transients
            auto monSampleL = useSidechain ? lsidesample : rawBuffer.getSample(0, sample);
            auto monSampleR = useSidechain ? rsidesample : rawBuffer.getSample(1, sample);
            if (lowcut > 20.0) {
                monSampleL = hpFilterL.df1(monSampleL);
                monSampleR = hpFilterR.df1(monSampleR);
            }
            if (highcut < 20000.0) {
                monSampleL = lpFilterL.df1(monSampleL);
                monSampleR = lpFilterR.df1(monSampleR);
            }
            latMonitorBufferL[latpos] = monSampleL;
            latMonitorBufferR[latpos] = monSampleR;

            if (transDetectorL.detect(algo, monSampleL, threshold, sense) ||
                transDetectorR.detect(algo, monSampleR, threshold, sense))
            {
                transDetectorL.startCooldown();
                transDetectorR.startCooldown();
                int offset = (int)(params.getRawParameterValue("offset")->load() * AUDIO_LATENCY_MILLIS / 1000.f * srate);
                audioTriggerCountdown = std::max(0, int((AUDIO_LATENCY_MILLIS / 1000.0 * srate) + offset));
                hitamp = transDetectorL.hit ? std::fabs(monSampleL) : std::fabs(monSampleR);
            }

            // read the sample 'latency' samples ago
            int readPos = (latpos + 1) % latency;
            lsample = latBufferL[readPos];
            rsample = latBufferR[readPos];
            monSampleL = latMonitorBufferL[readPos];
            monSampleR = latMonitorBufferR[readPos];

            // write delayed samples to buffer to later apply dry/wet mix
            for (int channel = 0; channel < audioOutputs; ++channel) {
                buffer.setSample(channel, sample, (float)(channel == 0 ? lsample : rsample));
            }

            bool hit = audioTriggerCountdown == 0; // there was an audio transient trigger in this sample, not counting the anticlick lag

            // HIT - start another countdown, this time for anticlick
            if (hit && (alwaysPlaying || !audioIgnoreHitsWhilePlaying || trigposSinceHit > 0.98)) {
                antiClickCooldown = getAntiClickLatency();
                antiClickSamples = antiClickCooldown;
                antiClickStart = ypos;
                antiClickTarget = getY(phase, min, max);
            }

            processMonitorSample(monSampleL, monSampleR, antiClickCooldown == 0);

            // envelope processing
            auto inc = sync > 0
                ? beatsPerSample / syncQN
                : 1 / srate * ratehz;
            xpos += inc;

            trigpos += inc;
            trigposSinceHit += inc;
            xpos -= std::floor(xpos);

            // send output midi notes on audio trigger hit
            if (antiClickCooldown == 0 && outputATMIDI > 0) {
                auto noteOn = MidiMessage::noteOn(1, outputATMIDI - 1, (float)hitamp);
                midiMessages.addEvent(noteOn, sample);

                auto offnoteDelay = static_cast<int>(srate * AUDIO_NOTE_LENGTH_MILLIS / 1000.0);
                int noteOffSample = sample + offnoteDelay;
                auto noteOff = MidiMessage::noteOff(1, outputATMIDI - 1);

                if (noteOffSample < sblock) {
                    midiMessages.addEvent(noteOff, noteOffSample);
                }
                else {
                    int offset = noteOffSample - sblock;
                    midiOut.push_back({ noteOff, offset });
                }
            }

            if (!alwaysPlaying) {
                if (audioTrigger) {
                    if (trigpos >= 1.0) { // envelope finished, stop trigger
                        audioTrigger = false;
                        xpos = phase ? phase : 1.0;
                    }
                }
                else {
                    xpos = phase ? phase : 1.0; // audioTrigger is stopped, hold last position
                }
            }

            double newypos = antiClickCooldown >= 0
                ? newypos = tween_ease_inout((double)(antiClickSamples - antiClickCooldown), antiClickStart, antiClickTarget, (double)antiClickSamples)
                : newypos = getY(xpos, min, max); // otherwise get the normal xposition value

            ypos = value->process(newypos, newypos > ypos);

            xpos2 = xpos;
            ypos2 = ypos;
            if (std::fabs(stereo) > 1e-4) {
                xpos2 = xpos + stereo;
                if (xpos2 < 0.0) xpos2 += 1;
                xpos2 -= std::floor(xpos2);
                double newypos2 = getY(xpos2, min, max);
                ypos2 = value2->process(newypos2, newypos2 > ypos2);
            }

            if (useMonitor) {
                for (int channel = 0; channel < audioOutputs; ++channel) {
                    buffer.setSample(channel, sample, (float)(channel == 0 ? monSampleL : monSampleR));
                }
            }
            else {
                applyGain(sample, ypos, ypos2, lsample, rsample);
            }

            double viewx = (alwaysPlaying || audioTrigger) ? xpos : (trigpos + trigphase) - std::floor(trigpos + trigphase);
            processDisplaySample(viewx, ypos, ypos2, lsample, rsample);
            processSideDisplaySample(viewx, sample);
            latpos = (latpos + 1) % latency;

            if (audioTriggerCountdown > -1)
                audioTriggerCountdown -= 1;
        }

        xenv.store(xpos);
        yenv.store(ypos);
        xenv2.store(xpos2);
        yenv2.store(ypos2);
        drawStereo.store(std::fabs(stereo) > 1e-4);
        beatPos += beatsPerSample;
        ratePos += 1 / srate * ratehz;
        if (playing)
            timeInSamples += 1;

        if (antiClickCooldown >= 0) {
            if (antiClickCooldown == 0 && trigger == MIDI) {
                clearDrawBuffers();
                midiTrigger = !alwaysPlaying;
                trigpos = 0.0;
                trigphase = phase;
                restartEnv(true);
            }
            if (antiClickCooldown == 0 && trigger == Audio) {
                clearDrawBuffers();
                audioTrigger = !alwaysPlaying;
                trigpos = 0.0;
                trigphase = phase;
                trigposSinceHit = 0.0;
                restartEnv(true);
            }
            antiClickCooldown -= 1;
        }
    }

    // finally if frequency splitting, add back the excluded frequencies
    if (splitter.freqLP > 20.0 || splitter.freqHP < 20000.0) {
        buffer.addFrom(0, 0, lowBuffer, 0, 0, numSamples);
        buffer.addFrom(0, 0, highBuffer, 0, 0, numSamples);
        if (audioInputs > 1) {
            buffer.addFrom(1, 0, lowBuffer, 1, 0, numSamples);
            buffer.addFrom(1, 0, highBuffer, 1, 0, numSamples);
        }
    }

    // prepare FFT buffer for band splitter display
    if (showBandsEditor) {
        auto ch0 = buffer.getReadPointer(0);
        auto ch1 = buffer.getReadPointer(audioInputs > 1 ? 1 : 0);
        for (int i = 0; i < numSamples; ++i) {
            bandsFFTBuffer[bandsFFTWriteIndex++] = 0.5f * (ch0[i] + ch1[i]);
            bandsFFTWriteIndex %= bandsFFTBuffer.size();
        }
        bandsFFTReady.store(true, std::memory_order_release);
    }
    else {
        std::fill(bandsFFTBuffer.begin(), bandsFFTBuffer.end(), 0.f);
    }

    drawSeek.store(playing && (trigger == Trigger::Sync || midiTrigger || audioTrigger));
}

//==============================================================================
bool GATE12AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GATE12AudioProcessor::createEditor()
{
    return new GATE12AudioProcessorEditor (*this);
}

//==============================================================================
void GATE12AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = ValueTree("PluginState");
    state.appendChild(params.copyState(), nullptr);
    state.setProperty("version", PROJECT_VERSION, nullptr);
    state.setProperty("currentProgram", currentProgram, nullptr);
    state.setProperty("alwaysPlaying",alwaysPlaying, nullptr);
    state.setProperty("dualSmooth",dualSmooth, nullptr);
    state.setProperty("dualTension",dualTension, nullptr);
    state.setProperty("triggerChn",triggerChn, nullptr);
    state.setProperty("useMonitor",useMonitor, nullptr);
    state.setProperty("useSidechain",useSidechain, nullptr);
    state.setProperty("outputCC", outputCC, nullptr);
    state.setProperty("outputCCChan", outputCCChan, nullptr);
    state.setProperty("outputCV", outputCV, nullptr);
    state.setProperty("outputATMIDI", outputATMIDI, nullptr);
    state.setProperty("bipolarCC", bipolarCC, nullptr);
    state.setProperty("paintTool", paintTool, nullptr);
    state.setProperty("paintPage", paintPage, nullptr);
    state.setProperty("pointMode", pointMode, nullptr);
    state.setProperty("audioIgnoreHitsWhilePlaying", audioIgnoreHitsWhilePlaying, nullptr);
    state.setProperty("linkSeqToGrid", linkSeqToGrid, nullptr);
    state.setProperty("currpattern", pattern->index + 1, nullptr);
    state.setProperty("antiClick", antiClick, nullptr);
    state.setProperty("midiTriggerChn", midiTriggerChn, nullptr);
    state.setProperty("drawSidechain", drawSidechain, nullptr);

    for (int i = 0; i < 12; ++i) {
        std::ostringstream oss;
        auto points = patterns[i]->points;

        if (sequencer->isOpen && i == sequencer->patternIdx) {
            points = sequencer->backup;
        }

        for (const auto& point : points) {
            oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
        }
        state.setProperty("pattern" + juce::String(i), var(oss.str()), nullptr);
    }

    // serialize sequencer cells
    std::ostringstream oss;
    for (const auto& cell : sequencer->cells) {
        oss << cell.shape << ' '
            << cell.lshape << ' '
            << cell.ptool << ' '
            << cell.invertx << ' '
            << cell.minx << ' '
            << cell.maxx << ' '
            << cell.miny << ' '
            << cell.maxy << ' '
            << cell.tenatt << ' '
            << cell.tenrel << ' '
            << cell.skew << '\n';
    }
    state.setProperty("seqcells", var(oss.str()), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GATE12AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (sequencer->isOpen) {
        sequencer->close();
    }

    std::unique_ptr<juce::XmlElement>xmlState (getXmlFromBinary (data, sizeInBytes));
    if (!xmlState) return;
    auto state = ValueTree::fromXml (*xmlState);
    if (!state.isValid()) return;

    params.replaceState(state.getChild(0));
    if (state.hasProperty("version")) {
        currentProgram = (int)state.getProperty("currentProgram");
        alwaysPlaying = (bool)state.getProperty("alwaysPlaying");
        dualSmooth = (bool)state.getProperty("dualSmooth");
        dualTension = (bool)state.getProperty("dualTension");
        triggerChn = (int)state.getProperty("triggerChn");
        useMonitor = (bool)state.getProperty("useMonitor");
        useSidechain = (bool)state.getProperty("useSidechain");
        outputCC = (int)state.getProperty("outputCC");
        outputCCChan = (int)state.getProperty("outputCCChan");
        bipolarCC = (bool)state.getProperty("bipolarCC");
        outputCV = (bool)state.getProperty("outputCV");
        outputATMIDI = (int)state.getProperty("outputATMIDI");
        paintTool = (int)state.getProperty("paintTool");
        paintPage = (int)state.getProperty("paintPage");
        pointMode = state.hasProperty("pointMode") ? (int)state.getProperty("pointMode") : 1;
        audioIgnoreHitsWhilePlaying = (bool)state.getProperty("audioIgnoreHitsWhilePlaying");
        linkSeqToGrid = state.hasProperty("linkSeqToGrid") ? (bool)state.getProperty("linkSeqToGrid") : true;
        antiClick = state.hasProperty("antiClick") ? (int)state.getProperty("antiClick") : 1;
        midiTriggerChn = (int)state.getProperty("midiTriggerChn");
        drawSidechain = (bool)state.getProperty("drawSidechain", true);

        for (int i = 0; i < 12; ++i) {
            patterns[i]->clear();
            patterns[i]->clearUndo();

            auto str = state.getProperty("pattern" + String(i)).toString().toStdString();
            if (!str.empty()) {
                double x, y, tension;
                int type;
                std::istringstream iss(str);
                while (iss >> x >> y >> tension >> type) {
                    patterns[i]->insertPoint(x,y,tension,type, false);
                }
            }

            auto tension = (double)params.getRawParameterValue("tension")->load();
            auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
            auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
            patterns[i]->setTension(tension, tensionatk, tensionrel, dualTension);
            patterns[i]->buildSegments();
        }

        if (state.hasProperty("seqcells")) {
            auto str = state.getProperty("seqcells").toString().toStdString();
            sequencer->cells.clear();
            std::istringstream iss(str);
            Cell cell;
            int shape, lshape;
            while (iss >> shape >> lshape >> cell.ptool >> cell.invertx
                >> cell.minx >> cell.maxx >> cell.miny >> cell.maxy >> cell.tenatt
                >> cell.tenrel >> cell.skew)
            {
                cell.shape = static_cast<CellShape>(shape);
                cell.lshape = static_cast<CellShape>(lshape);
                sequencer->cells.push_back(cell);
            }
        }

        int currpattern = 1;
        if (!state.hasProperty("currpattern"))
            currpattern = (int)params.getRawParameterValue("pattern")->load();
        else
            currpattern = state.getProperty("currpattern");
        queuePattern(currpattern);
        auto param = params.getParameter("pattern");
        param->setValueNotifyingHost(param->convertTo0to1((float)currpattern));
    }

    setUIMode(Normal);
}

void GATE12AudioProcessor::importPatterns()
{
    if (sequencer->isOpen)
        sequencer->close();

    auto tensionParams = TensionParameters((double)params.getRawParameterValue("tension")->load(),
                             (double)params.getRawParameterValue("tensionatk")->load(),
                             (double)params.getRawParameterValue("tensionrel")->load(), dualTension);

    patternManager.importPatterns(patterns,tensionParams);
    setUIMode(UIMode::Normal);
}

void GATE12AudioProcessor::exportPatterns()
{
    if (sequencer->isOpen)
        sequencer->close();
    patternManager.exportPatterns(patterns);
    setUIMode(UIMode::Normal);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GATE12AudioProcessor();
}
