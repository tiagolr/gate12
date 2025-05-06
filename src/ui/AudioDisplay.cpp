/*
  ==============================================================================

    Audio Display
    Author:  tiagolr

  ==============================================================================
*/

#include "AudioDisplay.h"
#include "../PluginProcessor.h"
#include "../Globals.h"

AudioDisplay::AudioDisplay(GATE12AudioProcessor& p) : audioProcessor(p)
{
    startTimerHz(60);
};

void AudioDisplay::timerCallback()
{
    if (isVisible())
        repaint();
}

void AudioDisplay::resized()
{
    MessageManager::callAsync([this] {
        audioProcessor.monW = getWidth();
    });
}

void AudioDisplay::paint(Graphics& g) {
    auto bounds = getLocalBounds();
    g.setColour(Colours::white.withAlpha(0.4f));
    g.drawRect(bounds);
    g.setColour(Colour(0xff7f7f7f));
    const int width = getWidth();
    const int height = getHeight();
    const int index = (int)audioProcessor.monpos.load();

    for (int i = 0; i < width; ++i) {
        double sample = audioProcessor.monSamples[(index + i) % width];
        if (i == 0) sample = 0.0f; // ignore first pixel, fixes glitching
        bool hit = sample >= 10.0; // trigger hits are encoded as amplitude +10
        if (hit)
            sample -= 10.0;
        sample = jlimit(0.0,1.0,sample);

        if (sample > 0.0) {
            g.drawLine((float)i, (float)height,(float)i, (float)(height - sample * height), 1.0f);
        }
        if (hit) {
            g.setColour(Colour(globals::COLOR_AUDIO));
            g.drawLine((float)i, (float)height,(float)i, (float)(height - sample * height), 1.0f);
            g.fillEllipse((float)(i - 2), (float)(height - sample * height)-2.f,4.f,4.f);
            g.setColour(Colour(0xff7f7f7f));
        }
    }

    auto thres = audioProcessor.params.getRawParameterValue("threshold")->load();
    g.setColour(Colours::white.withAlpha(.4f));
    g.drawLine(0.f, (float)(height - thres * height), (float)width, (float)(height - thres * height));
}