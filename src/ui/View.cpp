/*
  ==============================================================================

    View.cpp
    Author:  tiagolr

  ==============================================================================
*/

#include "View.h"
#include "../PluginProcessor.h"
#include "../Globals.h"
#include <utility>

View::View(GATE12AudioProcessor& p) : audioProcessor(p), multiselect(p)
{
    setWantsKeyboardFocus(true);
    startTimerHz(60);
};

View::~View()
{
};

void View::timerCallback()
{
    if (patternID != audioProcessor.pattern->versionID) {
        selectionStart = Point<int>(-1,-1);
        selectedMidpoint = -1;
        selectedPoint = -1;
        multiselect.mouseHover = -1;
        hoverPoint = -1;
        hoverMidpoint = -1;
        multiselect.recalcSelectionArea();
        patternID = audioProcessor.pattern->versionID;
    }
    if (audioProcessor.queuedPattern && isEnabled()) {
        setAlpha(0.5f);
        setEnabled(false);
    }
    else if (!audioProcessor.queuedPattern && !isEnabled()) {
        setAlpha(1.f);
        setEnabled(true);
    }
    repaint();
}

void View::resized()
{
    auto bounds = getLocalBounds();
    winx = bounds.getX() + globals::PAD;
    winy = bounds.getY() + globals::PAD;
    winw = bounds.getWidth() - globals::PAD * 2;
    winh = bounds.getHeight() - globals::PAD * 2;
    multiselect.setViewBounds(winx, winy, winw, winh);
    MessageManager::callAsync([this] {
        audioProcessor.viewW = winw;
    });
}

void View::paint(Graphics& g) {
    g.setColour(Colour(globals::COLOR_BG).darker(0.2f));
    g.fillRect(winx + winw/4, winy, winw/4, winh);
    g.fillRect(winx + winw - winw/4, winy, winw/4, winh);

    drawWave(g, audioProcessor.preSamples, Colour(0xff7f7f7f));
    drawWave(g, audioProcessor.postSamples, Colour(globals::COLOR_ACTIVE));
    drawGrid(g);
    multiselect.drawBackground(g);
    drawSegments(g);
    drawMidPoints(g);
    drawPoints(g);
    drawSelection(g);
    multiselect.draw(g);
    drawSeek(g);
}

void View::drawWave(Graphics& g, std::vector<double>& samples, Colour color) const
{
    Path wavePath;
    wavePath.startNewSubPath((float)winx, (float)(winy + winh));

    for (int i = 0; i < winw; ++i) {
        double ypos = std::min(std::abs(samples[i]), 1.0);
        float x = (float)(i + winx);
        float y = (float)(winh - ypos * winh + winy);

        wavePath.lineTo(x, y);
    }

    wavePath.lineTo((float)(winw + winx - 1), (float)(winy + winh));
    wavePath.closeSubPath();

    g.setColour(color.withAlpha(0.4f));
    g.fillPath(wavePath);
}

void View::drawGrid(Graphics& g)
{
    int grid = audioProcessor.getCurrentGrid();
    int maxLevel = static_cast<int>(std::log2(grid)); // used for grid emphasis calculation

    double gridx = double(winw) / grid;
    double gridy = double(winh) / grid;

    auto getScore = [maxLevel, grid](int i) {
        if (i == 0 || i == grid) return 1.0f; // Full border

        for (int level = maxLevel; level >= 0; --level) {
            int div = 1 << level;
            if (grid % div != 0) continue; // skip non-even subdivisions
            if (i % div == 0) {
                // Higher-level divisions should get higher alpha
                float score = (float(level) / maxLevel);
                return score;
            }
        }

        return 0.1f; // fallback for non-aligned edges
    };

    for (int i = 0; i < grid + 1; ++i) {
        auto score = getScore(i);
        g.setColour(Colours::white.withAlpha(0.025f + score * (0.15f - 0.025f))); // map score into min + score * (max - min)
        float y = std::round((float)(winy + gridy * i)) + 0.5f; // +0.5f removes aliasing
        float x = std::round((float)(winx + gridx * i)) + 0.5f;


        g.drawLine(x, (float)winy, x, (float)winy + winh);
        g.drawLine((float)winx, y, (float)winx + winw, y);
    }
}

