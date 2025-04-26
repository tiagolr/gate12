/*
  ==============================================================================

    View.h
    Author:  tiagolr

  ==============================================================================
*/

#pragma once

/*
class View : public IControl
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

  View(const IRECT&, GATE1&);
  void Draw(IGraphics& g) override;
  void OnResize() override;
  bool IsDirty() override {
    return true;
  }
  void drawWave(IGraphics& g, std::vector<sample> samples, IColor color);
  void drawGrid(IGraphics& g);
  void drawSegments(IGraphics& g);
  void drawMidPoints(IGraphics& g);
  void drawPoints(IGraphics& g);
  void drawSeek(IGraphics& g);
  std::vector<double> View::getMidpointXY(Segment seg);
  int getHoveredPoint(int x, int y);
  int getHoveredMidpoint(int x, int y);

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d);

  void paint(int x, int y, const IMouseMod& mod);
  void showRightMouseMenu(int x, int y);
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override;

private:
  GATE1& gate;
  double origTension = 0;
  int dragStartY = 0;

  bool isSnapping(const IMouseMod& mod);
  bool isCollinear(Segment seg);
  bool pointInRect(int x, int y, int xx, int yy, int w, int h);
};
*/