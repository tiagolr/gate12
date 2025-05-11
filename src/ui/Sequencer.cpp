#include "Sequencer.h"
#include "../PluginProcessor.h"

Sequencer::Sequencer(GATE12AudioProcessor& p) : audioProcessor(p) 
{
    tmp = new Pattern(-1);
    pat = new Pattern(-1);
    clear();
    ramp.push_back({ 0, 0.0, 1.0, 0.0, 1 });
    ramp.push_back({ 0, 1.0, 0.0, 0.0, 1 });
    ramp.push_back({ 0, 1.0, 1.0, 0.0, 1 });
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
    x = jlimit(0.0, 1.0, x);

    int grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    double gridx = winw / (double)grid;
    int seg = jlimit(0, std::min(grid-1, SEQ_MAX_CELLS-1), (int)(x * grid));

    g.setColour(Colours::white.withAlpha(0.1f));
    g.fillRect((int)std::round(seg * gridx) + winx, winy, (int)std::round(gridx), winh);
}

void Sequencer::draw(Graphics& g)
{
    (void)g;
}

void Sequencer::open()
{
    backup = audioProcessor.pattern->points;
    build();
    audioProcessor.pattern->points = pat->points;
    audioProcessor.pattern->buildSegments();
}

void Sequencer::close()
{
    audioProcessor.pattern->points = backup;
    audioProcessor.pattern->buildSegments();
}

void Sequencer::clear()
{
    cells.clear();
    for (int i = 0; i < SEQ_MAX_CELLS; ++i) {
        cells.push_back({ CellType::Ramp, 0, false, false, 0.0, 1.0, 0.0, 0.0 });
    }
}

void Sequencer::apply()
{
    auto snap = audioProcessor.pattern->points;
    audioProcessor.pattern->points = backup;
    audioProcessor.createUndoPointFromSnapshot(snap);
    backup = pat->points;
}

void Sequencer::build()
{
    pat->clear();
    double grid = (double)std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    double gridx = 1.0 / grid;

    for (int i = 0; i < cells.size(); ++i) {
        auto seg = buildSeg(i * gridx, (i+1)*gridx, cells[i]);
        for (auto& pt : seg) {
            pat->insertPoint(pt.x, pt.y, pt.tension, pt.type);
        }
    }

    auto& pattern = audioProcessor.pattern;
    pattern->points = pat->points;
    pattern->sortPoints();
    pattern->buildSegments();
}

std::vector<PPoint> Sequencer::buildSeg(double minx, double maxx, Cell cell)
{
    std::vector<PPoint> pts;
    double w = maxx-minx;
    double h = cell.maxy-cell.miny;

    auto paint = ramp;

    if (h == 0 && paint.size() > 1) { // collinear points, use only first and last
        paint = { paint.front(), paint.back() };
    }

    tmp->points = paint;
    if (cell.invertx) tmp->reverse();
    if (cell.inverty) tmp->invert();

    for (auto& point : tmp->points) {
        double px = minx + point.x * w; // map points to rectangle bounds
        double py = cell.miny + point.y * h;
        pts.push_back({ 0, px, py, point.tension, point.type });
    }

    return pts;
}

void Sequencer::mouseMove(const MouseEvent& e)
{
    lmousepos = e.getPosition();
}

void Sequencer::mouseDrag(const MouseEvent& e)
{
    lmousepos = e.getPosition();
    onMouse(e);
}

void Sequencer::mouseDown(const MouseEvent& e)
{
    snapshot = cells;
    onMouse(e);
}

void Sequencer::mouseUp(const MouseEvent& e)
{   
    (void)e;
    createUndo(snapshot);
}


void Sequencer::mouseDoubleClick(const juce::MouseEvent& e)
{
    (void)e;
}

void Sequencer::onMouse(const MouseEvent& e) {
    double x = (e.getPosition().x - winx) / (double)winw;
    double y = (e.getPosition().y - winy) / (double)winh;
    x = jlimit(0.0, 1.0, x);
    y = jlimit(0.0, 1.0, y);

    int grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    int seg = jlimit(0, SEQ_MAX_CELLS-1, (int)(x * grid));
    auto& cell = cells[seg];
    cell.maxy = y;
    build();
}

bool Sequencer::isSnapping(const MouseEvent& e) {
    bool snapping = audioProcessor.params.getRawParameterValue("snap")->load() == 1.0f;
    return (snapping && !e.mods.isShiftDown()) || (!snapping && e.mods.isShiftDown());
}

//=====================================================================

void Sequencer::createUndo(std::vector<Cell> snapshot)
{
    if (compareCells(snapshot, cells)) {
        return; // nothing to undo
    }
    if (undoStack.size() > globals::MAX_UNDO) {
        undoStack.erase(undoStack.begin());
    }
    undoStack.push_back(snapshot);
    redoStack.clear();
}
void Sequencer::undo()
{
    if (undoStack.empty())
        return;

    redoStack.push_back(cells);
    cells = undoStack.back();
    undoStack.pop_back();

    build();
}

void Sequencer::redo()
{
    if (redoStack.empty()) 
        return;

    undoStack.push_back(cells);
    cells = redoStack.back();
    redoStack.pop_back();

    build();
}

void Sequencer::clearUndo()
{
    undoStack.clear();
    redoStack.clear();
}

bool Sequencer::compareCells(const std::vector<Cell>& a, const std::vector<Cell>& b)
{
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].invertx != b[i].invertx ||
            a[i].inverty != b[i].inverty ||
            a[i].maxy != b[i].maxy ||
            a[i].miny != b[i].miny ||
            a[i].type != b[i].type ||
            a[i].tenatt != b[i].tenatt ||
            a[i].tenrel != b[i].tenrel ||
            a[i].ptool != b[i].ptool
        ) {
            return false;
        }
    }
    return true;
}