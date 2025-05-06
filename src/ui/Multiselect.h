/*
  ==============================================================================

    Multiselect.h
    Author:  tiagolr

  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>
#include <iostream>

class GATE12AudioProcessor;

struct SelPoint {
    uint64_t id;
    double x; // 0..1 in relation to viewport
    double y;
    double areax; // 0..1 in relation to selection area
    double areay;
};

struct Vec2 {
    double x, y;

    Vec2() : x(0), y(0) {}
    Vec2(double x_, double y_) : x(x_), y(y_) {}

    Vec2 operator*(double scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
};

inline Vec2 bilinearInterpolate(const Vec2& P00, const Vec2& P10,
                         const Vec2& P11, const Vec2& P01,
                         double u, double v) {
    Vec2 A = P00 * (1 - u) + P10 * u;  // Top edge interpolation
    Vec2 B = P01 * (1 - u) + P11 * u;  // Bottom edge interpolation
    Vec2 P = A * (1 - v) + B * v;      // Final vertical interpolation
    return P;
}

class Multiselect
{
public:
	Multiselect(GATE12AudioProcessor& p) : audioProcessor(p) {}
	~Multiselect() {}

    void setViewBounds(int _x, int _y, int _w, int _h);

	void mouseDown(const MouseEvent& e);
    void mouseMove(const MouseEvent& e);
	void drawBackground(Graphics& g);
	void draw(Graphics& g);

    bool contains(Point<int>p);

    void drawHandles(Graphics& g);
    void recalcSelectionArea();
    void clearSelection();
    void createSelection(const MouseEvent& e, Point<int>selectionStart, Point<int>selectionEnd);
    void dragSelection(const MouseEvent& e);
    void updatePointsToSelection(bool invertx, bool inverty);
    void deleteSelectedPoints();

    int mouseHover = -1; // flag for hovering selection drag handles, 0 area, 1 top left corner, 2 top center etc..
    std::vector<SelPoint> selectionPoints;

private:
    const int PADDING = 8;
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;

    Rectangle<int> selectionArea;
	Rectangle<int> selectionAreaStart = Rectangle<int>(); // used to drag or scale selection area
    GATE12AudioProcessor& audioProcessor;

    bool isSnapping(const MouseEvent& e);
};