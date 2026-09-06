#pragma once

#include <QGraphicsItem>

namespace cad {
class CadEntity;
}

// A thin QGraphicsItem that renders one domain entity. It does NOT own the
// entity (the document does); it only reads its geometry to draw and hit-test.
// One item per entity, kept in sync by CadScene.
class EntityItem : public QGraphicsItem
{
public:
    explicit EntityItem(cad::CadEntity* entity);

    // Re-read geometry after the entity changed.
    void refresh();

    // Id of the entity this item renders (used to map selection back to the document).
    int entityId() const;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    cad::CadEntity* m_entity;
};
