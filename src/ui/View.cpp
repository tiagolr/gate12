/*
  ==============================================================================

    View.cpp
    Author:  tiagolr

  ==============================================================================
*/

#include "View.h"
#include "../PluginProcessor.h"
#include <utility>

View::View(GATE12AudioProcessor& p) : audioProcessor(p), multiSelect(p), paintTool(p)
{
    setWantsKeyboardFocus(true);
    startTimerHz(60);
};

View::~View()
{
};

void View::timerCallback()
{
    if (patternID != audioProcessor.viewPattern->versionID || audioProcessor.uimode != luimode) {
        if (audioProcessor.uimode != luimode)
            multiSelect.clearSelection();
        preSelectionStart = Point<int>(-1,-1);
        selectedMidpoint = 0;
        selectedPoint = 0;
        multiSelect.mouseHover = -1;
        hoverPoint = 0;
        hoverMidpoint = 0;
        multiSelect.recalcSelectionArea();
        patternID = audioProcessor.viewPattern->versionID;
    }
    if (audioProcessor.queuedPattern && isEnabled()) {
        setAlpha(0.5f);
        setEnabled(false);
    }
    else if (!audioProcessor.queuedPattern && !isEnabled()) {
        setAlpha(1.f);
        setEnabled(true);
    }

    luimode = audioProcessor.uimode;
    repaint();
}

void View::resized()
{
    auto bounds = getLocalBounds();
    winx = bounds.getX() + PLUG_PADDING;
    winy = bounds.getY() + PLUG_PADDING + 10;
    winw = bounds.getWidth() - PLUG_PADDING * 2;
    winh = bounds.getHeight() - PLUG_PADDING * 2 - 10;

    multiSelect.setViewBounds(winx, winy, winw, winh);
    paintTool.setViewBounds(winx, winy, winw, winh);
    audioProcessor.sequencer->setViewBounds(winx, winy, winw, winh);
    juce::Component::SafePointer<View> safeThis(this); // FIX Renoise DAW crashing on plugin instantiated
    MessageManager::callAsync([safeThis] {
        if (safeThis != nullptr)
            safeThis->audioProcessor.viewW = safeThis->winw;
    });
    multiSelect.recalcSelectionArea();
}

void View::paint(Graphics& g) {
    g.setColour(Colour(COLOR_BG));
    g.fillRect(winx,winy,winw,winh);
    auto uimode = audioProcessor.uimode;
    if (uimode == UIMode::PaintEdit) {
        g.setColour(Colours::blue.withAlpha(0.05f));
        g.fillRect(winx, winy, winw, winh);
    }
    g.setColour(Colours::black.withAlpha(0.2f));
    g.fillRect(winx + winw/4, winy, winw/4, winh);
    g.fillRect(winx + winw - winw/4, winy, winw/4, winh);

    if (uimode == UIMode::Seq && isMouseOver())
        audioProcessor.sequencer->drawBackground(g);

    if (uimode == UIMode::Normal || uimode == UIMode::Seq) {
        if (audioProcessor.drawSidechain) {
            drawWave(g, audioProcessor.sideSamples, Colour(COLOR_ACTIVE).brighter(0.75f));
        }
        drawWave(g, audioProcessor.preSamples, Colour(0xff7f7f7f));
        drawWave(g, audioProcessor.postSamples, Colour(COLOR_ACTIVE));
    }

    drawGrid(g);

    if (selectedPoint > 0 || hoverPoint > 0) {

        PPoint pt = getPoint(selectedPoint > 0 ? selectedPoint : hoverPoint);
        auto area = Rectangle<float>((float)(winx + 2), 10.f, 80.f, (float)PLUG_PADDING);
        g.setColour(Colour(COLOR_NEUTRAL));
        g.setFont(FontOptions(14.f));
        auto db = pt.y >= 1.0 ? 0.f : 20 * std::log10(1.0 - pt.y);
        auto dbstr = pt.y >= 1.0 ? "-Inf" : String(std::round(db * 100) / 100) + " dB";
        String ystr = String("y = ") + dbstr;
        g.drawText(ystr, area.translated(70.f, 0.f), Justification::centredLeft, false);

        auto sync = (bool)audioProcessor.params.getRawParameterValue("sync")->load();
        double duration = audioProcessor.syncQN * audioProcessor.secondsPerBeat;
        if (!sync) {
            double ratehz = (double)audioProcessor.params.getRawParameterValue("rate")->load();
            duration = 1.f / ratehz;
        }
        duration *= pt.x;
        String xstr = "x = " + (duration < 1.0 
            ? String(std::round(duration * 1000.0)) + " ms" 
            : String(std::round(duration * 100.f) / 100.f) + " s");
        g.drawText(xstr, area, Justification::centredLeft, false);
    }

    multiSelect.drawBackground(g);
    drawSegments(g);

    if (uimode == UIMode::Normal || uimode == UIMode::PaintEdit) {
        drawMidPoints(g);
        drawPoints(g);
    }

    if (uimode == UIMode::Paint && (isMouseOver() || paintTool.dragging)) {
        paintTool.draw(g);
    }

    drawPreSelection(g);
    multiSelect.draw(g);

    if (uimode != UIMode::PaintEdit) {
        drawSeek(g);
    }

    if (uimode == UIMode::Seq)
        audioProcessor.sequencer->draw(g);
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
        g.setColour(Colours::white.withAlpha(0.02f + score * (0.25f - 0.02f))); // map score into min + score * (max - min)
        float y = (float)(winy + std::round(gridy * i) + 0.5f); // 0.5 removes aliasing
        float x = (float)(winx + std::round(gridx * i) + 0.5f);

        g.drawLine(x, (float)winy, x, (float)winy + winh);
        g.drawLine((float)winx, y, (float)winx + winw, y);
    }
}

