/*
  ==============================================================================

    View.h
    Author:  tiagolr

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/Pattern.h"
#include "Multiselect.h"
#include "PaintTool.h"
#include "../Globals.h"

class GATE12AudioProcessor;
using namespace globals;

class View : public juce::Component, private juce::Timer
{
public:
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;

    View(GATE12AudioProcessor&);
    ~View() override;
    void resized() override;
    void timerCallback() override;

    void paint(Graphics& g) override;
    void drawWave(Graphics& g, std::vector<double>& samples, Colour color) const;
    void drawGrid(Graphics& g);
    void drawSegments(Graphics& g);
    void drawMidPoints(Graphics& g);
    void drawPoints(Graphics& g);
    void drawSeek(Graphics& g);
    void drawPreSelection(Graphics& g);
    std::vector<double> getMidpointXY(Segment seg);
    int getHoveredPoint(int x, int y);
    int getHoveredMidpoint(int x, int y);
    PPoint& getPointFromMidpoint(int midpoint);

    // events
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseExit (const MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void showPointContextMenu(const juce::MouseEvent& event);
    void showContextMenu(const MouseEvent& e);

    bool isSnapping(const MouseEvent& e);
    bool isCollinear(Segment seg);
    bool pointInRect(int x, int y, int xx, int yy, int w, int h);

    void insertNewPoint(const MouseEvent& event);

private:
    int selectedPoint = -1;
    int selectedMidpoint = -1;
    int hoverPoint = -1;
    int hoverMidpoint = -1;
    int rmousePoint = -1;
    int luimode = false;

    GATE12AudioProcessor& audioProcessor;
    double origTension = 0;
    int dragStartY = 0; // used for midpoint dragging
    uint64_t patternID = 0; // used to detect pattern changes
    std::vector<PPoint> snapshot; // used for undo after drag
    int snapshotIdx = 0; // used for undo after drag

    // Multiselect
    Multiselect multiSelect;
    Point<int> preSelectionStart = Point(-1,-1);
    Point<int> preSelectionEnd = Point(-1,-1);

    // PaintTool
    PaintTool paintTool;
};