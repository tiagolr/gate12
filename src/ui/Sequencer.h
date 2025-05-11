#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "../dsp/Pattern.h"

using namespace globals;
class GATE12AudioProcessor;

enum CellType {
    PTool,
    Clear,
    Empty,
    Ramp,
    Tri,
};

enum SeqEditMode {
    SMin,
    SMax,
    STen,
    STenAtt,
    STenRel,
    SInvx,
    SInvy,
};

struct Cell { 
    CellType type;
    int ptool; // paint tool
    bool invertx;
    bool inverty;
    double miny;
    double maxy;
    double tenatt; // attack tension
    double tenrel; // release tension
};

class Sequencer {
public:
    SeqEditMode editMode = SeqEditMode::SMax;

    Sequencer(GATE12AudioProcessor& p);
    ~Sequencer() {}

    void setViewBounds(int _x, int _y, int _w, int _h);

    void draw(Graphics& g);
    void drawBackground(Graphics& g);
    void close();
    void open();
    void apply();
    void clear();
    void build();
    std::vector<PPoint> buildSeg(double minx, double maxx, Cell cell);
    void mouseMove(const MouseEvent& e);
    void mouseDrag(const MouseEvent& e);
    void mouseDown(const MouseEvent& e);
    void mouseUp(const MouseEvent& e);
    void mouseDoubleClick(const MouseEvent& e);
    void onMouse(const MouseEvent& e);

private:
    Point<int> lmousepos;
    std::vector<PPoint> ramp;
    std::vector<PPoint> backup;
    std::vector<Cell> cells;
    std::vector<Cell> snapshot;
    Pattern* pat;
    Pattern* tmp; // temp pattern used for painting
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;
    GATE12AudioProcessor& audioProcessor;

    bool isSnapping(const MouseEvent& e);


    std::vector<std::vector<Cell>> undoStack;
    std::vector<std::vector<Cell>> redoStack;
    void createUndo(std::vector<Cell> snapshot);
    void undo();
    void redo();
    void clearUndo();
    bool compareCells(const std::vector<Cell>& a, const std::vector<Cell>& b);
};