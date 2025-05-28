/*
==============================================================================

Multiselect.cpp
Author:  tiagolr

==============================================================================
*/
#include "Multiselect.h"
#include "../PluginProcessor.h"

void Multiselect::setViewBounds(int _x, int _y, int _w, int _h)
{
    winx = _x;
    winy = _y;
    winw = _w;
    winh = _h;
}

void Multiselect::drawBackground(Graphics& g)
{
    if (selectionPoints.size()) {
        g.setColour(Colour(COLOR_SELECTION).withAlpha(0.25f));
        Quad q = getQuadExpanded((double)MSEL_PADDING);
        juce::Path quadPath;
        quadPath.startNewSubPath((float)q[0].x, (float)q[0].y);
        quadPath.lineTo((float)q[1].x, (float)q[1].y);
        quadPath.lineTo((float)q[3].x, (float)q[3].y);
        quadPath.lineTo((float)q[2].x, (float)q[2].y);
        quadPath.closeSubPath();
        g.fillPath(quadPath);
    }
}

void Multiselect::draw(Graphics& g)
{
    g.setColour(Colour(COLOR_SELECTION));
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        auto xx = p.x * winw + winx;
        auto yy = p.y * winh + winy;
        g.fillEllipse((float)(xx - 2.0), (float)(yy - 2.0), 4.0f, 4.0f);
    }

    if (!selectionPoints.empty()) {
        g.setColour(Colour(COLOR_SELECTION));
        Quad q = getQuadExpanded((double)MSEL_PADDING);
        juce::Path quadPath;
        quadPath.startNewSubPath((float)q[0].x, (float)q[0].y);
        quadPath.lineTo((float)q[1].x, (float)q[1].y);
        quadPath.lineTo((float)q[3].x, (float)q[3].y);
        quadPath.lineTo((float)q[2].x, (float)q[2].y);
        quadPath.closeSubPath();
        g.strokePath(quadPath, PathStrokeType(1.0f));

        drawHandles(g);
    }
}

void Multiselect::drawHandles(Graphics& g)
{
    if (selectionPoints.size() < 2) 
        return;

    auto q = getQuadExpanded((double)MSEL_PADDING);
    Point tl = q[0].toPoint(); // top left
    Point tr = q[1].toPoint();
    Point bl = q[2].toPoint();
    Point br = q[3].toPoint();
    Point ml = bilinearInterpolate(q, 0, 0.5).toPoint(); // middle left
    Point mr = bilinearInterpolate(q, 1, 0.5).toPoint(); // middle right
    Point tm = bilinearInterpolate(q, 0.5, 0).toPoint();// top middle
    Point bm = bilinearInterpolate(q, 0.5, 1).toPoint();
    auto tlRect = Rectangle<int>(tl.getX(), tl.getY(), 0, 0).expanded(3);
    auto trRect = Rectangle<int>(tr.getX(), tr.getY(), 0, 0).expanded(3);
    auto blRect = Rectangle<int>(bl.getX(), bl.getY(), 0, 0).expanded(3);
    auto brRect = Rectangle<int>(br.getX(), br.getY(), 0, 0).expanded(3);
    auto mlRect = Rectangle<int>(ml.getX(), ml.getY(), 0, 0).expanded(3);
    auto mrRect = Rectangle<int>(mr.getX(), mr.getY(), 0, 0).expanded(3);
    auto tmRect = Rectangle<int>(tm.getX(), tm.getY(), 0, 0).expanded(3);
    auto bmRect = Rectangle<int>(bm.getX(), bm.getY(), 0, 0).expanded(3);

    bool isCollinearX = isCollinear(selectionPoints, true);
    bool isCollinearY = isCollinear(selectionPoints, false);

    g.setColour(Colour(COLOR_SELECTION));
    if (isCollinearX && isCollinearY) {
        // draw nothing
    }
    else if (isCollinearX) {
        g.fillRect(tmRect);g.fillRect(bmRect);
    } 
    else if (isCollinearY) {
        g.fillRect(mlRect);g.fillRect(mrRect);
    }
    else {
        g.fillRect(tlRect);g.fillRect(trRect);g.fillRect(blRect);g.fillRect(brRect);
        g.fillRect(mlRect);g.fillRect(mrRect);g.fillRect(tmRect);g.fillRect(bmRect);
    }

    g.setColour(Colours::white);
    if (isCollinearX && isCollinearY) {
        // draw nothing
    }
    else if (isCollinearX) {
        if (mouseHover == MouseHover::topMid) g.fillRect(tmRect);
        if (mouseHover == MouseHover::bottomMid) g.fillRect(bmRect);
    }
    else if (isCollinearY) {
        if (mouseHover == MouseHover::midLeft) g.fillRect(mlRect);
        if (mouseHover == MouseHover::midRight) g.fillRect(mrRect);
    }
    else {
        if (mouseHover == MouseHover::topLeft) g.fillRect(tlRect);
        if (mouseHover == MouseHover::topMid) g.fillRect(tmRect);
        if (mouseHover == MouseHover::topRight) g.fillRect(trRect);
        if (mouseHover == MouseHover::midLeft) g.fillRect(mlRect);
        if (mouseHover == MouseHover::midRight) g.fillRect(mrRect);
        if (mouseHover == MouseHover::bottomLeft) g.fillRect(blRect);
        if (mouseHover == MouseHover::bottomMid) g.fillRect(bmRect);
        if (mouseHover == MouseHover::bottomRight) g.fillRect(brRect);
    }
}

