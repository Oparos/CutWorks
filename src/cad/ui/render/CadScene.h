#pragma once

#include <QGraphicsScene>

#include <unordered_map>

class EntityItem;

namespace cad {
class CadDocument;
}

// Renders a CadDocument. It observes the document's signals and keeps one
// EntityItem per entity in sync. It owns the items (Qt parent/child); the
// document owns the entities. Also draws the background grid.
class CadScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit CadScene(cad::CadDocument* document, QObject* parent = nullptr);

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    void onEntityAdded(int id);
    void onEntityRemoved(int id);
    void onEntityChanged(int id);

    cad::CadDocument* m_document;
    std::unordered_map<int, EntityItem*> m_items;
};
