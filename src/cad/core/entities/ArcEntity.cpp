#include "cad/core/entities/ArcEntity.h"

#include <QTransform>

#include <algorithm>
#include <cmath>

namespace cad {

namespace {
constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
}

ArcEntity::ArcEntity(const QPointF& center, double radius, double startAngleDeg, double sweepDeg)
    : m_center(center)
    , m_radius(radius)
    , m_startAngle(startAngleDeg)
    , m_sweepAngle(sweepDeg)
{
}

std::unique_ptr<CadEntity> ArcEntity::clone() const
{
    return std::make_unique<ArcEntity>(m_center, m_radius, m_startAngle, m_sweepAngle);
}

QPainterPath ArcEntity::path() const
{
    // Sample the arc as short segments in our own Y-up coordinates. We do NOT use
    // QPainterPath::arcTo here: its angle convention assumes Qt's default Y-down
    // space, so through our Y-up view it would draw the arc mirrored vertically.
    QPainterPath p;
    const double a0 = m_startAngle * kDegToRad;
    const double sweep = m_sweepAngle * kDegToRad;
    const int steps = std::max(2, static_cast<int>(std::ceil(std::abs(m_sweepAngle) / 2.0)));

    p.moveTo(m_center.x() + m_radius * std::cos(a0),
             m_center.y() + m_radius * std::sin(a0));
    for (int i = 1; i <= steps; ++i) {
        const double a = a0 + sweep * (static_cast<double>(i) / steps);
        p.lineTo(m_center.x() + m_radius * std::cos(a),
                 m_center.y() + m_radius * std::sin(a));
    }
    return p;
}

void ArcEntity::translate(const QPointF& delta)
{
    m_center += delta;
}

void ArcEntity::rotate(const QPointF& pivot, double degrees)
{
    const QTransform t = QTransform()
                             .translate(pivot.x(), pivot.y())
                             .rotate(degrees)
                             .translate(-pivot.x(), -pivot.y());
    m_center = t.map(m_center);
    m_startAngle += degrees;
}

} // namespace cad
