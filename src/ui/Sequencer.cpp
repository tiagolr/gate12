#include "Sequencer.h"
#include "../PluginProcessor.h"

Sequencer::Sequencer(GATE12AudioProcessor& p) : audioProcessor(p)
{
    tmp = new Pattern(-1);
    pat = new Pattern(-1);
    clear();
    ramp.push_back({ 0, 0.0, 1.0, 0.0, 1 });
    ramp.push_back({ 0, 1e-10, 0.0, 0.0, 1 }); // 1e-10 makes it sort proof
    ramp.push_back({ 0, 1.0, 1.0, 0.0, 1 });
    line.push_back({ 0, 0.0, 0.0, 0.0, 1 });
    line.push_back({ 0, 1.0, 0.0, 0.0, 1 });
    sine.push_back({ 0, 0.25, 1.0, 0.2, 2 });
    sine.push_back({ 0, 0.75, 0.0, 0.2, 2 });
    tri.push_back({ 0, 0.0, 1.0, 0.0, 1 });
    tri.push_back({ 0, 0.5, 0.0, 0.0, 1 });
    tri.push_back({ 0, 1.0, 1.0, 0.0, 1 });
    silence.push_back({ 0, 0.0, 1.0, 0.0, 1 });
    silence.push_back({ 0, 1.0, 1.0, 0.0, 1 });
}

void Sequencer::setViewBounds(int _x, int _y, int _w, int _h)
{
    winx = _x;
    winy = _y;
    winw = _w;
    winh = _h;
}

void Sequencer::drawBackground(Graphics& g)
{
    double x = (lmousepos.x - winx) / (double)winw;
    double y = (lmousepos.y - winy) / (double)winh;
    x = jlimit(0.0, 1.0 - 1e-8, x);
    y = jlimit(0.0, 1.0, y);

    int grid = audioProcessor.getCurrentGrid();
    double gridx = winw / (double)grid;
    int seg = (int)(x * grid);
    int step = audioProcessor.getCurrentSeqStep();
    int segw = winw / step;

    g.setColour(Colours::white.withAlpha(0.1f));
    auto bounds = Rectangle<int>((int)std::round(seg * gridx) + winx, winy, segw, winh);
    if (bounds.getRight() > winx + winw)
        bounds = bounds.withRight(winx + winw);
    g.fillRect(bounds);

    if (editMode == EditMax || editMode == EditMin || editMode == EditNone) {
        auto segBounds = getSegBounds(seg).toNearestInt();
        g.setColour(Colour(COLOR_MIDI));
        double xx1 = segBounds.getX();
        double xx2 = segBounds.getRight();
        double yy = segBounds.getY();
        double yy2 = segBounds.getBottom();
        g.drawLine((float)xx1, (float)yy+0.5f, (float)xx2, (float)yy+0.5f);
        g.drawLine((float)xx1, (float)yy2+0.5f, (float)xx2, (float)yy2+0.5f);
        auto dymax = std::abs(lmousepos.y - segBounds.getY());
        auto dymin = std::abs(lmousepos.y - segBounds.getBottom());
        if (lmousepos.y > segBounds.getBottom() || (dymin < dymax && dymin < 50)) {
            g.fillRect((float)xx1, (float)yy2-1, (float)(xx2-xx1), 3.f);
        }
        else {
            g.fillRect((float)xx1, (float)yy-1, (float)(xx2-xx1), 3.f);
        }
    }
}

void Sequencer::draw(Graphics& g)
{
    // draw hovered cell min and max lines
    if (editMode == EditMax || editMode == EditMin || editMode == EditNone)
        return;

    g.setColour(Colours::black.withAlpha(0.25f));
    g.fillRect(winx,winy,winw,winh);

    if (editMode == EditInvertX) {
        g.setColour(getEditModeColour(EditInvertX).withAlpha(0.2f));
        for (auto& cell : cells) {
            if (cell.invertx)
                g.fillRect((float)winx+(float)(cell.minx*winw), (float)winy, (float)(cell.maxx - cell.minx) * winw, (float)winh);
        }
        return;
    }

    // draw selected edit mode overlay
    g.setColour(getEditModeColour(editMode).withAlpha(0.2f));

    for (auto& cell : cells) {
        double value = editMode == EditTenAtt ? (cell.invertx ? cell.tenrel : cell.tenatt) * -1
            : editMode == EditTenRel ? (cell.invertx ? cell.tenatt : cell.tenrel) * -1
            : editMode == EditTension ? (std::fabs(cell.tenatt) > std::fabs(cell.tenrel) ? cell.tenatt : cell.tenrel) * -1
            : editMode == EditSkew ? cell.skew * -1
            : 0.0;

        auto bounds = Rectangle<float>(
            (float)winx+(float)cell.minx*winw, 
            (float)winy + winh / 2.f, 
            (float)(cell.maxx - cell.minx) * winw, 
            winh / 2.f * std::fabs((float)value)
        );

        if (value > 0.0)
            bounds = bounds.withY(winy+winh/2.f-bounds.getHeight());
        if (value == 0.0)
            bounds = bounds.withHeight(2.0f).withY(winy+winh/2.f-1.f);

        g.fillRect(bounds);
    }
}

