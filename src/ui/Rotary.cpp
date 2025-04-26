#include "Rotary.h"
#include "../PluginProcessor.h"
#include "../Globals.h"

Rotary::Rotary(GATE12AudioProcessor& p, juce::String paramId, juce::String name, LabelFormat format, bool isSymmetric)
    : juce::SettableTooltipClient()
    , juce::Component()
    , audioProcessor(p)
    , paramId(paramId)
    , name(name)
    , format(format)
    , isSymmetric(isSymmetric)
{
    setName(name);
    audioProcessor.params.addParameterListener(paramId, this);
    if (velId.isNotEmpty()) {
        audioProcessor.params.addParameterListener(velId, this);
    }
}

Rotary::~Rotary()
{
    audioProcessor.params.removeParameterListener(paramId, this);
    if (velId.isNotEmpty()) {
        audioProcessor.params.removeParameterListener(velId, this);
    }
}

void Rotary::parameterChanged(const juce::String& parameterID, float newValue) 
{
    (void)parameterID;
    (void)newValue;
    juce::MessageManager::callAsync([this] { repaint(); });
}

void Rotary::paint(juce::Graphics& g) {
    g.fillAll(Colour(globals::COLOR_BG));

    auto param = audioProcessor.params.getParameter(paramId);
    auto normValue = param->getValue();
    auto value = param->convertFrom0to1(normValue);

    draw_rotary_slider(g, normValue); 
    draw_label_value(g, value);
    //g.drawRect(getLocalBounds());
}

void Rotary::draw_label_value(juce::Graphics& g, float slider_val)
{
    auto text = name;
    if (mouse_down) {
        if (format == LabelFormat::percent) text = std::to_string((int)std::round((slider_val * 100))) + " %";
        if (format == LabelFormat::integerx100) text = std::to_string((int)std::round((slider_val * 100)));
        else if (format == LabelFormat::hz) text = std::to_string((int)slider_val) + " Hz";
        else if (format == LabelFormat::hzFloat1) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << slider_val << " Hz";
            text = ss.str();
        }
        else if (format == LabelFormat::float1) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << slider_val;
            text = ss.str();
        }
        else if (format == LabelFormat::float2) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << slider_val;
            text = ss.str();
        }
        else if (format == LabelFormat::float2x100) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << slider_val * 100;
            text = ss.str();
        }
    }

    g.setColour(Colours::white);
    g.setFont(16.0f);
    g.drawText(text, 0, getHeight() - 16, getWidth(), 16, juce::Justification::centred, true);
}

void Rotary::mouseDown(const juce::MouseEvent& e) 
{
    e.source.enableUnboundedMouseMovement(true);
    mouse_down = true;
    auto param = audioProcessor.params.getParameter(paramId);
    auto cur_val = param->getValue();
    cur_normed_value = cur_val;
    last_mouse_position = e.getPosition();
    setMouseCursor(MouseCursor::NoCursor);
    start_mouse_pos = Desktop::getInstance().getMousePosition();
    repaint();
}

void Rotary::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    auto speed = (event.mods.isCtrlDown() ? 0.01f : 0.05f);
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

void Rotary::mouseUp(const juce::MouseEvent& e) {
    mouse_down = false;
    setMouseCursor(MouseCursor::NormalCursor);
    e.source.enableUnboundedMouseMovement(false);
    Desktop::getInstance().setMousePosition(start_mouse_pos);
    repaint();
}

void Rotary::mouseDoubleClick(const juce::MouseEvent& e) {
    (void)e;
    auto param = audioProcessor.params.getParameter(paramId);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getDefaultValue());
    param->endChangeGesture();
}

void Rotary::mouseDrag(const juce::MouseEvent& e) {
    auto change = e.getPosition() - last_mouse_position;
    last_mouse_position = e.getPosition();
    auto speed = (e.mods.isCtrlDown() ? 40.0f : 4.0f) * pixels_per_percent;
    auto slider_change = float(change.getX() - change.getY()) / speed;
    cur_normed_value += slider_change;
    auto param = audioProcessor.params.getParameter(paramId);
    param->beginChangeGesture();
    param->setValueNotifyingHost(cur_normed_value);
    param->endChangeGesture();
}

void Rotary::draw_rotary_slider(juce::Graphics& g, float slider_pos) {
    auto bounds = getBounds();
    const float radius = 16.0f;
    const float angle = -deg130 + slider_pos * (deg130 - -deg130);

    g.setColour(juce::Colour(globals::COLOR_KNOB));
    g.fillEllipse(bounds.getWidth()/2.0f-radius, bounds.getHeight()/2.0f-radius - 4.0f, radius*2.0f, radius*2.0f);

    g.setColour(Colour(globals::COLOR_KNOB));
    juce::Path arcKnob;
    arcKnob.addCentredArc(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f - 4.0f, radius + 5.0f, radius + 5.0f, 0,-deg130, deg130, true);
    g.strokePath(arcKnob, PathStrokeType(3.0, PathStrokeType::JointStyle::curved, PathStrokeType::rounded));

    g.setColour(Colour(globals::COLOR_ACTIVE));
    if ((isSymmetric && slider_pos != 0.5f) || (!isSymmetric && slider_pos)) {
        juce::Path arcActive;
        arcActive.addCentredArc(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f - 4.0f, radius + 5.0f, radius + 5.0f, 0, isSymmetric ? 0 : -deg130, angle, true);
        g.strokePath(arcActive, PathStrokeType(3.0, PathStrokeType::JointStyle::curved, PathStrokeType::rounded));
    }

    g.setColour(Colours::white);
    juce::Path p;
    p.addLineSegment (juce::Line<float>(0.0f, -5.0f, 0.0f, -radius + 5.0f), 0.1f);
    juce::PathStrokeType(3.0f, PathStrokeType::JointStyle::curved, PathStrokeType::rounded).createStrokedPath(p, p);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f - 4.0f));
}