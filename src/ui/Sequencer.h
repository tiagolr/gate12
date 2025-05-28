#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "../dsp/Pattern.h"
#include "algorithm"

using namespace globals;
class GATE12AudioProcessor;

enum CellShape {
    SNone,
    SSilence,
    SRampUp,
    SRampDn,
    STri,
    SLine,
    SPTool,
    SLPoint,
};

enum SeqEditMode {
    EditNone,
    EditMin,
    EditMax,
    EditTension,
    EditTenAtt,
    EditTenRel,
    EditInvertX,
    EditSkew,
    EditSilence, // used for randomize only
};

struct Cell { 
    CellShape shape;
    CellShape lshape; // used to temporarily change type and revert back
    int ptool; // paint tool
    bool invertx;
    double minx;
    double maxx;
    double miny;
    double maxy;
    double tenatt; // attack tension
    double tenrel; // release tension
    double skew;
};

class Sequencer {
public:
    std::vector<Cell> cells;
    bool editNoneEditsMax = true;
    SeqEditMode editMode = SeqEditMode::EditMax;
    CellShape selectedShape = CellShape::SRampDn;
    int patternIdx = -1;

    Sequencer(GATE12AudioProcessor& p);
    ~Sequencer() {}

    void setViewBounds(int _x, int _y, int _w, int _h);
    void draw(Graphics& g);
    void drawBackground(Graphics& g);
    Colour getEditModeColour(SeqEditMode mode);

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
    std::vector<PPoint> buildSeg(Cell cell);
    int getCellIndex(double minx, double maxx);
    int getCellIndexAt(double x);
    Rectangle<double> getSegBounds(int segidx);
    std::vector<Cell*> getCellsInRange(double minx, double maxx, bool getOverlapping);
    int addCell(double minx, double maxx);
    void clearSegment(double minx, double maxx, bool removeAll);
    void rotateRight();
    void rotateLeft();
    void doublePattern();
    void sortCells();

    void randomize(SeqEditMode mode, double min, double max);
    void clear(SeqEditMode mode);

    std::vector<std::vector<Cell>> undoStack;
    std::vector<std::vector<Cell>> redoStack;
    void clearUndo();
    void createUndo(std::vector<Cell> snapshot);
    void undo();
    void redo();

private:
    CellShape startHoverShape = CellShape::SLine; // used for dragging multiple buttons with the same type
    bool startInvertX = false; // used to drag toggle all segments to the same invertx
    Point<int> lmousepos;
    std::vector<PPoint> silence;
    std::vector<PPoint> ramp;
    std::vector<PPoint> tri;
    std::vector<PPoint> line;
    std::vector<PPoint> lpoint;

    std::vector<PPoint> backup;
    std::vector<Cell> snapshot;
    Pattern* pat;
    Pattern* tmp; // temp pattern used for painting
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;
    GATE12AudioProcessor& audioProcessor;

    bool isSnapping(const MouseEvent& e);
    std::vector<PPoint> removeCollinearPoints(std::vector<PPoint>& pts);
    bool compareCells(const std::vector<Cell>& a, const std::vector<Cell>& b);
};