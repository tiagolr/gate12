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

class GATE12AudioProcessor;

struct SelPoint {
    std::string id;
    double x; // 0..1 in relation to viewport
    double y;
    double areax; // 0..1 in relation to selection area
    double areay;
};

class View : public juce::Component, private juce::AudioProcessorValueTreeState::Listener, private juce::Timer
{
public:
  int winx = 0;
  int winy = 0;
  int winw = 0;
  int winh = 0;

  int selectedPoint = -1;
  int selectedMidpoint = -1;
  int hoverPoint = -1;
  int hoverMidpoint = -1;
  int rmousePoint = -1;
  const int HOVER_RADIUS = 8;
  const int MSEL_PADDING = 8;

  View(GATE12AudioProcessor&);
  ~View() override;
  void init();
  void timerCallback() override;
  void paint(Graphics& g) override;
  void drawWave(Graphics& g, std::vector<double> samples, Colour color) const;
  void drawGrid(Graphics& g);
  void drawSegments(Graphics& g);
  void drawMidPoints(Graphics& g);
  void drawPoints(Graphics& g);
  void drawSelection(Graphics& g);
  void drawSelectionHandles(Graphics& g);
  void drawSeek(Graphics& g);
  std::vector<double> View::getMidpointXY(Segment seg);
  int getHoveredPoint(int x, int y);
  int getHoveredMidpoint(int x, int y);

  void mouseDown(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  void mouseMove(const juce::MouseEvent& e) override;
  void mouseDoubleClick(const juce::MouseEvent& e) override;
  void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

  bool keyPressed(const juce::KeyPress& key) override;

  void parameterChanged (const juce::String& parameterID, float newValue) override;
  void showPointContextMenu(const juce::MouseEvent& event);

  void applyPaintTool(int x, int y, const MouseEvent& e);

private:
  GATE12AudioProcessor& audioProcessor;
  double origTension = 0;
  int dragStartY = 0;
  int pattern = -1; // used to detect pattern changes

  // Multi-select
  Point<int> selectionStart = Point(-1,-1);
  Point<int> selectionEnd = Point(-1,-1);
  Rectangle<int> selectionArea = Rectangle<int>();
  Rectangle<int> selectionAreaStart = Rectangle<int>(); // used to drag or scale selection area
  std::vector<SelPoint> selectionPoints;
  int selectionDragHover = -1; // flag for hovering selection drag handles, 1 top left corner, 2 top center etc..
  void createSelection(const MouseEvent& e);
  void recalcSelectionArea();
  void clearSelection();
  void dragSelection(const MouseEvent& e);
  void updatePointsToSelection(bool invertx, bool inverty);
  void resizeMultiSelection(int x, int y, int w, int h);

  bool isSnapping(const MouseEvent& e);
  bool isCollinear(Segment seg);
  bool pointInRect(int x, int y, int xx, int yy, int w, int h);
  
};