#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class GATE12AudioProcessorEditor;

class BandsWidget
	: public juce::Component
	, private juce::Timer
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	TextButton slopeBtn;

	BandsWidget(GATE12AudioProcessorEditor& e);
	~BandsWidget();

	void timerCallback();
	void parameterChanged(const juce::String& parameterID, float newValue) override;
	void paint(Graphics& g) override;
	void mouseMove(const MouseEvent& e) override;
	void mouseDown(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseDoubleClick(const MouseEvent& e) override;
	void resized() override;
	void drawWaveform(juce::Graphics& g);
	void recalcFFTMags();

private:
	GATE12AudioProcessorEditor& editor;
	juce::dsp::FFT fft{ BANDS_FFT_ORDER };
	juce::dsp::WindowingFunction<float> window{ 1 << BANDS_FFT_ORDER, juce::dsp::WindowingFunction<float>::blackmanHarris };
	std::array<float, (1 << BANDS_FFT_ORDER) * 2> fftData{};
	std::array<float, (1 << BANDS_FFT_ORDER) / 2> fftMagnitudes{};

	Rectangle<float> lband;
	Rectangle<float> rband;

	int mouseOverBand = 0;
	bool isDragging = false;
	float cur_normed_value = 0.f;
	Point<int> last_mouse_pos;
	Point<int> start_mouse_pos;
};