void View::drawSegments(Graphics& g)
{
    double lastX = winx;
    double lastY = audioProcessor.pattern->get_y_at(0) * winh + winy;

    Path linePath;
    Path shadePath;

    linePath.startNewSubPath((float)lastX, (float)lastY);
    shadePath.startNewSubPath((float)lastX, (float)(winy + winh)); // Start from bottom
    shadePath.lineTo((float)lastX, (float)lastY);                   // Line to first point on wave

    for (int i = 0; i < winw + 1; ++i)
    {
        double px = double(i) / double(winw);
        double py = audioProcessor.pattern->get_y_at(px) * winh + winy;
        float x = (float)(i + winx);
        float y = (float)py;

        linePath.lineTo(x, y);
        shadePath.lineTo(x, y);
    }

    shadePath.lineTo((float)(winw + winx), (float)(winy + winh)); // Bottom-right
    shadePath.closeSubPath();

    g.setColour(Colours::white.withAlpha(0.125f));
    g.fillPath(shadePath);
    g.setColour(Colours::white);
    g.strokePath(linePath, PathStrokeType(1.f));
}

void View::drawPoints(Graphics& g)
{
    auto& points = audioProcessor.pattern->points;

    g.setColour(Colours::white);
    for (auto pt = points.begin(); pt != points.end(); ++pt) {
        auto xx = pt->x * winw + winx;
        auto yy = pt->y * winh + winy;
        g.fillEllipse((float)(xx - POINT_RADIUS), (float)(yy - 4.0), (float)(POINT_RADIUS * 2), (float)(POINT_RADIUS * 2));
    }

    g.setColour(Colours::white.withAlpha(0.5f));
    if (selectedPoint == -1 && selectedMidpoint == -1 && hoverPoint > -1)
    {
        auto xx = points[hoverPoint].x * winw + winx;
        auto yy = points[hoverPoint].y * winh + winy;
        g.fillEllipse((float)(xx - HOVER_RADIUS), (float)(yy - HOVER_RADIUS), (float)HOVER_RADIUS * 2.f, (float)HOVER_RADIUS * 2.f);
    }

    g.setColour(Colours::red.withAlpha(0.5f));
    if (selectedPoint != -1)
    {
        auto xx = points[selectedPoint].x * winw + winx;
        auto yy = points[selectedPoint].y * winh + winy;
        g.fillEllipse((float)(xx - 4.0), (float)(yy-4.0), 8.0f, 8.0f);
    }
}

void View::drawSelection(Graphics& g)
{
    if (selectionStart.x > -1 && (selectionStart.x != selectionEnd.x || selectionStart.y != selectionEnd.y)) {
        int x = std::min(selectionStart.x, selectionEnd.x);
        int y = std::min(selectionStart.y, selectionEnd.y);
        int w = std::abs(selectionStart.x - selectionEnd.x);
        int h = std::abs(selectionStart.y - selectionEnd.y);
        auto bounds = Rectangle<int>(x,y,w,h);
        g.setColour(Colour(globals::COLOR_SELECTION));
        g.drawRect(bounds);
        g.setColour(Colour(globals::COLOR_SELECTION).withAlpha(0.25f));
        g.fillRect(bounds);
    }
}

std::vector<double> View::getMidpointXY(Segment seg)
{
    double x = (std::max(seg.x1, 0.0) + std::min(seg.x2, 1.0)) * 0.5;
    double y = seg.type > 1 && seg.x1 >= 0.0 && seg.x2 <= 1.0
        ? (seg.y1 + seg.y2) / 2
        : audioProcessor.pattern->get_y_at(x);

    return {
        x * winw + winx,
        y * winh + winy
    };
}

