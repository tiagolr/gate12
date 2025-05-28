#include "GridSelector.h"
#include "../PluginProcessor.h"

GridSelector::GridSelector(GATE12AudioProcessor& p, bool seqStepSelector) 
    : audioProcessor(p)
    , seqStepSelector(seqStepSelector)
{
    audioProcessor.params.addParameterListener(seqStepSelector ? "seqstep" : "grid", this);
}

GridSelector::~GridSelector()
{
    audioProcessor.params.removeParameterListener(seqStepSelector ? "seqstep" : "grid", this);
}

void GridSelector::parameterChanged(const juce::String& parameterID, float newValue) 
{
    auto seqstep = audioProcessor.params.getParameter("seqstep")->getValue();
    auto grid = audioProcessor.params.getParameter("grid")->getValue();
    if (audioProcessor.linkSeqToGrid) {
        if (parameterID == "seqstep" && newValue != grid) {
            juce::MessageManager::callAsync([this, seqstep] {
                audioProcessor.params.getParameter("grid")
                    ->setValueNotifyingHost(seqstep);
                });
        }
        if (parameterID == "grid" && newValue != seqstep) {
            juce::MessageManager::callAsync([this, grid] {
                audioProcessor.params.getParameter("seqstep")
                    ->setValueNotifyingHost(grid);
                });
        }
    }
    juce::MessageManager::callAsync([this] { repaint(); });
}

void GridSelector::paint(juce::Graphics& g) {
    g.fillAll(Colour(COLOR_BG));

    int gridSize = seqStepSelector
        ? audioProcessor.getCurrentSeqStep()
        : audioProcessor.getCurrentGrid();
    g.setFont(16);
    g.setColour(Colour(COLOR_ACTIVE));
    g.drawFittedText((seqStepSelector ? "Step " : "Grid ") + String(gridSize), getLocalBounds(), Justification::centredLeft, 1);
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
            MessageManager::callAsync([this, result]() {
                auto param = audioProcessor.params.getParameter(seqStepSelector ? "seqstep": "grid");
                param->setValueNotifyingHost(param->convertTo0to1(result-1.f));
                });
        }
    );

}