Colour Sequencer::getEditModeColour(SeqEditMode mode)
{
    if (mode == EditInvertX) return Colour(COLOR_SEQ_INVX);
    if (mode == EditMax) return Colour(COLOR_SEQ_MAX);
    if (mode == EditMin) return Colour(COLOR_SEQ_MIN);
    if (mode == EditTension) return Colour(COLOR_SEQ_TEN);
    if (mode == EditTenAtt) return Colour(COLOR_SEQ_TENA);
    if (mode == EditTenRel) return Colour(COLOR_SEQ_TENR);
    if (mode == EditSkew) return Colour(COLOR_SEQ_SKEW);
    return Colours::white;
}

void Sequencer::mouseMove(const MouseEvent& e)
{
    lmousepos = e.getPosition();
}

void Sequencer::mouseDrag(const MouseEvent& e)
{
    lmousepos = e.getPosition();
    onMouseSegment(e, true);
}

void Sequencer::mouseDown(const MouseEvent& e)
{
    snapshot = cells;
    onMouseSegment(e, false);
}

void Sequencer::mouseUp(const MouseEvent& e)
{
    (void)e;
    if (editMode == EditMin)
        editMode = EditMax;
    createUndo(snapshot);
}


void Sequencer::mouseDoubleClick(const juce::MouseEvent& e)
{
    (void)e;
}

