#include "BandsWidget.h"
#include "../PluginEditor.h"

BandsWidget::BandsWidget(GATE12AudioProcessorEditor& e)
	: editor(e)
{
	startTimerHz(30);
	addAndMakeVisible(slopeBtn);
	slopeBtn.setAlpha(0.f);
	slopeBtn.onClick = [this]
		{
			auto param = editor.audioProcessor.params.getParameter("split_slope");
			auto val = int(param->convertFrom0to1(param->getValue()) + 1) % 3;
			param->setValueNotifyingHost(param->convertTo0to1((float)val));
		};
}
BandsWidget::~BandsWidget()
{
}

void BandsWidget::timerCallback()
{
	if (isShowing()) {
		if (editor.audioProcessor.showBandsEditor &&
			editor.audioProcessor.bandsFFTReady.exchange(false, std::memory_order_acquire)) {
			recalcFFTMags();
		}

		repaint();
	}
}

void BandsWidget::drawWaveform(juce::Graphics& g)
{
	auto size = fftMagnitudes.size();
	auto bounds = getLocalBounds().reduced(1).toFloat();

	if (size == 0)
		return;

	juce::Path waveformPath;

	const float minDB = -90.0f;
	const float maxDB = 0.0f;
	const float minFreq = 20.0f;
	const float maxFreq = 20000.f;
	const float srate = static_cast<float>(editor.audioProcessor.srate);

	waveformPath.startNewSubPath(bounds.getX(), bounds.getBottom());

	for (size_t i = 0; i < size; ++i) {
		float freq = (i * srate) / (2.0f * (size - 1));
		freq = std::max(freq, minFreq);
		float logPos = std::log10(freq / minFreq) / std::log10(maxFreq / minFreq);
		float x = bounds.getX() + logPos * bounds.getWidth();

		float magnitudeDB = juce::Decibels::gainToDecibels(fftMagnitudes[i], minDB);
		float y = juce::jmap(magnitudeDB, minDB, maxDB, bounds.getBottom(), bounds.getY());

		waveformPath.lineTo(x, y);
	}

	waveformPath.lineTo(bounds.getX() + bounds.getWidth(), bounds.getBottom());

	g.setColour(Colour(COLOR_ACTIVE).withAlpha(0.5f));
	g.fillPath(waveformPath);
}

void BandsWidget::recalcFFTMags()
{
	auto fftSize = 1 << BANDS_FFT_ORDER;
	auto writeIndex = editor.audioProcessor.bandsFFTWriteIndex;
	auto bufferSize = fftData.size();
	size_t startIndex = (writeIndex + bufferSize - fftSize) % bufferSize;

	if (startIndex + fftSize <= bufferSize) {
		std::copy_n(&editor.audioProcessor.bandsFFTBuffer[startIndex], fftSize, fftData.data());
	}
	else {
		// Wrap-around copy
		size_t firstPart = bufferSize - startIndex;
		std::copy_n(&editor.audioProcessor.bandsFFTBuffer[startIndex], firstPart, fftData.data());
		std::copy_n(&editor.audioProcessor.bandsFFTBuffer[0], fftSize - firstPart, fftData.data() + firstPart);
	}

	window.multiplyWithWindowingTable(fftData.data(), fftData.size());
	fft.performFrequencyOnlyForwardTransform(fftData.data(), true);

	float norm = 1.f / (fftData.size() * 0.5f);
	for (size_t j = 0; j < fftSize / 2; ++j) {
	    float mag = fftData[j] * norm;
	    fftMagnitudes[j] = mag * 0.8f + fftMagnitudes[j] * 0.2f;
	}
}

void BandsWidget::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
}

