#include "cnc/core/gcode/Simulator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace cnc {

namespace {

constexpr double kPi = 3.14159265358979323846;

double distance(const Point2D& a, const Point2D& b)
{
    return std::hypot(b.x - a.x, b.y - a.y);
}

double arcLength(const PathSegment& seg)
{
    const double radius = distance(seg.center, seg.start);
    if (radius <= 0.0) {
        return distance(seg.start, seg.end);
    }

    const double a0 = std::atan2(seg.start.y - seg.center.y, seg.start.x - seg.center.x);
    const double a1 = std::atan2(seg.end.y - seg.center.y, seg.end.x - seg.center.x);

    double sweep = seg.clockwise ? (a0 - a1) : (a1 - a0);
    constexpr double kTwoPi = 2.0 * kPi;
    // Normalize into (0, 2π]; start==end means a full circle.
    while (sweep <= 1e-9) {
        sweep += kTwoPi;
    }
    return radius * sweep;
}

double segmentLength(const PathSegment& seg)
{
    return (seg.shape == SegmentShape::Arc) ? arcLength(seg) : distance(seg.start, seg.end);
}

} // namespace

JobEstimate estimateJob(const Toolpath& path, double rapidRateMmMin, double fallbackFeedMmMin)
{
    JobEstimate estimate;
    if (path.empty()) {
        return estimate;
    }

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    auto expand = [&](const Point2D& p) {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    };

    double seconds = 0.0;
    for (const PathSegment& seg : path) {
        expand(seg.start);
        expand(seg.end);

        const double length = segmentLength(seg);
        double rate = (seg.move == MoveType::Rapid) ? rapidRateMmMin : seg.feed;
        if (rate <= 0.0) {
            rate = fallbackFeedMmMin;
        }
        if (rate > 0.0) {
            seconds += (length / rate) * 60.0;
        }
    }

    estimate.seconds = seconds;
    estimate.bounds = QRectF(minX, minY, maxX - minX, maxY - minY);
    return estimate;
}

} // namespace cnc
