#pragma once

#include <vector>

namespace cnc {

// A point in the XY cutting plane (millimeters).
struct Point2D
{
    double x = 0.0;
    double y = 0.0;
};

// How a segment is travelled: a non-cutting positioning move (G0) or a cut
// (G1/G2/G3).
enum class MoveType
{
    Rapid,
    Cut
};

enum class SegmentShape
{
    Line,
    Arc
};

// One drawable move in the toolpath. This is the intermediate representation the
// preview and the estimator work on, decoupled from raw G-code text. It is 2D
// (the plasma cutting plane); extra axes parsed from the program do not appear
// here.
struct PathSegment
{
    MoveType move = MoveType::Rapid;
    SegmentShape shape = SegmentShape::Line;

    Point2D start;
    Point2D end;

    // Arc only:
    Point2D center;
    bool clockwise = false;  // G2 = clockwise, G3 = counter-clockwise

    double feed = 0.0;  // mm/min in effect for this move (Cut moves)
};

using Toolpath = std::vector<PathSegment>;

} // namespace cnc
