#pragma once

#include <QPointF>
#include <QVector>

namespace cad {

class CadEntity;

// Geometric intersection queries used by editing tools (trim, extend and later
// fillet/chamfer/offset). Pure domain math: it works on entities and value
// types only, with no dependency on QGraphicsItem, scenes or widgets.
//
// Conventions match the rest of the CAD core: coordinates are millimeters, Y is
// up, and angles are degrees measured counter-clockwise from +X, so a point at
// angle a on a circle is center + radius * (cos a, sin a).
namespace geom {

// --- angle helpers (degrees, CCW from +X) ---

// Wrap an angle into [0, 360).
double normalizeDeg(double deg);

// Angle of point p as seen from center, in [0, 360).
double angleAtDeg(const QPointF& center, const QPointF& p);

// True if angleDeg lies on the arc that starts at startDeg and spans sweepDeg
// (positive = CCW, negative = CW). A full circle (|sweep| >= 360) contains any
// angle. Both endpoints are treated as on the arc.
bool angleWithinSweep(double angleDeg, double startDeg, double sweepDeg);

// --- intersection queries ---

// All points where entities a and b cross, both taken exactly as drawn (finite
// segments, real arc sweeps). Order is unspecified; near-duplicate points are
// merged. Points/empty entities simply yield no intersections.
QVector<QPointF> intersect(const CadEntity& a, const CadEntity& b);

// Intersections of the *infinite* line through p1/p2 with entity e (as drawn).
// Used by Extend: the picked line is projected to its supporting line and shot
// against the other geometry.
QVector<QPointF> supportLineVsEntity(const QPointF& p1, const QPointF& p2,
                                     const CadEntity& e);

// Intersections of the *full* circle (center, radius) with entity e (as drawn).
// Used by Extend on arcs.
QVector<QPointF> supportCircleVsEntity(const QPointF& center, double radius,
                                       const CadEntity& e);

// Shortest distance from point p to entity e (as drawn). Used for precise
// hit-testing / picking (choose the entity nearest the cursor) and later
// snapping.
double distanceToEntity(const CadEntity& e, const QPointF& p);

} // namespace geom
} // namespace cad
