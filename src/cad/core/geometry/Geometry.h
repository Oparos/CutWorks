#pragma once

#include <QPointF>

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

} // namespace cad
