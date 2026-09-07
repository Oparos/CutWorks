#include "cad/core/entities/PolylineEntity.h"

#include "cad/core/geometry/Geometry.h"

#include <QTransform>

namespace cad {

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
    return std::make_unique<PolylineEntity>(m_vertices, m_closed);
}

QPainterPath PolylineEntity::path() const
{
    QPainterPath p;
    if (m_vertices.isEmpty()) {
        return p;
    }

    // TODO: honor per-vertex bulge (render arcs) once the slot tool / DXF import
    // produce curved polylines. Straight segments for now.
    p.moveTo(m_vertices.first().pos);
    for (int i = 1; i < m_vertices.size(); ++i) {
        p.lineTo(m_vertices.at(i).pos);
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

} // namespace cad
