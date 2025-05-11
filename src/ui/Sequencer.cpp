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
    x = jlimit(0.0, 1.0, x);

    int grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    double gridx = winw / (double)grid;
    int seg = jlimit(0, std::min(grid-1, SEQ_MAX_CELLS-1), (int)(x * grid));

    if (hoverButton == -1 && lmousepos.y > 10) {
        g.setColour(Colours::white.withAlpha(0.1f));
        g.fillRect((int)std::round(seg * gridx) + winx, winy, (int)std::round(gridx), winh);
    }
}

void Sequencer::draw(Graphics& g)
{
    // draw buttons above each grid segment
    auto buttons = getSegButtons();
    if (hoverButton > -1 && hoverButton < buttons.size()) {
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillRect(buttons[hoverButton]);
    }
    g.setColour(Colours::white);
    for (int i = 0; i < buttons.size(); ++i) {
        auto& cell = cells[i];
        if (cell.type != CellType::Silence) {
            auto& button = buttons[i];
            auto line = button.expanded(-button.getWidth()/4,0).toFloat();
            g.drawLine(line.getX(), line.getCentreY(), line.getRight(), line.getCentreY(), 2.f);
        }
    }
}

void Sequencer::mouseMove(const MouseEvent& e)
{
    lmousepos = e.getPosition();

    hoverButton = -1;
    auto buttons = getSegButtons();
    for (int i = 0; i < buttons.size(); i++) {
        auto& button = buttons[i];
        if (button.contains(e.getPosition())) {
            hoverButton = i;
            break;
        }
    }
}

void Sequencer::mouseDrag(const MouseEvent& e)
{
    lmousepos = e.getPosition();
    hoverButton = -1;
    onMouseSegment(e);
}

void Sequencer::mouseDown(const MouseEvent& e)
{
    snapshot = cells;
    if (hoverButton > -1) {
        auto& cell = cells[hoverButton];
        cell.type = cell.type == CellType::Silence ? CellType::Ramp : CellType::Silence;
        build();
    }
    else if (e.getPosition().y > 10) {
        onMouseSegment(e);
    }
}

void Sequencer::mouseUp(const MouseEvent& e)
{   
    hoverButton = -1;
    auto buttons = getSegButtons();
    for (int i = 0; i < buttons.size(); i++) {
        auto& button = buttons[i];
        if (button.contains(e.getPosition())) {
            hoverButton = i;
            break;
        }
    }

    createUndo(snapshot);
}


void Sequencer::mouseDoubleClick(const juce::MouseEvent& e)
{
    (void)e;
}

void Sequencer::onMouseSegment(const MouseEvent& e) {
    double x = (e.getPosition().x - winx) / (double)winw;
    double y = (e.getPosition().y - winy) / (double)winh;
    
    x = jlimit(0.0, 1.0, x);
    y = jlimit(0.0, 1.0, y);

    int grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    int seg = jlimit(0, SEQ_MAX_CELLS-1, (int)(x * grid));

    if (isSnapping(e)) {
        auto snapy = grid%12 == 0 ? 12.0 : 16.0;
        y = std::round(y * snapy) / snapy;
    }

    auto& cell = cells[seg];

    if (editMode == SeqEditMode::SMin) {
        cell.maxy = y; // y coordinates are inverted
    }
    else if (editMode == SeqEditMode::SMax) {
        cell.miny = y; // y coordinates are inverted
    }
    else if (editMode == SeqEditMode::SFlipX) {
        cell.invertx = !snapshot[seg].invertx;
    }
    else if (editMode == SeqEditMode::SFlipY) {
        cell.inverty = !snapshot[seg].inverty;
    }
    else if (editMode == SeqEditMode::STension) {
        cell.tenatt = y * 2 - 1;
        cell.tenrel = y * 2 - 1;
    }
    else if (editMode == SeqEditMode::STenAtt) {
        cell.tenatt = y * 2 - 1;
    } 
    else if (editMode == SeqEditMode::STenRel) {
        cell.tenrel = y * 2 - 1;
    } 
    build();
}

std::vector<Rectangle<int>> Sequencer::getSegButtons() 
{
    std::vector<Rectangle<int>> rects;
    
    auto grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    auto gridx = winw / grid;
    for (int i = 0; i < grid; ++i) {
        auto rect = Rectangle<int>(winx+gridx*i,10, gridx, PLUG_PADDING);
        rects.push_back(rect);
    }
    return rects;
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
    double miny = cell.miny;
    double h = cell.maxy-cell.miny;

    auto paint = cell.type == CellType::Silence ? silence : ramp;

    if (cell.type == CellType::Silence) {
        miny = 1;
        h = 0;
    }

    if (h == 0 && paint.size() > 1) { // collinear points, use only first and last
        paint = { paint.front(), paint.back() };
    }

    tmp->points = paint;
    const auto size = tmp->points.size();
    for (int i = 0; i < size; ++i) {
        auto& point = tmp->points[i];
        auto isAttack = i < size - 2 && point.y > tmp->points[i + 1].y;
        point.tension = isAttack ? cell.tenatt : cell.tenrel;
        if (cell.inverty) point.tension *= -1;
    }
    if (cell.invertx) tmp->reverse();
    if (cell.inverty) tmp->invert();

    for (auto& point : tmp->points) {
        double px = minx + point.x * w; // map points to rectangle bounds
        double py = miny + point.y * h;
        pts.push_back({ 0, px, py, point.tension, point.type });
    }

    return pts;
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