#pragma once

#include <QGraphicsItem>

namespace cad {
class CadDocument;
class CadEntity;
}

// A thin QGraphicsItem that renders one document entity, looked up by id. It
// does not cache the entity pointer — it fetches it from the document each time —
// so an edit that replaces the entity object (ModifyEntityCommand) never leaves
// a dangling pointer here. One item per entity, kept in sync by CadScene.
class EntityItem : public QGraphicsItem
{
public:
    EntityItem(cad::CadDocument* document, int id);

    void prepareForChange();  // before the entity's geometry changes
    void refresh();           // after: repaint
    int entityId() const { return m_id; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    cad::CadEntity* entity() const;

    cad::CadDocument* m_document;
    int m_id;
};