void BandsWidget::paint(Graphics& g)
{
	g.fillAll(Colour(COLOR_ACTIVE));
	auto bounds = getLocalBounds().reduced(1).toFloat();
	g.setColour(Colour(COLOR_BG));
	g.fillRect(bounds);
	
	float splitLeft = editor.audioProcessor.params.getRawParameterValue("split_low")->load();
	float splitRight = editor.audioProcessor.params.getRawParameterValue("split_high")->load();

	const float logMin = std::log(20.f);
	const float logScale = 1.f / (std::log(20000.f) - logMin);
	const float bandw = 15;
	auto lnorm = (std::log(splitLeft) - logMin) * logScale;
	auto rnorm = (std::log(splitRight) - logMin) * logScale;

	auto lx = bounds.getX() + bounds.getWidth() * lnorm;
	lband = { lx - bandw / 2, bounds.getY(), bandw, bounds.getHeight() };
	auto rx = bounds.getX() + bounds.getWidth() * rnorm;
	rband = { rx - bandw / 2, bounds.getY(), bandw, bounds.getHeight() };

	g.setColour(Colour(COLOR_ACTIVE).darker(0.8f).withAlpha(0.25f));
	g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth() * lnorm, bounds.getHeight());
	g.fillRect(bounds.getRight() - bounds.getWidth() * (1.f - rnorm), bounds.getY(), bounds.getWidth() * (1.f - rnorm), bounds.getHeight());

	drawWaveform(g);

	g.saveState();
	g.setColour(Colours::white);
	g.reduceClipRegion(bounds.toNearestInt());
	g.fillRect(lband.reduced(bandw / 2.f - 1, 0));
	g.fillRect(rband.reduced(bandw / 2.f - 1, 0));
	Path p;
	p.addTriangle(lband.getBottomLeft(), lband.getCentre().withY(bounds.getBottom() - 10), lband.getBottomRight());
	p.addTriangle(rband.getBottomLeft(), rband.getCentre().withY(bounds.getBottom() - 10), rband.getBottomRight());
	g.fillPath(p);
	g.restoreState();

	g.setColour(Colours::white.withAlpha(0.25f));
	if (mouseOverBand == 1 && isMouseOver() && !isDragging)
		g.fillRect(lband);
	if (mouseOverBand == 2 && isMouseOver() && !isDragging)
		g.fillRect(rband);

	auto formatHz = [](float hz)
		{
			if (hz < 1000)
				return String(hz, 0) + " Hz";
			return String(hz / 1000.f, 1) + " kHz";
		};

	g.setFont(14.f);
	g.setColour(Colours::white);
	if (mouseOverBand == 1)
		g.drawText(formatHz(splitLeft), lband.expanded(40, 0), lnorm > 0.5 ? Justification::left : Justification::right);
	if (mouseOverBand == 2)
		g.drawText(formatHz(splitRight), rband.expanded(40, 0), rnorm > 0.5 ? Justification::left : Justification::right);

	auto slope = (int)editor.audioProcessor.params.getRawParameterValue("split_slope")->load();
	g.setColour(Colour(COLOR_ACTIVE));
	g.drawText(slope == 0 ? "6dB" : slope == 1 ? "12dB" : "24dB", slopeBtn.getBounds().toFloat(), Justification::centred);
}

void BandsWidget::mouseMove(const MouseEvent& e)
{
	if (isDragging) return;
	auto bounds = getLocalBounds();
	bool preferLeft = e.getPosition().x > bounds.getCentreX(); // FIX remove conflicts when picking overlayed bars

	if (preferLeft) {
		if (lband.contains(e.getPosition().toFloat()))
			mouseOverBand = 1;
		else if (rband.contains(e.getPosition().toFloat()))
			mouseOverBand = 2;
		else 
			mouseOverBand = 0;
	}
	else {
		if (rband.contains(e.getPosition().toFloat()))
			mouseOverBand = 2;
		else if (lband.contains(e.getPosition().toFloat()))
			mouseOverBand = 1;
		else
			mouseOverBand = 0;
	}
}

void BandsWidget::mouseDown(const MouseEvent& e)
{
	if (mouseOverBand == 0) return;
	isDragging = true;
	setMouseCursor(MouseCursor::NoCursor);
	start_mouse_pos = Desktop::getInstance().getMousePosition();
	last_mouse_pos = e.getPosition();
	e.source.enableUnboundedMouseMovement(true);
	cur_normed_value = mouseOverBand == 1
		? editor.audioProcessor.params.getParameter("split_low")->getValue()
		: editor.audioProcessor.params.getParameter("split_high")->getValue();
}

void BandsWidget::mouseUp(const MouseEvent& e)
{
	if (!isDragging) return;
	setMouseCursor(MouseCursor::NormalCursor);
	isDragging = false;
	e.source.enableUnboundedMouseMovement(false);
	auto pt = mouseOverBand == 1
		? localPointToGlobal(lband.getCentre()).toInt()
		: localPointToGlobal(rband.getCentre()).toInt();
	Desktop::getInstance().setMousePosition(pt);
}

void BandsWidget::mouseDrag(const MouseEvent& e)
{
	if (!isDragging) return;

	auto change = e.getPosition() - last_mouse_pos;
	last_mouse_pos = e.getPosition();
	auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * 100.f;
	auto changeX = change.getX() / speed;

	cur_normed_value += changeX;
	cur_normed_value = std::clamp(cur_normed_value, 0.f, 1.f);

	if (mouseOverBand == 1) {
		auto limit = editor.audioProcessor.params.getParameter("split_high")->getValue();
		if (cur_normed_value > limit) cur_normed_value = limit;
	}
	else {
		auto limit = editor.audioProcessor.params.getParameter("split_low")->getValue();
		if (cur_normed_value < limit) cur_normed_value = limit;
	}

	editor.audioProcessor.params.getParameter(mouseOverBand == 1 ? "split_low" : "split_high")
		->setValueNotifyingHost(cur_normed_value);
}

void BandsWidget::mouseDoubleClick(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown() || mouseOverBand == 0)
		return;

	auto param = editor.audioProcessor.params.getParameter(mouseOverBand == 1 ? "split_low" : "split_high");
	param->setValueNotifyingHost(param->getDefaultValue());
}

void BandsWidget::resized()
{
	auto b = getLocalBounds().reduced(1);
	slopeBtn.setBounds(b.getRight() - 45, 2, 45, 25);
}