void Multiselect::mouseDown(const MouseEvent& e)
{
    (void)e;
    selectionQuadStart = getQuadExpanded();
    selectionAreaStart = quadToRect(selectionQuadStart);
    calcRelativeQuadCoords(selectionAreaStart);
}

void Multiselect::mouseUp(const MouseEvent& e)
{
    (void)e;
    // finalize inversions of points by storing updated areax and areay
    if (inverty) {
        for (auto& p : selectionPoints) {
            p.areay = 1.0 - p.areay;
        }
        inverty = false;
    }
    if (invertx) {
        for (auto& p : selectionPoints) {
            p.areax = 1.0 - p.areax;
        }
        invertx = false;
    }
}

void Multiselect::calcRelativeQuadCoords(Rectangle<double> area)
{
    const double x = area.getX();
    const double y = area.getY();
    const double w = area.getWidth();
    const double h = area.getHeight();
    const double invW = (w == 0.0 ? 0.0 : 1.0 / w); // safety against divisions by zero
    const double invH = (h == 0.0 ? 0.0 : 1.0 / h);

    quadrel[0] = Vec2((quad[0].x - x) * invW, (quad[0].y - y) * invH);
    quadrel[1] = Vec2((quad[1].x - x) * invW, (quad[1].y - y) * invH);
    quadrel[2] = Vec2((quad[2].x - x) * invW, (quad[2].y - y) * invH);
    quadrel[3] = Vec2((quad[3].x - x) * invW, (quad[3].y - y) * invH);
}

void Multiselect::applyRelativeQuadCoords(Rectangle<double> area)
{
    const double x = area.getX();
    const double y = area.getY();
    const double w = area.getRight() == winx + winw 
        ? area.getWidth() - 1e-8 // FIX - glitch where area right overlaps the last point at x=1.0
        : area.getWidth(); 
    const double h = area.getHeight();

    Quad rel = quadrel;
    if (invertx) {
        for (auto& p : rel) {
            p.x = 1.0 - p.x;
        }
        std::swap(rel[0], rel[1]); // top-left <> top-right
        std::swap(rel[2], rel[3]); // bottom-left <> bottom-right
    }
    if (inverty) {
        for (auto& p : rel) {
            p.y = 1.0 - p.y;
        }
        std::swap(rel[0], rel[2]); // top-left <> bottom-left
        std::swap(rel[1], rel[3]); // top-right <> bottom-right
    }

    quad[0] = Vec2(x + rel[0].x * w, y + rel[0].y * h);
    quad[1] = Vec2(x + rel[1].x * w, y + rel[1].y * h);
    quad[2] = Vec2(x + rel[2].x * w, y + rel[2].y * h);
    quad[3] = Vec2(x + rel[3].x * w, y + rel[3].y * h);
}

