// Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Globals.h"
#include <ctime>

GATE12AudioProcessor::GATE12AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
         .withInput("Input", juce::AudioChannelSet::stereo(), true)
         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
     )
    , settings{}
    , params(*this, &undoManager, "PARAMETERS", {
        std::make_unique<juce::AudioParameterChoice>("trigger", "Trigger", StringArray { "Sync", "MIDI", "Audio" }, 0),
        std::make_unique<juce::AudioParameterInt>("pattern", "Pattern", 1, 12, 1),
        std::make_unique<juce::AudioParameterChoice>("sync", "Sync", StringArray { "Rate Hz", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "1/16t", "1/8t", "1/4t", "1/2t", "1/1t", "1/16.", "1/8.", "1/4.", "1/2.", "1/1." }, 5),
        std::make_unique<juce::AudioParameterFloat>("rate", "Rate", juce::NormalisableRange<float>(0.01f, 140.0f, 0.01f, 0.3f), 1.0f),
        std::make_unique<juce::AudioParameterFloat>("phase", "Phase", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("min", "Min", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("max", "Max", 0.0f, 1.0f, 1.0f),
        std::make_unique<juce::AudioParameterFloat>("smooth", "Smooth", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("attack", "Attack", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("release", "Release", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tension", "Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterChoice>("paint", "Paint", StringArray { "Erase", "Line", "Saw Up", "Saw Down", "Triangle" }, 1),
        std::make_unique<juce::AudioParameterChoice>("point", "Point", StringArray { "Hold", "Curve", "S-Curve", "Pulse", "Wave", "Triangle", "Stairs", "Smooth St" }, 1),
        std::make_unique<juce::AudioParameterBool>("snap", "Snap", false),
        std::make_unique<juce::AudioParameterInt>("grid", "Grid", 2, 32, 8),
        std::make_unique<juce::AudioParameterBool>("retrigger", "Retrigger", false),
        std::make_unique<juce::AudioParameterChoice>("patsync", "Pattern Sync", StringArray { "Off", "1/4 Beat", "1/2 Beat", "1 Beat", "2 Beats", "4 Beats"}, 0),
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

    // init patterns
    for (int i = 0; i < 12; i++) {
        patterns[i] = new Pattern(*this, i);
        patterns[i]->insertPoint(0, 1, 0, 1);
        patterns[i]->insertPoint(0.5, 0, 0, 1);
        patterns[i]->insertPoint(1, 1, 0, 1);
        patterns[i]->buildSegments();
    }

    pattern = patterns[0];
    preSamples.resize(globals::PLUG_WIDTH, 0); // samples array size must be >= viewport width 
    postSamples.resize(globals::PLUG_WIDTH, 0);
    value = new SmoothParam();

    loadSettings();
}

GATE12AudioProcessor::~GATE12AudioProcessor()
{
}

void GATE12AudioProcessor::setSmooth() 
{
    if (dualSmooth) {
        float attack = params.getRawParameterValue("attack")->load();
        float release = params.getRawParameterValue("release")->load();
        value->rcSet2(attack * 0.25, release * 0.25, getSampleRate());
    }
    else {
        float lfosmooth = params.getRawParameterValue("smooth")->load();
        value->rcSet2(lfosmooth * 0.25, lfosmooth * 0.25, getSampleRate());
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
    if (auto* file = settings.getUserSettings()) {
        scale = (float)file->getDoubleValue("scale", 1.0f);
    }
}

void GATE12AudioProcessor::saveSettings ()
{
    if (auto* file = settings.getUserSettings()) {
        file->setValue("scale", scale);
    }
    settings.saveIfNeeded();
}

// Set UI scale factor
void GATE12AudioProcessor::setScale(float s)
{
    scale = s;
    saveSettings();
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
    return 0;
}

int GATE12AudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void GATE12AudioProcessor::setCurrentProgram (int index)
{
    (void)index;
    // if (currentProgram == index) return;
    // currentProgram = index;
    // auto data = BinaryData::Init_xml;
    // auto size = BinaryData::Init_xmlSize;
    // if (currentProgram == -1) return;

    // if (index == 1) { data = BinaryData::Harpsi_xml; size = BinaryData::Harpsi_xmlSize; }

    // auto xmlState = XmlDocument::parse (juce::String (data, size));
    // if (xmlState.get() != nullptr) {
    //     if (xmlState->hasTagName(params.state.getType())) {
    //         clearVoices();
    //         params.replaceState(juce::ValueTree::fromXml (*xmlState));
    //         resetLastModels();
    //     }
    // }
}

const juce::String GATE12AudioProcessor::getProgramName (int index)
{
    (void)index;
    // if (index == 0) return "Init";
    return "";
}

void GATE12AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    (void)index;
    (void)newName;
}

//==============================================================================
void GATE12AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    (void)sampleRate;
    (void)samplesPerBlock;
    onSlider();
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

    int pat = (int)params.getRawParameterValue("pattern")->load();
    if (pat != pattern->index + 1) {
        queuePattern(pat);
    }

    grid = (int)params.getRawParameterValue("grid")->load();
    auto tension = (double)params.getRawParameterValue("tension")->load();
    if (pattern->getTension() != tension) {
        pattern->setTension(tension);
        pattern->buildSegments();
    }

    auto sync = (int)params.getRawParameterValue("sync")->load();
    if (sync == 0) syncQN = 1.; // not used
    else if (sync == 1) syncQN = 1./4.; // 1/16
    else if (sync == 2) syncQN = 1./2.; // 1/8
    else if (sync == 3) syncQN = 1./1.; // 1/4
    else if (sync == 4) syncQN = 1.*2.; // 1/2
    else if (sync == 5) syncQN = 1.*4.; // 1bar
    else if (sync == 6) syncQN = 1.*8.; // 2bar
    else if (sync == 7) syncQN = 1.*16.; // 4bar
    else if (sync == 8) syncQN = 1./6.; // 1/16t
    else if (sync == 9) syncQN = 1./3.; // 1/8t
    else if (sync == 10) syncQN = 2./3.; // 1/4t
    else if (sync == 11) syncQN = 4./3.; // 1/2t
    else if (sync == 12) syncQN = 8./3.; // 1/1t
    else if (sync == 13) syncQN = 1./4.*1.5; // 1/16.
    else if (sync == 14) syncQN = 1./2.*1.5; // 1/8.
    else if (sync == 15) syncQN = 1./1.*1.5; // 1/4.
    else if (sync == 16) syncQN = 2./1.*1.5; // 1/2.
    else if (sync == 17) syncQN = 4./1.*1.5; // 1/1.
}

void GATE12AudioProcessor::onPlay()
{
    std::fill(preSamples.begin(), preSamples.end(), 0.0);
    std::fill(postSamples.begin(), postSamples.end(), 0.0);
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    bool sync = (int)params.getRawParameterValue("sync")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();
    double min = (double)params.getRawParameterValue("min")->load();
    double max = (double)params.getRawParameterValue("max")->load();

    midiTrigger = false;
    audioTrigger = false;

    // reset value.smooth on play
    if (sync) {
        double x = ppqPosition / syncQN + phase;
        x -= std::floor(x);
        value->smooth = getY(x, min, max);
    }

    // reset beatPos on play in Hz mode
    if (trigger == Trigger::Sync && !sync) {
        xpos = 0.0;
        beatPos = 0.0; 
    }
}

void GATE12AudioProcessor::onStop()
{
    std::fill(preSamples.begin(), preSamples.end(), 0.0);
    std::fill(postSamples.begin(), postSamples.end(), 0.0);

    midiTrigger = false;
    audioTrigger = false;

    if (!alwaysPlaying) {
        xpos = 0.0; // stops envelope seek draw
    }
}

double inline GATE12AudioProcessor::getY(double x, double min, double max)
{
    return min + (max - min) * (1 - pattern->get_y_at(x));
}

void GATE12AudioProcessor::retriggerEnvelope()
{
    if (!alwaysPlaying)
        return // retrigger should only work in this mode

    std::fill(preSamples.begin(), preSamples.end(), 0.0);
    std::fill(postSamples.begin(), postSamples.end(), 0.0);

    double phase = (double)params.getRawParameterValue("phase")->load();
    int sync = (int)params.getRawParameterValue("sync")->load();

    if (sync != 0)
        beatPos = -phase * syncQN;
    else 
        beatPos = -phase;
}

void GATE12AudioProcessor::queuePattern(int patidx)
{
    queuedPattern = patidx;
    queuedPatternCounter = 0;
    int patsync = (int)params.getRawParameterValue("patsync")->load();

    if (isPlaying && patsync != PatSync::Off) {
        int interval = samplesPerBeat;
        if (patsync == PatSync::QuarterBeat) 
            interval = interval / 4;
        else if (patsync == PatSync::HalfBeat)
            interval = interval / 2;
        else if (patsync == PatSync::Beat_x2)
            interval = interval * 2;
        else if (patsync == PatSync::Beat_x4)
            interval = interval * 4;
        queuedPatternCounter = (interval - timeInSamples % interval) % interval;
    }

    auto param = params.getParameter("pattern");
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->convertTo0to1((float)(patidx)));
    param->endChangeGesture();
}

bool GATE12AudioProcessor::supportsDoublePrecisionProcessing() const
{
    return true;
}

void GATE12AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockByType(buffer, midiMessages);
}

void GATE12AudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockByType(buffer, midiMessages);
}

template <typename FloatType>
void GATE12AudioProcessor::processBlockByType (AudioBuffer<FloatType>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals disableDenormals;
    double srate = getSampleRate();
    
    if (auto* phead = getPlayHead()) {
        if (auto pos = phead->getPosition()) {
            if (auto ppq = pos->getPpqPosition()) 
                ppqPosition = *ppq;
            if (auto tempo = pos->getBpm()) {
                beatsPerSample = *tempo / (60.0 * srate);
                samplesPerBeat = (int)((60.0 / *tempo) * srate);
            }
            auto play = pos->getIsPlaying();
            if (!isPlaying && play) // playback started
                onPlay();
            else if (isPlaying && !play) // playback stopped
                onStop();
            
            isPlaying = play;
            if (isPlaying) {
                if (auto samples = pos->getTimeInSamples()) {
                    timeInSamples = *samples;
                }
            }
        }
    }

    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    bool retrigger = (bool)params.getRawParameterValue("retrigger")->load();
    int sync = (int)params.getRawParameterValue("sync")->load();
    double min = (double)params.getRawParameterValue("min")->load();
    double max = (double)params.getRawParameterValue("max")->load();
    double ratehz = (double)params.getRawParameterValue("rate")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();
    int numSamples = buffer.getNumSamples();

    if (retrigger) {
        retriggerEnvelope();
        auto param = params.getParameter("retrigger");
        param->beginChangeGesture();
        param->setValueNotifyingHost(0.0f);
        param->endChangeGesture();
    }

    auto processDisplaySample = [&](double amplitude, double env) {
        winpos = (int)std::floor(xpos * viewW);
        if (lwinpos != winpos) {
            preSamples[winpos] = 0;
            postSamples[winpos] = 0;
        }
        lwinpos = winpos;
        double presample = amplitude;
        double postsample = amplitude * env;
        if (preSamples[winpos] < presample)
            preSamples[winpos] = presample;
        if (postSamples[winpos] < postsample)
            postSamples[winpos] = postsample;
    };

    auto applyGain = [&](int sampIdx, double env, bool processDisplay = true) {
        double maxAmp = 0.0;
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            auto s = buffer.getSample(channel, sampIdx);
            maxAmp = std::fmax(std::fabs(maxAmp), (double)s);
            buffer.setSample(channel, sampIdx, static_cast<FloatType>(s * env));
        }

        if (processDisplay) {
            processDisplaySample(maxAmp, env);
        }
    };

    // update beatpos only in tempo sync mode during playback
    if (trigger == 0 && isPlaying && sync > 0) {
        beatPos = ppqPosition;
    }

    // remove midi messages that have been processed
    midi.erase(std::remove_if(midi.begin(), midi.end(), [](const MIDIMsg& msg) {
        return msg.offset < 0;
    }), midi.end());

    if (paramChanged) {
        onSlider();
        paramChanged = false;
    }

    // Process new MIDI messages
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
    for (const auto metadata : midiMessages) {
        juce::MidiMessage message = metadata.getMessage();
        if (message.isNoteOn() || message.isNoteOff()) {
            midi.push_back({ // queue midi message
                metadata.samplePosition,
                message.isNoteOn(),
                message.getNoteNumber(),
                message.getVelocity(),
                message.getChannel() - 1
            });
        }
    }

    for (int sample = 0; sample < numSamples; ++sample) {
        // process midi queue
        for (auto& msg : midi) {
            if (msg.offset == 0) {
                if (msg.isNoteon) {
                    if (msg.channel == triggerChn || triggerChn == 16) {
                        auto patidx = msg.note % 12;
                        queuePattern(patidx + 1);
                    }
                    else if (trigger == Trigger::MIDI) {
                        midiTrigger = true;
                        xpos = phase;
                    }
                }
            }
            msg.offset -= 1;
        }

        if (queuedPattern) {
            if (!isPlaying || queuedPatternCounter == 0) {
                pattern = patterns[queuedPattern - 1];
                auto tension = (double)params.getRawParameterValue("tension")->load();
                pattern->setTension(tension);
                pattern->buildSegments();
                queuedPattern = 0;
            }
            if (queuedPatternCounter > 0) {
                queuedPatternCounter -= 1;
            }
        }

        if (trigger == Trigger::Sync && (isPlaying || alwaysPlaying)) { // Sync trigger mode
            if (sync > 0) {
                beatPos += beatsPerSample;
                xpos = beatPos / syncQN + phase;
            }
            else {
                beatPos += 1 / srate * ratehz;
                xpos = beatPos + phase;
            }
            xpos -= std::floor(xpos);
            
            double nextValue = getY(xpos, min, max);
            ypos = value->smooth2(nextValue, nextValue > ypos);
            
            applyGain(sample, ypos);
        }

        else if (trigger == Trigger::MIDI && (alwaysPlaying || midiTrigger)) {
            if (alwaysPlaying && midiTrigger) { // reset phase on midiTrigger
                xpos = phase;
                midiTrigger = false;
            }
            xpos += sync > 0
                ? beatsPerSample / syncQN
                : 1 / srate * ratehz;
            if (!alwaysPlaying && xpos >= 1) {
                midiTrigger = false;
                xpos = holdEnvelopeTail ? 1.0 : 0.0; // 0.0 stops drawing seek, 1.0 keeps processing last envelope position on finish
            }
            else {
                xpos -= std::floor(xpos);
            }
            double nextValue = getY(xpos, min, max);
            ypos = value->smooth2(nextValue, nextValue > ypos);    

            applyGain(sample, ypos);
        }

        // keep applying last state after MIDI envelope finishes
        else if (trigger == Trigger::MIDI && !alwaysPlaying && !midiTrigger && xpos == 1.0 && holdEnvelopeTail) {
            applyGain(sample, getY(xpos, min, max), false);
        }

        else if (trigger == Trigger::Audio && (audioTrigger || alwaysPlaying)) {

        }
        
        // AUDIO detect transients to trigger audio envelope
        else if (trigger == Trigger::Audio && !audioTrigger && !alwaysPlaying) {

        }

        xenv.store(xpos);
        yenv.store(ypos);
        if (isPlaying)
            timeInSamples += 1;
    }
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
    auto state = params.copyState();
    state.setProperty("currentProgram", currentProgram, nullptr);
    std::unique_ptr<juce::XmlElement>xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GATE12AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement>xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(params.state.getType())) {
            auto state = juce::ValueTree::fromXml (*xmlState);
            if (state.hasProperty("currentProgram")) {
                currentProgram = static_cast<int>(state.getProperty("currentProgram"));
            }
            params.replaceState(state);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GATE12AudioProcessor();
}
