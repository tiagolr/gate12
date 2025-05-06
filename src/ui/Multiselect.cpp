/*
  ==============================================================================

    Multiselect.cpp
    Author:  tiagolr

  ==============================================================================
*/
#include "Multiselect.h"
#include "../Globals.h"
#include "../PluginProcessor.h"

void Multiselect::drawBackground(Graphics& g)
{
    if (selectionPoints.size()) {
        g.setColour(Colour(globals::COLOR_SELECTION).withAlpha(0.25f));
        g.fillRect(selectionArea.expanded(PADDING));
    }
}

void Multiselect::draw(Graphics& g)
{
    g.setColour(Colour(globals::COLOR_SELECTION));
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        auto xx = p.x * winw + winx;
        auto yy = p.y * winh + winy;
        g.fillEllipse((float)(xx - 2.0), (float)(yy - 2.0), 4.0f, 4.0f);
    }

    if (selectionPoints.size() > 0) {
        g.setColour(Colour(globals::COLOR_SELECTION));
        g.drawRect(selectionArea.expanded(PADDING));

        if (selectionPoints.size() > 1) {
            drawHandles(g);
        }
    }
}

void Multiselect::drawHandles(Graphics& g)
{
    auto area = selectionArea.expanded(PADDING);
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
    g.setColour(Colour(globals::COLOR_SELECTION));
    g.fillRect(tlRect);g.fillRect(trRect);g.fillRect(blRect);g.fillRect(brRect);
    g.fillRect(mlRect);g.fillRect(mrRect);g.fillRect(tmRect);g.fillRect(bmRect);
    g.setColour(Colours::white);
    if (mouseHover == 1) g.fillRect(tlRect);
    if (mouseHover == 2) g.fillRect(tmRect);
    if (mouseHover == 3) g.fillRect(trRect);
    if (mouseHover == 4) g.fillRect(mlRect);
    if (mouseHover == 5) g.fillRect(mrRect);
    if (mouseHover == 6) g.fillRect(blRect);
    if (mouseHover == 7) g.fillRect(bmRect);
    if (mouseHover == 8) g.fillRect(brRect);
}

void Multiselect::mouseDown(const MouseEvent& e)
{
    (void)e;
    selectionAreaStart = selectionArea;
}

