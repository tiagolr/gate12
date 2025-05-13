#include "SequencerWidget.h"
#include "../PluginProcessor.h"

SequencerWidget::SequencerWidget(GATE12AudioProcessor& p) : audioProcessor(p) 
{
	auto addButton = [this](TextButton& button, String label, int col, int row, SeqEditMode mode) {
		Colour color = audioProcessor.sequencer->getEditModeColour(mode);
		addAndMakeVisible(button);
		button.setButtonText(label);
		button.setComponentID("button");
		button.setColour(TextButton::buttonColourId, color);
		button.setColour(TextButton::buttonOnColourId, color);
		button.setColour(TextButton::textColourOnId, Colour(COLOR_BG));
		button.setColour(TextButton::textColourOffId, color);
		button.setBounds(col,row,55,25);
		button.onClick = [this, mode]() {
			audioProcessor.sequencer->editMode = audioProcessor.sequencer->editMode == mode ? EditNone : mode;
			updateButtonsState();
		};
	};

	auto addToolButton = [this](TextButton& button, int col, int row, int w, int h, CellShape shape) {
		addAndMakeVisible(button);
		button.setBounds(col, row, w, h);
		button.onClick = [this, shape]() {
			audioProcessor.sequencer->selectedShape = shape;
			audioProcessor.showPaintWidget = shape == SPTool;
			auto editMode = audioProcessor.sequencer->editMode;
			if (editMode != EditMin && editMode != EditMax && editMode != EditNone) {
				audioProcessor.sequencer->editMode = EditMax;
				updateButtonsState();
			}
			audioProcessor.sendChangeMessage(); // refresh ui
		};
		button.setAlpha(0.f);
	};

	int col = 0;int row = 0;
	addButton(flipXBtn, "FlipX", col, row, EditInvertX);col += 65;
	addButton(minBtn, "Min", col, row, EditMin);col += 65;
	addButton(maxBtn, "Max", col, row, EditMax);col = 0;row = 35;
	addButton(tenBtn, "Ten", col, row, EditTension);col += 65;
	addButton(tenaBtn, "TAtk", col, row, EditTenAtt);col += 65;
	addButton(tenrBtn, "TRel", col, row, EditTenRel);col += 65;

	row = 0;
	col = maxBtn.getRight() + 10;
	addToolButton(silenceBtn, col, row, 25, 25, CellShape::SSilence); col += 25;
	addToolButton(lineBtn, col, row, 25, 25, CellShape::SLine); col += 25;
	addToolButton(rampdnBtn, col, row, 25, 25, CellShape::SRampDn); col += 25;
	addToolButton(rampupBtn, col, row, 25, 25, CellShape::SRampUp); col += 25;
	addToolButton(triBtn, col, row, 25, 25, CellShape::STri); col += 25;
	addToolButton(ptoolBtn, col, row, 25, 25, CellShape::SPTool); col += 25;

	row = 35;
	col = maxBtn.getRight() + 10;
	addAndMakeVisible(randomBtn);
	randomBtn.setTooltip("Randomize selection");
	randomBtn.setBounds(col,row,25,25);
	randomBtn.setAlpha(0.f);
	randomBtn.onClick = [this]() {
		audioProcessor.sequencer->randomize(audioProcessor.sequencer->editMode, randomMin, randomMax);
	};

	col += 25;
	addAndMakeVisible(randomMenuBtn);
	randomMenuBtn.setAlpha(0.f);
	randomMenuBtn.setBounds(col,row,25,25);
	randomMenuBtn.onClick = [this]() {
		PopupMenu menu;
		menu.addItem(1, "Random All");
		menu.addItem(3, "Random Silence");
		menu.addItem(2, "Random Not Silence");
		Point<int> pos = localPointToGlobal(randomMenuBtn.getBounds().getTopRight());
		menu.showMenuAsync(PopupMenu::Options().withTargetScreenArea({ pos.getX(), pos.getY(), 1, 1 }), [this](int result) {
			if (result == 1 || result == 2) {
				audioProcessor.sequencer->clear(EditMax);
				audioProcessor.sequencer->clear(EditMin);
				audioProcessor.sequencer->randomize(EditMax, randomMin, randomMax);
				audioProcessor.sequencer->randomize(EditMin, randomMin, randomMax);
				audioProcessor.sequencer->randomize(EditTenAtt, randomMin, randomMax);
				audioProcessor.sequencer->randomize(EditTenRel, randomMin, randomMax);
				audioProcessor.sequencer->randomize(EditInvertX, randomMin, randomMax);
				audioProcessor.sequencer->randomize(EditSilence, randomMin, randomMax);
				if (result == 1) {
					audioProcessor.sequencer->randomize(EditSilence, randomMin, randomMax);
				}
			}
			else if (result == 3) {
				audioProcessor.sequencer->randomize(EditSilence, randomMin, randomMax);
			}
		});
	};

	addAndMakeVisible(randomRange);
	randomRange.setTooltip("Random min and max values");
	randomRange.setSliderStyle(Slider::SliderStyle::TwoValueHorizontal);
	randomRange.setRange(0.0, 1.0);
	randomRange.setMinAndMaxValues(0.0, 1.0);
	randomRange.setTextBoxStyle(Slider::NoTextBox, false, 80, 20);
	randomRange.setBounds(randomMenuBtn.getRight(), row, ptoolBtn.getRight() - randomMenuBtn.getRight(), 25);
	randomRange.onValueChange = [this]() {
		randomMin = randomRange.getMinValue();
		randomMax = randomRange.getMaxValue();
		if (randomMin > randomMax)
			randomRange.setMinAndMaxValues(randomMax, randomMax);
	};
	
	updateButtonsState();
}

