#include "SequencerWidget.h"
#include "../PluginProcessor.h"

SequencerWidget::SequencerWidget(GATE12AudioProcessor& p) : audioProcessor(p) 
{
}

void SequencerWidget::paint(Graphics& g)
{
	(void)g;
}

void SequencerWidget::mouseDown(const juce::MouseEvent& e) 
{
	(void)e;
}