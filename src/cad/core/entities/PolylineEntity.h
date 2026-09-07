#pragma once

#include "cad/core/entities/CadEntity.h"

#include <QVector>

namespace cad {

// One polyline vertex. `bulge` encodes an arc to the next vertex (0 = straight
// segment, the DXF convention). Bulge rendering is added when a producer needs
// it (slot tool / DXF import); today's tools create straight segments.
struct PolyVertex
{
    QPointF pos;
    double bulge = 0.0;
};

// A connected chain of segments, optionally closed. The basis for polylines,
// rectangles, regular polygons and slots.
class PolylineEntity : public CadEntity
{
public:
    PolylineEntity(const QVector<QPointF>& points, bool closed = false);
    PolylineEntity(const QVector<PolyVertex>& vertices, bool closed = false);

    EntityType type() const override { return EntityType::Polyline; }
    std::unique_ptr<CadEntity> clone() const override;
    QPainterPath path() const override;
    void translate(const QPointF& delta) override;
    void rotate(const QPointF& pivot, double degrees) override;
    void mirror(const QPointF& axisA, const QPointF& axisB) override;

    const QVector<PolyVertex>& vertices() const { return m_vertices; }
    bool isClosed() const { return m_closed; }

private:
    QVector<PolyVertex> m_vertices;
    bool m_closed;
};

} // namespace cad
