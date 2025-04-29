/*
  ==============================================================================

    View.cpp
    Author:  tiagolr

  ==============================================================================
*/

#include "View.h"
#include "../PluginProcessor.h"
#include "../Globals.h"

View::View(GATE12AudioProcessor& p) : audioProcessor(p)
{
    startTimerHz(60);
};

View::~View()
{
};

void View::parameterChanged (const juce::String& parameterID, float newValue) 
{
    (void)parameterID;
    (void)newValue;
};

void View::init() 
{
    auto bounds = getLocalBounds();
    winx = bounds.getX() + 10;
    winy = bounds.getY() + 10;
    winw = bounds.getWidth() - 20;
    winh = bounds.getHeight() - 20;
    audioProcessor.viewW = winw;
}

void View::paint(Graphics& g) {
    if (audioProcessor.drawWave && (audioProcessor.alwaysPlaying || audioProcessor.isPlaying)) {
        drawWave(g, audioProcessor.preSamples, Colour(0xff7f7f7f));
        drawWave(g, audioProcessor.postSamples, Colour(globals::COLOR_ACTIVE));
    }
    drawGrid(g);
    drawSegments(g);
    drawMidPoints(g);
    drawPoints(g);
    if (audioProcessor.xpos > 0.0) {
        drawSeek(g);
    }
}

void View::drawWave(Graphics& g, std::vector<double> samples, Colour color) const
{
    double lastX = winx;
    double lastY = winh - std::min(std::abs(samples[0]),1.) * winh + winy;
    g.setColour(color.withAlpha(0.4f));
    for (int i = 0; i < winw; ++i) {
        double ypos = std::min(samples[i], 1.);
        if (ypos > 0.0) {
            g.drawLine((float)i + winx, (float)winy + winh, (float)i + winx, (float)(winh - ypos * winh + winy));
        }
        lastX = i + winx;
        lastY = winh - ypos * winh + winy;
    }
}

void View::drawGrid(Graphics& g)
{
    int grid = audioProcessor.grid;
    double gridx = double(winw) / grid;
    double gridy = double(winh) / grid;

    auto colorNormal = Colours::white.withAlpha(0.15f);
    auto colorBold = Colours::white.withAlpha(0.30f);
    int c = -1; // flag used to toggle color only when it changes

    for (int i = 0; i < grid + 1; ++i) {
        auto color = grid % 4 == 0 && i && i % 4 == 0 && i < grid ? 0 : 1;
        if (color != c) {
            g.setColour(color ? colorNormal : colorBold);
            c = color;
        }
        g.drawLine((float)winx, (float)(winy + gridy * i), (float)winx + winw, (float)(winy + gridy * i));
        g.drawLine((float)(winx + gridx * i), (float)winy, (float)(winx + gridx * i), (float)winy + winh);
    }
}

void View::drawSegments(Graphics& g)
{
    auto& points = audioProcessor.pattern->points;
    double lastX = winx;
    double lastY = points[0].y * winh + winy;

    auto colorBold = Colours::white;
    auto colorLight = Colours::white.withAlpha(0.125f);

    for (int i = 0; i < winw + 1; ++i)
    {
        double px = double(i) / double(winw);
        double py = audioProcessor.pattern->get_y_at(px) * winh + winy;
        g.setColour(colorLight);
        g.drawLine((float)(i + winx), (float)(winy + winh), (float)(i + winx), (float)py);
        g.setColour(colorBold);
        g.drawLine((float)lastX, (float)lastY, (float)(i + winx), (float)py, 2.f);
        lastX = i + winx;
        lastY = py;
    }
    g.setColour(colorBold);
    g.drawLine((float)lastX, (float)lastY, (float)(winw + winx), (float)(points[points.size() - 1].y * winh + winy));
}

void View::drawPoints(Graphics& g)
{
    auto& points = audioProcessor.pattern->points;

    for (auto pt = points.begin(); pt != points.end(); ++pt) {
        auto xx = pt->x * winw + winx;
        auto yy = pt->y * winh + winy;
        g.setColour(Colours::white);
        g.fillEllipse((float)(xx - 4.0), (float)(yy - 4.0), 8.0f, 8.0f);
    }

    if (selectedPoint == -1 && selectedMidpoint == -1 && hoverPoint > -1)
    {
        auto xx = points[hoverPoint].x * winw + winx;
        auto yy = points[hoverPoint].y * winh + winy;
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillEllipse((float)(xx - HOVER_RADIUS), (float)(yy - HOVER_RADIUS), (float)HOVER_RADIUS * 2.f, (float)HOVER_RADIUS * 2.f);
    }

    if (selectedPoint != -1)
    {
        auto xx = points[selectedPoint].x * winw + winx;
        auto yy = points[selectedPoint].y * winh + winy;
        g.setColour(Colours::red.withAlpha(0.5f));
        g.fillEllipse((float)(xx - 4.0), (float)(yy-4.0), 8.0f, 8.0f);
    }
}

