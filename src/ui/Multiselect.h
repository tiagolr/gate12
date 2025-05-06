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
    Point<int> toPoint() { return Point<int>((int)x , (int)y); }

    Vec2 operator*(double scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
};

using Quad = std::array<Vec2, 4>;

inline Vec2 bilinearInterpolate(const Quad& quad, double u, double v) {
    Vec2 A = quad[0] * (1 - u) + quad[1] * u;  // Top edge interpolation
    Vec2 B = quad[2] * (1 - u) + quad[3] * u;  // Bottom edge interpolation
    Vec2 P = A * (1 - v) + B * v;      // Final vertical interpolation
    return P;
}

//inline bool isPointInPolygon(const Vec2& p, const Quad& quad)
//{
//    int n = (int)quad.size();
//    bool inside = false;
//
//    // Iterate through each edge of the polygon
//    for (int i = 0, j = n - 1; i < n; j = i++) 
//    {
//        const Vec2& vertex1 = quad[i];
//        const Vec2& vertex2 = quad[j];
//
//        // Check if the point P lies on the line segment between vertex1 and vertex2
//        if (((vertex1.y > p.y) != (vertex2.y > p.y)) && 
//            (p.x < (vertex2.x - vertex1.x) * (p.y - vertex1.y) / (vertex2.y - vertex1.y) + vertex1.x))
//        {
//            inside = !inside;
//        }
//    }
//
//    return inside;
//}

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
    void makeSelection(const MouseEvent& e, Point<int>selectionStart, Point<int>selectionEnd);
    void dragSelection(const MouseEvent& e);
    void updatePointsToSelection(bool invertx, bool inverty);
    void deleteSelectedPoints();

    int mouseHover = -1; // flag for hovering selection drag handles, 0 area, 1 top left corner, 2 top center etc..
    std::vector<SelPoint> selectionPoints;

private:
    const int PAD = 8;
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;

    // select quad coordinates
    Quad quad = { Vec2(0.0,0.0),Vec2(1.0,0.0),Vec2(0.0,1.0),Vec2(1.0,1.0) };
    // select quad relative coordinates to selection area
    Quad quadrel = { Vec2(0.0, 0.0), Vec2(0.0, 0.0), Vec2(0.0, 0.0), Vec2(0.0, 0.0) };

    Quad getQuadExpanded(double expand = 0.0);
    void calcRelativeQuadCoords(Rectangle<double> area);
    void applyRelativeQuadCoords(Rectangle<double> area);

    Rectangle<int> selectionArea;
	Rectangle<int> selectionAreaStart = Rectangle<int>(); // used to drag or scale selection area
    Quad selectionQuadStart = {Vec2(0.0,0.0), Vec2(1.0, 0.0), Vec2(0.0, 1.0), Vec2(1.0,1.0)};
    GATE12AudioProcessor& audioProcessor;

    bool isSnapping(const MouseEvent& e);
    Vec2 pointToVec(Point<double> p);
    Rectangle<int> quadToRect(Quad q);
};