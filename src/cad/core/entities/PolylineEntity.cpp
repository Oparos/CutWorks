#include "cad/core/entities/PolylineEntity.h"

#include "cad/core/geometry/Geometry.h"

#include <QTransform>

#include <algorithm>
#include <cmath>

namespace cad {

namespace {

// Append a bulge arc to the path, sampled as short segments in our Y-up space
// (like ArcEntity — QPainterPath::arcTo would mirror it). The path is assumed to
// already sit at the arc's start point, so sampling begins after it.
void appendArc(QPainterPath& path, const BulgeArc& arc)
{
    constexpr double kDegToRad = 0.017453292519943295;
    const double a0 = arc.startAngleDeg * kDegToRad;
    const double sweep = arc.sweepDeg * kDegToRad;
    const int steps = std::max(2, static_cast<int>(std::ceil(std::abs(arc.sweepDeg) / 2.0)));
    for (int i = 1; i <= steps; ++i) {
        const double a = a0 + sweep * (static_cast<double>(i) / steps);
        path.lineTo(arc.center.x() + arc.radius * std::cos(a),
                    arc.center.y() + arc.radius * std::sin(a));
    }
}

} // namespace

PolylineEntity::PolylineEntity(const QVector<QPointF>& points, bool closed)
    : m_closed(closed)
{
    m_vertices.reserve(points.size());
    for (const QPointF& p : points) {
        m_vertices.push_back({p, 0.0});
    }
}

PolylineEntity::PolylineEntity(const QVector<PolyVertex>& vertices, bool closed)
    : m_vertices(vertices)
    , m_closed(closed)
{
}

std::unique_ptr<CadEntity> PolylineEntity::clone() const
{
    auto copy = std::make_unique<PolylineEntity>(m_vertices, m_closed);
    cloneBaseInto(*copy);
    return copy;
}

QPainterPath PolylineEntity::path() const
{
    QPainterPath p;
    if (m_vertices.isEmpty()) {
        return p;
    }

    const int n = m_vertices.size();
    const int segments = m_closed ? n : n - 1;
    p.moveTo(m_vertices.first().pos);
    for (int i = 0; i < segments; ++i) {
        const PolyVertex& v = m_vertices.at(i);
        const QPointF& next = m_vertices.at((i + 1) % n).pos;
        const BulgeArc arc = bulgeToArc(v.pos, next, v.bulge);
        if (arc.isArc) {
            appendArc(p, arc);
        }
        else {
            p.lineTo(next);
        }
    }
    if (m_closed) {
        p.closeSubpath();
    }
    return p;
}

void PolylineEntity::translate(const QPointF& delta)
{
    for (PolyVertex& v : m_vertices) {
        v.pos += delta;
    }
}

void PolylineEntity::rotate(const QPointF& pivot, double degrees)
{
    const QTransform t = QTransform()
                             .translate(pivot.x(), pivot.y())
                             .rotate(degrees)
                             .translate(-pivot.x(), -pivot.y());
    for (PolyVertex& v : m_vertices) {
        v.pos = t.map(v.pos);
    }
}

void PolylineEntity::mirror(const QPointF& axisA, const QPointF& axisB)
{
    for (PolyVertex& v : m_vertices) {
        v.pos = reflectPoint(v.pos, axisA, axisB);
        v.bulge = -v.bulge;  // reflection reverses arc direction
    }
}

void PolylineEntity::scale(const QPointF& pivot, double factor)
{
    for (PolyVertex& v : m_vertices) {
        v.pos = pivot + (v.pos - pivot) * factor;  // bulge unchanged (angle preserved)
    }
}

} // namespace cad
