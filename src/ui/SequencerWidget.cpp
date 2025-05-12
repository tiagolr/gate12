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

	auto addToolButton = [this](TextButton& button, int col, int row, int w, int h, CellShape shape) {
		addAndMakeVisible(button);
		button.setBounds(col, row, w, h);
		button.onClick = [this, shape]() {
			audioProcessor.sequencer->selectedShape = shape;
			repaint();
		};
		button.setAlpha(0.f);
	};

	int col = 0;int row = 0;
	addButton(maxBtn, "Max", col, row, Colours::white, SeqEditMode::SMax);col += 65;
	addButton(minBtn, "Min", col, row, Colours::white, SeqEditMode::SMin);col += 65;
	addButton(flipXBtn, "FlipX", col, row, Colours::aqua, SeqEditMode::SFlipX);col += 65;
	addButton(flipYBtn, "FlipY", col, row, Colours::aqua, SeqEditMode::SFlipY);col = 0; row += 35;
	addButton(tenaBtn, "TenA", col, row, Colours::yellow, SeqEditMode::STenAtt);col += 65;
	addButton(tenrBtn, "TenR", col, row, Colours::yellow, SeqEditMode::STenRel);col += 65;
	addButton(tenBtn, "Ten", col, row, Colour(0xffff8080), SeqEditMode::STension);col += 65;
	addButton(gateBtn, "Gate", col, row, Colours::chocolate, SeqEditMode::SGate);col += 65;

	row = 0;
	col = gateBtn.getRight() + 10;
	addToolButton(silenceBtn, col, row, 23, 23, CellShape::SSilence); col += 25;
	addToolButton(lineBtn, col, row, 23, 23, CellShape::SLine); col += 25;
	addToolButton(rampdnBtn, col, row, 23, 23, CellShape::SRampDn); col += 25;
	addToolButton(rampupBtn, col, row, 23, 23, CellShape::SRampUp); col += 25;
	addToolButton(triBtn, col, row, 23, 23, CellShape::STri); col += 25;
	addToolButton(ptoolBtn, col, row, 23, 23, CellShape::SPTool); col += 25;
	
	updateButtonsState();
}

void SequencerWidget::updateButtonsState()
{
	auto mode = audioProcessor.sequencer->editMode;
	maxBtn.setToggleState(mode == SeqEditMode::SMax, dontSendNotification);
	minBtn.setToggleState(mode == SeqEditMode::SMin, dontSendNotification);
	flipXBtn.setToggleState(mode == SeqEditMode::SFlipX, dontSendNotification);
	flipYBtn.setToggleState(mode == SeqEditMode::SFlipY, dontSendNotification);
	tenaBtn.setToggleState(mode == SeqEditMode::STenAtt, dontSendNotification);
	tenrBtn.setToggleState(mode == SeqEditMode::STenRel, dontSendNotification);
	tenBtn.setToggleState(mode == SeqEditMode::STension, dontSendNotification);
	gateBtn.setToggleState(mode == SeqEditMode::SGate, dontSendNotification);
}

void SequencerWidget::paint(Graphics& g)
{
	auto bounds = silenceBtn.getBounds().toFloat();
	g.setColour(audioProcessor.sequencer->selectedShape == CellShape::SSilence ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-4,-4);
	Path silencePath;
	silencePath.startNewSubPath(bounds.getBottomLeft());
	silencePath.lineTo(bounds.getTopRight());
	silencePath.startNewSubPath(bounds.getTopLeft());
	silencePath.lineTo(bounds.getBottomRight());
	g.strokePath(silencePath, PathStrokeType(1.f));

	bounds = rampdnBtn.getBounds().toFloat();
	g.setColour(audioProcessor.sequencer->selectedShape == CellShape::SRampDn ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-4,-4);
	Path rampdnPath;
	rampdnPath.startNewSubPath(bounds.getBottomLeft());
	rampdnPath.lineTo(bounds.getTopLeft());
	rampdnPath.lineTo(bounds.getBottomRight());
	g.strokePath(rampdnPath, PathStrokeType(1.f));

	bounds = rampupBtn.getBounds().toFloat();
	g.setColour(audioProcessor.sequencer->selectedShape == CellShape::SRampUp ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-4,-4);
	Path rampupPath;
	rampupPath.startNewSubPath(bounds.getBottomLeft());
	rampupPath.lineTo(bounds.getTopRight());
	rampupPath.lineTo(bounds.getBottomRight());
	g.strokePath(rampupPath, PathStrokeType(1.f));

	bounds = triBtn.getBounds().toFloat();
	g.setColour(audioProcessor.sequencer->selectedShape == CellShape::STri ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-4,-4);
	Path triPath;
	triPath.startNewSubPath(bounds.getBottomLeft());
	triPath.lineTo(bounds.getTopRight().withX(bounds.getCentreX()));
	triPath.lineTo(bounds.getBottomRight());
	g.strokePath(triPath, PathStrokeType(1.f));

	bounds = lineBtn.getBounds().toFloat();
	g.setColour(audioProcessor.sequencer->selectedShape == CellShape::SLine ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-4,-4);
	Path linePath;
	linePath.startNewSubPath(bounds.getBottomLeft().withY(bounds.getCentreY()));
	linePath.lineTo(bounds.getBottomRight().withY(bounds.getCentreY()));
	g.strokePath(linePath, PathStrokeType(1.f));

	bounds = ptoolBtn.getBounds().toFloat();
	g.setColour(audioProcessor.sequencer->selectedShape == CellShape::SPTool ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-2,-2);
	Path paintPath;
	paintPath.startNewSubPath(bounds.getCentre());
	paintPath.lineTo(bounds.getCentre().translated(0,4));
	paintPath.startNewSubPath(bounds.getCentre());
	paintPath.lineTo(bounds.getTopLeft().translated(0,8));
	paintPath.lineTo(bounds.getTopLeft().translated(0,2));
	paintPath.lineTo(bounds.getTopLeft().translated(4,2));
	g.strokePath(paintPath, PathStrokeType(1.f));
	g.fillRoundedRectangle(Rectangle<float>(bounds.getCentreX() - 2, bounds.getCentreY()+2, 4.f,8.f).withBottom(bounds.getBottom()), 1.f);
	g.fillRoundedRectangle(Rectangle<float>(bounds.getX() + 4, bounds.getY(), 0.f, 4.f).withRight(bounds.getRight()), 1.f);
}

void SequencerWidget::mouseDown(const juce::MouseEvent& e) 
{
	(void)e;
}