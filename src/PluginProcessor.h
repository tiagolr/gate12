/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include "dsp/Pattern.h"

struct MIDIMsg {
    int offset;
    int isNoteon;
    int note;
    int vel;
    int channel;
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
    // Global settings
    float scale = 1.0f; // UI scale factor

    // Settings
    bool alwaysPlaying = false;
    bool drawWave = true;
    bool linkEdgePoints = false;
    bool dualSmooth = true; // use either single smooth or attack and release
    int triggerChn = 9; // Midi pattern trigger channel, defaults to channel 10
    int grid = 8; // grid divisions

    // State
    Pattern* pattern; // current pattern
    int queuedPattern = 0; // queued pat index, 0 = off
    bool isPlaying = false;
    int currentProgram = -1;
    int viewW = 1; // viewport width, used for buffers of samples to draw waveforms
    std::vector<double> preSamples; // used by view to draw pre audio
    std::vector<double> postSamples; // used by view to draw post audio
    double xpos = 0.0; // envelope x pos (0..1)
    double ypos = 0.0; // envelope y pos (0..1)
    double syncQN = 0.0; // sync quarter notes
    bool midiTrigger = false; // flag midi has triggered envelope
    SmoothParam* value;
    double beatPos = 0.0;
    double ppqPosition = 0.0;
    double beatsPerSample = 4.0;

    //==============================================================================
    GATE12AudioProcessor();
    ~GATE12AudioProcessor() override;
    void setSmooth();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;
    bool supportsDoublePrecisionProcessing() const override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void onSlider ();
    void onPlay ();
    double getY(double x, double min, double max);
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

    void loadSettings();
    void saveSettings();
    void setScale(float value);

    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState params;
    juce::UndoManager undoManager;

private:
    Pattern* patterns[12];
    bool paramChanged = false; // flag that triggers on any param change
    juce::ApplicationProperties settings;
    std::vector<MIDIMsg> midi; // midi buffer used to process midi messages offset

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GATE12AudioProcessor)
};
