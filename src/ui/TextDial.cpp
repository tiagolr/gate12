#include "TextDial.h"
#include "../PluginProcessor.h"

TextDial::TextDial(GATE12AudioProcessor& p, String paramId, String prefix, String suffix, TextDialLabel format, float fontSize, unsigned int color)
    : audioProcessor(p), paramId(paramId), prefix(prefix), suffix(suffix), format(format), fontSize(fontSize), fontColor(color)
{
    audioProcessor.params.addParameterListener(paramId, this);
}

TextDial::~TextDial()
{
    audioProcessor.params.removeParameterListener(paramId, this);
}

void TextDial::parameterChanged(const juce::String& parameterID, float newValue)
{
    (void)parameterID;
    (void)newValue;
    repaint();
}

void TextDial::paint(juce::Graphics& g) {
    g.fillAll(Colour(globals::COLOR_BG));

    auto value = (int)(audioProcessor.params.getRawParameterValue(paramId)->load() * 100);
    g.setFont(fontSize);
    g.setColour(Colour(fontColor));
    g.drawFittedText(prefix + String(value) + "%" + suffix, getLocalBounds(), Justification::centredLeft, 1);
}

void TextDial::mouseDown(const juce::MouseEvent& e)
{
    e.source.enableUnboundedMouseMovement(true);
    mouse_down = true;
    auto param = audioProcessor.params.getParameter(paramId);
    cur_normed_value = param->getValue();
    last_mouse_position = e.getPosition();
    setMouseCursor(MouseCursor::NoCursor);
    param->beginChangeGesture();
}

void TextDial::mouseUp(const juce::MouseEvent& e) {
    mouse_down = false;
    setMouseCursor(MouseCursor::NormalCursor);
    e.source.enableUnboundedMouseMovement(false);
    Desktop::getInstance().setMousePosition(e.getMouseDownScreenPosition());
    auto param = audioProcessor.params.getParameter(paramId);
    param->endChangeGesture();
}

void TextDial::mouseDrag(const juce::MouseEvent& e) {
    auto change = e.getPosition() - last_mouse_position;
    last_mouse_position = e.getPosition();
    auto speed = (e.mods.isCtrlDown() ? 40.0f : 4.0f) * 200.0f;
    auto slider_change = float(change.getX() - change.getY()) / speed;
    cur_normed_value += slider_change;
    cur_normed_value = jlimit(0.0f, 1.0f, cur_normed_value);
    auto param = audioProcessor.params.getParameter(paramId);
    
    param->setValueNotifyingHost(cur_normed_value);
}

void TextDial::mouseDoubleClick(const juce::MouseEvent& e)
{
    (void)e;
    auto param = audioProcessor.params.getParameter(paramId);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getDefaultValue());
    param->endChangeGesture();
};

void TextDial::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isAnyMouseButtonDown()) {
        return; // prevent crash, param has already begin change gesture
    }
    auto speed = (event.mods.isShiftDown() ? 0.01f : 0.05f);
    auto slider_change = wheel.deltaY > 0 ? speed : wheel.deltaY < 0 ? -speed : 0;
    auto param = audioProcessor.params.getParameter(paramId);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getValue() + slider_change);
    while (wheel.deltaY > 0.0f && param->getValue() == 0.0f) { // FIX wheel not working when value is zero, first step takes more than 0.05% 
        slider_change += 0.05f;
        param->setValueNotifyingHost(param->getValue() + slider_change);
    }
    param->endChangeGesture();
}