void SequencerWidget::updateButtonsState()
{
	auto mode = audioProcessor.sequencer->editMode;
	auto modeColor = audioProcessor.sequencer->getEditModeColour(audioProcessor.sequencer->editMode);
	maxBtn.setToggleState(mode == SeqEditMode::EditMax, dontSendNotification);
	minBtn.setToggleState(mode == SeqEditMode::EditMin, dontSendNotification);
	flipXBtn.setToggleState(mode == SeqEditMode::EditInvertX, dontSendNotification);
	tenaBtn.setToggleState(mode == SeqEditMode::EditTenAtt, dontSendNotification);
	tenrBtn.setToggleState(mode == SeqEditMode::EditTenRel, dontSendNotification);
	tenBtn.setToggleState(mode == SeqEditMode::EditTension, dontSendNotification);
	randomBtn.setColour(TextButton::buttonColourId, modeColor);
	randomRange.setColour(Slider::backgroundColourId, Colour(COLOR_BG).brighter(0.1f));
	randomRange.setColour(Slider::trackColourId, Colour(COLOR_ACTIVE).darker(0.5f));
	randomRange.setColour(Slider::thumbColourId, Colour(COLOR_ACTIVE));

	repaint();
}

void SequencerWidget::paint(Graphics& g)
{
	auto& seq = audioProcessor.sequencer;
	auto bounds = silenceBtn.getBounds().toFloat();
	g.setColour(seq->selectedShape == CellShape::SSilence ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-6,-6);
	Path silencePath;
	silencePath.startNewSubPath(bounds.getBottomLeft());
	silencePath.lineTo(bounds.getTopRight());
	silencePath.startNewSubPath(bounds.getTopLeft());
	silencePath.lineTo(bounds.getBottomRight());
	g.strokePath(silencePath, PathStrokeType(1.f));

	bounds = rampdnBtn.getBounds().toFloat();
	g.setColour(seq->selectedShape == CellShape::SRampDn ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-5,-5);
	Path rampdnPath;
	rampdnPath.startNewSubPath(bounds.getBottomLeft());
	rampdnPath.lineTo(bounds.getTopLeft());
	rampdnPath.lineTo(bounds.getBottomRight());
	g.strokePath(rampdnPath, PathStrokeType(1.f));

	bounds = rampupBtn.getBounds().toFloat();
	g.setColour(seq->selectedShape == CellShape::SRampUp ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-5,-5);
	Path rampupPath;
	rampupPath.startNewSubPath(bounds.getBottomLeft());
	rampupPath.lineTo(bounds.getTopRight());
	rampupPath.lineTo(bounds.getBottomRight());
	g.strokePath(rampupPath, PathStrokeType(1.f));

	bounds = triBtn.getBounds().toFloat();
	g.setColour(seq->selectedShape == CellShape::STri ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-5,-5);
	Path triPath;
	triPath.startNewSubPath(bounds.getBottomLeft());
	triPath.lineTo(bounds.getTopRight().withX(bounds.getCentreX()));
	triPath.lineTo(bounds.getBottomRight());
	g.strokePath(triPath, PathStrokeType(1.f));

	bounds = lineBtn.getBounds().toFloat();
	g.setColour(seq->selectedShape == CellShape::SLine ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-5,-5);
	Path linePath;
	linePath.startNewSubPath(bounds.getBottomLeft().withY(bounds.getCentreY()));
	linePath.lineTo(bounds.getBottomRight().withY(bounds.getCentreY()));
	g.strokePath(linePath, PathStrokeType(1.f));

	bounds = ptoolBtn.getBounds().toFloat();
	g.setColour(seq->selectedShape == CellShape::SPTool ? Colour(COLOR_ACTIVE) : Colour(COLOR_NEUTRAL));
	//g.drawRect(bounds);
	bounds.expand(-3,-3);
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

	// draw random dice
	g.setColour(seq->getEditModeColour(seq->editMode));
	bounds = randomBtn.getBounds().expanded(-2,-2).toFloat();
	g.fillRoundedRectangle(bounds, 3.0f);
	g.setColour(Colour(COLOR_BG));
	auto r = 3.0f;
	auto circle = Rectangle<float>(bounds.getCentreX() - r, bounds.getCentreY() - r, r*2.f, r*2.f);
	g.fillEllipse(circle);
	g.fillEllipse(circle.translated(-6.f,-6.f));
	g.fillEllipse(circle.translated(6.f,-6.f));
	g.fillEllipse(circle.translated(-6.f,6.f));
	g.fillEllipse(circle.translated(6.f,6.f));

	// draw random menu btn
	g.setColour(Colour(COLOR_ACTIVE));
	bounds = randomMenuBtn.getBounds().translated(2,0).toFloat();
	Path p; r = 5.f;
	p.addTriangle(bounds.getCentre().translated(-r,-r),
		bounds.getCentre().translated(0.0,r),
		bounds.getCentre().translated(r,-r));
	g.fillPath(p);
}

void SequencerWidget::mouseDown(const juce::MouseEvent& e) 
{
	(void)e;
}