void View::drawSegments(Graphics& g)
{
    double lastX = winx;
    double lastY = audioProcessor.viewPattern->get_y_at(0) * winh + winy;

    Path linePath;
    Path shadePath;

    linePath.startNewSubPath((float)lastX, (float)lastY);
    shadePath.startNewSubPath((float)lastX, (float)(winy + winh)); // Start from bottom
    shadePath.lineTo((float)lastX, (float)lastY);                   // Line to first point on wave

    for (int i = 0; i < winw + 1; ++i)
    {
        double px = double(i) / double(winw);
        double py = audioProcessor.viewPattern->get_y_at(px) * winh + winy;
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
    auto& points = audioProcessor.viewPattern->points;

    g.setColour(Colours::white);
    for (auto pt = points.begin(); pt != points.end(); ++pt) {
        auto xx = pt->x * winw + winx;
        auto yy = pt->y * winh + winy;
        g.fillEllipse((float)(xx - POINT_RADIUS), (float)(yy - 4.0), (float)(POINT_RADIUS * 2), (float)(POINT_RADIUS * 2));
    }

    g.setColour(Colours::white.withAlpha(0.5f));
    if (!selectedPoint && !selectedMidpoint && hoverPoint)
    {
        auto& pt = getPoint(hoverPoint);
        auto xx = pt.x * winw + winx;
        auto yy = pt.y * winh + winy;
        g.fillEllipse((float)(xx - HOVER_RADIUS), (float)(yy - HOVER_RADIUS), (float)HOVER_RADIUS * 2.f, (float)HOVER_RADIUS * 2.f);
    }

    g.setColour(Colours::red.withAlpha(0.5f));
    if (selectedPoint)
    {
        auto& pt = getPoint(selectedPoint);
        auto xx = pt.x * winw + winx;
        auto yy = pt.y * winh + winy;
        g.fillEllipse((float)(xx - 4.0), (float)(yy-4.0), 8.0f, 8.0f);
    }
}

void View::drawPreSelection(Graphics& g)
{
    if (preSelectionStart.x == -1 || (preSelectionStart.x == preSelectionEnd.x && preSelectionStart.y == preSelectionEnd.y))
        return;

    int x1 = std::clamp(preSelectionStart.x, 0, getWidth());
    int y1 = std::clamp(preSelectionStart.y, 0, getHeight());
    int x2 = std::clamp(preSelectionEnd.x, 0, getWidth());
    int y2 = std::clamp(preSelectionEnd.y, 0, getHeight());

    int x = std::min(x1, x2);
    int y = std::min(y1, y2);
    int w = std::abs(x2 - x1);
    int h = std::abs(y2 - y1);

    auto bounds = Rectangle<int>(x,y,w,h);

    g.setColour(Colour(COLOR_SELECTION));
    g.drawRect(bounds);
    g.setColour(Colour(COLOR_SELECTION).withAlpha(0.25f));
    g.fillRect(bounds);
}

std::vector<double> View::getMidpointXY(Segment seg)
{
    double x = (std::max(seg.x1, 0.0) + std::min(seg.x2, 1.0)) * 0.5;
    double y = seg.type > 1 && seg.type != PointType::HalfSine && seg.x1 >= 0.0 && seg.x2 <= 1.0
        ? (seg.y1 + seg.y2) / 2
        : audioProcessor.viewPattern->get_y_at(x);

    return {
        x * winw + winx,
        y * winh + winy
    };
}

void View::drawMidPoints(Graphics& g)
{
    auto segs = audioProcessor.viewPattern->getSegments();

    // draw midpoints
    g.setColour(Colour(COLOR_ACTIVE));
    for (auto seg = segs.begin(); seg != segs.end(); ++seg) {
        if (!isCollinear(*seg) && seg->type != PointType::Hold) {
            auto xy = getMidpointXY(*seg);
            g.drawEllipse((float)xy[0] - MPOINT_RADIUS, (float)xy[1] - MPOINT_RADIUS, (float)MPOINT_RADIUS * 2.f, (float)MPOINT_RADIUS * 2.f, 2.f);
        }
    }

    // draw hovered midpoint
    g.setColour(Colour(COLOR_ACTIVE).withAlpha(0.5f));
    if (!selectedPoint && !selectedMidpoint && hoverMidpoint) {
        auto idx = getPointIndex(hoverMidpoint);
        auto next = idx + 1;
        auto& seg = segs[next];
        auto xy = getMidpointXY(seg);
        g.fillEllipse((float)xy[0] - HOVER_RADIUS, (float)xy[1] - HOVER_RADIUS, (float)HOVER_RADIUS * 2.f, (float)HOVER_RADIUS * 2.f);
    }

    // draw selected midpoint
    g.setColour(Colour(COLOR_ACTIVE));
    if (selectedMidpoint) {
        auto idx = getPointIndex(hoverMidpoint);
        auto next = idx + 1;
        auto& seg = segs[next];
        auto xy = getMidpointXY(seg);
        g.fillEllipse((float)xy[0] - 3.0f, (float)xy[1] - 3.0f, 6.0f, 6.0f);
        auto waveCount = audioProcessor.viewPattern->getWaveCount(seg);
        if (waveCount > 0) {
            auto bounds = Rectangle<int>((int)xy[0]-15,(int)xy[1]-25, 30, 20);
            g.setColour(Colour(COLOR_BG).withAlpha(.75f));
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
    auto xpos2 = audioProcessor.xenv2.load();
    auto ypos2 = audioProcessor.yenv2.load();
    bool drawSeek = audioProcessor.drawSeek.load();
    bool drawStereo = audioProcessor.drawStereo.load();

    if (drawSeek) {
        g.setColour(Colour(COLOR_SEEK).withAlpha(0.5f));
        g.drawLine((float)(xpos * winw + winx), (float)winy, (float)(xpos * winw + winx), (float)(winy + winh));
    }
    if (drawStereo) {
        g.setColour(Colour(COLOR_AUDIO));
        g.drawEllipse((float)(xpos2 * winw + winx - 5.f), (float)((1 - ypos2) * winh + winy - 5.f), 10.0f, 10.0f, 1.0f);
    }

    g.setColour(Colour(COLOR_SEEK));
    g.drawEllipse((float)(xpos * winw + winx - 5.f), (float)((1 - ypos) * winh + winy - 5.f), 10.0f, 10.0f, 1.0f);
}

uint64_t View::getHoveredPoint(int x, int y)
{
    auto points = audioProcessor.viewPattern->points;
    for (auto i = 0; i < points.size(); ++i) {
        auto xx = (int)(points[i].x * winw + winx);
        auto yy = (int)(points[i].y * winh + winy);
        if (pointInRect(x, y, xx - HOVER_RADIUS, yy - HOVER_RADIUS, HOVER_RADIUS * 2, HOVER_RADIUS * 2)) {
            return points[i].id;
        }
    }
    return 0;
};

uint64_t View::getHoveredMidpoint(int x, int y)
{
    auto segs = audioProcessor.viewPattern->getSegments();
    for (auto i = 0; i < segs.size(); ++i) {
        auto& seg = segs[i];
        auto xy = getMidpointXY(seg);
        if (!isCollinear(seg) && seg.type != PointType::Hold && pointInRect(x, y, (int)xy[0] - MPOINT_HOVER_RADIUS,
            (int)xy[1] - MPOINT_HOVER_RADIUS, MPOINT_HOVER_RADIUS * 2, MPOINT_HOVER_RADIUS * 2))
        {
            return getPointFromSegmentIndex(i).id;
        }
    }
    return 0;
};

PPoint& View::getPoint(uint64_t id)
{
    for (auto& point : audioProcessor.viewPattern->points) {
        if (point.id == id) {
            return point;
        }
    }
    return dummyPoint;
}

int View::getPointIndex(uint64_t id)
{
    auto sz = audioProcessor.viewPattern->points.size();
    for (int i = 0; i < sz; ++i) {
        if (audioProcessor.viewPattern->points[i].id == id) {
            return i;
        }
    }
    return 0;
}


// Midpoint index is derived from segment nu
// there is an extra segment before the first point
// so the matching pattern point to each midpoint is midpoint - 1
PPoint& View::getPointFromSegmentIndex(int segidx)
{
    auto size = (int)audioProcessor.viewPattern->points.size();
    auto index = segidx == 0 ? size - 1 : segidx - 1;

    if (index >= size)
        index -= size;

    return audioProcessor.viewPattern->points[index];
}

void View::mouseDown(const juce::MouseEvent& e)
{
    if (!isEnabled() || patternID != audioProcessor.viewPattern->versionID)
        return;

    if (audioProcessor.uimode == UIMode::Seq) {
        audioProcessor.sequencer->mouseDown(e);
        return;
    }

    // save snapshon, compare with changes after mouseup
    // if changes were made save this snapshot as undo
    snapshot = audioProcessor.viewPattern->points;
    snapshotIdx = audioProcessor.viewPattern->index;

    Point pos = e.getPosition();
    int x = pos.x;
    int y = pos.y;

    if (audioProcessor.uimode == UIMode::Paint) {
        setMouseCursor(MouseCursor::NoCursor);
        e.source.enableUnboundedMouseMovement(true);
        paintTool.mouseDown(e);
    }
    else if (e.mods.isLeftButtonDown()) {
        if (multiSelect.mouseHover > -1) {
            setMouseCursor(MouseCursor::NoCursor);
            multiSelect.mouseDown(e);
            return;
        }

        selectedPoint = getHoveredPoint(x, y);
        if (!selectedPoint)
            selectedMidpoint = getHoveredMidpoint(x, y);

        if (!selectedPoint && !selectedMidpoint) {
            preSelectionStart = e.getPosition();
            preSelectionEnd = e.getPosition();
        }

        if (selectedPoint || selectedMidpoint) {
            if (selectedPoint) {
                setMouseCursor(MouseCursor::NoCursor);
            }
            if (selectedMidpoint) {
                origTension = getPointFromSegmentIndex(getPointIndex(selectedMidpoint) + 1).tension;
                dragStartY = y;
                e.source.enableUnboundedMouseMovement(true);
                setMouseCursor(MouseCursor::NoCursor);
            }
        }
    }
    else if (e.mods.isRightButtonDown()) {
        if (multiSelect.mouseHover > -1) {
            return;
        }
        rmousePoint = getHoveredPoint(x, y);
        if (rmousePoint) {
            showPointContextMenu(e);
        }
    }
}

void View::mouseUp(const juce::MouseEvent& e)
{
    setMouseCursor(MouseCursor::NormalCursor);
    e.source.enableUnboundedMouseMovement(false);

    if (!isEnabled() || patternID != audioProcessor.viewPattern->versionID)
        return;

    if (audioProcessor.uimode == UIMode::Seq) {
        audioProcessor.sequencer->mouseUp(e);
        return;
    }

    if (e.mods.isRightButtonDown() && !rmousePoint) {
        if (std::abs(e.getDistanceFromDragStartX()) < 4 && std::abs(e.getDistanceFromDragStartY()) < 4)
            showContextMenu(e);
    }
    else if (!e.mods.isRightButtonDown())
    {
        if (audioProcessor.uimode == UIMode::Paint) {
            Desktop::getInstance().setMousePosition(e.getMouseDownScreenPosition());
            paintTool.mouseUp(e);
        }
        else if (selectedPoint) { // finished dragging point
            // ----
        }
        else if (selectedMidpoint) { // finished dragging midpoint, place cursor at midpoint
            int x = e.getMouseDownX();
            int y = (int)(audioProcessor.viewPattern->get_y_at((x - winx) / (double)winw) * winh + winy);
            Desktop::getInstance().setMousePosition(juce::Point<int>(x + getScreenPosition().x, y + getScreenPosition().y));
        }
        else if (preSelectionStart.x > -1 && (std::abs(preSelectionStart.x - preSelectionEnd.x) > 4 ||
                 std::abs(preSelectionStart.y - preSelectionEnd.y) > 4))
        {
            // finished creating preselection
            multiSelect.makeSelection(e, preSelectionStart, preSelectionEnd);
        }
        else if (multiSelect.mouseHover > -1) {
            multiSelect.mouseUp(e);
        }
        else if (!multiSelect.selectionPoints.empty()) { // finished dragging selection
            multiSelect.clearSelection();
        }
        else if (!hoverPoint && !hoverMidpoint && e.mods.isAltDown()) {
            insertNewPoint(e);
        }
    }
    // if there were changes on mouseup, create undo point from previous snapshot
    if (snapshotIdx == audioProcessor.viewPattern->index) {
        audioProcessor.createUndoPointFromSnapshot(snapshot);
    }

    preSelectionStart = Point<int>(-1,-1);
    selectedMidpoint = 0;
    selectedPoint = 0;
    rmousePoint = 0;
}

void View::insertNewPoint(const MouseEvent& e)
{
    double px = e.getPosition().x;
    double py = e.getPosition().y;
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
        audioProcessor.viewPattern->insertPoint(px, py, 0, audioProcessor.pointMode);
        audioProcessor.viewPattern->sortPointsSafe(); // keep things consistent, avoids reorders later
    }
    audioProcessor.viewPattern->buildSegments();
}

void View::mouseMove(const juce::MouseEvent& e)
{
    hoverPoint = 0;
    hoverMidpoint = 0;
    multiSelect.mouseHover = -1;

    if (!isEnabled() || patternID != audioProcessor.viewPattern->versionID)
        return;

    if (audioProcessor.uimode == UIMode::Seq) {
        audioProcessor.sequencer->mouseMove(e);
        return;
    }

    if (audioProcessor.uimode == UIMode::Paint) {
        paintTool.mouseMove(e);
        return;
    }

    auto pos = e.getPosition();

    // if currently dragging a point ignore mouse over events
    if (selectedPoint || selectedMidpoint) {
        return;
    }

    // multi selection mouse over
    multiSelect.mouseMove(e);
    if (multiSelect.mouseHover > -1) {
        return;
    }

    int x = pos.x;
    int y = pos.y;
    hoverPoint = getHoveredPoint(x , y);
    if (!hoverPoint)
        hoverMidpoint = getHoveredMidpoint(x, y);
}

void View::mouseDrag(const juce::MouseEvent& e)
{
    if (!isEnabled() || patternID != audioProcessor.viewPattern->versionID)
        return;

    if (audioProcessor.uimode == UIMode::Seq) {
        audioProcessor.sequencer->mouseDrag(e);
        return;
    }

    if (audioProcessor.uimode == UIMode::Paint) {
        paintTool.mouseDrag(e);
        return;
    }

    if (multiSelect.mouseHover > -1 && e.mods.isRightButtonDown()) {
        return;
    }

    if (multiSelect.mouseHover > -1 && e.mods.isLeftButtonDown()) {
        multiSelect.mouseDrag(e);
        return;
    }

    if (e.mods.isRightButtonDown()) {
        return;
    }

    Point pos = e.getPosition();
    int x = pos.x;
    int y = pos.y;

    if (selectedPoint) {
        auto& points = audioProcessor.viewPattern->points;
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

        auto& point = getPoint(selectedPoint);
        auto idx = getPointIndex(selectedPoint);
        point.y = yy;
        point.x = xx;
        if (point.x > 1) point.x = 1;
        if (point.x < 0) point.x = 0;
        if (idx < points.size() - 1) {
            auto nextidx = idx + 1;
            auto& next = points[nextidx];
            if (point.x >= next.x && point.x - next.x < 20. / winw)
                point.x = next.x - 1e-8;
        }
        if (idx > 0) {
            auto previdx = idx - 1;
            auto& prev = points[previdx];
            if (point.x <= prev.x && prev.x - point.x < 20. / winw)
                point.x = prev.x + 1e-8;
        }
        audioProcessor.viewPattern->sortPointsSafe();
        audioProcessor.viewPattern->buildSegments();
    }

    else if (selectedMidpoint) {
        int distance = y - dragStartY;
        auto mididx = getPointIndex(selectedMidpoint);
        auto& mpoint = getPointFromSegmentIndex(mididx + 1);
        auto& next = getPointFromSegmentIndex(mididx + 2);
        if (mpoint.y < next.y) distance *= -1;
        float tension = (float)origTension + float(distance) / 500.f;
        if (tension > 1) tension = 1;
        if (tension < -1) tension = -1;
        mpoint.tension = tension;
        audioProcessor.viewPattern->buildSegments();
    }

    else if (preSelectionStart.x > -1) {
        preSelectionEnd = e.getPosition();
    }
}

void View::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!isEnabled() || patternID != audioProcessor.viewPattern->versionID)
        return;

    if (audioProcessor.uimode == UIMode::Paint) {
        return;
    }

    if (audioProcessor.uimode == UIMode::Seq) {
        audioProcessor.sequencer->mouseDrag(e);
        return;
    }

    if (e.mods.isRightButtonDown()) {
        return;
    }

    if (multiSelect.mouseHover > -1) {
        multiSelect.clearSelection();
        return;
    }

    int x = e.getPosition().x;
    int y = e.getPosition().y;
    uint64_t pt = getHoveredPoint((int)x, (int)y);
    uint64_t mid = getHoveredMidpoint((int)x, (int)y);
    snapshot = audioProcessor.viewPattern->points;

    if (pt) {
        audioProcessor.viewPattern->removePoint(getPointIndex(pt));
        hoverPoint = 0;
        hoverMidpoint = 0;
    }
    else if (!pt && mid) {
        getPointFromSegmentIndex(getPointIndex(mid) + 1).tension = 0;
    }
    else if (!pt && !mid) {
        insertNewPoint(e);
    }

    if (snapshotIdx == audioProcessor.viewPattern->index) {
        audioProcessor.createUndoPointFromSnapshot(snapshot);
    }
    audioProcessor.viewPattern->buildSegments();
}