void Sequencer::onMouseSegment(const MouseEvent& e, bool isDrag) {
    double x = (e.getPosition().x - winx) / (double)winw;
    double y = (e.getPosition().y - winy) / (double)winh;

    x = jlimit(0.0, 1.0 - 1e-8, x);
    y = jlimit(0.0, 1.0, y);

    int grid = audioProcessor.getCurrentGrid();
    int step = audioProcessor.getCurrentSeqStep();
    int seg = (int)(x * grid);
    double x1 = int(x / (1.0/grid)) * (1.0/grid);
    double x2 = std::min(x1 + 1.0/step, 1.0);

    if (isSnapping(e)) {
        auto snapy = grid % 6 == 0 ? 12.0 : 16.0;
        y = std::round(y * snapy) / snapy;
    }

    bool canAddCell = (editMode == EditMax || editMode == EditMin) && editMode != EditNone;
    auto segCells = getCellsInRange(x1, x2, !canAddCell);
    auto segBounds = getSegBounds(seg);

    // toggle editMin if the edit point is closer to min than max
    if (editMode == EditMax && !isDrag && selectedShape != SLine) {
        auto dymax = std::abs(e.getPosition().y - segBounds.getY()); ////
        auto dymin = std::abs(e.getPosition().y - segBounds.getBottom());
        if ((e.getPosition().y > segBounds.getBottom()) || (dymin < dymax && dymin < 50)) {
            editMode = EditMin;
        }
    }
    else if (editMode == EditNone && !isDrag) {
        auto dymax = std::abs(e.getPosition().y - segBounds.getY()); ////
        auto dymin = std::abs(e.getPosition().y - segBounds.getBottom());
        editNoneEditsMax = dymin > dymax || selectedShape == SLine;
    }

    if (e.mods.isRightButtonDown()) {
        for (auto cell : segCells) {
            if (cell->shape == SSilence) continue;
            else if (editMode == EditMin || editMode == EditMax || editMode == EditNone) {
                clearSegment(x1, x2, true);
                build();
            }
            else if (editMode == EditTenAtt) cell->tenatt = 0.0;
            else if (editMode == EditTenRel) cell->tenrel = 0.0;
            else if (editMode == EditTension) cell->tenatt = cell->tenrel = 0.0;
            else if (editMode == EditInvertX) cell->invertx = false;
            else if (editMode == EditSkew) cell->skew = 0.0;
        }
        build();
        return;
    }

    bool isNewCell = false;
    if (canAddCell) {
        clearSegment(x1, x2, false);
        if (getCellIndex(x1, x2) == -1) {
            addCell(x1, x2);
            isNewCell = true;
        }
        segCells = getCellsInRange(x1, x2, false);
        if (isNewCell) {
            segCells[0]->miny = (segBounds.getY() - winy) / (double)winh; //////
            segCells[0]->maxy = (segBounds.getBottom() - winy) / (double)winh;
        }
    }

    if (segCells.empty())
        return;

    if (canAddCell) {
        auto cell = segCells[0]; // there is only one cell on this segment because it has been cleared

        if (selectedShape == SSilence) {
            clearSegment(x1, x2, true);
            build();
            return;
        }

        if (selectedShape == SPTool) {
            cell->shape = selectedShape;
            cell->lshape = selectedShape;
            cell->ptool = audioProcessor.paintTool;
        }
        else {
            // Apply selected shape to clicked or dragged cell
            cell->invertx = selectedShape == SRampUp;
            cell->shape = selectedShape;
            cell->lshape = selectedShape;
        }
    }

    for (auto cell : segCells) {
        if (editMode == EditMin || (editMode == EditNone && !editNoneEditsMax)) {
            cell->maxy = y; // y coordinates are inverted
            if (cell->miny > y)
                cell->miny = y;
        }
        else if (editMode == EditMax || (editMode == EditNone && editNoneEditsMax)) {
            cell->miny = y; // y coordinates are inverted
            if (cell->maxy < y)
                cell->maxy = y;
        }
        else if (editMode == EditInvertX) {
            if (!isDrag && cell == segCells[0]) {
                auto idx = getCellIndexAt(x); // get cell at mouse position
                if (idx > 0)
                    startInvertX = !cells[idx].invertx;
                else
                    startInvertX = !segCells[0]->invertx;
            }
            cell->invertx = startInvertX;
        }
        else if (editMode == EditTension) {
            cell->tenatt = y * 2 - 1;
            cell->tenrel = y * 2 - 1;
        }
        else if (editMode == EditTenAtt) {
            if (cell->invertx)
                cell->tenrel = y * 2 - 1;
            else
                cell->tenatt = y * 2 - 1;
        }
        else if (editMode == EditTenRel) {
            if (cell->invertx)
                cell->tenatt = y * 2 - 1;
            else
                cell->tenrel = y * 2 - 1;
        }
        else if (editMode == EditSkew) {
            cell->skew = y * 2 - 1;
        }
    }

    build();
}

int Sequencer::getCellIndex(double minx, double maxx)
{
    double eps = 1e-10;
    for (int i = 0; i < cells.size(); ++i) {
        auto& cell = cells[i];
        if (cell.minx + eps >= minx && cell.maxx - eps <= maxx)
            return i;
    }

    return -1;
}

int Sequencer::getCellIndexAt(double x)
{
    for (int i = 0; i < cells.size(); ++i) {
        auto& cell = cells[i];
        if (cell.minx <= x && cell.maxx >= x)
            return i;
    }

    return -1;
}



////
Rectangle<double> Sequencer::getSegBounds(int segidx)
{
    int grid = audioProcessor.getCurrentGrid();
    int step = audioProcessor.getCurrentSeqStep();
    int segw = winw / step;

    double minx = (double)segidx/(grid);
    double maxx = std::min((double)segidx/grid + 1.0/step, 1.0);
    double miny = 0.0;
    double maxy = 1.0;

    for (auto& pt : pat->points) {
        if (pt.x >= minx && pt.x <= maxx) {
            if (pt.y <= maxy) maxy = pt.y;
            if (pt.y >= miny) miny = pt.y;
        }
    }

    auto bounds = Rectangle<double>(minx * winw + winx, std::min(maxy, miny) * winh + winy, (double)segw, std::fabs(miny - maxy) * winh); 
    bounds.setRight(std::min(bounds.getRight(), (double)(winw + winx)));
    return bounds;
}

