#include "cad/core/entities/CircleEntity.h"

#include "cad/core/geometry/Geometry.h"

#include <QTransform>

namespace cad {

CircleEntity::CircleEntity(const QPointF& center, double radius)
    : m_center(center)
    , m_radius(radius)
{
}

std::unique_ptr<CadEntity> CircleEntity::clone() const
{
    return std::make_unique<CircleEntity>(m_center, m_radius);
}

QPainterPath CircleEntity::path() const
{
    QPainterPath p;
    p.addEllipse(m_center, m_radius, m_radius);
    return p;
}

void CircleEntity::translate(const QPointF& delta)
{
    m_center += delta;
}

void CircleEntity::rotate(const QPointF& pivot, double degrees)
{
    // Rotating a circle only moves its center around the pivot.
    const QTransform t = QTransform()
                             .translate(pivot.x(), pivot.y())
                             .rotate(degrees)
                             .translate(-pivot.x(), -pivot.y());
    m_center = t.map(m_center);
}

void CircleEntity::mirror(const QPointF& axisA, const QPointF& axisB)
{
    m_center = reflectPoint(m_center, axisA, axisB);  // radius is unchanged
}

} // namespace cad
