#include "PaintToolWidget.h"
#include "../PluginProcessor.h"

PaintToolWidget::PaintToolWidget(GATE12AudioProcessor& p) : audioProcessor(p) 
{
    startTimerHz(60);
}

void PaintToolWidget::timerCallback()
{
    if (isVisible())
        repaint();
}

void PaintToolWidget::paint(Graphics& g)
{
    auto rects = getPatRects();
    for (int i = 0; i < (int)rects.size(); ++i) {
        int pati = i + audioProcessor.paintPage * 8;
        bool isSelected = pati == audioProcessor.paintTool;
        g.setColour(isSelected && audioProcessor.uimode == UIMode::PaintEdit 
            ? Colours::blue.withAlpha(0.05f) 
            : Colours::black.withAlpha(0.1f)
        );
        g.fillRect(rects[i]);
        auto color = isSelected ? Colours::white : Colour(COLOR_NEUTRAL);
        g.setColour(color);
        g.drawRect(rects[i]);
        drawPattern(g, rects[i].expanded(-4, -4), pati, color);
    }
}

std::vector<Rectangle<int>> PaintToolWidget::getPatRects()
{
    auto bounds = getLocalBounds();
    int patnum = 8;
    float gap = 5.0f;

    float totalWidth = bounds.toFloat().getWidth();
    float rectWidth = (totalWidth - (patnum - 1) * gap) / patnum;
    std::vector<Rectangle<int>> rects;

    for (int i = 0; i < patnum; ++i) {
        auto rect = bounds.removeFromLeft(roundToInt(rectWidth));
        rects.push_back(rect);

        if (i < patnum - 1)
            bounds.removeFromLeft(roundToInt(gap));
    }

    return rects;
}

void PaintToolWidget::drawPattern(Graphics& g, Rectangle<int> bounds, int index, Colour color)
{
    auto pattern = audioProcessor.getPaintPatern(index);

    int x = bounds.getX();
    int y = bounds.getY();
    int w = bounds.getWidth();
    int h = bounds.getHeight();
    double lastX = x;
    double lastY = pattern->get_y_at(0) * h + y;

    Path path;

    path.startNewSubPath((float)lastX, (float)lastY);

    for (int i = 0; i < w + 1; ++i)
    {
        double px = double(i) / double(w);
        double py = pattern->get_y_at(px) * h + y;
        float xx = (float)(i + x);
        float yy = (float)py;

        path.lineTo(xx, yy);
    }

    g.setColour(color);
    g.strokePath(path, PathStrokeType(1.f));
}

void PaintToolWidget::mouseDown(const juce::MouseEvent& e) 
{
    auto rects = getPatRects();
    for (int i = 0; i < (int)rects.size(); ++i) {
        if (rects[i].contains(e.getPosition())) {
            int pati = i + audioProcessor.paintPage * 8;
            audioProcessor.paintTool = pati;
            if (audioProcessor.uimode == UIMode::PaintEdit) {
                audioProcessor.setViewPattern(audioProcessor.getPaintPatern(pati)->index);
            }
        }
    }
}