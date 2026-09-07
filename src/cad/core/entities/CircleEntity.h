#pragma once

#include "cad/core/entities/CadEntity.h"

namespace cad {

// A full circle defined by a center and radius (millimeters, Y up).
class CircleEntity : public CadEntity
{
public:
    CircleEntity(const QPointF& center, double radius);

    EntityType type() const override { return EntityType::Circle; }
    std::unique_ptr<CadEntity> clone() const override;
    QPainterPath path() const override;
    void translate(const QPointF& delta) override;
    void rotate(const QPointF& pivot, double degrees) override;
    void mirror(const QPointF& axisA, const QPointF& axisB) override;

    QPointF center() const { return m_center; }
    double radius() const { return m_radius; }
    void setCenter(const QPointF& center) { m_center = center; }
    void setRadius(double radius) { m_radius = radius; }

private:
    QPointF m_center;
    double m_radius;
};

} // namespace cad
