/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include "dsp/Pattern.h"
#include "dsp/Filter.h"
#include "dsp/Transient.h"
#include "Presets.h"
#include <atomic>
#include <deque>
#include "Globals.h"

using namespace globals;

struct MIDIMsg {
    int offset;
    int isNoteon;
    int note;
    int vel;
    int channel;
};

enum Trigger {
    sync,
    MIDI,
    audio
};

enum PatSync {
    off,
    quarterBeat,
    halfBeat,
    beat_x1,
    beat_x2,
    beat_x4
};

/*
    RC lowpass filter with two resitances a or b
    Used for attack release smooth of ypos
*/
class RCSmoother
{
public:
    double a; // resistance a
    double b; // resistance b
    double state;
    double output;

    void setup(double ra, double rb, double srate)
    {
        a = 1.0 / (ra * srate + 1);
        b = 1.0 / (rb * srate + 1);
    }

    double process(double input, bool useAorB)
    {
        state += (useAorB ? a : b) * (input - state);
        output = state;
        return output;
    }

    void reset(double value = 0.0)
    {
        output = state = value;
    }
};

//==============================================================================
/**
*/
class GATE12AudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorParameter::Listener, public juce::ChangeBroadcaster
{
public:
    static constexpr int GRID_SIZES[] = { 
        8, 16, 32, 64, // Straight
        12, 24, 48,  // Triplet
    };

    // Plugin settings
    float scale = 1.0f; // UI scale factor
    int plugWidth = PLUG_WIDTH;
    int plugHeight = PLUG_HEIGHT;

    // Instance Settings
    bool alwaysPlaying = false;
    bool dualSmooth = true; // use either single smooth or attack and release
    bool dualTension = true;
    bool MIDIHoldEnvelopeTail = true; // MIDI trigger - keep processing last position after envelope finishes
    bool AudioHoldEnvelopeTail = false; // Audio trigger - keep processing last position after envelope finishes
    int triggerChn = 9; // Midi pattern trigger channel, defaults to channel 10
    bool useMonitor = false;
    bool useSidechain = false;
    int paintTool = 0; // index of pattern used for paint mode
    int paintPage = 0;

    // State
    Pattern* pattern; // current pattern used for audio processing
    Pattern* viewPattern; // pattern being edited on the view, usually the audio pattern but can also be a paint mode pattern
    int queuedPattern = 0; // queued pat index, 0 = off
    int64_t queuedPatternCountdown = 0; // samples counter until queued pattern is applied
    int currentProgram = -1;
    double xpos = 0.0; // envelope x pos (0..1)
    double ypos = 0.0; // envelope y pos (0..1)
    double trigpos = 0.0; // used by trigger (Audio and MIDI) to detect one one shot envelope play
    double trigphase = 0.0; // phase when trigger occurs, used to sync the background wave draw
    double syncQN = 1.0; // sync quarter notes
    int ltrigger = -1; // last trigger mode
    bool midiTrigger = false; // flag midi has triggered envelope
    int winpos = 0;
    int lwinpos = 0;
    double ltension = -10.0;
    double ltensionatk = -10.0;
    double ltensionrel = -10.0;
    RCSmoother* value; // smooths envelope value
    bool showLatencyWarning = false;

    // Audio mode state
    bool audioTrigger = false; // flag audio has triggered envelope
    int audioTriggerCountdown = -1; // samples until audio envelope starts
    std::vector<double> latBufferL; // latency buffer left
    std::vector<double> latBufferR; // latency buffer right
    std::vector<double> latMonitorBufferL; // latency buffer left
    std::vector<double> latMonitorBufferR; // latency buffer right
    int latpos = 0; // latency buffer pos
    Filter lpFilterL{};
    Filter lpFilterR{};
    Filter hpFilterL{};
    Filter hpFilterR{};
    double hitamp = 0.0;
    
    // PlayHead state
    bool playing = false;
    int64_t timeInSamples = 0;
    double beatPos = 0.0; // position in quarter notes
    double ratePos = 0.0; // position in hertz
    double ppqPosition = 0.0;
    double beatsPerSample = 0.00005;
    double beatsPerSecond = 1.0;
    int samplesPerBeat = 44100;
    double secondsPerBeat = 0.1;

    // UI State
    std::vector<double> preSamples; // used by view to draw pre audio
    std::vector<double> postSamples; // used by view to draw post audio
    int viewW = 1; // viewport width, used for buffers of samples to draw waveforms
    std::atomic<double> xenv = 0.0; // xpos copy using atomic, read by UI thread - attempt to fix rare crash
    std::atomic<double> yenv = 0.0; // ypos copy using atomic, read by UI thread - attempt to fix rare crash
    std::atomic<bool> drawSeek = false;
    std::vector<double> monSamples; // used to draw transients + waveform preview
    std::atomic<double> monpos = 0.0; // write index of monitor circular buf
    int lmonpos = 0; // last index
    int monW = 1; // audio monitor width used to rotate monitor samples buffer
    bool showAudioKnobs = false; // used by UI to toggle audio knobs
    bool showPaintWidget = false;

    //==============================================================================
    GATE12AudioProcessor();
    ~GATE12AudioProcessor() override;

    void loadSettings();
    void saveSettings();
    void setScale(float value);
    int getCurrentGrid();
    void createUndoPoint(int patindex = -1);
    void createUndoPointFromSnapshot(std::vector<PPoint> snapshot);
    bool isPaintEdit();
    bool isPaintMode();
    void togglePaintEdit();
    Pattern* getPaintPatern(int index);
    void setViewPattern(int index);
    void setPaintTool(int index);
    void restorePaintPatterns();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;
    bool supportsDoublePrecisionProcessing() const override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    //==============================================================================
    void onSlider ();
    void onTensionChange();
    void onPlay ();
    void onStop ();
    void restartEnv (bool fromZero = false);
    void setSmooth();
    void clearDrawBuffers();
    void clearLatencyBuffers();
    void toggleUseSidechain();
    void toggleMonitorSidechain();
    double getY(double x, double min, double max);
    void queuePattern(int patidx);

    //==============================================================================
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    template <typename FloatType>
    void processBlockByType(AudioBuffer<FloatType>& buffer, MidiBuffer& midiMessages);

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    void loadProgram(int index);
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //=========================================================

    juce::AudioProcessorValueTreeState params;
    juce::UndoManager undoManager;

private:
    Pattern* patterns[12]; // audio process patterns
    Pattern* paintPatterns[PAINT_PATS]; // paint mode patterns
    Transient transDetectorL;
    Transient transDetectorR;
    bool paramChanged = false; // flag that triggers on any param change
    juce::ApplicationProperties settings;
    std::vector<MIDIMsg> midi; // midi buffer used to process midi messages offset

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GATE12AudioProcessor)
};
