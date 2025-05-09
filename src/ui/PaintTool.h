#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class GATE12AudioProcessor;

class PaintTool {
public:
    bool dragging = false;

    PaintTool(GATE12AudioProcessor& p) : audioProcessor(p) {}
    ~PaintTool() {}

    void setViewBounds(int _x, int _y, int _w, int _h);

    void draw(Graphics& g);
    void apply();
    void mouseMove(const MouseEvent& e);
    void mouseDrag(const MouseEvent& e);
    void mouseDown(const MouseEvent& e);
    void mouseUp(const MouseEvent& e);

private:
    int paintW = 100;
    int paintH = 150;
    int startW = 100;
    int startH = 150;
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;
    bool invertx = false;
    bool inverty = false;
    bool snap = false;
    Point<int> mousePos;
    GATE12AudioProcessor& audioProcessor;

    Rectangle<double> getBounds();
    bool isSnapping(const MouseEvent& e);
};