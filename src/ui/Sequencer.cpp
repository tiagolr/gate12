#include "Sequencer.h"
#include "../PluginProcessor.h"

Sequencer::Sequencer(GATE12AudioProcessor& p) : audioProcessor(p) 
{
    tmp = new Pattern(-1);
    pat = new Pattern(-1);
    clear();
    ramp.push_back({ 0, 0.0, 1.0, 0.0, 1 });
    ramp.push_back({ 0, 0.0, 0.0, 0.0, 1 });
    ramp.push_back({ 0, 1.0, 1.0, 0.0, 1 });
    line.push_back({ 0, 0.0, 0.0, 0.0, 1 });
    line.push_back({ 0, 1.0, 0.0, 0.0, 1 });
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

    g.setColour(Colours::white.withAlpha(0.1f));
    g.fillRect((int)std::round(seg * gridx) + winx, winy, (int)std::round(gridx), winh);

    if (editMode == EditMax || editMode == EditMin) {
        double x1 = int(x / (1.0/grid)) * (1.0/grid);
        double x2 = x1 + 1.0/grid;
        auto idx = getCellIndex(x1, x2);
        if (idx > -1) {
            auto& cell = cells[idx];
            g.setColour(Colour(COLOR_ACTIVE));
            double xx1 = seg * gridx + winx;
            double xx2 = xx1 + gridx;
            double yy = winy + winh * cell.maxy;
            double yy2 = winy + winh * cell.miny;
            g.drawLine((float)xx1, (float)yy+0.5f, (float)xx2, (float)yy+0.5f);
            g.drawLine((float)xx1, (float)yy2+0.5f, (float)xx2, (float)yy2+0.5f);
        }
    }
}

void Sequencer::draw(Graphics& g)
{
    // draw hovered cell min and max lines
    if (editMode == EditMax || editMode == EditMin)
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

        auto bounds = Rectangle<float>((float)winx+(float)cell.minx*winw, (float)winy + winh / 2.f, (float)(cell.maxx - cell.minx) * winw, winh / 2.f * std::fabs((float)value));
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
    double x1 = int(x / (1.0/grid)) * (1.0/grid);
    double x2 = x1 + 1.0/grid;

    if (isSnapping(e)) {
        auto snapy = grid % 6 == 0 ? 12.0 : 16.0;
        y = std::round(y * snapy) / snapy;
    }

    bool canAddCell = (editMode == EditMax) && selectedShape != SNone;
    auto segCells = getCellsInRange(x1, x2);

    if (e.mods.isRightButtonDown()) {
        for (auto cell : segCells) {
            if (cell->shape == SSilence) continue;
            else if (editMode == EditMin) cell->maxy = 1.0;
            else if (editMode == EditMax) cell->miny = 0.0;
            else if (editMode == EditTenAtt) cell->tenatt = 0.0;
            else if (editMode == EditTenRel) cell->tenrel = 0.0;
            else if (editMode == EditTension) cell->tenatt = cell->tenrel = 0.0;
            else if (editMode == EditInvertX) cell->invertx = false;
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
        segCells = getCellsInRange(x1, x2);
    }

    if (!segCells.size())
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

    // toggle editMin if the edit point is closer to min than max
    if (editMode == EditMax && !isDrag && selectedShape != SLine && !isNewCell) {
        auto dymin = std::abs(y - segCells[0]->maxy);
        auto dymax = std::abs(y - segCells[0]->miny);
        if (dymin < dymax) {
            editMode = EditMin;
        }
    }

    for (auto cell : segCells) {
        if (cell->shape == SLine) {
            cell->maxy = 1.0;
        }
        if (editMode == EditMin) {
            cell->maxy = y; // y coordinates are inverted
            if (cell->miny > y)
                cell->miny = y;
        }
        else if (editMode == EditMax) {
            cell->miny = y; // y coordinates are inverted
            if (cell->maxy < y)
                cell->maxy = y;
        }
        else if (editMode == EditInvertX) {
            if (!isDrag && cell == segCells[0])
                startInvertX = !segCells[0]->invertx;
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
    for (int i = 0; i < cells.size(); ++i) {
        auto& cell = cells[i];
        if (cell.minx >= minx && cell.minx < maxx)
            return i;
    }

    return -1;
}

int Sequencer::addCell(double minx, double maxx)
{
    cells.erase(std::remove_if(cells.begin(), cells.end(), [minx, maxx](const Cell& cell) {
        return cell.minx < maxx && cell.maxx > minx;
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
std::vector<Cell*> Sequencer::getCellsInRange(double minx, double maxx) {
    std::vector<Cell*> result;

    for (auto& cell : cells) {
        if (cell.minx >= minx && cell.minx < maxx) {
            result.push_back(&cell);
        }
    }

    return result;
}

/*
* Remove cells in a segment if they don't start and end in the segments range
*/
void Sequencer::clearSegment(double minx, double maxx, bool removeAll)
{
    cells.erase(std::remove_if(cells.begin(), cells.end(), [minx, maxx, removeAll](const Cell& cell) {
        double eps = 1e-10;
        bool exactMatch = std::abs(cell.minx - minx) < eps && std::abs(cell.maxx - maxx) < eps;
        bool overlaps = cell.minx < maxx - eps && cell.maxx > minx + eps;
        return overlaps && (!exactMatch || removeAll);
        }), cells.end());
}

void Sequencer::open()
{
    backup = audioProcessor.pattern->points;
    patternIdx = audioProcessor.pattern->index;
    build();
    audioProcessor.pattern->points = pat->points;
    audioProcessor.pattern->buildSegments();
}

void Sequencer::close()
{
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
            a[i].ptool != b[i].ptool
            ) {
            return false;
        }
    }
    return true;
}