void View::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (!isEnabled() || patternID != audioProcessor.viewPattern->versionID)
        return;

    if (audioProcessor.uimode == UIMode::Seq && (event.mods.isShiftDown() || event.mods.isAltDown())) {
        int grid = (int)audioProcessor.params.getRawParameterValue("seqstep")->load();
        auto param = audioProcessor.params.getParameter("seqstep");
        int newgrid = grid + (wheel.deltaY > 0.f ? -1 : 1);
        if (audioProcessor.linkSeqToGrid) {
            audioProcessor.linkSeqToGrid = false;
        }
        // constrain grid size to stay on straights or tripplets
        if (!(grid == 4 && newgrid == 5) && !(grid == 5 && newgrid == 4)) {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1((float)newgrid));
            param->endChangeGesture();
        }
        return;
    }

    int grid = (int)audioProcessor.params.getRawParameterValue("grid")->load();
    auto param = audioProcessor.params.getParameter("grid");
    int newgrid = grid + (wheel.deltaY > 0.f ? -1 : 1);
    // constrain grid size to stay on straights or tripplets
    if (!(grid == 4 && newgrid == 5) && !(grid == 5 && newgrid == 4)) {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1((float)newgrid));
        param->endChangeGesture();
    }
}

bool View::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled() || patternID != audioProcessor.viewPattern->versionID)
        return false;

    if (audioProcessor.uimode == UIMode::Seq) {
        return false;
    }

    // remove selected points
    if (key == KeyPress::deleteKey && !multiSelect.selectionPoints.empty()) {
        audioProcessor.createUndoPoint();
        multiSelect.deleteSelectedPoints();
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
    hoverPoint = 0;
    selectedPoint = 0;
    selectedMidpoint = 0;
    hoverMidpoint = 0;
}

