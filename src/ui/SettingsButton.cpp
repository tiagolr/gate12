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
	options.addSubMenu("Trigger Channel", triggerChn);
	options.addItem(30, "Dual smooth", true, audioProcessor.dualSmooth);
	options.addItem(31, "Dual tension", true, audioProcessor.dualTension);

	PopupMenu load;
	load.addItem(100, "Sine", audioProcessor.uimode != UIMode::Seq);
	load.addItem(101, "Triangle", audioProcessor.uimode != UIMode::Seq);
	load.addItem(102, "Random", audioProcessor.uimode != UIMode::Seq);
	load.addSeparator();
	load.addItem(109, "Init");

	PopupMenu patterns1;
	PopupMenu patterns2;
	PopupMenu patterns3;

	patterns1.addItem(110, "Load All");
	patterns1.addSeparator();
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

	patterns2.addItem(123, "Load All");
	patterns2.addSeparator();
	patterns2.addItem(124, "Saw 1");
	patterns2.addItem(125, "Saw 2");
	patterns2.addItem(126, "Step 1");
	patterns2.addItem(127, "Step 1 FadeIn");
	patterns2.addItem(128, "Step 4 Gate");
	patterns2.addItem(129, "Off Beat");
	patterns2.addItem(130, "Dynamic 1/4");
	patterns2.addItem(131, "Swing");
	patterns2.addItem(132, "Gate Out");
	patterns2.addItem(133, "Gate In");
	patterns2.addItem(134, "Speed up");
	patterns2.addItem(135, "Speed Down");

	patterns3.addItem(136, "Load All");
	patterns3.addSeparator();
	patterns3.addItem(137, "End Fade");
	patterns3.addItem(138, "End Gate");
	patterns3.addItem(139, "Tremolo Slow");
	patterns3.addItem(140, "Tremolo Fast");
	patterns3.addItem(141, "Sidechain");
	patterns3.addItem(142, "Drum Loop");
	patterns3.addItem(143, "Copter");
	patterns3.addItem(144, "AM");
	patterns3.addItem(145, "Fade In");
	patterns3.addItem(146, "Fade Out");
	patterns3.addItem(147, "Fade OutIn");
	patterns3.addItem(148, "Mute");

	PopupMenu loadOther;
	loadOther.addItem(150, "Restore paint patterns");

	load.addSubMenu("Patterns 01-12", patterns1);
	load.addSubMenu("Patterns 13-25", patterns2);
	load.addSubMenu("Patterns 26-38", patterns3);
	load.addSeparator();
	load.addSubMenu("Other", loadOther);
	

	PopupMenu menu;
	auto menuPos = localPointToGlobal(getLocalBounds().getBottomRight());
	menu.addSubMenu("UI Scale", uiScale);
	menu.addSubMenu("Options", options);
	menu.addSeparator();
	//menu.addItem(50, "Invert");
	//menu.addItem(51, "Reverse");
	menu.addItem(53, "Copy", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(54, "Paste", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(52, audioProcessor.uimode == UIMode::Seq ? "Reset" : "Clear");
	menu.addSeparator();
	menu.addSubMenu("Load", load);
	menu.addItem(1000, "About");
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetScreenArea({menuPos.getX() -110, menuPos.getY(), 1, 1}),
		[this](int result) {
			if (result == 0) return;
			auto snapshot = audioProcessor.viewPattern->points;

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
			if (result == 31) { // Dual tension
				MessageManager::callAsync([this]() {
					audioProcessor.dualTension = !audioProcessor.dualTension;
					audioProcessor.onTensionChange();
					toggleUIComponents();
				});
			}
			if (result == 50) {
				audioProcessor.viewPattern->invert();
				audioProcessor.viewPattern->buildSegments();
				audioProcessor.createUndoPointFromSnapshot(snapshot);
			}
			if (result == 51) {
				audioProcessor.viewPattern->reverse();
				audioProcessor.viewPattern->buildSegments();
				audioProcessor.createUndoPointFromSnapshot(snapshot);
			}
			if (result == 52) {
				if (audioProcessor.uimode == UIMode::Seq) {
					auto snap = audioProcessor.sequencer->cells;
					audioProcessor.sequencer->clear();
					audioProcessor.sequencer->createUndo(snap);
					audioProcessor.sequencer->build();
				}
				else {
					audioProcessor.viewPattern->clear();
					audioProcessor.viewPattern->buildSegments();
					audioProcessor.createUndoPointFromSnapshot(snapshot);
				}
			}
			if (result == 53) {
				audioProcessor.viewPattern->copy();
			}
			if (result == 54) {
				audioProcessor.viewPattern->paste();
				audioProcessor.viewPattern->buildSegments();
				audioProcessor.createUndoPointFromSnapshot(snapshot);
			}
			if (result == 109) {
				audioProcessor.loadProgram(0);
			}
			if (result >= 100 && result <= 200) { // load
				if (result == 100) { // load sine
					audioProcessor.viewPattern->loadSine();
					audioProcessor.viewPattern->buildSegments();
				}
				if (result == 101) { // load triangle
					audioProcessor.viewPattern->loadTriangle();
					audioProcessor.viewPattern->buildSegments();
				}
				if (result == 102) { // load random
					int grid = audioProcessor.getCurrentGrid();
					audioProcessor.viewPattern->loadRandom(grid);
					audioProcessor.viewPattern->buildSegments();
				}
				if (result >= 110 && result < 150) {
					MessageManager::callAsync([this, result]() {
						audioProcessor.loadProgram(result-110+1);
					});
				}
				if (result == 150) {
					audioProcessor.restorePaintPatterns();
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

