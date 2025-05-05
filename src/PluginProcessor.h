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
#include <atomic>
#include <deque>

struct MIDIMsg {
    int offset;
    int isNoteon;
    int note;
    int vel;
    int channel;
};

enum Trigger {
    Sync,
    MIDI,
    Audio
};

enum PatSync {
    Off,
    QuarterBeat,
    HalfBeat,
    Beat_x1,
    Beat_x2,
    Beat_x4
};

/*
    RC lowpass filter with two resitances a or b
    Used for attack release smooth or single smooth of ypos
*/
class SmoothParam
{
public:
    double a; // resistance a
    double b; // resistance b
    double lp; // last value
    double smooth; // latest value

    void rcSet2(double rca, double rcb, double srate)
    {
        a = 1 / (rca * srate + 1);
        b = 1 / (rcb * srate + 1);
    }

    double rcLP2(double s, bool ab)
    {
        return lp += ab ? a * (s - lp) : b * (s - lp);
    }

    double smooth2(double s, bool ab)
    {
        lp = smooth;
        return smooth = rcLP2(s, ab);
    }
};

//==============================================================================
/**
*/
class GATE12AudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorParameter::Listener
{
public:
    static constexpr int GRID_SIZES[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 24, 28, 32, 36, 40, 48, 56, 64 };

    // Plugin settings
    float scale = 1.0f; // UI scale factor

    // Instance Settings
    bool alwaysPlaying = false;
    bool dualSmooth = true; // use either single smooth or attack and release
    bool MIDIHoldEnvelopeTail = true; // MIDI trigger - keep processing last position after envelope finishes
    bool AudioHoldEnvelopeTail = false; // Audio trigger - keep processing last position after envelope finishes
    int triggerChn = 9; // Midi pattern trigger channel, defaults to channel 10
    bool useMonitor = false; 
    bool useSidechain = false;

    // State
    Pattern* pattern; // current pattern
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
    SmoothParam* value; // smooths envelope value

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

    //==============================================================================
    GATE12AudioProcessor();
    ~GATE12AudioProcessor() override;

    void loadSettings();
    void saveSettings();
    void setScale(float value);
    int getCurrentGrid();

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
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState params;
    juce::UndoManager undoManager;

private:
    Pattern* patterns[12];
    Transient transDetectorL;
    Transient transDetectorR;
    bool paramChanged = false; // flag that triggers on any param change
    juce::ApplicationProperties settings;
    std::vector<MIDIMsg> midi; // midi buffer used to process midi messages offset

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GATE12AudioProcessor)
};
