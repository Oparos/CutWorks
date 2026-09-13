#pragma once

#include "cad/core/entities/CadEntity.h"

namespace cad {

// A single point / marker at a position (millimeters, Y up). Rendered as a small
// cross. Useful e.g. as a pierce/reference point.
class PointEntity : public CadEntity
{
public:
    explicit PointEntity(const QPointF& pos);

    EntityType type() const override { return EntityType::Point; }
    std::unique_ptr<CadEntity> clone() const override;
    QPainterPath path() const override;
    void translate(const QPointF& delta) override;
    void rotate(const QPointF& pivot, double degrees) override;
    void mirror(const QPointF& axisA, const QPointF& axisB) override;
    void scale(const QPointF& pivot, double factor) override;

    QPointF pos() const { return m_pos; }
    void setPos(const QPointF& pos) { m_pos = pos; }

private:
    QPointF m_pos;
};

} // namespace cad
