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
        if (cell.shape != SSilence) {
            auto& button = buttons[i];
            auto l = button.expanded(-button.getWidth()/4,0).toFloat();
            g.drawLine(l.getX(), l.getCentreY(), l.getRight(), l.getCentreY(), 2.f);
        }
    }

    if (editMode == SMax || editMode == SMin)
        return;

    g.setColour(Colours::black.withAlpha(0.25f));
    g.fillRect(winx,winy,winw,winh);

    int grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    float gridx = winw / (float)grid;

    if (editMode == SInvertx || editMode == SInverty) {
        g.setColour(Colour(0xff0080ff).darker(editMode == SInvertx ? 0.0f : 0.3f).withAlpha(0.2f));
        for (int i = 0; i < grid; ++i) {
            auto& cell = cells[i];
            if ((editMode == SInvertx && cell.invertx) || (editMode == SInverty && cell.inverty)) {
                g.fillRect((float)winx+i*gridx, (float)winy, (float)gridx, (float)winh);
            }
        }

        return;
    }

    // draw selected edit mode overlay
    Colour c = editMode == STenAtt ? Colours::gold
        : editMode == STenRel ? Colours::gold.darker(0.3f)
        : editMode == STension ? Colour(0xffff8080)
        : editMode == SInvertx ? Colours::aqua
        : editMode == SInverty ? Colours::aqua.darker(0.3f)
        : editMode == SGate ? Colours::chocolate
        : Colours::white;

    c = c.withAlpha(0.5f);
    

    
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
    if (hoverButton == -1) {
        onMouseSegment(e, true);
    }
}

void Sequencer::mouseDown(const MouseEvent& e)
{
    snapshot = cells;

    // process mouse down on seg buttons
    if (hoverButton > -1) {
        auto& cell = cells[hoverButton];
        if (cell.shape != SSilence) 
            cell.lshape = cell.shape;
        cell.shape = cell.shape == SSilence 
            ? cell.lshape 
            : SSilence;
        build();
    }
    // process mouse down on viewport
    else if (e.getPosition().y > 10) {
        onMouseSegment(e, false);
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

void Sequencer::onMouseSegment(const MouseEvent& e, bool isDrag) {
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

    if (editMode == SMin || editMode == SMax) {
        if (isDrag && cell.shape == SSilence) {
            return; // ignore cell when dragging over silence cell
        }

        if (selectedShape == SSilence) {
            cell.shape = SSilence;
            build();
            return;
        }

        if (selectedShape == SPTool) {
            cell.shape = selectedShape;
            cell.lshape = selectedShape;
            cell.ptool = audioProcessor.paintTool;
        }

        // Apply selected shape to clicked or dragged cell
        else if (cell.shape != selectedShape) {
            cell.invertx = selectedShape == SRampUp;
            cell.shape = selectedShape;
            cell.lshape = selectedShape;
        }
    }

    // Apply edit mode to the cell fields
    if (editMode == SMin) {
        cell.maxy = y; // y coordinates are inverted
        if (cell.miny > y)
            cell.miny = y;
    }
    else if (editMode == SMax) {
        cell.miny = y; // y coordinates are inverted
        if (cell.maxy < y)
            cell.maxy = y;
    }
    else if (editMode == SInvertx) {
        cell.invertx = !snapshot[seg].invertx;
    }
    else if (editMode == SInverty) {
        cell.inverty = !snapshot[seg].inverty;
    }
    else if (editMode == STension) {
        cell.tenatt = y * 2 - 1;
        cell.tenrel = y * 2 - 1;
    }
    else if (editMode == STenAtt) {
        cell.tenatt = y * 2 - 1;
    } 
    else if (editMode == STenRel) {
        cell.tenrel = y * 2 - 1;
    } 
    build();
}

/*
* Returns the boundaries of the buttons above the pattern
* These buttons toggle the pattern type/shape from silence to previous shape
*/
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
        cells.push_back({ SRampDn, SRampDn, 0, false, false, 0.0, 1.0, 0.0, 0.0 });
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

    for (int i = 0; i < grid; ++i) {
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
    const auto size = tmp->points.size();
    for (int i = 0; i < size; ++i) {
        auto& point = tmp->points[i];
        if (cell.tenatt != 0.0 || cell.tenrel != 0.0) {
            auto isAttack = i < size - 2 && point.y > tmp->points[i + 1].y;
            point.tension = isAttack ? cell.tenatt : cell.tenrel * -1;
        }
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

void Sequencer::rotateRight()
{
    snapshot = cells;
    int grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    std::rotate(cells.begin(), cells.begin() + grid - 1, cells.begin() + grid);
    createUndo(snapshot);
    build();
}
void Sequencer::rotateLeft()
{
    snapshot = cells;
    int grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    std::rotate(cells.begin(), cells.begin() + 1, cells.begin() + grid);
    createUndo(snapshot);
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
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].invertx != b[i].invertx ||
            a[i].inverty != b[i].inverty ||
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