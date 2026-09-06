#pragma once

#include "cad/core/entities/CadEntity.h"

namespace cad {

// A circular arc: center + radius, from startAngle sweeping sweepAngle degrees
// (positive = counter-clockwise). Angles in degrees, Y up.
class ArcEntity : public CadEntity
{
public:
    ArcEntity(const QPointF& center, double radius, double startAngleDeg, double sweepDeg);

    EntityType type() const override { return EntityType::Arc; }
    std::unique_ptr<CadEntity> clone() const override;
    QPainterPath path() const override;
    void translate(const QPointF& delta) override;
    void rotate(const QPointF& pivot, double degrees) override;

    QPointF center() const { return m_center; }
    double radius() const { return m_radius; }
    double startAngle() const { return m_startAngle; }
    double sweepAngle() const { return m_sweepAngle; }

private:
    QPointF m_center;
    double m_radius;
    double m_startAngle;
    double m_sweepAngle;
};

} // namespace cad
