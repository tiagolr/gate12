#include "SequencerWidget.h"
#include "../PluginProcessor.h"

SequencerWidget::SequencerWidget(GATE12AudioProcessor& p) : audioProcessor(p) 
{
	auto addButton = [this](TextButton& button, String label, int col, int row, Colour color, SeqEditMode mode) {
		addAndMakeVisible(button);
		button.setButtonText(label);
		button.setComponentID("button");
		button.setColour(TextButton::buttonColourId, color);
		button.setColour(TextButton::buttonOnColourId, color);
		button.setColour(TextButton::textColourOnId, Colour(COLOR_BG));
		button.setColour(TextButton::textColourOffId, color);
		button.setBounds(col,row,55,25);
		button.onClick = [this, mode]() {
			audioProcessor.sequencer->editMode = mode;
			updateButtonsState();
			repaint();
		};
	};

	int col = 0;int row = 0;
	addButton(maxButton, "Max", col, row, Colours::white, SeqEditMode::SMax);col += 65;
	addButton(minButton, "Min", col, row, Colours::white, SeqEditMode::SMin);col += 65;
	addButton(flipXButton, "FlipX", col, row, Colours::aqua, SeqEditMode::SFlipX);col += 65;
	addButton(flipYButton, "FlipY", col, row, Colours::aqua, SeqEditMode::SFlipY);col = 0; row += 35;
	addButton(tenaButton, "TenA", col, row, Colours::yellow, SeqEditMode::STenAtt);col += 65;
	addButton(tenrButton, "TenR", col, row, Colours::yellow, SeqEditMode::STenRel);col += 65;
	addButton(tenButton, "Ten", col, row, Colour(0xffff8080), SeqEditMode::STension);col += 65;
	addButton(gateButton, "Gate", col, row, Colours::chocolate, SeqEditMode::SGate);col += 65;
	maxButton.setToggleState(true, dontSendNotification);
	updateButtonsState();
}

void SequencerWidget::updateButtonsState()
{
	auto mode = audioProcessor.sequencer->editMode;
	maxButton.setToggleState(mode == SeqEditMode::SMax, dontSendNotification);
	minButton.setToggleState(mode == SeqEditMode::SMin, dontSendNotification);
	flipXButton.setToggleState(mode == SeqEditMode::SFlipX, dontSendNotification);
	flipYButton.setToggleState(mode == SeqEditMode::SFlipY, dontSendNotification);
	tenaButton.setToggleState(mode == SeqEditMode::STenAtt, dontSendNotification);
	tenrButton.setToggleState(mode == SeqEditMode::STenRel, dontSendNotification);
	tenButton.setToggleState(mode == SeqEditMode::STension, dontSendNotification);
	gateButton.setToggleState(mode == SeqEditMode::SGate, dontSendNotification);
}

void SequencerWidget::paint(Graphics& g)
{
	(void)g;
	//g.setColour(Colour(COLOR_NEUTRAL));
	//auto grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
	//auto gridx = getWidth() / grid;
	//for (int i = 0; i < grid; ++i) {
	//	auto w = gridx/2;
	//	auto h = 2;
	//	auto bounds = Rectangle<int>(gridx*i+gridx/2-w/2,getHeight()-PLUG_PADDING+h/2, w, h);
	//	g.fillRoundedRectangle(bounds.toFloat(), 3.0f);
	//}
}

void SequencerWidget::mouseDown(const juce::MouseEvent& e) 
{
	(void)e;
}