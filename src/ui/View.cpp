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

View::View(GATE12AudioProcessor& p) : audioProcessor(p)
{
    startTimerHz(60);
    audioProcessor.params.addParameterListener("pattern", this);
};

View::~View()
{
    audioProcessor.params.removeParameterListener("pattern", this);
};

void View::parameterChanged (const juce::String& parameterID, float newValue) 
{
    (void)parameterID;
    (void)newValue;
    if (parameterID == "pattern") {
        selectionStart = Point<int>(-1,-1);
        selectedMidpoint = -1;
        selectedPoint = -1;
        clearSelection();
    }
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
    drawSelection(g);
    if (audioProcessor.xpos > 0.0) {
        drawSeek(g);
    }
    // TODO - IF IS PATTERN CHANGE PENDING
    // PREVENT MOUSE EVENTS AND DRAW WHITE OVERLAY
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

void View::drawSelection(Graphics& g)
{
    if (selectionStart.x > -1 && (selectionStart.x != selectionEnd.x || selectionStart.y != selectionEnd.x)) {
        int x = std::min(selectionStart.x, selectionEnd.x);
        int y = std::min(selectionStart.y, selectionEnd.y);
        int w = std::abs(selectionStart.x - selectionEnd.x);
        int h = std::abs(selectionStart.y - selectionEnd.y);
        auto bounds = Rectangle<int>(x,y,w,h);
        g.setColour(Colour(globals::COLOR_MIDI));
        g.drawRect(bounds);
        g.setColour(Colour(globals::COLOR_MIDI).withAlpha(0.5f));
        g.fillRect(bounds);
    }

    if (selectionPoints.size() > 0) {
        g.setColour(Colour(globals::COLOR_MIDI).withAlpha(0.5f));
        g.fillRect(selectionArea.expanded(MSEL_PADDING));
        g.setColour(Colour(globals::COLOR_MIDI));
        g.drawRect(selectionArea.expanded(MSEL_PADDING));

        if (selectionPoints.size() > 1) {
            drawSelectionHandles(g);
        }
    }
}

void View::drawSelectionHandles(Graphics& g)
{
    auto area = selectionArea.expanded(MSEL_PADDING);
    Point tl = area.getTopLeft(); // top left
    Point tr = area.getTopRight();
    Point bl = area.getBottomLeft();
    Point br = area.getBottomRight();
    Point ml = tl.withY((tl.getY() + bl.getY()) / 2); // middle left
    Point mr = ml.withX(br.getX()); // middle right
    Point tm = tl.withX((tl.getX() + tr.getX()) / 2);// top middle
    Point bm = tm.withY(br.getY());
    auto tlRect = Rectangle<int>(tl.getX(), tl.getY(), 0, 0).expanded(3);
    auto trRect = Rectangle<int>(tr.getX(), tr.getY(), 0, 0).expanded(3);
    auto blRect = Rectangle<int>(bl.getX(), bl.getY(), 0, 0).expanded(3);
    auto brRect = Rectangle<int>(br.getX(), br.getY(), 0, 0).expanded(3);
    auto mlRect = Rectangle<int>(ml.getX(), ml.getY(), 0, 0).expanded(3);
    auto mrRect = Rectangle<int>(mr.getX(), mr.getY(), 0, 0).expanded(3);
    auto tmRect = Rectangle<int>(tm.getX(), tm.getY(), 0, 0).expanded(3);
    auto bmRect = Rectangle<int>(bm.getX(), bm.getY(), 0, 0).expanded(3);
    g.setColour(Colour(globals::COLOR_MIDI));
    g.fillRect(tlRect);g.fillRect(trRect);g.fillRect(blRect);g.fillRect(brRect);
    g.fillRect(mlRect);g.fillRect(mrRect);g.fillRect(tmRect);g.fillRect(bmRect);
    g.setColour(Colours::white);
    if (selectionDragHover == 1) g.fillRect(tlRect);
    if (selectionDragHover == 2) g.fillRect(tmRect);
    if (selectionDragHover == 3) g.fillRect(trRect);
    if (selectionDragHover == 4) g.fillRect(mlRect);
    if (selectionDragHover == 5) g.fillRect(mrRect);
    if (selectionDragHover == 6) g.fillRect(blRect);
    if (selectionDragHover == 7) g.fillRect(bmRect);
    if (selectionDragHover == 8) g.fillRect(brRect); 
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
    Point pos = e.getPosition();
    int x = pos.x;
    int y = pos.y;

    if (e.mods.isLeftButtonDown()) {
        if (selectionDragHover > -1) {
            selectionAreaStart = selectionArea.expanded(0);
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
                origTension = audioProcessor.pattern->points[selectedMidpoint].tension;
                dragStartY = y;
                e.source.enableUnboundedMouseMovement(true);
                setMouseCursor(MouseCursor::NoCursor);
            }
        }
    }
    else if (e.mods.isRightButtonDown()) {
        if (selectionDragHover > -1) {
            return;
        }
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
    else if (selectionStart.x > -1) {
        createSelection();
    }

    selectionStart = Point<int>(-1,-1);
    selectedMidpoint = -1;
    selectedPoint = -1;
}

void View::createSelection()
{
    selectionPoints.clear();
    selectionArea = Rectangle<int>();

    Rectangle<int> selArea = Rectangle<int>(
        std::min(selectionStart.x, selectionEnd.x),
        std::min(selectionStart.y, selectionEnd.y),
        std::abs(selectionStart.x - selectionEnd.x),
        std::abs(selectionStart.y - selectionEnd.y)
    );

    
    auto& points = audioProcessor.pattern->points;
    for (size_t i = 0; i < points.size(); ++i) {
        if (i == 0 || i == points.size() - 1)
            continue;

        auto& p = points[i];
        int x = (int)(p.x * winw + winx);
        int y = (int)(p.y * winh + winy);

        if (selArea.contains(x, y)) {
            selectionPoints.push_back({ p.id, p.x, p.y, 0., 0. });
        }
    }

    if (selectionPoints.size() == 0) {
        return;
    }

    // calculate selection area based on points positions
    int minx = globals::PLUG_WIDTH;
    int maxx = -1;
    int miny = globals::PLUG_HEIGHT;
    int maxy = -1;
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        int x = (int)(p.x * winw + winx);
        int y = (int)(p.y * winh + winy);
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
    }
    selectionArea = Rectangle<int>(minx, miny, maxx - minx, maxy - miny);

    // calculate points positios relative to area
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        double x = p.x * winw + winx;
        double y = p.y * winh + winy;
        p.areax = std::max(0.0, std::min(1.0, (x - selectionArea.getX()) / (double)selectionArea.getWidth()));
        p.areay = std::max(0.0, std::min(1.0, (y - selectionArea.getY()) / (double)selectionArea.getHeight()));
    }
}