std::vector<double> View::getMidpointXY(Segment seg)
{
    double x = (seg.x1 + seg.x2) * 0.5;
    double y = seg.type > 1
        ? (seg.y1 + seg.y2) / 2
        : audioProcessor.pattern->get_y_at(x);

    return {
        x * winw + winx,
        y * winh + winy
    };
}

void View::drawMidPoints(Graphics& g)
{
    auto& segs = audioProcessor.pattern->segments;

    g.setColour(Colour(globals::COLOR_ACTIVE));
    for (auto seg = segs.begin(); seg != segs.end(); ++seg) {
        if (!isCollinear(*seg)) {
            auto xy = getMidpointXY(*seg);
            g.drawEllipse((float)xy[0] - 3.0f, (float)xy[1] - 3.0f, 6.f, 6.f, 2.f);
        }
    }

    g.setColour(Colour(globals::COLOR_ACTIVE).withAlpha(0.5f));
    if (selectedPoint == -1 && selectedMidpoint == -1 && hoverMidpoint != -1) {
        auto& seg = segs[hoverMidpoint];
        auto xy = getMidpointXY(seg);
        g.fillEllipse((float)xy[0] - HOVER_RADIUS, (float)xy[1] - HOVER_RADIUS, (float)HOVER_RADIUS * 2.f, (float)HOVER_RADIUS * 2.f);
    }

    g.setColour(Colour(globals::COLOR_ACTIVE));
    if (selectedMidpoint != -1) {
        auto& seg = segs[selectedMidpoint];
        auto xy = getMidpointXY(seg);
        g.fillEllipse((float)xy[0] - 3.0f, (float)xy[1] - 3.0f, 6.0f, 6.0f);
    }
}

void View::drawSeek(Graphics& g)
{
    auto xpos = audioProcessor.xpos;
    auto ypos = audioProcessor.ypos;
    g.setColour(Colour(globals::COLOR_SEEK).withAlpha(0.5f));
    g.drawLine((float)(xpos * winw + winx), (float)winy, (float)(xpos * winw + winx), (float)(winy + winh));
    g.setColour(Colour(globals::COLOR_SEEK));
    g.drawEllipse((float)(xpos * winw + winx - 5), (float)((1 - ypos) * winh + winy - 5), 10.0f, 10.0f, 1.0f);
}

int View::getHoveredPoint(int x, int y)
{
    auto& points = audioProcessor.pattern->points;
    for (auto i = 0; i < points.size(); ++i) {
        auto xx = (int)(points[i].x * winw + winx);
        auto yy = (int)(points[i].y * winh + winy);
        if (pointInRect(x, y, xx - HOVER_RADIUS, yy - HOVER_RADIUS, HOVER_RADIUS * 2, HOVER_RADIUS * 2)) {
            return i;
        }
    }
    return -1;
};

int View::getHoveredMidpoint(int x, int y)
{
    auto& segs = audioProcessor.pattern->segments;
    for (auto i = 0; i < segs.size(); ++i) {
        auto& seg = segs[i];
        auto xy = getMidpointXY(seg);
        if (!isCollinear(seg) && pointInRect(x, y, (int)xy[0] - HOVER_RADIUS, (int)xy[1] - HOVER_RADIUS, HOVER_RADIUS * 2, HOVER_RADIUS * 2)) {
            return i;
        }
    }
    return -1;
};

void View::mouseDown(const juce::MouseEvent& e)
{
    int x = e.getMouseDownX();
    int y = e.getMouseDownY();
    if (e.mods.isLeftButtonDown()) {
        selectedPoint = getHoveredPoint(x, y);
        if (selectedPoint == -1)
            selectedMidpoint = getHoveredMidpoint(x, y);

        if (selectedPoint > -1 || selectedMidpoint > -1) {
            if (selectedPoint > -1) {
                setMouseCursor(MouseCursor::NoCursor);
            }
            if (selectedMidpoint > -1) {
                origTension = audioProcessor.pattern->points[selectedMidpoint].tension;
                dragStartY = y;
                e.source.enableUnboundedMouseMovement(true);
                setMouseCursor(MouseCursor::NoCursor);
            }
        }
    }
    else if (e.mods.isRightButtonDown()) {
        rmousePoint = getHoveredPoint(x, y);
        if (rmousePoint > -1) {
            showPointContextMenu(e);
        }
        else {
            applyPaintTool(x, y, e);
        }
    }
}

