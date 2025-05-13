#include "PaintTool.h"
#include "../PluginProcessor.h"

PaintTool::PaintTool(GATE12AudioProcessor& p) : audioProcessor(p) 
{
    pat = new Pattern(-1);
}

void PaintTool::setViewBounds(int _x, int _y, int _w, int _h)
{
    winx = _x;
    winy = _y;
    winw = _w;
    winh = _h;
}

void PaintTool::draw(Graphics& g)
{
    auto bounds = getBounds();
    auto paintPat = audioProcessor.getPaintPatern(audioProcessor.paintTool);
    pat->points = paintPat->points;
    if (invertx) pat->reverse();
    if (inverty) pat->invert();
    pat->setTension(paintPat->tensionMult.load(), paintPat->tensionAtk.load(), paintPat->tensionRel.load(), audioProcessor.dualTension);
    pat->buildSegments();

    double x = bounds.getX();
    double y = bounds.getY();
    double w = std::max(1.0, bounds.getWidth());
    double h = bounds.getHeight();

    double minx = pat->points.empty() ? 0.0 : pat->points.front().x;
    double maxx = pat->points.empty() ? 0.0 : pat->points.back().x;

    Path path;
    double startX = x + minx * w;
    double startY = y + pat->get_y_at(minx) * h;
    path.startNewSubPath((float)startX, (float)startY);

    for (int i = 0; i < (int)w + 1; ++i) {
        double px = i / w;
        if (px < minx || px > maxx) {
            continue;
        }
        double py = pat->get_y_at(px);
        py = py * h + y;
        path.lineTo((float)(i + x), (float)py);
    }

    g.setColour(Colours::white.withAlpha(0.75f));
    g.strokePath(path, PathStrokeType(1.f));

    float y0 = (float)pat->get_y_at(minx);
    float y1 = (float)pat->get_y_at(maxx);
    float x0 = (float)(x + minx * w);
    float x1 = (float)(x + maxx * w);
    g.fillEllipse(x0 - POINT_RADIUS,(float)y + (float)h * y0 - (float)POINT_RADIUS, POINT_RADIUS * 2.f, POINT_RADIUS*2.f);
    g.fillEllipse(x1 - POINT_RADIUS,(float)y + (float)h * y1 - (float)POINT_RADIUS, POINT_RADIUS * 2.f, POINT_RADIUS * 2.f);
}

void PaintTool::mouseMove(const MouseEvent& e)
{
	mousePos = e.getPosition();
    snap = isSnapping(e);
}

void PaintTool::mouseDrag(const MouseEvent& e)
{
    dragging = true;
    if (e.mods.isRightButtonDown()) {
        double speed = (e.mods.isShiftDown() ? 40.0 : 4.0) * 100;
        auto diff = e.getPosition() - lmousePos;
        double change = double(diff.getX() - diff.getY()) / speed;
        auto pattern = audioProcessor.getPaintPatern(audioProcessor.paintTool);
        for (auto& point : pattern->points) {
            point.tension = jlimit(-1.0,1.0,point.tension+change);
        }
        pattern->buildSegments();
        lmousePos = e.getPosition();
        return;
    }

    snap = isSnapping(e);
    int dx = e.getDistanceFromDragStartX();
    int dy = e.getDistanceFromDragStartY();
    invertx = startW + dx < 0;
    inverty = startH - dy < 0;
    paintW = std::abs(startW + dx);
    paintH = std::abs(startH - dy);
    if (snap) {
        // apply snapping to bounds to fix the size at snapped values
        auto bounds = getBounds().toNearestInt();
        paintW = bounds.getWidth();
        paintH = bounds.getHeight();
    }
}

void PaintTool::mouseDown(const MouseEvent& e)
{
    mousePos = e.getPosition();
    lmousePos = e.getPosition();
    snap = isSnapping(e);
    startW = invertx ? -paintW : paintW;
    startH = inverty ? -paintH : paintH;
}

