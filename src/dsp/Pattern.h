/*
  ==============================================================================

    Pattern.h
    Author:  tiagolr

  ==============================================================================
*/

#pragma once
#include <vector>
#include <mutex>
#include <atomic>

class GATE12AudioProcessor;

struct PPoint {
    std::string id;
    double x;
    double y;
    double tension;
    int type;
};

struct Segment {
    double x1;
    double x2;
    double y1;
    double y2;
    double tension;
    double power;
    int type;
};

class Pattern
{
public:
    static std::vector<PPoint> copy_pattern;
    const double PI = 3.14159265358979323846;
    int index;
    std::vector<PPoint> points;
    std::vector<Segment> segments;

    Pattern(GATE12AudioProcessor&, int);
    int insertPoint(double x, double y, double tension, int type);
    void sortPoints();
    void setTension(double t); // sets global tension multiplier
    double getTension(); // gets global tension multiplier
    void removePoint(double x, double y);
    void removePoint(int i);
    void removePointsInRange(double x1, double x2);
    void invert();
    void reverse();
    void clear();
    void buildSegments();
    void loadSine();
    void loadTriangle();
    void loadRandom(int grid);
    void copy();
    void paste();

    double get_y_curve(Segment seg, double x);
    double get_y_scurve(Segment seg, double x);
    double get_y_pulse(Segment seg, double x);
    double get_y_wave(Segment seg, double x);
    double get_y_triangle(Segment seg, double x);
    double get_y_stairs(Segment seg, double x);
    double get_y_smooth_stairs(Segment seg, double x);
    double get_y_at(double x);

private:
    std::atomic<double> tensionMult = 0.0; // global tension param
    GATE12AudioProcessor& gate;
    std::mutex mtx;
    double mod(double a, double b);
};