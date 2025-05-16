#include "PaintToolWidget.h"
#include "../PluginProcessor.h"

PaintToolWidget::PaintToolWidget(GATE12AudioProcessor& p) : audioProcessor(p) 
{


    addAndMakeVisible(paintEditButton);
    paintEditButton.setButtonText("Edit");
    paintEditButton.setComponentID("button");
    paintEditButton.onClick = [this]() {
        audioProcessor.togglePaintEditMode();
    };


    addAndMakeVisible(paintPrevButton);
    paintPrevButton.setAlpha(0.f);
    paintPrevButton.onClick = [this]() {
        int page = audioProcessor.paintPage - 1;
        if (page < 0) page = 3;
        audioProcessor.paintPage = page;
        MessageManager::callAsync([this]() { audioProcessor.sendChangeMessage(); });
    };

    addAndMakeVisible(paintPageLabel);
    paintPageLabel.setText("16-24", dontSendNotification);
    paintPageLabel.setJustificationType(Justification::centred);
    paintPageLabel.setColour(Label::textColourId, Colour(COLOR_NEUTRAL));

    addAndMakeVisible(paintNextButton);
    paintNextButton.setAlpha(0.f);
    paintNextButton.onClick = [this]() {
        int page = audioProcessor.paintPage + 1;
        if (page > 3) page = 0;
        audioProcessor.paintPage = page;
        MessageManager::callAsync([this]() { audioProcessor.sendChangeMessage(); });
    };

    startTimerHz(60);
}

void PaintToolWidget::resized()
{
    auto h = getHeight();
    auto col = 0;
    paintEditButton.setBounds(col, h/2-25/2, 60, 25);
    col += 65;
    paintPrevButton.setBounds(col+2, 2, 20, 20);
    paintPageLabel.setBounds(col, 25-2, 45, 16);
    col += 25;
    paintNextButton.setBounds(col-2, 2, 20, 20);
}

void PaintToolWidget::toggleUIComponents()
{
    paintEditButton.setVisible(audioProcessor.showPaintWidget);
    paintEditButton.setToggleState(audioProcessor.uimode == UIMode::PaintEdit, dontSendNotification);
    paintNextButton.setVisible(audioProcessor.showPaintWidget);
    paintPrevButton.setVisible(audioProcessor.showPaintWidget);
    paintPageLabel.setVisible(audioProcessor.showPaintWidget);
    int firstPaintPat = audioProcessor.paintPage * 8 + 1;
    paintPageLabel.setText(String(firstPaintPat) + "-" + String(firstPaintPat+7), dontSendNotification);
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

    // draw rotate paint page triangles
    g.setColour(Colour(COLOR_ACTIVE));
    auto triCenter = paintNextButton.getBounds().toFloat().getCentre();
    auto triRadius = 5.f;
    juce::Path paintNextTriangle;
    paintNextTriangle.addTriangle(
        triCenter.translated(-triRadius, -triRadius),
        triCenter.translated(-triRadius, triRadius),
        triCenter.translated(triRadius, 0)
    );
    g.fillPath(paintNextTriangle);

    triCenter = paintPrevButton.getBounds().toFloat().getCentre();
    juce::Path paintPrevTriangle;
    paintPrevTriangle.addTriangle(
        triCenter.translated(-triRadius, 0),
        triCenter.translated(triRadius, -triRadius),
        triCenter.translated(triRadius, triRadius)
    );
    g.fillPath(paintPrevTriangle);
}

std::vector<Rectangle<int>> PaintToolWidget::getPatRects()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(115);
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