void View::drawMidPoints(Graphics& g)
{
    auto segs = audioProcessor.pattern->segments;

    g.setColour(Colour(globals::COLOR_ACTIVE));
    for (auto seg = segs.begin(); seg != segs.end(); ++seg) {
        if (!isCollinear(*seg) && seg->type != PointType::Hold) {
            auto xy = getMidpointXY(*seg);
            g.drawEllipse((float)xy[0] - MPOINT_RADIUS, (float)xy[1] - MPOINT_RADIUS, (float)MPOINT_RADIUS * 2.f, (float)MPOINT_RADIUS * 2.f, 2.f);
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
        auto waveCount = audioProcessor.pattern->getWaveCount(seg);
        if (waveCount > 0) {
            auto bounds = Rectangle<int>((int)xy[0]-15,(int)xy[1]-25, 30, 20);
            g.setColour(Colour(globals::COLOR_BG).withAlpha(.75f));
            g.fillRoundedRectangle(bounds.toFloat(), 4.f);
            g.setColour(Colours::white);
            g.setFont(FontOptions(14.f));
            g.drawText(String(waveCount), bounds, Justification::centred);
        }
    }
}

void View::drawSeek(Graphics& g)
{
    auto xpos = audioProcessor.xenv.load();
    auto ypos = audioProcessor.yenv.load();
    bool drawSeek = audioProcessor.drawSeek.load();

    if (drawSeek) {
        g.setColour(Colour(globals::COLOR_SEEK).withAlpha(0.5f));
        g.drawLine((float)(xpos * winw + winx), (float)winy, (float)(xpos * winw + winx), (float)(winy + winh));
    }
    g.setColour(Colour(globals::COLOR_SEEK));
    g.drawEllipse((float)(xpos * winw + winx - 5.f), (float)((1 - ypos) * winh + winy - 5.f), 10.0f, 10.0f, 1.0f);
}

int View::getHoveredPoint(int x, int y)
{
    auto points = audioProcessor.pattern->points;
    for (auto i = 0; i < points.size(); ++i) {
        auto xx = (int)(points[i].x * winw + winx);
        auto yy = (int)(points[i].y * winh + winy);
        if (pointInRect(x, y, xx - POINT_RADIUS, yy - POINT_RADIUS, POINT_RADIUS * 2, POINT_RADIUS * 2)) {
            return i;
        }
    }
    return -1;
};

int View::getHoveredMidpoint(int x, int y)
{
    auto segs = audioProcessor.pattern->segments;
    for (auto i = 0; i < segs.size(); ++i) {
        auto& seg = segs[i];
        auto xy = getMidpointXY(seg);
        if (!isCollinear(seg) && seg.type != PointType::Hold && pointInRect(x, y, (int)xy[0] - HOVER_RADIUS, (int)xy[1] - HOVER_RADIUS, HOVER_RADIUS * 2, HOVER_RADIUS * 2)) {
            return i;
        }
    }
    return -1;
};

// Midpoint index is derived from segment nu
// there is an extra segment before the first point
// so the matching pattern point to each midpoint is midpoint - 1
PPoint& View::getPointFromMidpoint(int midpoint)
{
    auto size = (int)audioProcessor.pattern->points.size();
    auto index = midpoint == 0 ? size - 1 : midpoint - 1;

    if (index >= size)
        index -= size;

    return audioProcessor.pattern->points[index];
}

void View::mouseDown(const juce::MouseEvent& e)
{
    if (!isEnabled() || patternID != audioProcessor.pattern->versionID)
        return;

    Point pos = e.getPosition();
    int x = pos.x;
    int y = pos.y;

    if (e.mods.isLeftButtonDown()) {
        if (multiselect.mouseHover > -1) {
            multiselect.mouseDown(e);
            return;
        }

        selectedPoint = getHoveredPoint(x, y);
        if (selectedPoint == -1)
            selectedMidpoint = getHoveredMidpoint(x, y);

        if (selectedPoint == -1 && selectedMidpoint == -1) {
            selectionStart = e.getPosition();
            selectionEnd = e.getPosition();
        }

        if (selectedPoint > -1 || selectedMidpoint > -1) {
            if (selectedPoint > -1) {
                setMouseCursor(MouseCursor::NoCursor);
            }
            if (selectedMidpoint > -1) {
                origTension = getPointFromMidpoint(selectedMidpoint).tension;
                dragStartY = y;
                e.source.enableUnboundedMouseMovement(true);
                setMouseCursor(MouseCursor::NoCursor);
            }
        }
    }
    else if (e.mods.isRightButtonDown()) {
        if (multiselect.mouseHover > -1) {
            return;
        }
        rmousePoint = getHoveredPoint(x, y);
        if (rmousePoint > -1) {
            showPointContextMenu(e);
        }
        else {
            multiselect.clearSelection();
            applyPaintTool(x, y, e);
        }
    }
}

void View::mouseUp(const juce::MouseEvent& e)
{
    if (!isEnabled() || patternID != audioProcessor.pattern->versionID)
        return;

    if (selectedPoint > -1) { // finished dragging point
        setMouseCursor(MouseCursor::NormalCursor);
    }
    else if (selectedMidpoint > -1) { // finished dragging midpoint, place cursor at midpoint
        setMouseCursor(MouseCursor::NormalCursor);
        e.source.enableUnboundedMouseMovement(false);
        auto& mpoint = getPointFromMidpoint(selectedMidpoint);
        auto& next = getPointFromMidpoint(selectedMidpoint + 1);
        double midx = (mpoint.x + next.x) / 2.;
        int x = (int)(midx * winw + winx) + getScreenPosition().x;
        int y = (int)(audioProcessor.pattern->get_y_at(midx) * winh + winy) + getScreenPosition().y;
        Desktop::getInstance().setMousePosition(juce::Point<int>(x, y));
    }
    else if (selectionStart.x > -1) { // finished creating selection
        multiselect.createSelection(e, selectionStart, selectionEnd);
    }
    else if (multiselect.selectionPoints.size() > 0) { // finished dragging selection
        multiselect.recalcSelectionArea(); // FIX - points may have been inverted due to selection drag
    }

    selectionStart = Point<int>(-1,-1);
    selectedMidpoint = -1;
    selectedPoint = -1;
}



void View::mouseMove(const juce::MouseEvent& e)
{
    hoverPoint = -1;
    hoverMidpoint = -1;
    multiselect.mouseHover = -1;

    if (!isEnabled() || patternID != audioProcessor.pattern->versionID)
        return;

    auto pos = e.getPosition();

    // if currently dragging a point ignore mouse over events
    if (selectedPoint > -1 || selectedMidpoint > -1) {
        return;
    }

    // multi selection mouse over
    if (multiselect.selectionPoints.size() > 0 && multiselect.contains(e.getPosition())) {
        multiselect.mouseMove(e);
        return;
    }

    int x = pos.x;
    int y = pos.y;

    hoverPoint = getHoveredPoint(x , y);
    if (hoverPoint == -1)
        hoverMidpoint = getHoveredMidpoint(x, y);
}

void View::mouseDrag(const juce::MouseEvent& e)
{
    if (!isEnabled() || patternID != audioProcessor.pattern->versionID)
        return;

    Point pos = e.getPosition();
    int x = pos.x;
    int y = pos.y;

    if (multiselect.mouseHover > -1 && e.mods.isRightButtonDown()) {
        return;
    }

    if (multiselect.mouseHover > -1 && e.mods.isLeftButtonDown()) {
        multiselect.dragSelection(e);
        return;
    }

    if (rmousePoint == -1 && e.mods.isRightButtonDown()) {
        applyPaintTool(x, y, e);
        return;
    }

    auto& points = audioProcessor.pattern->points;
    double grid = (double)audioProcessor.getCurrentGrid();
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
    if (yy > 1) yy = 1.0;
    if (yy < 0) yy = 0.0;

    if (selectedPoint > -1) {
        auto& point = points[selectedPoint];
        point.y = yy;
        point.x = xx;
        if (point.x > 1) point.x = 1;
        if (point.x < 0) point.x = 0;
        if (selectedPoint < points.size() - 1) {
            auto& next = points[static_cast<size_t>(selectedPoint) + 1];
            if (point.x > next.x) point.x = next.x;
        }
        if (selectedPoint > 0) {
            auto& prev = points[static_cast<size_t>(selectedPoint) - 1];
            if (point.x < prev.x) point.x = prev.x;
        }
    }

    else if (selectedMidpoint > -1) {
        int distance = y - dragStartY;
        auto& mpoint = getPointFromMidpoint(selectedMidpoint);
        auto& next = getPointFromMidpoint(selectedMidpoint + 1);
        if (mpoint.y < next.y) distance *= -1;
        float tension = (float)origTension + float(distance) / 500.f;
        if (tension > 1) tension = 1;
        if (tension < -1) tension = -1;
        mpoint.tension = tension;
    }

    else if (selectionStart.x > -1) {
        selectionEnd = e.getPosition();
    }

    audioProcessor.pattern->buildSegments();
}

void View::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!isEnabled() || patternID != audioProcessor.pattern->versionID)
        return;

    if (e.mods.isRightButtonDown()) {
        return;
    }

    if (multiselect.mouseHover > -1) {
        multiselect.clearSelection();
        return;
    }

    int x = e.getPosition().x;
    int y = e.getPosition().y;
    int pt = getHoveredPoint((int)x, (int)y);
    int mid = getHoveredMidpoint((int)x, (int)y);

    if (pt > -1) {
        audioProcessor.pattern->removePoint(pt);
        hoverPoint = -1;
        hoverMidpoint = -1;
    }
    if (pt == -1 && mid > -1) {
        getPointFromMidpoint(mid).tension = 0;
    }
    if (pt == -1 && mid == -1) {
        double px = (double)x;
        double py = (double)y;
        if (isSnapping(e)) {
            double grid = (double)audioProcessor.getCurrentGrid();
            double gridx = double(winw) / grid;
            double gridy = double(winh) / grid;
            px = std::round(double(px - winx) / gridx) * gridx + winx;
            py = std::round(double(py - winy) / gridy) * gridy + winy;
        }
        px = double(px - winx) / (double)winw;
        py = double(py - winy) / (double)winh;
        if (px >= 0 && px <= 1 && py >= 0 && py <= 1) { // point in env window
            audioProcessor.pattern->insertPoint(px, py, 0, (int)audioProcessor.params.getRawParameterValue("point")->load());
        }
    }

    audioProcessor.pattern->buildSegments();
}

