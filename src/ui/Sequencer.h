#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "../dsp/Pattern.h"
#include "algorithm"

using namespace globals;
class GATE12AudioProcessor;

enum CellShape {
    SSilence,
    SRampUp,
    SRampDn,
    STri,
    SLine,
    SPTool,
};

enum SeqEditMode {
    SMin,
    SMax,
    STension,
    STenAtt,
    STenRel,
    SInvertx,
    SInverty,
    SGate,
};

struct Cell { 
    CellShape shape;
    CellShape lshape; // used to temporarily change type and revert back
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
    int hoverButton = -1;
    CellShape hoverButtonType = CellShape::SSilence; // used for dragging multiple buttons assigning the same type
    CellShape selectedShape = CellShape::SRampDn;

    Sequencer(GATE12AudioProcessor& p);
    ~Sequencer() {}

    void setViewBounds(int _x, int _y, int _w, int _h);
    void draw(Graphics& g);
    void drawBackground(Graphics& g);

    void mouseMove(const MouseEvent& e);
    void mouseDrag(const MouseEvent& e);
    void mouseDown(const MouseEvent& e);
    void mouseUp(const MouseEvent& e);
    void mouseDoubleClick(const MouseEvent& e);
    void onMouseSegment(const MouseEvent& e, bool isDrag);

    void close();
    void open();
    void apply();
    void clear();
    void build();
    std::vector<PPoint> buildSeg(double minx, double maxx, Cell cell);
    std::vector<Rectangle<int>> getSegButtons();
    void rotateRight();
    void rotateLeft();

    std::vector<std::vector<Cell>> undoStack;
    std::vector<std::vector<Cell>> redoStack;
    void clearUndo();
    void createUndo(std::vector<Cell> snapshot);
    void undo();
    void redo();

private:
    Point<int> lmousepos;
    std::vector<PPoint> silence;
    std::vector<PPoint> ramp;
    std::vector<PPoint> tri;
    std::vector<PPoint> line;

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
    bool compareCells(const std::vector<Cell>& a, const std::vector<Cell>& b);
};