int Sequencer::addCell(double minx, double maxx)
{
    double eps = 1e-10;
    cells.erase(std::remove_if(cells.begin(), cells.end(), [eps, minx, maxx](const Cell& cell) {
        bool overlaps = cell.minx < maxx - eps && cell.maxx > minx + eps;
        return overlaps;
        }), cells.end());

    Cell cell = { selectedShape, selectedShape, audioProcessor.paintTool, false, minx, maxx, 0.0, 1.0, 0.0, 0.0, 0.0 };

    auto it = std::lower_bound(cells.begin(), cells.end(), cell.minx,
        [](const Cell& cell, double x) {
            return cell.minx < x;
        });

    auto idx = (int)std::distance(cells.begin(), it);
    cells.insert(it, cell);
    return idx;
}

/*
* Returns cells that start between minx and maxx
*/
std::vector<Cell*> Sequencer::getCellsInRange(double minx, double maxx, bool getOverlapped) {
    std::vector<Cell*> result;
    auto eps = 1e-10;

    for (auto& cell : cells) {
        auto overlaps = cell.maxx - eps >= minx && cell.minx + eps <= maxx;
        auto contains = cell.minx + eps >= minx && cell.maxx - eps <= maxx;

        if ((!getOverlapped && contains) || (getOverlapped && overlaps)) {
            result.push_back(&cell);
        }
    }

    return result;
}

/*
* clears a segment so new cells can be added
*/
void Sequencer::clearSegment(double minx, double maxx, bool removeAll)
{
    double eps = 1e-10;
    for (auto& cell : cells) {
        bool exactMatch = std::abs(cell.minx - minx) < eps && std::abs(cell.maxx - maxx) < eps;
        bool overlaps = cell.minx < maxx - eps && cell.maxx > minx + eps;
        bool contains = cell.minx + eps > minx && cell.maxx - eps < maxx;
        if (overlaps && !exactMatch && !contains && cell.minx >= 0.0 && cell.maxx <= 1.0) {
            double cellCenter = 0.5 * (cell.minx + cell.maxx);
            double rangeCenter = 0.5 * (minx + maxx);
            if (cellCenter < rangeCenter) 
                cell.maxx = minx;
            else
                cell.minx = maxx;
        }
    }
    // remove cells contained in the segment unless is an exact match
    cells.erase(std::remove_if(cells.begin(), cells.end(), [eps, minx, maxx, removeAll](const Cell& cell) {
        bool exactMatch = std::abs(cell.minx - minx) < eps && std::abs(cell.maxx - maxx) < eps;
        bool overlaps = cell.minx < maxx - eps && cell.maxx > minx + eps;
        bool contains = cell.minx + eps > minx && cell.maxx - eps < maxx;
        //bool contains = cell.minx + eps > minx && cell.maxx - eps < maxx;
        return (overlaps || contains) && (!exactMatch || removeAll);
        }), cells.end());
}

void Sequencer::open()
{
    isOpen = true;
    backup = audioProcessor.pattern->points;
    patternIdx = audioProcessor.pattern->index;
    build();
    audioProcessor.pattern->points = pat->points;
    audioProcessor.pattern->buildSegments();
}

void Sequencer::close()
{
    isOpen = false;
    if (audioProcessor.pattern->index != patternIdx)
        return;

    patternIdx = -1;
    audioProcessor.pattern->points = backup;
    audioProcessor.pattern->buildSegments();
}

void Sequencer::clear()
{
    cells.clear();
}

void Sequencer::clear(SeqEditMode mode)
{
    auto snap = cells;
    for (auto& cell : cells) {
        if (mode == EditMax) cell.miny = 0.0;
        else if (mode == EditMin) cell.maxy = 1.0;
        else if (mode == EditInvertX) cell.invertx = cell.shape == CellShape::SRampUp;
        else if (mode == EditTenAtt) {
            if (cell.invertx)
                cell.tenrel = 0.0;
            else
                cell.tenatt = 0.0;
        }
        else if (mode == EditTenRel) {
            if (cell.invertx)
                cell.tenatt = 0.0;
            else
                cell.tenrel = 0.0;
        }
        else if (mode == EditTension) cell.tenatt = cell.tenrel = 0.0;
    }
    createUndo(snap);
    build();
}

void Sequencer::apply()
{
    audioProcessor.createUndoPointFromSnapshot(backup);
    backup = pat->points;
}

