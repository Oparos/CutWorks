#include "cad/ui/render/EntityItem.h"

#include "cad/core/entities/CadEntity.h"

#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>

EntityItem::EntityItem(cad::CadEntity* entity)
    : m_entity(entity)
{
    setFlag(ItemIsSelectable, true);
}

void EntityItem::refresh()
{
    prepareGeometryChange();
    update();
}

QRectF EntityItem::boundingRect() const
{
    // A little margin so the (cosmetic) stroke is never clipped.
    return m_entity->bounds().adjusted(-1, -1, 1, 1);
}

QPainterPath EntityItem::shape() const
{
    // Give the thin geometry some width so it is easy to click.
    QPainterPathStroker stroker;
    stroker.setWidth(1.5);
    return stroker.createStroke(m_entity->path());
}

void EntityItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    const bool selected = (option->state & QStyle::State_Selected);

    QPen pen(selected ? QColor(0xff, 0x9c, 0x33) : QColor(0xe6, 0xe6, 0xe6));
    pen.setCosmetic(true);  // constant on-screen width regardless of zoom
    pen.setWidth(selected ? 2 : 1);

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_entity->path());
}