void View::mouseUp(const juce::MouseEvent& e)
{
    if (selectedPoint > -1) {
        setMouseCursor(MouseCursor::NormalCursor);
    }
    else if (selectedMidpoint > -1) {
        setMouseCursor(MouseCursor::NormalCursor);
        e.source.enableUnboundedMouseMovement(false);
        auto& mpoint = audioProcessor.pattern->points[selectedMidpoint];
        auto& next = audioProcessor.pattern->points[static_cast<size_t>(selectedMidpoint) + 1];
        double midx = (mpoint.x + next.x) / 2.;
        int x = (int)(midx * winw + winx) + getScreenPosition().x;
        int y = (int)(audioProcessor.pattern->get_y_at(midx) * winh + winy) + getScreenPosition().y;
        Desktop::getInstance().setMousePosition(juce::Point<int>(x, y));
    }
    selectedMidpoint = -1;
    selectedPoint = -1;
}

void View::mouseMove(const juce::MouseEvent& e)
{
    if (selectedPoint > -1 || selectedMidpoint > -1) 
        return;

    int x = e.getPosition().x;
    int y = e.getPosition().y;

    hoverPoint = getHoveredPoint(x , y);
    if (hoverPoint == -1)
        hoverMidpoint = getHoveredMidpoint(x, y);
}

void View::mouseDrag(const juce::MouseEvent& e)
{
    int x = e.getPosition().x;
    int y = e.getPosition().y;

    if (rmousePoint == -1 && e.mods.isRightButtonDown()) {
        applyPaintTool(x, y, e);
        return;
    }

    auto& points = audioProcessor.pattern->points;
    double grid = (double)audioProcessor.grid;
    double gridx = double(winw) / grid;
    double gridy = double(winh) / grid;
    double xx = (double)x;
    double yy = (double)y;
    if (isSnapping(e)) {
        xx = std::round((xx - winx) / gridx) * gridx + winx;
        yy = std::round((yy - winy) / gridy) * gridy + winy;
    }
    xx = (xx - winx) / winw;
    yy = (yy - winy) / winh;

    if (selectedPoint > -1) {
        auto& point = points[selectedPoint];
        point.y = yy;
        if (point.y > 1) point.y = 1;
        if (point.y < 0) point.y = 0;

        if (selectedPoint == 0 && audioProcessor.linkEdgePoints) {
            auto& next = points[points.size() - 1];
            next.y = point.y;
        }

        if (selectedPoint == points.size() - 1 && audioProcessor.linkEdgePoints) {
            auto& first = points[0];
            first.y = point.y;
        }

        if (selectedPoint > 0 && selectedPoint < points.size() - 1) {
            point.x = xx;
            if (point.x > 1) point.x = 1;
            if (point.x < 0) point.x = 0;
            auto& prev = points[static_cast<size_t>(selectedPoint) - 1];
            auto& next = points[static_cast<size_t>(selectedPoint) + 1];
            if (point.x < prev.x) point.x = prev.x;
            if (point.x > next.x) point.x = next.x;
        }
    }

    else if (selectedMidpoint > -1) {
        int distance = y - dragStartY;
        auto& mpoint = points[selectedMidpoint];
        auto& next = points[static_cast<size_t>(selectedMidpoint) + 1];
        if (mpoint.y < next.y) distance *= -1;
        float tension = (float)origTension + float(distance) / 500.f;
        if (tension > 1) tension = 1;
        if (tension < -1) tension = -1;
        mpoint.tension = tension;
    }

    audioProcessor.pattern->buildSegments();
}

void View::mouseDoubleClick(const juce::MouseEvent& e)
{
    int x = e.getPosition().x;
    int y = e.getPosition().y;
    auto& points = audioProcessor.pattern->points;
    int pt = getHoveredPoint((int)x, (int)y);
    int mid = getHoveredMidpoint((int)x, (int)y);
    if (pt > 0 && pt < points.size() - 1) {
        audioProcessor.pattern->removePoint(pt);
    }
    if (pt == -1 && mid > -1) {
        points[mid].tension = 0;
    }
    if (pt == -1 && mid == -1) {
        double px = (double)x;
        double py = (double)y;
        if (isSnapping(e)) {
            double grid = (double)audioProcessor.grid;
            double gridx = double(winw) / grid;
            double gridy = double(winh) / grid;
            px = std::round(double(px - winx) / gridx) * gridx + winx;
            py = std::round(double(py - winy) / gridy) * gridy + winy;
        }
        px = double(px - winx) / (double)winw;
        py = double(py - winy) / (double)winh;
        if (px >= 0 && px <= 1 && py >= 0 && py <= 1) { // point in env window
            if (px == 1) px -= 0.000001; // special case avoid inserting point after last point
            audioProcessor.pattern->insertPoint(px, py, 0, (int)audioProcessor.params.getRawParameterValue("point")->load());
        }
    }

    audioProcessor.pattern->buildSegments();
}

