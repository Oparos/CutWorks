#pragma once

#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QString>

#include <memory>

namespace cad {

enum class EntityType
{
    Point,
    Line,
    Circle,
    Arc,
    Polyline
};

// Base class for a pure geometry entity. It knows nothing about QGraphicsItem,
// scenes or widgets — only its own geometry. Rendering, hit-testing and export
// are all derived from `path()`. Ownership lives in CadDocument.
class CadEntity
{
public:
    virtual ~CadEntity() = default;

    virtual EntityType type() const = 0;
    virtual std::unique_ptr<CadEntity> clone() const = 0;

    // The entity's geometry as a path — used to draw it, hit-test it and, later,
    // export it. Qt's QPainterPath is a plain value type (no GUI needed).
    virtual QPainterPath path() const = 0;

    virtual void translate(const QPointF& delta) = 0;
    virtual void rotate(const QPointF& pivot, double degrees) = 0;
    virtual void mirror(const QPointF& axisA, const QPointF& axisB) = 0;
    // Uniform scale about `pivot` by `factor` (> 0). Angles are preserved, so
    // circles stay circles and polyline bulges are unchanged.
    virtual void scale(const QPointF& pivot, double factor) = 0;

    QRectF bounds() const { return path().boundingRect(); }

    // Stable id assigned by the document; used by rendering and undo to refer to
    // this entity without holding a raw pointer.
    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    // The layer this entity belongs to (by name). Empty means "unassigned" — the
    // document stamps the active layer when the entity is first added. clone()
    // copies it, so copy/paste/array/mirror keep the layer.
    QString layer() const { return m_layer; }
    void setLayer(const QString& layer) { m_layer = layer; }

protected:
    // Copy the non-geometric attributes (currently just the layer) onto a fresh
    // clone. Each clone() calls this so those attributes are never lost.
    void cloneBaseInto(CadEntity& other) const { other.m_layer = m_layer; }

    int m_id = 0;
    QString m_layer;
};

} // namespace cad