void Multiselect::mouseMove(const MouseEvent& e)
{
    auto pos = e.getPosition();
    mouseHover = 0;
    if (selectionPoints.size() > 1) {
        Point tl = selectionArea.expanded(PADDING).getTopLeft(); // top left
        Point tr = selectionArea.expanded(PADDING).getTopRight();
        Point bl = selectionArea.expanded(PADDING).getBottomLeft();
        Point br = selectionArea.expanded(PADDING).getBottomRight();
        Point ml = tl.withY((tl.getY() + bl.getY()) / 2); // middle left
        Point mr = ml.withX(br.getX()); // middle right
        Point tm = tl.withX((tl.getX() + tr.getX()) / 2);// top middle
        Point bm = tm.withY(br.getY());
        if (Rectangle<int>(tl.getX(), tl.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = 1; // mouse over top left drag handle
        if (Rectangle<int>(tm.getX(), tm.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = 2;
        if (Rectangle<int>(tr.getX(), tr.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = 3;
        if (Rectangle<int>(ml.getX(), ml.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = 4;
        if (Rectangle<int>(mr.getX(), mr.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = 5;
        if (Rectangle<int>(bl.getX(), bl.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = 6;
        if (Rectangle<int>(bm.getX(), bm.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = 7;
        if (Rectangle<int>(br.getX(), br.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = 8;
    }
}

void Multiselect::setViewBounds(int _x, int _y, int _w, int _h)
{
    winx = _x;
    winy = _y;
    winw = _w;
    winh = _h;
}

void Multiselect::createSelection(const MouseEvent& e, Point<int>selectionStart, Point<int>selectionEnd)
{
    selectionArea = Rectangle<int>();

    if (!e.mods.isShiftDown() && !e.mods.isCtrlDown()) {
        selectionPoints.clear();
    }

    Rectangle<int> selArea = Rectangle<int>(
        std::min(selectionStart.x, selectionEnd.x),
        std::min(selectionStart.y, selectionEnd.y),
        std::abs(selectionStart.x - selectionEnd.x),
        std::abs(selectionStart.y - selectionEnd.y)
    );


    auto points = audioProcessor.pattern->points;
    for (size_t i = 0; i < points.size(); ++i) {
        auto& p = points[i];
        auto id = p.id;
        int x = (int)(p.x * winw + winx);
        int y = (int)(p.y * winh + winy);

        if (selArea.contains(x, y)) {
            // if ctrl is down remove point from selection
            if (e.mods.isCtrlDown()) {
                selectionPoints.erase(
                    std::remove_if(
                        selectionPoints.begin(),
                        selectionPoints.end(),
                        [id](const SelPoint& p) { return p.id == id; }
                    ),
                    selectionPoints.end()
                );
            }
            // if point is not on selection, add it
            else if (!std::any_of(selectionPoints.begin(), selectionPoints.end(),
                [id](const SelPoint& p) { return p.id == id; })) {
                selectionPoints.push_back({ p.id, p.x, p.y, 0.0, 0.0 });
            }
        }
    }

    if (selectionPoints.size() == 0) {
        return;
    }

    recalcSelectionArea();
}

void Multiselect::recalcSelectionArea()
{
    // the pattern may have changed, first update the selected points
    std::vector<SelPoint> selPoints;
    std::vector<PPoint> patPoints = audioProcessor.pattern->points;
    for (auto i = patPoints.begin(); i < patPoints.end(); ++i) {
        for (auto j = selectionPoints.begin(); j < selectionPoints.end(); ++j) {
            if (i->id == j->id) {
                selPoints.push_back({ i->id, i->x, i->y, 0.0, 0.0 });
            }
        }
    }
    selectionPoints = selPoints;

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

    // calculate points positions relative to area
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        double x = p.x * winw + winx;
        double y = p.y * winh + winy;
        p.areax = std::max(0.0, std::min(1.0, (x - selectionArea.getX()) / (double)selectionArea.getWidth()));
        p.areay = std::max(0.0, std::min(1.0, (y - selectionArea.getY()) / (double)selectionArea.getHeight()));
    }
}

void Multiselect::clearSelection()
{
    selectionArea = Rectangle<int>();
    selectionPoints.clear();
    mouseHover = -1;
}

bool Multiselect::contains(Point<int> p)
{
    return selectionArea.expanded(PADDING + 3).contains(p);
}

void Multiselect::dragSelection(const MouseEvent& e)
{
    auto mouse = e.getPosition();
    auto mouseDown = e.getMouseDownPosition();

    selectionArea = selectionAreaStart.expanded(0,0);
    int left = selectionArea.getX();
    int right = selectionArea.getRight();
    int top = selectionArea.getY();
    int bottom = selectionArea.getBottom();

    int distl = 0; // distance left to grid used for snapping
    int distr = 0; // distance right
    int distt = 0;
    int distb = 0;

    if (isSnapping(e)) {
        double grid = (double)audioProcessor.getCurrentGrid();
        double gridx = double(winw) / grid;
        double gridy = double(winh) / grid;
        mouse.x = (int)(std::round((mouse.x - winx) / gridx) * gridx + winx);
        mouse.y = (int)(std::round((mouse.y - winy) / gridy) * gridy + winy);
        mouseDown.x = (int)(std::round((mouseDown.x - winx) / gridx) * gridx + winx);
        mouseDown.y = (int)(std::round((mouseDown.y - winy) / gridy) * gridy + winy);
        distl = (int)(std::round((left - winx) / gridx) * gridx + winx) - left;
        distr = (int)(std::round((right - winx) / gridx) * gridx + winx) - right;
        distt = (int)(std::round((top - winy) / gridy) * gridy + winy) - top;
        distb = (int)(std::round((bottom - winy) / gridy) * gridy + winy) - bottom;
    }

    int dx = mouse.x - mouseDown.x;
    int dy = mouse.y - mouseDown.y;

    if (mouseHover == 0) { // area drag
        left += dx + distl;
        right += dx + distr;
        top += dy + distt;
        bottom += dy + distb;
    }
    else if (mouseHover == 1) { // top left corner
        left += dx + distl;
        top += dy + distt;
        right = selectionArea.getRight() - (e.mods.isShiftDown() ? dx + distl : 0);
        bottom = selectionArea.getBottom() - (e.mods.isShiftDown() ? dy + distt : 0);
    }
    else if (mouseHover == 2) { // top middle corner
        top += dy + distt;
        bottom = selectionArea.getBottom() - (e.mods.isShiftDown() ? dy + distt : 0);
    }
    else if (mouseHover == 3) { // top right
        right += dx;
        top += dy;
        left = selectionArea.getX() - (e.mods.isShiftDown() ? dx : 0);
        bottom = selectionArea.getBottom() - (e.mods.isShiftDown() ? dy : 0);
    }
    else if (mouseHover == 4) { // mid left
        left += dx + distl;
        right = selectionArea.getRight() - (e.mods.isShiftDown() ? dx + distl : 0);
    }
    else if (mouseHover == 5) { // mid right
        right += dx + distr;
        left = selectionArea.getX() - (e.mods.isShiftDown() ? dx + distr : 0);
    }
    else if (mouseHover == 6) { // bottom left
        left += dx + distl;
        bottom += dy + distb;
        right = selectionArea.getRight() - (e.mods.isShiftDown() ? dx + distl : 0);
        top = selectionArea.getY() - (e.mods.isShiftDown() ? dy + distb : 0);
    }
    else if (mouseHover == 7) { // bottom mid
        bottom += dy + distb;
        top = selectionArea.getY() - (e.mods.isShiftDown() ? dy + distb : 0);
    }
    else if (mouseHover == 8) { // bottom right
        right += dx + distr;
        bottom += dy + distb;
        left = selectionArea.getX() - (e.mods.isShiftDown() ? dx + distr : 0);
        top = selectionArea.getY() - (e.mods.isShiftDown() ? dy + distb : 0);
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

// updates points position to match the selection area after a scaling or translation
// points have a position relative to area: xarea, yarea normalized from 0 to 1
// xarea and yarea are used calculate the new points position on the view
void Multiselect::updatePointsToSelection(bool invertx, bool inverty)
{
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];

        double areax = invertx ? 1.0 - p.areax : p.areax;
        double areay = inverty ? 1.0 - p.areay : p.areay;
        double absx = selectionArea.getX() + (areax * selectionArea.getWidth());
        double absy = selectionArea.getY() + (areay * selectionArea.getHeight());
        double newx = (absx - winx) / (double)winw;
        double newy = (absy - winy) / (double)winh;

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

    audioProcessor.pattern->sortPoints();
    audioProcessor.pattern->buildSegments();
}

void Multiselect::deleteSelectedPoints()
{
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        auto& points = audioProcessor.pattern->points;
        for (size_t j = 0; j < points.size(); ++j) {
            if (points[j].id == p.id) {
                audioProcessor.pattern->removePoint(static_cast<int>(j));
                break;
            }
        }
    }
    clearSelection();
    audioProcessor.pattern->buildSegments();
}

bool Multiselect::isSnapping(const MouseEvent& e) {
    bool snap = audioProcessor.params.getRawParameterValue("snap")->load() == 1.0f;
    return (snap && !e.mods.isCtrlDown()) || (!snap && e.mods.isCtrlDown());
}