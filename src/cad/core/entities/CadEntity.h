#pragma once

#include <QPainterPath>
#include <QPointF>
#include <QRectF>

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

    QRectF bounds() const { return path().boundingRect(); }

    // Stable id assigned by the document; used by rendering and undo to refer to
    // this entity without holding a raw pointer.
    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

protected:
    int m_id = 0;
};

} // namespace cad