void Multiselect::mouseMove(const MouseEvent& e)
{
    mouseHover = -1;
    auto pos = e.getPosition();

    if (!selectionPoints.empty()) {
        int size = (int)selectionPoints.size();
        Quad q = getQuadExpanded((double)MSEL_PADDING);
        Point tl = q[0].toPoint(); // top left
        Point tr = q[1].toPoint();
        Point bl = q[2].toPoint();
        Point br = q[3].toPoint();
        Point ml = bilinearInterpolate(q, 0, 0.5).toPoint(); // middle left
        Point mr = bilinearInterpolate(q, 1, 0.5).toPoint(); // middle right
        Point tm = bilinearInterpolate(q, 0.5, 0).toPoint();// top middle
        Point bm = bilinearInterpolate(q, 0.5, 1).toPoint();
        bool cx = isCollinear(selectionPoints, true);
        bool cy = isCollinear(selectionPoints, false);
        if (!cx && !cy && size > 1 && Rectangle<int>(tl.getX(), tl.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = MouseHover::topLeft;
        else if (!cy && size > 1 && Rectangle<int>(tm.getX(), tm.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = MouseHover::topMid;
        else if (!cx && !cy && size > 1 && Rectangle<int>(tr.getX(), tr.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = MouseHover::topRight;
        else if (!cx && size > 1 && Rectangle<int>(ml.getX(), ml.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = MouseHover::midLeft;
        else if (!cx && size > 1 && Rectangle<int>(mr.getX(), mr.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = MouseHover::midRight;
        else if (!cx && !cy && size > 1 && Rectangle<int>(bl.getX(), bl.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = MouseHover::bottomLeft;
        else if (!cy && size > 1 && Rectangle<int>(bm.getX(), bm.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = MouseHover::bottomMid;
        else if (!cx && !cy && size > 1 && Rectangle<int>(br.getX(), br.getY(), 0, 0).expanded(3).contains(pos)) mouseHover = MouseHover::bottomRight;
        else if (quadToRect(q).contains(pos.toDouble())) {
            juce::Path quadPath;
            quadPath.startNewSubPath((float)q[0].x, (float)q[0].y);
            quadPath.lineTo((float)q[1].x, (float)q[1].y);
            quadPath.lineTo((float)q[3].x, (float)q[3].y);
            quadPath.lineTo((float)q[2].x, (float)q[2].y);
            quadPath.closeSubPath();

            if (quadPath.contains(pos.toFloat())) {
                mouseHover = MouseHover::area;
            };
        }
    }
}

void Multiselect::makeSelection(const MouseEvent& e, Point<int>selectionStart, Point<int>selectionEnd)
{
    if (!e.mods.isShiftDown() && !e.mods.isCtrlDown()) {
        selectionPoints.clear();
    }

    Rectangle<int> selArea = Rectangle<int>(
        std::min(selectionStart.x, selectionEnd.x),
        std::min(selectionStart.y, selectionEnd.y),
        std::abs(selectionStart.x - selectionEnd.x),
        std::abs(selectionStart.y - selectionEnd.y)
    );


    auto points = audioProcessor.viewPattern->points;
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

    if (selectionPoints.size() > 0) {
        recalcSelectionArea();
    }
}

void Multiselect::recalcSelectionArea()
{
    // the pattern may have changed, first update the selected points
    std::vector<SelPoint> selPoints;
    std::vector<PPoint> patPoints = audioProcessor.viewPattern->points;
    for (auto i = patPoints.begin(); i < patPoints.end(); ++i) {
        for (auto j = selectionPoints.begin(); j < selectionPoints.end(); ++j) {
            if (i->id == j->id) {
                selPoints.push_back({ i->id, i->x, i->y, 0.0, 0.0 });
            }
        }
    }
    selectionPoints = selPoints;

    // calculate selection area based on points positions
    double minx = (double)winx + winw;
    double maxx = (double)-1;
    double miny = (double)winy + winh;
    double maxy = (double)-1;
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        double x = (p.x * winw + winx);
        double y = (p.y * winh + winy);
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
    }

    auto selectionArea = Rectangle<double>(minx, miny, maxx - minx, maxy - miny);
    quad[0] = pointToVec(selectionArea.getTopLeft());
    quad[1] = pointToVec(selectionArea.getTopRight());
    quad[2] = pointToVec(selectionArea.getBottomLeft());
    quad[3] = pointToVec(selectionArea.getBottomRight());

    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        double x = p.x * winw + winx;
        double y = p.y * winh + winy;
        p.areax = std::max(0.0, std::min(1.0, (x - selectionArea.getX()) / selectionArea.getWidth()));
        p.areay = std::max(0.0, std::min(1.0, (y - selectionArea.getY()) / selectionArea.getHeight()));
    }
}

void Multiselect::clearSelection()
{
    quad = { Vec2(0.0, 0.0), Vec2(1.0, 0.0), Vec2(0.0, 1.0), Vec2(1.0, 1.0) };
    selectionPoints.clear();
    mouseHover = -1;
}

void Multiselect::mouseDrag(const MouseEvent& e)
{
    if (e.mods.isAltDown() && mouseHover != MouseHover::topMid && mouseHover != MouseHover::bottomMid) {
        dragQuad(e);
    }
    else {
        dragArea(e);
    }
}

void Multiselect::dragArea(const MouseEvent& e) 
{
    auto mouse = e.getPosition().toDouble();
    auto mouseDown = e.getMouseDownPosition().toDouble();

    auto selectionArea = selectionAreaStart;
    double left = selectionArea.getX();
    double right = selectionArea.getRight();
    double top = selectionArea.getY();
    double bottom = selectionArea.getBottom();

    double distl = 0; // distance left to grid used for snapping
    double distr = 0; // distance right
    double distt = 0;
    double distb = 0;

    if (isSnapping(e)) {
        double grid = (double)audioProcessor.getCurrentGrid();
        double gridx = double(winw) / grid;
        double gridy = double(winh) / grid;
        mouse.x = std::round((mouse.x - winx) / gridx) * gridx + winx;
        mouse.y = std::round((mouse.y - winy) / gridy) * gridy + winy;
        mouseDown.x = std::round((mouseDown.x - winx) / gridx) * gridx + winx;
        mouseDown.y = std::round((mouseDown.y - winy) / gridy) * gridy + winy;
        distl = std::round((left - winx) / gridx) * gridx + winx - left;
        distr = std::round((right - winx) / gridx) * gridx + winx - right;
        distt = std::round((top - winy) / gridy) * gridy + winy - top;
        distb = std::round((bottom - winy) / gridy) * gridy + winy - bottom;

        if (std::fabs(distl) < 1) distl = 0; // dont move points if already very close to the grid, fixes dragging sequencer points after apply
        if (std::fabs(distr) < 1) distr = 0;
        if (std::fabs(distt) < 1) distt = 0; 
        if (std::fabs(distb) < 1) distb = 0;
    }

    double dx = mouse.x - mouseDown.x;
    double dy = mouse.y - mouseDown.y;

    if (mouseHover == MouseHover::area) {
        left += dx + distl;
        right += dx + distr;
        top += dy + distt;
        bottom += dy + distb;
    }
    else if (mouseHover == MouseHover::topLeft) { 
        left += dx + distl;
        top += dy + distt;
        right = selectionArea.getRight() - (e.mods.isCtrlDown() ? dx + distl : 0);
        bottom = selectionArea.getBottom() - (e.mods.isCtrlDown() ? dy + distt : 0);
    }
    else if (mouseHover == MouseHover::topMid) {
        top += dy + distt;
        bottom = selectionArea.getBottom() - (e.mods.isCtrlDown() ? dy + distt : 0);
    }
    else if (mouseHover == MouseHover::topRight) {
        right += dx;
        top += dy;
        left = selectionArea.getX() - (e.mods.isCtrlDown() ? dx : 0);
        bottom = selectionArea.getBottom() - (e.mods.isCtrlDown() ? dy : 0);
    }
    else if (mouseHover == MouseHover::midLeft) {
        left += dx + distl;
        right = selectionArea.getRight() - (e.mods.isCtrlDown() ? dx + distl : 0);
    }
    else if (mouseHover == MouseHover::midRight) {
        right += dx + distr;
        left = selectionArea.getX() - (e.mods.isCtrlDown() ? dx + distr : 0);
    }
    else if (mouseHover == MouseHover::bottomLeft) {
        left += dx + distl;
        bottom += dy + distb;
        right = selectionArea.getRight() - (e.mods.isCtrlDown() ? dx + distl : 0);
        top = selectionArea.getY() - (e.mods.isCtrlDown() ? dy + distb : 0);
    }
    else if (mouseHover == MouseHover::bottomMid) {
        bottom += dy + distb;
        top = selectionArea.getY() - (e.mods.isCtrlDown() ? dy + distb : 0);
    }
    else if (mouseHover == MouseHover::bottomRight) {
        right += dx + distr;
        bottom += dy + distb;
        left = selectionArea.getX() - (e.mods.isCtrlDown() ? dx + distr : 0);
        top = selectionArea.getY() - (e.mods.isCtrlDown() ? dy + distb : 0);
    }

    invertx = false;
    inverty = false;

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
    applyRelativeQuadCoords(selectionArea);
    updatePointsToSelection();
}

void Multiselect::dragQuad(const MouseEvent& e) 
{
    auto mouse = e.getPosition();
    auto mouseDown = e.getMouseDownPosition();
    auto selectionArea = quadToRect(quad);
    auto snap = isSnapping(e);

    if (snap) {
        double grid = (double)audioProcessor.getCurrentGrid();
        double gridy = double(winh) / grid;
        mouse.y = (int)(std::round((mouse.y - winy) / gridy) * gridy + winy);
        mouseDown.y = (int)(std::round((mouseDown.y - winy) / gridy) * gridy + winy);
    }

    double dy = (double)(mouse.y - mouseDown.y);

    if (mouseHover == MouseHover::topLeft) {
        quad[0].y = std::fmin(selectionQuadStart[0].y + dy, selectionArea.getBottom());
    }
    else if (mouseHover == MouseHover::topRight) {
        quad[1].y = std::fmin(selectionQuadStart[1].y + dy, selectionArea.getBottom());
    }
    else if (mouseHover == MouseHover::bottomLeft) {
        quad[2].y = std::fmax(selectionQuadStart[2].y + dy, selectionArea.getY());
    }
    else if (mouseHover == MouseHover::bottomRight) {
        quad[3].y = std::fmax(selectionQuadStart[3].y + dy, selectionArea.getY());
    }
    else if (mouseHover == MouseHover::midLeft) {
        quad[0].y = selectionQuadStart[0].y + dy;
        quad[2].y = selectionQuadStart[2].y + dy;
    }
    else if (mouseHover == MouseHover::midRight) {
        quad[1].y = selectionQuadStart[1].y + dy;
        quad[3].y = selectionQuadStart[3].y + dy;
    }
    quad[0].y = std::fmax((double)winy, std::fmin(quad[0].y, (double)winy+winh));
    quad[1].y = std::fmax((double)winy, std::fmin(quad[1].y, (double)winy+winh));
    quad[2].y = std::fmax((double)winy, std::fmin(quad[2].y, (double)winy+winh));
    quad[3].y = std::fmax((double)winy, std::fmin(quad[3].y, (double)winy+winh));

    updatePointsToSelection();
}

// updates points position to match the selection area after a scaling or translation
// points have a position relative to area: xarea, yarea normalized from 0 to 1
// xarea and yarea are used calculate the new points position on the view
// now uses bilinear interpolation to place the points in a defined quad instead of rectangle area
void Multiselect::updatePointsToSelection()
{
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];

        double areax = invertx ? 1.0 - p.areax : p.areax;
        double areay = inverty ? 1.0 - p.areay : p.areay;
        auto newpos = bilinearInterpolate(quad, areax, areay);
        newpos.x = (newpos.x - winx) / (double)winw; // normalize viewport coords
        newpos.y = (newpos.y - winy) / (double)winh;

        // update selection point
        p.x = newpos.x;
        p.y = newpos.y;

        // update pattern point
        auto& points = audioProcessor.viewPattern->points;
        for (size_t j = 0; j < points.size(); ++j) {
            auto& pp = points[j];
            if (pp.id == p.id) {
                pp.x = newpos.x;
                pp.y = newpos.y;
                break;
            }
        }
    }

    audioProcessor.viewPattern->sortPoints();
    audioProcessor.viewPattern->buildSegments();
}

void Multiselect::deleteSelectedPoints()
{
    for (size_t i = 0; i < selectionPoints.size(); ++i) {
        auto& p = selectionPoints[i];
        auto& points = audioProcessor.viewPattern->points;
        for (size_t j = 0; j < points.size(); ++j) {
            if (points[j].id == p.id) {
                audioProcessor.viewPattern->removePoint(static_cast<int>(j));
                break;
            }
        }
    }
    clearSelection();
    audioProcessor.viewPattern->buildSegments();
}

void Multiselect::selectAll()
{
    selectionPoints.clear();
    for (auto& point : audioProcessor.viewPattern->points) {
        selectionPoints.push_back({ point.id, point.x, point.y, 0.0, 0.0 });
    }
    recalcSelectionArea();
}

Quad Multiselect::getQuadExpanded(double expand)
{
    return {
        quad[0] + Vec2(-expand,-expand),
        quad[1] + Vec2(expand, -expand),
        quad[2] + Vec2(-expand, expand),
        quad[3] + Vec2(expand, expand)
    };
}

bool Multiselect::isSnapping(const MouseEvent& e) {
    bool snap = audioProcessor.params.getRawParameterValue("snap")->load() == 1.0f;
    return (snap && !e.mods.isShiftDown()) || (!snap && e.mods.isShiftDown());
}

Vec2 Multiselect::pointToVec(Point<double> p)
{
    return Vec2(p.getX(), p.getY());
}

Rectangle<double> Multiselect::quadToRect(Quad q)
{
    double minx = double(winx + winw);
    double maxx = double(-1);
    double miny = double(winy + winh);
    double maxy = double(-1);
    for (auto& p : q) {
        auto x = p.x;
        auto y = p.y;
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
    }
    return Rectangle<double>(minx, miny, maxx-minx, maxy-miny);
}

bool Multiselect::isCollinear(const std::vector<SelPoint>& points, bool xaxis)
{
    if (points.size() < 2) return true;

    const double EPSILON = 1e-5;
    double firstCoord = xaxis ? points[0].x : points[0].y;

    for (const auto& p : points) {
        double coord = xaxis ? p.x : p.y;
        if (std::fabs(coord - firstCoord) > EPSILON) 
            return false;
    }

    return true;
}