void View::clearSelection()
{
    selectionArea = Rectangle<int>();
    selectionPoints.clear();
}

void View::mouseMove(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    hoverPoint = -1;
    hoverMidpoint = -1;
    selectionDragHover = -1;

    // if currently dragging a point ignore mouse over events
    if (selectedPoint > -1 || selectedMidpoint > -1) {
        return;
    }

    // multi selection mouse over
    if (selectionPoints.size() > 0 && selectionArea.expanded(MSEL_PADDING + 3).contains(e.getPosition())) {
        selectionDragHover = 0;
        Point tl = selectionArea.expanded(MSEL_PADDING).getTopLeft(); // top left
        Point tr = selectionArea.expanded(MSEL_PADDING).getTopRight();
        Point bl = selectionArea.expanded(MSEL_PADDING).getBottomLeft();
        Point br = selectionArea.expanded(MSEL_PADDING).getBottomRight();
        Point ml = tl.withY((tl.getY() + bl.getY()) / 2); // middle left
        Point mr = ml.withX(br.getX()); // middle right
        Point tm = tl.withX((tl.getX() + tr.getX()) / 2);// top middle
        Point bm = tm.withY(br.getY());
        if (Rectangle<int>(tl.getX(), tl.getY(), 0, 0).expanded(3).contains(pos)) selectionDragHover = 1; // mouse over top left drag handle
        if (Rectangle<int>(tm.getX(), tm.getY(), 0, 0).expanded(3).contains(pos)) selectionDragHover = 2; 
        if (Rectangle<int>(tr.getX(), tr.getY(), 0, 0).expanded(3).contains(pos)) selectionDragHover = 3; 
        if (Rectangle<int>(ml.getX(), ml.getY(), 0, 0).expanded(3).contains(pos)) selectionDragHover = 4; 
        if (Rectangle<int>(mr.getX(), mr.getY(), 0, 0).expanded(3).contains(pos)) selectionDragHover = 5; 
        if (Rectangle<int>(bl.getX(), bl.getY(), 0, 0).expanded(3).contains(pos)) selectionDragHover = 6; 
        if (Rectangle<int>(bm.getX(), bm.getY(), 0, 0).expanded(3).contains(pos)) selectionDragHover = 7; 
        if (Rectangle<int>(br.getX(), br.getY(), 0, 0).expanded(3).contains(pos)) selectionDragHover = 8;
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
    Point pos = e.getPosition();
    int x = pos.x;
    int y = pos.y;

    if (selectionDragHover > -1 && e.mods.isLeftButtonDown()) {
        dragSelection(e);
        return;
    }

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

    else if (selectionStart.x > -1) {
        selectionEnd = e.getPosition();
    }

    audioProcessor.pattern->buildSegments();
}

void View::dragSelection(const MouseEvent& e)
{
    auto mouse = e.getPosition();
    auto mouseDown = e.getMouseDownPosition();

    if (isSnapping(e)) {
        double grid = (double)audioProcessor.grid;
        double gridx = double(winw) / grid;
        double gridy = double(winh) / grid;
        mouse.x = (int)(std::round((mouse.x - winx) / gridx) * gridx + winx);
        mouse.y = (int)(std::round((mouse.y - winy) / gridy) * gridy + winy);
        mouseDown.x = (int)(std::round((mouseDown.x - winx) / gridx) * gridx + winx);
        mouseDown.y = (int)(std::round((mouseDown.y - winy) / gridy) * gridy + winy);
    }

    selectionArea = selectionAreaStart.expanded(0,0);
    int dx = mouse.x - mouseDown.x;
    int dy = mouse.y - mouseDown.y;
    int left = selectionArea.getX();
    int right = selectionArea.getRight();
    int top = selectionArea.getY();
    int bottom = selectionArea.getBottom();

    if (selectionDragHover == 0) { // area drag
        left += dx;
        right += dx;
        top += dy;
        bottom += dy;
    }
    else if (selectionDragHover == 1) { // top left corner
        left += dx;
        top += dy;
        right = selectionArea.getRight() - (e.mods.isShiftDown() ? dx : 0);
        bottom = selectionArea.getBottom() - (e.mods.isShiftDown() ? dy : 0);
    }
    else if (selectionDragHover == 2) { // top middle corner
        top += dy;
        bottom = selectionArea.getBottom() - (e.mods.isShiftDown() ? dy : 0);
    }
    else if (selectionDragHover == 3) { // top right
        right += dx;
        top += dy;
        left = selectionArea.getX() - (e.mods.isShiftDown() ? dx : 0);
        bottom = selectionArea.getBottom() - (e.mods.isShiftDown() ? dy : 0);
    }
    else if (selectionDragHover == 4) { // mid left
        left += dx;
        right = selectionArea.getRight() - (e.mods.isShiftDown() ? dx : 0);
    }
    else if (selectionDragHover == 5) { // mid right
        right += dx;
        left = selectionArea.getX() - (e.mods.isShiftDown() ? dx : 0);
    }
    else if (selectionDragHover == 6) { // bottom left
        left += dx;
        bottom += dy;
        right = selectionArea.getRight() - (e.mods.isShiftDown() ? dx : 0);
        top = selectionArea.getY() - (e.mods.isShiftDown() ? dy : 0);
    }
    else if (selectionDragHover == 7) { // bottom mid
        bottom += dy;
        top = selectionArea.getY() - (e.mods.isShiftDown() ? dy : 0);
    }
    else if (selectionDragHover == 8) { // bottom right
        right += dx;
        bottom += dy;
        left = selectionArea.getX() - (e.mods.isShiftDown() ? dx : 0);
        top = selectionArea.getY() - (e.mods.isShiftDown() ? dy : 0);
    }

    bool invertx = false;
    bool inverty = false;

    if (right < left) {
        invertx = true;
        std::swap(left, right);
    }
    if (top > bottom) {
        inverty = true;
        std::swap(top, bottom);
    }

    if (left < winx) {
        right = right - left + winx;
        left = winx;
    }
    if (right > winx + winw) {
        left = winx + winw + left - right;
        right = winx + winw;
    }
    if (top < winy) {
        bottom = bottom - top + winy;
        top = winy;
    }
    if (bottom > winy + winh) {
        top = winy + winh + top - bottom;
        bottom = winy + winh;
    }

    auto p = selectionPoints;
    selectionArea.setX(left);
    selectionArea.setRight(right);
    selectionArea.setY(top);
    selectionArea.setBottom(bottom);
    if (selectionArea.getWidth() > winw) {
        selectionArea.setX(winx);
        selectionArea.setWidth(winw);
    }
    if (selectionArea.getHeight() > winh) {
        selectionArea.setY(winy);
        selectionArea.setHeight(winh);
    }

    updatePointsToSelection(invertx, inverty);
}

// updates points position to match the selection area
// points have a position relative to area: xarea, yarea normalized from 0 to 1
// xarea and yarea are used calculate the new points position on the view
void View::updatePointsToSelection(bool invertx, bool inverty)
{
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        
        double areax = invertx ? 1.0 - p.areax : p.areax;
        double areay = inverty ? 1.0 - p.areay : p.areay;
        double absx = selectionArea.getX() + (areax * selectionArea.getWidth());
        double absy = selectionArea.getY() + (areay * selectionArea.getHeight());
        double newx = (absx - winx) / (double)winw;
        double newy = (absy - winy) / (double)winh;

        if (newx <= 0.0) newx = 0.000001;
        if (newx >= 1.0) newx = 1-0.000001;
        if (newy <= 0.0) newy = 0.000001;
        if (newy >= 1.0) newy = 1-0.000001;

        // update selection point
        p.x = newx;
        p.y = newy;

        // update pattern point
        auto& points = audioProcessor.pattern->points;
        for (size_t j = 0; j < points.size(); ++j) {
            auto& pp = points[j];
            if (pp.id == p.id) {
                pp.x = newx;
                pp.y = newy;
                break;
            }
        }
    }

    // TODO sort points
    audioProcessor.pattern->sortPoints();
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
