#include "cad/core/entities/PointEntity.h"

#include "cad/core/geometry/Geometry.h"

#include <QTransform>

namespace cad {

namespace {
constexpr double kMarkerHalf = 3.0;  // cross half-length (mm)
}

PointEntity::PointEntity(const QPointF& pos)
    : m_pos(pos)
{
}

std::unique_ptr<CadEntity> PointEntity::clone() const
{
    auto copy = std::make_unique<PointEntity>(m_pos);
    cloneBaseInto(*copy);
    return copy;
}

QPainterPath PointEntity::path() const
{
    QPainterPath p;
    p.moveTo(m_pos.x() - kMarkerHalf, m_pos.y());
    p.lineTo(m_pos.x() + kMarkerHalf, m_pos.y());
    p.moveTo(m_pos.x(), m_pos.y() - kMarkerHalf);
    p.lineTo(m_pos.x(), m_pos.y() + kMarkerHalf);
    return p;
}

void PointEntity::translate(const QPointF& delta)
{
    m_pos += delta;
}

void PointEntity::rotate(const QPointF& pivot, double degrees)
{
    const QTransform t = QTransform()
                             .translate(pivot.x(), pivot.y())
                             .rotate(degrees)
                             .translate(-pivot.x(), -pivot.y());
    m_pos = t.map(m_pos);
}

void PointEntity::mirror(const QPointF& axisA, const QPointF& axisB)
{
    m_pos = reflectPoint(m_pos, axisA, axisB);
}

void PointEntity::scale(const QPointF& pivot, double factor)
{
    m_pos = pivot + (m_pos - pivot) * factor;
}

} // namespace cad
