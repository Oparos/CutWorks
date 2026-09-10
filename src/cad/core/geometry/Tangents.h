#pragma once

#include <QLineF>
#include <QPointF>
#include <QVector>

namespace cad {
namespace geom {

// Common tangent segments between two circles. Each returned segment runs from
// the touch point on the first circle to the touch point on the second, so it
// is exactly the drawable tangent line. Coordinates are millimeters, Y up.
//
// Both functions return the two tangents of their family, or an empty list when
// that family does not exist:
//   external — the pair that does not pass between the circles; empty when the
//     centers coincide or one circle lies inside the other (|rA - rB| > dist).
//   internal — the pair that crosses between the circles; empty unless the
//     circles are fully separate (dist >= rA + rB).
QVector<QLineF> externalTangents(const QPointF& cA, double rA, const QPointF& cB, double rB);
QVector<QLineF> internalTangents(const QPointF& cA, double rA, const QPointF& cB, double rB);

} // namespace geom
} // namespace cad
