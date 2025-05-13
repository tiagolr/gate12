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

enum PointType {
    Hold,
    Curve,
    SCurve,
    Pulse,
    Wave,
    Triangle,
    Stairs,
    SmoothSt,
};

struct PPoint {
    uint64_t id; // unique point id
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
    uint64_t versionID = 0; // unique pattern ID, used by UI to detect pattern changes and update selection
    static std::vector<PPoint> copy_pattern;
    const double PI = 3.14159265358979323846;
    int index;
    std::vector<PPoint> points;
    std::vector<Segment> segments;
    std::vector<std::vector<PPoint>> undoStack;
    std::vector<std::vector<PPoint>> redoStack;
    std::atomic<double> tensionMult = 0.0; // tension multiplier applied to all points
    std::atomic<double> tensionAtk = 0.0; // tension multiplier for attack only
    std::atomic<double> tensionRel = 0.0; // tension multiplier for release only

    Pattern(int index);
    void incrementVersion(); // generates a new unique ID for this pattern

    int insertPoint(double x, double y, double tension, int type);
    void sortPoints();
    void setTension(double t, double tatk, double trel, bool dual); // sets global tension multiplier
    void removePoint(double x, double y);
    void removePoint(int i);
    void removePointsInRange(double x1, double x2);
    void invert();
    void reverse();
    void rotate(double x);
    void clear();
    void buildSegments();
    void loadSine();
    void loadTriangle();
    void loadRandom(int grid);
    void copy();
    void paste();
    int getWaveCount(Segment seg);

    double get_y_curve(Segment seg, double x);
    double get_y_scurve(Segment seg, double x);
    double get_y_pulse(Segment seg, double x);
    double get_y_wave(Segment seg, double x);
    double get_y_triangle(Segment seg, double x);
    double get_y_stairs(Segment seg, double x);
    double get_y_smooth_stairs(Segment seg, double x);
    double get_y_at(double x);

    void createUndo();
    void undo();
    void redo();
    void clearUndo();
    static bool comparePoints(const std::vector<PPoint>& a, const std::vector<PPoint>& b);

private:
    static inline uint64_t versionIDCounter = 1; // static global ID counter
    static inline uint64_t pointsIDCounter = 1; // static global ID counter
    bool dualTension = false;
    std::mutex mtx;
};