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
		button.setBounds(col,row,60,25);
		button.onClick = [this, mode]() {
			audioProcessor.sequencer->editMode = audioProcessor.sequencer->editMode == mode ? EditMax : mode;
			updateButtonsState();
		};
	};

	auto addToolButton = [this](TextButton& button, int col, int row, int w, int h, CellShape shape) {
		addAndMakeVisible(button);
		button.setBounds(col, row, w, h);
		button.onClick = [this, shape]() {
			audioProcessor.sequencer->selectedShape = audioProcessor.sequencer->selectedShape == shape ? SNone : shape;
			audioProcessor.showPaintWidget = audioProcessor.sequencer->selectedShape == SPTool;
			auto editMode = audioProcessor.sequencer->editMode;
			if (editMode != EditMin && editMode != EditMax) {
				audioProcessor.sequencer->editMode = EditMax;
				updateButtonsState();
			}
			audioProcessor.sendChangeMessage(); // refresh ui
		};
		button.setAlpha(0.f);
	};

	int col = 0;int row = 0;
	addButton(flipXBtn, "FlipX", col, row, EditInvertX);col += 70;
	addButton(maxBtn, "Paint", col, row, EditMax);col = 0;row = 35;
	addButton(skewBtn, "Skew", col, row, EditSkew);col += 70;
	addButton(tenBtn, "Ten", col, row, EditTension);col += 70;

	row = 0;
	col = maxBtn.getBounds().getRight() + 20;
	addToolButton(silenceBtn, col, row, 25, 25, CellShape::SSilence); col += 25;
	addToolButton(lineBtn, col, row, 25, 25, CellShape::SLine); col += 25;
	addToolButton(rampdnBtn, col, row, 25, 25, CellShape::SRampDn); col += 25;
	addToolButton(rampupBtn, col, row, 25, 25, CellShape::SRampUp); col += 25;
	addToolButton(triBtn, col, row, 25, 25, CellShape::STri); col += 25;
	addToolButton(ptoolBtn, col, row, 25, 25, CellShape::SPTool); col += 25;

	row = 35;
	col = maxBtn.getRight() + 10;
	addAndMakeVisible(randomBtn);
	randomBtn.setBounds(col,row,25,25);
	randomBtn.setAlpha(0.f);
	randomBtn.onClick = [this]() {
		auto snap = audioProcessor.sequencer->cells;
		audioProcessor.sequencer->randomize(audioProcessor.sequencer->editMode, randomMin, randomMax);
		audioProcessor.sequencer->createUndo(snap);
	};

	col += 25;
	addAndMakeVisible(randomMenuBtn);
	randomMenuBtn.setAlpha(0.f);
	randomMenuBtn.setBounds(col,row,25,25);
	randomMenuBtn.onClick = [this]() {
		PopupMenu menu;
		menu.addItem(1, "Random All");
		Point<int> pos = localPointToGlobal(randomMenuBtn.getBounds().getTopRight());
		menu.showMenuAsync(PopupMenu::Options().withTargetScreenArea({ pos.getX(), pos.getY(), 1, 1 }), [this](int result) {
			if (result == 1 || result == 2) {
				auto snap = audioProcessor.sequencer->cells;
				audioProcessor.sequencer->clear(EditMax);
				audioProcessor.sequencer->clear(EditMin);
				audioProcessor.sequencer->randomize(EditMax, randomMin, randomMax);
				audioProcessor.sequencer->randomize(EditMin, randomMin, randomMax);
				audioProcessor.sequencer->randomize(EditTenAtt, randomMin, randomMax);
				audioProcessor.sequencer->randomize(EditTenRel, randomMin, randomMax);
				audioProcessor.sequencer->randomize(EditInvertX, randomMin, randomMax);
				if (result == 2) {
					audioProcessor.sequencer->randomize(EditSilence, randomMin, randomMax);
				}
				audioProcessor.sequencer->createUndo(snap);
			}
			else if (result == 3) {
				auto snap = audioProcessor.sequencer->cells;
				audioProcessor.sequencer->randomize(EditSilence, randomMin, randomMax);
				audioProcessor.sequencer->createUndo(snap);
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
	randomRange.setVelocityModeParameters(1.0,1,0.0,true,ModifierKeys::Flags::shiftModifier);

	addAndMakeVisible(clearBtn);
	clearBtn.setButtonText("Clear");
	clearBtn.setComponentID("button");
	clearBtn.setBounds(getRight() - 60, row, 60, 25);
	clearBtn.onClick = [this]() {
		audioProcessor.sequencer->clear(audioProcessor.sequencer->editMode);
	};

	row = 0;
	col = getWidth();

	addAndMakeVisible(resetBtn);
	resetBtn.setButtonText("Reset");
	resetBtn.setComponentID("button");
	resetBtn.setBounds(col-60,row,60,25);
	resetBtn.onClick = [this]() {
		auto snap = audioProcessor.sequencer->cells;
		audioProcessor.sequencer->clear();
		audioProcessor.sequencer->createUndo(snap);
		audioProcessor.sequencer->build();
	};

	col -= 70;
	addAndMakeVisible(applyBtn);
	applyBtn.setButtonText("Apply");
	applyBtn.setComponentID("button");
	applyBtn.setBounds(col-60,row,60,25);
	applyBtn.onClick = [this]() {
		audioProcessor.sequencer->apply();
		audioProcessor.toggleSequencerMode();
	};

	updateButtonsState();
}

void SequencerWidget::resized()
{
	auto row = 0;
	auto col = getWidth();
	resetBtn.setBounds(col-60,row,60,25);
	col -= 70;
	applyBtn.setBounds(col-60,row,60,25);

	auto bounds = clearBtn.getBounds();
	clearBtn.setBounds(bounds.withRightX(getWidth()));

	randomBtn.setBounds(randomBtn.getBounds().withX(silenceBtn.getX()));
	randomMenuBtn.setBounds(randomMenuBtn.getBounds().withX(silenceBtn.getRight()));
	randomRange.setBounds(randomRange.getBounds()
		.withX(randomMenuBtn.getBounds().getRight())
		.withRight(ptoolBtn.getBounds().getRight()));
}

void SequencerWidget::updateButtonsState()
{
	auto mode = audioProcessor.sequencer->editMode;
	auto modeColor = audioProcessor.sequencer->getEditModeColour(audioProcessor.sequencer->editMode);
	maxBtn.setToggleState(mode == SeqEditMode::EditMax, dontSendNotification);
	flipXBtn.setToggleState(mode == SeqEditMode::EditInvertX, dontSendNotification);
	skewBtn.setToggleState(mode == SeqEditMode::EditSkew, dontSendNotification);
	tenBtn.setToggleState(mode == SeqEditMode::EditTension, dontSendNotification);
	randomBtn.setColour(TextButton::buttonColourId, modeColor);
	randomRange.setColour(Slider::backgroundColourId, Colour(COLOR_BG).brighter(0.1f));
	randomRange.setColour(Slider::trackColourId, Colour(COLOR_ACTIVE).darker(0.5f));
	randomRange.setColour(Slider::thumbColourId, Colour(COLOR_ACTIVE));
	clearBtn.setColour(TextButton::buttonColourId, modeColor);
	clearBtn.setColour(TextButton::buttonOnColourId, modeColor);
	clearBtn.setColour(TextButton::textColourOnId, Colour(COLOR_BG));
	clearBtn.setColour(TextButton::textColourOffId, modeColor);

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
	g.fillRoundedRectangle(Rectangle<float>(bounds.getX() + 4, bounds.getY(), 0.f, 4.f).withRight(bounds.getRight()-2.f), 1.f);

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
	g.setColour(seq->getEditModeColour(seq->editMode));
	bounds = randomMenuBtn.getBounds().toFloat();
	r = 2.0f;
	bounds = Rectangle<float>(bounds.getCentreX() - r, bounds.getCentreY()-r, r*2.f,r*2.f);
	g.fillEllipse(bounds);
	g.fillEllipse(bounds.translated(0.f,-r*3.f));
	g.fillEllipse(bounds.translated(0.f,r*3.f));
}

void SequencerWidget::mouseDown(const juce::MouseEvent& e) 
{
	(void)e;
}