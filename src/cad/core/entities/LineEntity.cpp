#include "cad/core/entities/LineEntity.h"

#include "cad/core/geometry/Geometry.h"

#include <QTransform>

namespace cad {

LineEntity::LineEntity(const QPointF& p1, const QPointF& p2)
    : m_p1(p1)
    , m_p2(p2)
{
}

std::unique_ptr<CadEntity> LineEntity::clone() const
{
    return std::make_unique<LineEntity>(m_p1, m_p2);
}

QPainterPath LineEntity::path() const
{
    QPainterPath p;
    p.moveTo(m_p1);
    p.lineTo(m_p2);
    return p;
}

void LineEntity::translate(const QPointF& delta)
{
    m_p1 += delta;
    m_p2 += delta;
}

void LineEntity::rotate(const QPointF& pivot, double degrees)
{
    const QTransform t = QTransform()
                             .translate(pivot.x(), pivot.y())
                             .rotate(degrees)
                             .translate(-pivot.x(), -pivot.y());
    m_p1 = t.map(m_p1);
    m_p2 = t.map(m_p2);
}

void LineEntity::mirror(const QPointF& axisA, const QPointF& axisB)
{
    m_p1 = reflectPoint(m_p1, axisA, axisB);
    m_p2 = reflectPoint(m_p2, axisA, axisB);
}

} // namespace cad
