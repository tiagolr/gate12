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

	PopupMenu load;
	load.addItem(100, "Sine");
	load.addItem(101, "Triangle");
	load.addItem(102, "Random");
	load.addSeparator();

	PopupMenu patterns1;
	PopupMenu patterns2;
	PopupMenu patterns3;
	load.addSubMenu("Patterns 1", patterns1);
	load.addSubMenu("Patterns 2", patterns2);
	load.addSubMenu("Patterns 3", patterns3);

	patterns1.addItem(110, "All");
	patterns1.addItem(111, "Empty");
	patterns1.addItem(112, "Gate 2");
	patterns1.addItem(113, "Gate 4");
	patterns1.addItem(114, "Gate 8");
	patterns1.addItem(115, "Gate 12");
	patterns1.addItem(116, "Gate 16");
	patterns1.addItem(117, "Gate 24");
	patterns1.addItem(118, "Gate 32");
	patterns1.addItem(119, "Trance 1");
	patterns1.addItem(120, "Trance 2");
	patterns1.addItem(121, "Trance 3");
	patterns1.addItem(122, "Trance 4");

	patterns2.addItem(130, "All");
	patterns2.addItem(131, "Saw 1");
	patterns2.addItem(132, "Saw 2");
	patterns2.addItem(133, "Step 1");
	patterns2.addItem(134, "Step 1 FadeIn");
	patterns2.addItem(135, "Step 4 Gate");
	patterns2.addItem(136, "Off Beat");
	patterns2.addItem(137, "Dynamic 1/4");
	patterns2.addItem(138, "Swing");
	patterns2.addItem(139, "Gate Out");
	patterns2.addItem(140, "Gate In");
	patterns2.addItem(141, "Speed up");
	patterns2.addItem(142, "Speed Down");

	patterns3.addItem(150, "All");
	patterns3.addItem(151, "End Fade");
	patterns3.addItem(152, "End Gate");
	patterns3.addItem(152, "Tremolo Slow");
	patterns3.addItem(152, "Tremolo Fast");
	patterns3.addItem(152, "Sidechain");
	patterns3.addItem(152, "Drum Loop");
	patterns3.addItem(152, "Copter");
	patterns3.addItem(152, "AM");
	patterns3.addItem(152, "Fade In");
	patterns3.addItem(152, "Fade Out");
	patterns3.addItem(152, "Fade OutIn");
	patterns3.addItem(152, "Mute");

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
					int grid = audioProcessor.getCurrentGrid();
					audioProcessor.pattern->loadRandom(grid);
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