void View::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (!isEnabled() || patternID != audioProcessor.pattern->versionID)
        return;

    (void)event;
    int grid = (int)audioProcessor.params.getRawParameterValue("grid")->load();
    auto param = audioProcessor.params.getParameter("grid");
    int newgrid = grid + (wheel.deltaY > 0.f ? 1 : -1);
    // constrain grid size to stay on straights or tripplets
    if (!(grid == 3 && newgrid == 4) && !(grid == 4 && newgrid == 3)) {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1((float)newgrid));
        param->endChangeGesture();
    }
}

bool View::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled() || patternID != audioProcessor.pattern->versionID)
        return false;

    // remove selected points
    if (key == KeyPress::deleteKey)
    {
        multiselect.deleteSelectedPoints();
        return true;
    }

    // Let the parent class handle other keys (optional)
    return Component::keyPressed(key);
}

// Just in case reset the mouse active points
// trying to fix crashes, points should be using point ids instead
void View::mouseExit(const MouseEvent& event)
{
    (void)event;
    hoverPoint = -1;
    selectedPoint = -1;
    selectedMidpoint = -1;
    hoverMidpoint = -1;
}

void View::showPointContextMenu(const juce::MouseEvent& event)
{
    (void)event;
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
    double gridsegs = (double)audioProcessor.getCurrentGrid();
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

    hoverPoint = -1;
    hoverMidpoint = -1;
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