void View::showPointContextMenu(const juce::MouseEvent& event)
{
    (void)event;
    auto point = getPointIndex(rmousePoint);
    int type = audioProcessor.viewPattern->points[point].type;
    PopupMenu menu;
    menu.addItem(1, "Hold", true, type == 0);
    menu.addItem(2, "Curve", true, type == 1);
    menu.addItem(3, "S-Curve", true, type == 2);
    menu.addItem(4, "Pulse", true, type == 3);
    menu.addItem(5, "Wave", true, type == 4);
    menu.addItem(6, "Triangle", true, type == 5);
    menu.addItem(7, "Stairs", true, type == 6);
    menu.addItem(8, "Smooth stairs", true, type == 7);
    menu.addItem(9, "Half Sine", true, type == 8);
    menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this).withMousePosition(),[this, point](int result) {
        int type = audioProcessor.viewPattern->points[point].type;
        if (result > 0 && type != result - 1) {
            audioProcessor.createUndoPoint();
            audioProcessor.viewPattern->points[point].type = result - 1;
            audioProcessor.viewPattern->buildSegments();
        }
    });
}

void View::showContextMenu(const juce::MouseEvent& event)
{
    (void)event;
    PopupMenu menu;
    menu.addItem(1, "Select all");
    menu.addItem(2, "Deselect");
    menu.addSeparator();
    menu.addItem(5, "Copy");
    menu.addItem(6, "Paste");
    menu.addItem(3, "Clear");
    if (!multiSelect.selectionPoints.empty()) {
        menu.addItem(4, "Delete points");
    }

    menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this).withMousePosition(),[this](int result) {
        if (result == 0) return;
        else if (result == 1) multiSelect.selectAll();
        else if (result == 2) multiSelect.clearSelection();
        else if (result == 3) {
            auto snapshot = audioProcessor.viewPattern->points;
            audioProcessor.viewPattern->clear();
            audioProcessor.viewPattern->buildSegments();
            audioProcessor.createUndoPointFromSnapshot(snapshot);
        }
        else if (result == 4 && !multiSelect.selectionPoints.empty()) {
            audioProcessor.createUndoPoint();
            multiSelect.deleteSelectedPoints();
        }
        else if (result == 5) {
            audioProcessor.viewPattern->copy();
        }
        else if (result == 6) {
            auto snapshot = audioProcessor.viewPattern->points;
            audioProcessor.viewPattern->paste();
            audioProcessor.viewPattern->buildSegments();
            audioProcessor.createUndoPointFromSnapshot(snapshot);
        }
    });
}

// ==================================================

bool View::isSnapping(const MouseEvent& e) {
  bool snap = audioProcessor.params.getRawParameterValue("snap")->load() == 1.0f;
  return (snap && !e.mods.isShiftDown()) || (!snap && e.mods.isShiftDown());
}

bool View::isCollinear(Segment seg)
{
  return std::fabs(seg.x1 - seg.x2) < 0.01 || std::fabs(seg.y1 - seg.y2) < 0.01;
};

bool View::pointInRect(int x, int y, int xx, int yy, int w, int h)
{
  return x >= xx && x <= xx + w && y >= yy && y <= yy + h;
};
