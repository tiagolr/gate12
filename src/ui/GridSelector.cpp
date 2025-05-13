#include "GridSelector.h"
#include "../PluginProcessor.h"

GridSelector::GridSelector(GATE12AudioProcessor& p) : audioProcessor(p)
{
    audioProcessor.params.addParameterListener("grid", this);
}

GridSelector::~GridSelector()
{
    audioProcessor.params.removeParameterListener("grid", this);
}

void GridSelector::parameterChanged(const juce::String& parameterID, float newValue) 
{
    (void)parameterID;
    (void)newValue;
    juce::MessageManager::callAsync([this] { repaint(); });
}

void GridSelector::paint(juce::Graphics& g) {
    g.fillAll(Colour(COLOR_BG));

    int gridSize = audioProcessor.getCurrentGrid();
    g.setFont(16);
    g.setColour(Colour(COLOR_ACTIVE));
    g.drawFittedText("Grid " + String(gridSize), getLocalBounds(), Justification::centredLeft, 1);
}

void GridSelector::mouseDown(const juce::MouseEvent& e) 
{
    (void)e;
    PopupMenu menu;
    menu.addSectionHeader("Straight");
    menu.addItem(1, "4");
    menu.addItem(2, "8");
    menu.addItem(3, "16");
    menu.addItem(4, "32");
    menu.addItem(5, "64");
    menu.addSectionHeader("Triplet");
    menu.addItem(6, "6");
    menu.addItem(7, "12");
    menu.addItem(8, "24");
    menu.addItem(9, "48");

    auto menuPos = localPointToGlobal(getLocalBounds().getBottomLeft());

    menu.showMenuAsync(PopupMenu::Options()
        .withTargetScreenArea({menuPos.getX(), menuPos.getY(), 1, 1}),
        [this](int result) {
            if (result == 0) return;
            auto param = audioProcessor.params.getParameter("grid");
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(result-1.f));
            param->endChangeGesture();
        }
    );
    
}