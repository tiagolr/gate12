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

    if (editMode == EditMax || editMode == EditMin || editMode == EditNone)
        return;

    g.setColour(Colours::black.withAlpha(0.25f));
    g.fillRect(winx,winy,winw,winh);

    int grid = std::min(SEQ_MAX_CELLS, audioProcessor.getCurrentGrid());
    float gridx = winw / (float)grid;

    if (editMode == EditInvertX) {
        g.setColour(Colour(0xff0080ff).darker(editMode == EditInvertX ? 0.0f : 0.3f).withAlpha(0.2f));
        for (int i = 0; i < grid; ++i) {
            auto& cell = cells[i];
            if (cell.invertx)
                g.fillRect((float)winx+i*gridx, (float)winy, (float)gridx, (float)winh);
        }
        return;
    }

    // draw selected edit mode overlay
    g.setColour(getEditModeColour(editMode).withAlpha(0.2f));

    for (int i = 0; i < grid; ++i) {
        auto& cell = cells[i];
        double value = editMode == EditTenAtt ? (cell.invertx ? cell.tenrel : cell.tenatt) * -1
            : editMode == EditTenRel ? (cell.invertx ? cell.tenatt : cell.tenrel) * -1
            : editMode == EditTension ? (std::fabs(cell.tenatt) > std::fabs(cell.tenrel) ? cell.tenatt : cell.tenrel) * -1
            : 0.0;

        auto bounds = Rectangle<float>((float)winx+i* gridx, (float)winy+winh/2.f, gridx, winh/2.f*std::fabs((float)value));
        if (value > 0.0)
            bounds = bounds.withY(winy+winh/2.f-bounds.getHeight());
        if (value == 0.0)
            bounds = bounds.withHeight(2.0f).withY(winy+winh/2.f-1.f);

        g.fillRect(bounds);
    }
}

Colour Sequencer::getEditModeColour(SeqEditMode mode)
{
    if (mode == EditInvertX) return Colours::aqua;
    if (mode == EditMax) return Colours::white;
    if (mode == EditMin) return Colours::white;
    if (mode == EditTension) return Colour(0xffff8080);
    if (mode == EditTenAtt) return Colours::yellow;
    if (mode == EditTenRel) return Colours::yellow;
    return Colours::white;
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
    
    // process mouse drag on seg buttons
    if (hoverButton > -1) {
        auto buttons = getSegButtons();
        for (int i = 0; i < buttons.size(); ++i) {
            auto& button = buttons[i];
            if (button.contains(e.getPosition())) {
                hoverButton = i;
                auto& cell = cells[i];
                if (cell.shape != SSilence) {
                    cell.lshape = cell.shape;
                }
                cell.shape = startHoverSilence ? SSilence : cell.lshape;
                build();
            }
        }
    }
    else {
        onMouseSegment(e, true);
    }
}

void Sequencer::mouseDown(const MouseEvent& e)
{
    snapshot = cells;

    // process mouse down on seg buttons
    if (hoverButton > -1) {
        auto& cell = cells[hoverButton];
        if (cell.shape != SSilence) {
            cell.lshape = cell.shape;
        }
        cell.shape = cell.shape == SSilence 
            ? cell.lshape 
            : SSilence;
        startHoverSilence = cell.shape == SSilence;
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

    if (editMode == EditMin || editMode == EditMax || editMode == EditNone) {
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
    if (editMode == EditMin) {
        cell.maxy = y; // y coordinates are inverted
        if (cell.miny > y)
            cell.miny = y;
    }
    else if (editMode == EditMax) {
        cell.miny = y; // y coordinates are inverted
        if (cell.maxy < y)
            cell.maxy = y;
    }
    else if (editMode == EditInvertX) {
        if (!isDrag)
            startInvertX = !snapshot[seg].invertx;
        cell.invertx = startInvertX;
    }
    else if (editMode == EditTension) {
        cell.tenatt = y * 2 - 1;
        cell.tenrel = y * 2 - 1;
    }
    else if (editMode == EditTenAtt) {
        if (cell.invertx)
            cell.tenrel = y * 2 - 1;
        else
            cell.tenatt = y * 2 - 1;
    } 
    else if (editMode == EditTenRel) {
        if (cell.invertx) 
            cell.tenatt = y * 2 - 1;
        else
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
        cells.push_back({ SRampDn, SRampDn, 0, false, 0.0, 1.0, 0.0, 0.0 });
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

void Sequencer::clear(SeqEditMode mode)
{
    for (auto& cell : cells) {
        if (mode == EditMax) cell.miny = 0.0;
        else if (mode == EditMin) cell.maxy = 1.0;
        else if (mode == EditInvertX) cell.invertx = cell.shape == CellShape::SRampUp;
        else if (mode == EditTenAtt) cell.tenatt = 0.0;
        else if (mode == EditTenRel) cell.tenrel = 0.0;
        else if (mode == EditTension) cell.tenatt = cell.tenrel = 0.0;
        else if (mode == EditNone) cell.miny = 0.0;
    }
    build();
}

void Sequencer::randomize(SeqEditMode mode, double min, double max)
{
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

        if (mode == EditTenAtt) cell.tenatt = (value * 2 - 1) * -1;
        else if (mode == EditTenRel) cell.tenrel = (value * 2 - 1) * -1;
        else if (mode == EditTension) cell.tenrel = cell.tenatt = (value * 2 - 1) * -1;
        else if (mode == EditMax || mode == EditNone) cell.miny = std::min(1.0 - value, cell.maxy);
        else if (mode == EditMin) cell.maxy = std::max(1.0 - value, cell.miny);
        else if (mode == EditInvertX) cell.invertx = flag;
        else if (mode == EditSilence) {
            if (flag && cell.shape != SSilence)
                cell.lshape = cell.shape;
            cell.shape = flag ? SSilence : cell.lshape;
        }
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
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].invertx != b[i].invertx ||
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