void PaintTool::mouseUp(const MouseEvent& e)
{
    dragging = false;
    int dx = std::abs(e.getDistanceFromDragStartX());
    int dy = std::abs(e.getDistanceFromDragStartY());
    if (dx < 5 && dy < 5 && e.mods.isLeftButtonDown())
        apply();
}

void PaintTool::apply()
{
    auto bounds = getBounds();
    double rx = bounds.getX();
    double ry = bounds.getY();
    double rw = bounds.getWidth();
    double rh = bounds.getHeight();

    double x1 = (bounds.getX() - (double)winx) / (double)winw;
    double x2 = (bounds.getRight() - (double)winx) / (double)winw;
    x1 = jlimit(0.0, 1.0, x1);
    x2 = jlimit(0.0, 1.0, x2);

    audioProcessor.viewPattern->removePointsInRange(x1, x2);

    auto points = audioProcessor.getPaintPatern(audioProcessor.paintTool)->points;

    // when the bounds are flat (no width or height) apply only first and last points
    if (rw == 0 && rh == 0 && !points.empty()) {
        points = { points.front() };
    }
    else if ((rw == 0 || rh == 0) && points.size() > 1) {
        points = { points.front(), points.back() };
    }

    pat->points = points;
    if (invertx) pat->reverse();
    if (inverty) pat->invert();
    pat->buildSegments();

    for (auto& point : pat->points) {
        double px = rx + point.x * rw; // map points to rectangle bounds
        double py = ry + point.y * rh;
        px = (px - winx) / winw; // normalize again 
        py = (py - winy) / winh;
        px = jlimit(0.0, 1.0, px);
        py = jlimit(0.0, 1.0, py);
        audioProcessor.viewPattern->insertPoint(px, py, point.tension, point.type);
    }

    audioProcessor.viewPattern->buildSegments();
}

Rectangle<double> PaintTool::getBounds()
{
    double x = mousePos.x - (invertx ? paintW : 0);
    double y = mousePos.y - (inverty ? 0 : paintH);
    double r = x + paintW;
    double b = y + paintH;

    if (x < winx) {
        if (!invertx) r += winx - x;
        x = winx;
    }
    if (r > winx + winw) {
        if (invertx) x += winx + winw - r;
        r = winx + winw;
    }
    if (y < winy) {
        if (inverty) b += winy - y;
        y = winy;
    }
    if (b > winy + winh) {
        if (!inverty) y += winy + winh - b;
        b = winy + winh;
    }

    if (x > r) x = r;
    if (y > b) y = b;

    if (snap) {
        auto snapToGrid = [](double coord, int offset, double gridsize) {
            return offset + std::round((coord - offset) / gridsize) * gridsize;
        };
        double grid = (double)audioProcessor.getCurrentGrid();
        double gridx = winw / grid;
        double gridy = winh / grid;

        double w = snapToGrid(r-x, 0, gridx);
        double h = snapToGrid(b-y, 0, gridy);
        double xx = snapToGrid(invertx ? r : x, winx, gridx);
        double yy = snapToGrid(inverty ? y : b, winy, gridy);

        x = invertx ? xx - w : xx;
        y = inverty ? yy : yy - h;
        x = jlimit((double)winx, (double)winx+winw, x);
        y = jlimit((double)winy, (double)winy+winh, y);
        r = x+w;
        b = y+h;
    }

    x = jlimit((double)winx, (double)winx+winw, x);
    y = jlimit((double)winy, (double)winy+winh, y);
    r = jlimit((double)winx, (double)winx+winw, r);
    b = jlimit((double)winy, (double)winy+winh, b);
    return Rectangle<double>(x,y,r-x,b-y);
}

void PaintTool::resetPatternTension()
{
    auto pattern = audioProcessor.getPaintPatern(audioProcessor.paintTool);
    for (auto& point : pattern->points) {
        point.tension = 0.0;
    }
    pattern->buildSegments();
}

bool PaintTool::isSnapping(const MouseEvent& e) {
    bool snapping = audioProcessor.params.getRawParameterValue("snap")->load() == 1.0f;
    return (snapping && !e.mods.isShiftDown()) || (!snapping && e.mods.isShiftDown());
}