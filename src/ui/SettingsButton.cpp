#include "SettingsButton.h"
#include "../PluginProcessor.h"
#include "../Globals.h"

void SettingsButton::mouseDown(const juce::MouseEvent& e) 
{
	(void)e;

	PopupMenu uiScale;
	uiScale.addItem(1, "100%", true, audioProcessor.scale == 1.0f);
	uiScale.addItem(2, "125%", true, audioProcessor.scale == 1.25f);
	uiScale.addItem(3, "150%", true, audioProcessor.scale == 1.5f);
	uiScale.addItem(4, "175%", true, audioProcessor.scale == 1.75f);
	uiScale.addItem(5, "200%", true, audioProcessor.scale == 2.0f);

	PopupMenu triggerChn;
	triggerChn.addItem(10, "Off", true, audioProcessor.triggerChn == -1);
	for (int i = 0; i < 16; i++) {
		triggerChn.addItem(10 + i + 1, String(i + 1), true, audioProcessor.triggerChn == i);
	}
	triggerChn.addItem(27, "Any", true, audioProcessor.triggerChn == 16);

	PopupMenu options;
	options.addSubMenu("UI Scale", uiScale);
	options.addSeparator();
	options.addSubMenu("Trigger Channel", triggerChn);
	options.addItem(30, "Dual smooth", true, audioProcessor.dualSmooth);
	options.addItem(31, "Link edge points", true, audioProcessor.linkEdgePoints);
	options.addItem(32, "Draw wave", true, audioProcessor.drawWave);

	PopupMenu load;
	load.addItem(100, "Sine");
	load.addItem(101, "Triangle");
	load.addItem(102, "Random");

	PopupMenu menu;
	auto menuPos = localPointToGlobal(getLocalBounds().getBottomRight());
	menu.addSubMenu("Options", options);
	menu.addSeparator();
	menu.addItem(50, "Invert");
	menu.addItem(51, "Reverse");
	menu.addItem(52, "Clear");
	menu.addItem(53, "Copy");
	menu.addItem(54, "Paste");
	menu.addSeparator();
	menu.addSubMenu("Load", load);
	menu.addItem(1000, "About");
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetScreenArea({menuPos.getX() -110, menuPos.getY(), 1, 1}),
		[this](int result) {
			if (result == 0) return;
			if (result >= 1 && result <= 5) { // UI Scale
				audioProcessor.setScale(result == 5 ? 2.0f : result == 4 ? 1.75f : result == 3 ? 1.5f : result == 2 ? 1.25f : 1.0f);
				onScaleChange();
			}
			if (result >= 10 && result <= 27) { // Trigger channel
				audioProcessor.triggerChn = result - 10 - 1;
			}
			if (result == 30) { // Dual smooth
				audioProcessor.dualSmooth = !audioProcessor.dualSmooth;
				toggleUIComponents();
			}
			if (result == 31) { // Link edge points
				audioProcessor.linkEdgePoints = !audioProcessor.linkEdgePoints;
			}
			if (result == 32) { // Draw wave
				audioProcessor.drawWave = !audioProcessor.drawWave;
			}
			if (result == 50) {
				audioProcessor.pattern->invert();
				audioProcessor.pattern->buildSegments();
			}
			if (result == 51) {
				audioProcessor.pattern->reverse();
				audioProcessor.pattern->buildSegments();
			}
			if (result == 52) {
				audioProcessor.pattern->clear();
				audioProcessor.pattern->buildSegments();
			}
			if (result == 53) {
				audioProcessor.pattern->copy();
			}
			if (result == 54) {
				audioProcessor.pattern->paste();
				audioProcessor.pattern->buildSegments();
			}
			if (result >= 100 && result <= 200) { // load
				if (result == 100) { // load sine
					audioProcessor.pattern->loadSine();
					audioProcessor.pattern->buildSegments();
				}
				if (result == 101) { // load triangle
					audioProcessor.pattern->loadTriangle();
					audioProcessor.pattern->buildSegments();
				}
				if (result == 102) { // load random
					audioProcessor.pattern->loadRandom(audioProcessor.grid);
					audioProcessor.pattern->buildSegments();
				}
			}
			if (result == 1000) {
				toggleAbout();
			}
		}
	);
};

void SettingsButton::paint(Graphics& g) 
{
	auto bounds = getLocalBounds();
	bounds.removeFromTop(4);
	bounds.removeFromBottom(4);
	auto middle = bounds.expanded(0,0);
	auto h = bounds.getHeight();
	auto thick = 3;
	g.setColour(Colour(globals::COLOR_ACTIVE));
	g.fillRoundedRectangle(bounds.removeFromBottom(thick).toFloat(), 2.f);
	g.fillRoundedRectangle(bounds.removeFromTop(thick).toFloat(), 2.f);
	middle.removeFromBottom(h/2 - thick/2);
	middle.removeFromTop(h/2 - thick/2);
	g.fillRoundedRectangle(middle.toFloat(), 2.f);
};

