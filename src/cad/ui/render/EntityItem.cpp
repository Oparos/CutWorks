#include "cad/ui/render/EntityItem.h"

#include "cad/core/CadDocument.h"
#include "cad/core/entities/CadEntity.h"
#include "cad/core/geometry/Intersections.h"

#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>

#include <limits>

EntityItem::EntityItem(cad::CadDocument* document, int id)
    : m_document(document)
    , m_id(id)
{
    setFlag(ItemIsSelectable, true);
}

cad::CadEntity* EntityItem::entity() const
{
    return m_document->entity(m_id);
}

void EntityItem::prepareForChange()
{
    prepareGeometryChange();
}

void EntityItem::refresh()
{
    update();
}

double EntityItem::distanceTo(const QPointF& scenePos) const
{
    const cad::CadEntity* e = entity();
    return e ? cad::geom::distanceToEntity(*e, scenePos) : std::numeric_limits<double>::max();
}

QRectF EntityItem::boundingRect() const
{
    const cad::CadEntity* e = entity();
    return e ? e->bounds().adjusted(-1, -1, 1, 1) : QRectF();
}

QPainterPath EntityItem::shape() const
{
    const cad::CadEntity* e = entity();
    if (!e) {
        return QPainterPath();
    }
    QPainterPathStroker stroker;
    stroker.setWidth(1.5);  // give the thin geometry a clickable width
    return stroker.createStroke(e->path());
}

void EntityItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    const cad::CadEntity* e = entity();
    if (!e) {
        return;
    }
    const bool selected = (option->state & QStyle::State_Selected);

    const QColor color = selected ? QColor(0xff, 0x9c, 0x33) : m_document->layers().colorOf(e->layer());
    QPen pen(color);
    pen.setCosmetic(true);  // constant on-screen width regardless of zoom
    pen.setWidth(selected ? 2 : 1);

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(e->path());
}
