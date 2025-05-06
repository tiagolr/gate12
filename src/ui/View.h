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

class GATE12AudioProcessor;

class View : public juce::Component, private juce::AudioProcessorValueTreeState::Listener, private juce::Timer
{
public:
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;

    View(GATE12AudioProcessor&);
    ~View() override;
    void init();
    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void paint(Graphics& g) override;
    void drawWave(Graphics& g, std::vector<double>& samples, Colour color) const;
    void drawGrid(Graphics& g);
    void drawSegments(Graphics& g);
    void drawMidPoints(Graphics& g);
    void drawPoints(Graphics& g);
    void drawSeek(Graphics& g);
    std::vector<double> getMidpointXY(Segment seg);
    int getHoveredPoint(int x, int y);
    int getHoveredMidpoint(int x, int y);
    PPoint& getPointFromMidpoint(int midpoint);

    // multi selection
    void createSelection(const MouseEvent& e);
    void drawSelection(Graphics& g);

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
    void applyPaintTool(int x, int y, const MouseEvent& e);

    bool isSnapping(const MouseEvent& e);
    bool isCollinear(Segment seg);
    bool pointInRect(int x, int y, int xx, int yy, int w, int h);

private:
    int selectedPoint = -1;
    int selectedMidpoint = -1;
    int hoverPoint = -1;
    int hoverMidpoint = -1;
    int rmousePoint = -1;
    const int HOVER_RADIUS = 8;
    const int POINT_RADIUS = 4;
    const int MPOINT_RADIUS = 3;

    GATE12AudioProcessor& audioProcessor;
    double origTension = 0;
    int dragStartY = 0; // used for midpoint dragging
    uint64_t patternID = 0; // used to detect pattern changes

    // Multiselect
    Multiselect multiselect;
    Point<int> selectionStart = Point(-1,-1);
    Point<int> selectionEnd = Point(-1,-1);
};