void Sequencer::build()
{
    pat->clear();

    for (auto& cell : cells) {
        auto seg = buildSeg(cell);
        for (auto& pt : seg) {
            auto x = pt.x;
            if (x < 0.0) x += 1.0;
            if (x > 1.0) x -= 1.0;
            pat->insertPoint(x, pt.y, pt.tension, pt.type, false);
        }
    }

    pat->sortPoints();
    //pat->points = removeCollinearPoints(pat->points);
    auto& pattern = audioProcessor.pattern;
    pattern->points = pat->points;
    pattern->buildSegments();

}

/*
* Removes sequential points with the same x coordinate leaving just the first and last
* Useful for removing extra points in successive patterns like ramps
*/
std::vector<PPoint> Sequencer::removeCollinearPoints(std::vector<PPoint>& points)
{
    if (points.size() < 2) return points;
    std::vector<PPoint> result;

    result.push_back(points[0]);
    for (size_t i = 1; i + 1 < points.size(); ++i) {
        if (points[i].x != points[i - 1].x || points[i].x != points[i + 1].x) {
            result.push_back(points[i]);
        }
    }
    result.push_back(points.back());
    return result;
}

std::vector<PPoint> Sequencer::buildSeg(Cell cell)
{
    std::vector<PPoint> pts;
    double minx = cell.minx + 1e-8;
    double maxx = cell.maxx - 1e-8;
    double miny = cell.miny;
    double w = maxx - minx;
    double h = cell.maxy-cell.miny;

    auto paint = cell.shape == SRampUp ? ramp
        : cell.shape == SRampDn ? ramp
        : cell.shape == STri ? tri
        : cell.shape == SLine ? line
        : cell.shape == SSine ? sine
        : cell.shape == SPTool ? audioProcessor.getPaintPatern(cell.ptool)->points
        : silence;

    if (cell.shape == SSilence) {
        miny = 1;
        h = 0;
    }

    if (h == 0 && paint.size() > 1) { // collinear points, use only first and last
        paint = { paint.front(), paint.back() };
    }

    tmp->points = paint;
    const auto size = (int)tmp->points.size();
    auto skew = cell.skew * -1;
    for (int i = 0; i < size; ++i) {
        auto& point = tmp->points[i];
        if (cell.tenatt != 0.0 || cell.tenrel != 0.0) {
            auto isAttack = i < size - 2 && point.y > tmp->points[i + 1].y;
            point.tension = isAttack ? cell.tenatt : cell.tenrel * -1;
        }
        if (i > 0 && i < size - 1 && skew != 0.0) {
            if (skew > 0)
                point.x = point.x + skew * (1 - point.x);
            else
                point.x = point.x + skew * point.x;
        }
    }
    if (cell.invertx) tmp->reverse();

    for (auto& point : tmp->points) {
        double px = minx + point.x * w; // map points to rectangle bounds
        double py = miny + point.y * h;
        pts.push_back({ 0, px, py, point.tension, point.type });
    }

    return pts;
}

void Sequencer::rotateRight()
{
    snapshot = cells;
    int grid = audioProcessor.getCurrentGrid();
    double gridx = 1.0/grid;
    for (auto& cell : cells) {
        cell.minx += gridx;
        cell.maxx += gridx;
        if (cell.minx >= 1.0) {
            cell.minx -= 1.0;
            cell.maxx -= 1.0;
        }
    }
    sortCells();
    createUndo(snapshot);
    build();
}

void Sequencer::rotateLeft()
{
    snapshot = cells;
    int grid = audioProcessor.getCurrentGrid();
    double gridx = 1.0/grid;
    for (auto& cell : cells) {
        cell.minx -= gridx;
        cell.maxx -= gridx;
        if (cell.maxx <= 0.0) {
            cell.minx += 1.0;
            cell.maxx += 1.0;
        }
    }
    sortCells();
    createUndo(snapshot);
    build();
}

void Sequencer::doublePattern()
{
    snapshot = cells;
    auto cellsarr = cells;
    for (auto cell : cellsarr) {
        cell.minx += 1.0;
        cell.maxx += 1.0;
        cells.push_back(cell);
    }

    for (auto& cell : cells) {
        cell.minx /= 2.0;
        cell.maxx /= 2.0;
    }

    createUndo(snapshot);
    build();
}

void Sequencer::sortCells()
{
    std::sort(cells.begin(), cells.end(), [](const Cell& a, const Cell& b) {
        return a.minx < b.minx;
        });
}