void View::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    (void)event;
    int grid = audioProcessor.grid;
    auto param = audioProcessor.params.getParameter("grid");
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->convertTo0to1((float)grid + (wheel.deltaY > 0 ? 1.0f : -1.0f)));
    param->endChangeGesture();
}

void View::showPointContextMenu(const juce::MouseEvent& event)
{
    int type = audioProcessor.pattern->points[rmousePoint].type;
    PopupMenu menu;
    menu.addItem(1, "Hold", true, type == 0);
    menu.addItem(2, "Curve", true, type == 1);
    menu.addItem(3, "S-Curve", true, type == 2);
    menu.addItem(4, "Pulse", true, type == 3);
    menu.addItem(5, "Wave", true, type == 4);
    menu.addItem(6, "Triangle", true, type == 5);
    menu.addItem(7, "Stairs", true, type == 6);
    menu.addItem(8, "Smooth stairs", true, type == 7);
    menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this).withMousePosition(),[this](int result) {
        if (result > 0) {
            audioProcessor.pattern->points[rmousePoint].type = result - 1;
            audioProcessor.pattern->buildSegments();
        }
    });
}

void View::applyPaintTool(int x, int y, const MouseEvent& e)
{
    double mousex = std::min(std::max(double(x - winx) / (double)winw, 0.), 0.9999999);
    double mousey = std::min(std::max(double(y - winy) / (double)winh, 0.), 1.);
    double gridsegs = (double)audioProcessor.grid;
    if (isSnapping(e)) {
        mousey = std::round(mousey * gridsegs) / gridsegs;
    }
    double seg = std::floor(mousex * gridsegs);
    int paintMode = (int)audioProcessor.params.getRawParameterValue("paint")->load();

    if (paintMode == 0 || e.mods.isAltDown()) {  // erase mode
        audioProcessor.pattern->removePointsInRange(seg / gridsegs, (seg + 1) / gridsegs);
    }
    else if (paintMode == 1) { // line mode
        audioProcessor.pattern->removePointsInRange(seg / gridsegs + 0.00001, (seg + 1) / gridsegs - 0.00001);
        audioProcessor.pattern->insertPoint(seg / gridsegs + 0.00001, mousey, 0, 1);
        audioProcessor.pattern->insertPoint((seg + 1) / gridsegs - 0.00001, mousey, 0, 1);
    }
    else if (paintMode == 2) { // saw up
        audioProcessor.pattern->removePointsInRange(seg / gridsegs + 0.00001, (seg + 1) / gridsegs - 0.00001);
        audioProcessor.pattern->insertPoint(seg / gridsegs + 0.00001, 1, 0, 1);
        audioProcessor.pattern->insertPoint((seg + 1) / gridsegs - 0.00001, mousey, 0, 1);
    }
    else if (paintMode == 3) { // saw down
        audioProcessor.pattern->removePointsInRange(seg / gridsegs + 0.00001, (seg + 1) / gridsegs - 0.00001);
        audioProcessor.pattern->insertPoint(seg / gridsegs + 0.00001, mousey, 0, 1);
        audioProcessor.pattern->insertPoint((seg + 1) / gridsegs - 0.00001, 1, 0, 1);
    }
    else if (paintMode == 4) { // triangle
        audioProcessor.pattern->removePointsInRange(seg / gridsegs + 0.00001, (seg + 1) / gridsegs - 0.00001);
        audioProcessor.pattern->insertPoint(seg / gridsegs + 0.00001, 1, 0, 1);
        audioProcessor.pattern->insertPoint(seg / gridsegs + (((seg + 1) / gridsegs - seg / gridsegs) / 2), mousey, 0, 1);
        audioProcessor.pattern->insertPoint((seg + 1) / gridsegs - 0.00001, 1, 0, 1);
    }

    audioProcessor.pattern->buildSegments();
}

// ==================================================

bool View::isSnapping(const MouseEvent& e) {
  bool snap = audioProcessor.params.getRawParameterValue("snap")->load() == 1.0f;
  return (snap && !e.mods.isCtrlDown()) || (!snap && e.mods.isCtrlDown());
}

bool View::isCollinear(Segment seg)
{
  return std::fabs(seg.x1 - seg.x2) < 0.01 || std::fabs(seg.y1 - seg.y2) < 0.01;
};

bool View::pointInRect(int x, int y, int xx, int yy, int w, int h)
{
  return x >= xx && x <= xx + w && y >= yy && y <= yy + h;
};
