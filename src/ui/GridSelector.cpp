#include "GridSelector.h"
#include "../PluginProcessor.h"
#include "../Globals.h"

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
    g.fillAll(Colour(globals::COLOR_BG));

    auto gridSize = audioProcessor.params.getRawParameterValue("grid")->load();
    g.setFont(16);
    g.setColour(Colour(globals::COLOR_ACTIVE));
    g.drawFittedText("Grid " + String(gridSize), getLocalBounds(), Justification::centredLeft, 1);
    //g.drawRect(getLocalBounds());
}

void GridSelector::mouseDown(const juce::MouseEvent& e) 
{
    e.source.enableUnboundedMouseMovement(true);
    mouse_down = true;
    auto param = audioProcessor.params.getParameter("grid");
    auto cur_val = param->getValue();
    cur_normed_value = cur_val;
    last_mouse_position = e.getPosition();
    setMouseCursor(MouseCursor::NoCursor);
    start_mouse_pos = Desktop::getInstance().getMousePosition();
}

void GridSelector::mouseUp(const juce::MouseEvent& e) {
    mouse_down = false;
    setMouseCursor(MouseCursor::NormalCursor);
    e.source.enableUnboundedMouseMovement(false);
    Desktop::getInstance().setMousePosition(start_mouse_pos);
}

void GridSelector::mouseDrag(const juce::MouseEvent& e) {
    auto change = e.getPosition() - last_mouse_position;
    last_mouse_position = e.getPosition();
    auto speed = (e.mods.isCtrlDown() ? 40.0f : 4.0f) * 200.0f;
    auto slider_change = float(change.getX() - change.getY()) / speed;
    cur_normed_value += slider_change;
    auto param = audioProcessor.params.getParameter("grid");
    param->beginChangeGesture();
    param->setValueNotifyingHost(cur_normed_value);
    param->endChangeGesture();
}