void Sequencer::randomize(SeqEditMode mode, double min, double max)
{
    bool snap = audioProcessor.params.getRawParameterValue("snap")->load() == 1.0f;
    int grid = audioProcessor.getCurrentGrid();
    auto snapy = grid % 6 == 0 ? 12.0 : 16.0;

    for (auto& cell : cells) {
        auto rmin = min;
        auto rmax = max;

        // this bit of code gets confusing because the variables are inverted
        // maxy and miny are inverted as well as the y coordinates on screen
        // the logic is to map the random min to the cell bottom
        // the random max gets scaled proportionally to the new range
        if (mode == EditMax) {
            rmin = std::max(rmin, 1.0 - cell.maxy);
            rmax = std::max(rmin + (max - min) * (1 - rmin), rmax);
        }
        // same thing but random max is mapped to the cell ceiling
        // and random min is scaled proportionally
        else if (mode == EditMin) {
            rmax = std::min(rmax, 1.0 - cell.miny);
            rmin = std::min(rmax - (max - min) * rmax, rmin);
        }

        double random = (rand() / (double)RAND_MAX);
        double value = rmin + (rmax - rmin) * random;
        bool flag = random <= (rmax - rmin) / 2.0 + rmin; // the slider is a double range, arrange it so that when the range is full the prob is 50%

        if (snap) {
            value = std::round(value * snapy) / snapy;
        }

        if (mode == EditTenAtt) {
            if (cell.invertx) cell.tenrel = (value * 2 - 1) * -1;
            else cell.tenatt = (value * 2 - 1) * -1;
        }
        else if (mode == EditTenRel) {
            if (cell.invertx) cell.tenatt = (value * 2 -1) * -1;
            else cell.tenrel = (value * 2 - 1) * -1;
        }
        else if (mode == EditTension) cell.tenrel = cell.tenatt = (value * 2 - 1) * -1;
        else if (mode == EditMax) cell.miny = std::min(1.0 - value, cell.maxy);
        else if (mode == EditMin) cell.maxy = std::max(1.0 - value, cell.miny);
        else if (mode == EditInvertX) cell.invertx = flag;
    }

    build();
}

bool Sequencer::isSnapping(const MouseEvent& e) {
    bool snapping = audioProcessor.params.getRawParameterValue("snap")->load() == 1.0f;
    return (snapping && !e.mods.isShiftDown()) || (!snapping && e.mods.isShiftDown());
}

//=====================================================================

void Sequencer::createUndo(std::vector<Cell> snap)
{
    if (compareCells(snap, cells)) {
        return; // nothing to undo
    }
    if (undoStack.size() > globals::MAX_UNDO) {
        undoStack.erase(undoStack.begin());
    }
    undoStack.push_back(snap);
    redoStack.clear();
    MessageManager::callAsync([this]() { audioProcessor.sendChangeMessage(); }); // repaint undo/redo buttons
}
void Sequencer::undo()
{
    if (undoStack.empty())
        return;

    redoStack.push_back(cells);
    cells = undoStack.back();
    undoStack.pop_back();

    build();
    MessageManager::callAsync([this]() {
        audioProcessor.sendChangeMessage(); // repaint undo/redo buttons
        });
}

void Sequencer::redo()
{
    if (redoStack.empty())
        return;

    undoStack.push_back(cells);
    cells = redoStack.back();
    redoStack.pop_back();

    build();
    MessageManager::callAsync([this]() {
        audioProcessor.sendChangeMessage(); // repaint undo/redo buttons
        });
}

void Sequencer::clearUndo()
{
    undoStack.clear();
    redoStack.clear();
    MessageManager::callAsync([this]() {
        audioProcessor.sendChangeMessage(); // repaint undo/redo buttons
        });
}

bool Sequencer::compareCells(const std::vector<Cell>& a, const std::vector<Cell>& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].invertx != b[i].invertx ||
            a[i].minx != b[i].minx ||
            a[i].maxx != b[i].maxx ||
            a[i].maxy != b[i].maxy ||
            a[i].miny != b[i].miny ||
            a[i].shape != b[i].shape ||
            a[i].tenatt != b[i].tenatt ||
            a[i].tenrel != b[i].tenrel ||
            a[i].ptool != b[i].ptool ||
            a[i].skew != b[i].skew
            ) {
            return false;
        }
    }
    return true;
}