/*
  ==============================================================================

    ParamAccessibility.h

    Lets screen readers (VoiceOver, NVDA, JAWS) read and change a custom
    knob that is tied to a plugin parameter. The knob is exposed as a slider
    named after the control, its value is the same text the knob shows,
    and increment/decrement or the arrow keys change the parameter.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <functional>

class ParamAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    // step and minValue are in the parameter's normalized 0..1 range
    ParamAccessibilityHandler(juce::Component& c,
                              juce::RangedAudioParameter& param,
                              double step,
                              double minValue,
                              std::function<juce::String()> getText)
        : juce::AccessibilityHandler(c, juce::AccessibilityRole::slider, {},
            { std::make_unique<ValueInterface>(param, step, minValue, std::move(getText)) })
        , comp(c)
    {
    }

    juce::String getTitle() const override
    {
        auto title = comp.getTitle();
        return title.isNotEmpty() ? title : comp.getName();
    }

    // Arrow keys for screen readers that send keystrokes (NVDA, JAWS).
    // Up/Right raise, Down/Left lower, Shift for fine steps, Page Up/Down for
    // coarse steps, Home/End for max/min, Delete/Backspace resets to default.
    static bool handleKey(juce::RangedAudioParameter& param, const juce::KeyPress& key, double step, double minValue)
    {
        auto code = key.getKeyCode();
        auto value = (double)param.getValue();
        auto fine = key.getModifiers().isShiftDown() ? 0.1 : 1.0;
        auto choiceStep = param.getNumSteps() < 100; // choices and ints always move one item

        if (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey)
            value += step * (choiceStep ? 1.0 : fine);
        else if (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey)
            value -= step * (choiceStep ? 1.0 : fine);
        else if (code == juce::KeyPress::pageUpKey)
            value += step * 10.0;
        else if (code == juce::KeyPress::pageDownKey)
            value -= step * 10.0;
        else if (code == juce::KeyPress::homeKey)
            value = 1.0;
        else if (code == juce::KeyPress::endKey)
            value = minValue;
        else if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey)
            value = param.getDefaultValue();
        else
            return false;

        setParam(param, value, minValue);
        return true;
    }

    static void setParam(juce::RangedAudioParameter& param, double value, double minValue)
    {
        value = juce::jlimit(minValue, 1.0, value);
        auto old = param.getValue();
        auto delta = value - old;
        param.beginChangeGesture();
        param.setValueNotifyingHost((float)value);
        // a small step on a parameter with a coarse interval can snap back to
        // where it was (Low Cut at 20 Hz), so keep stepping until it moves
        while (delta != 0.0 && param.getValue() == old && value > minValue && value < 1.0) {
            value = juce::jlimit(minValue, 1.0, value + delta);
            param.setValueNotifyingHost((float)value);
        }
        param.endChangeGesture();
    }

private:
    class ValueInterface : public juce::AccessibilityValueInterface
    {
    public:
        ValueInterface(juce::RangedAudioParameter& p, double s, double m, std::function<juce::String()> t)
            : param(p), step(s), minValue(m), getText(std::move(t)) {}

        bool isReadOnly() const override { return false; }
        double getCurrentValue() const override { return param.getValue(); }
        void setValue(double newValue) override { setParam(param, newValue, minValue); }
        juce::String getCurrentValueAsString() const override { return getText(); }

        // typing a value isn't supported yet
        void setValueAsString(const juce::String&) override {}

        AccessibleValueRange getRange() const override
        {
            return { { minValue, 1.0 }, step };
        }

    private:
        juce::RangedAudioParameter& param;
        double step;
        double minValue;
        std::function<juce::String()> getText;
    };

    juce::Component& comp;
};
