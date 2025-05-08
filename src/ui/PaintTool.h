#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class GATE12AudioProcessor;

class PaintTool {
public:
    PaintTool(GATE12AudioProcessor& p) : audioProcessor(p) {}
    ~PaintTool() {}

    void setViewBounds(int _x, int _y, int _w, int _h);

    void draw(Graphics& g);
    void apply();
    void mouseMove(const MouseEvent& e);
    void mouseDrag(const MouseEvent& e);
    void mouseDown(const MouseEvent& e);

private:
    int paintW = 50;
    int paintH = 100;
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;
    bool invertx = false;
    bool inverty = false;
    bool dragging = false;
    Point<int> mousePos;
    GATE12AudioProcessor& audioProcessor;

    Rectangle<int> getBounds(bool snap);
    bool isSnapping(const MouseEvent& e);
};