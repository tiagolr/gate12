#pragma once

#include <JuceHeader.h>
#include <iostream>

struct Vec2 {
    double x, y;

    Vec2() : x(0), y(0) {}
    Vec2(double x_, double y_) : x(x_), y(y_) {}

    Vec2 operator*(double scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
};

Vec2 bilinearInterpolate(const Vec2& P00, const Vec2& P10,
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
	MultiSelect() {}
	~Multiselect() {}

	void mouseDown(const juce::MouseEvent& e);
	void paintBackground(Graphics& g);
	void paint(Graphics& g);

private:
    int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
    std::vector<SelPoint> selectionPoints;
    int selectionDragHover = -1; // flag for hovering selection drag handles, 0 area, 1 top left corner, 2 top center etc..
};