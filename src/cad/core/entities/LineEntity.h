#pragma once

#include "cad/core/entities/CadEntity.h"

namespace cad {

// A straight line segment between two points (millimeters, Y up).
class LineEntity : public CadEntity
{
public:
    LineEntity(const QPointF& p1, const QPointF& p2);

    EntityType type() const override { return EntityType::Line; }
    std::unique_ptr<CadEntity> clone() const override;
    QPainterPath path() const override;
    void translate(const QPointF& delta) override;
    void rotate(const QPointF& pivot, double degrees) override;
    void mirror(const QPointF& axisA, const QPointF& axisB) override;
    void scale(const QPointF& pivot, double factor) override;

    QPointF p1() const { return m_p1; }
    QPointF p2() const { return m_p2; }
    void setP1(const QPointF& p1) { m_p1 = p1; }
    void setP2(const QPointF& p2) { m_p2 = p2; }

private:
    QPointF m_p1;
    QPointF m_p2;
};

} // namespace cad
