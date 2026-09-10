#pragma once

#include <QPointF>

#include <cmath>

namespace cad {

// Reflect point p across the line through a and b. Shared by entity mirroring.
inline QPointF reflectPoint(const QPointF& p, const QPointF& a, const QPointF& b)
{
    const QPointF d = b - a;
    const double len2 = d.x() * d.x() + d.y() * d.y();
    if (len2 <= 0.0) {
        return p;  // degenerate axis
    }
    const double t = ((p.x() - a.x()) * d.x() + (p.y() - a.y()) * d.y()) / len2;
    const QPointF proj(a.x() + t * d.x(), a.y() + t * d.y());
    return QPointF(2.0 * proj.x() - p.x(), 2.0 * proj.y() - p.y());
}

// A polyline vertex carries a `bulge` describing the arc to the next vertex
// (the DXF convention): bulge = tan(sweep/4), 0 = straight, + = CCW. This turns
// one bulge segment (p1 → p2) into concrete arc parameters, shared by rendering
// (PolylineEntity::path) and the geometry engine (decompose), so the two never
// disagree. Angles are degrees, CCW from +X (Y up).
struct BulgeArc
{
    bool isArc = false;  // false: treat p1 → p2 as a straight segment
    QPointF center;
    double radius = 0.0;
    double startAngleDeg = 0.0;  // angle of p1 as seen from center
    double sweepDeg = 0.0;       // signed; + = CCW
};

inline BulgeArc bulgeToArc(const QPointF& p1, const QPointF& p2, double bulge)
{
    BulgeArc arc;
    if (std::abs(bulge) < 1e-9) {
        return arc;  // straight segment
    }
    constexpr double kRadToDeg = 57.29577951308232;
    // Center via the classic bulge construction (cot = (1 - b^2) / 2b).
    const double cot = (1.0 / bulge - bulge) / 2.0;
    const double cx = 0.5 * (p1.x() + p2.x()) - cot * 0.5 * (p2.y() - p1.y());
    const double cy = 0.5 * (p1.y() + p2.y()) + cot * 0.5 * (p2.x() - p1.x());

    arc.isArc = true;
    arc.center = QPointF(cx, cy);
    arc.radius = std::hypot(p1.x() - cx, p1.y() - cy);
    arc.startAngleDeg = std::atan2(p1.y() - cy, p1.x() - cx) * kRadToDeg;
    arc.sweepDeg = 4.0 * std::atan(bulge) * kRadToDeg;
    return arc;
}

} // namespace cad
