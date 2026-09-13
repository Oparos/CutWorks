#pragma once

#include <QGraphicsScene>
#include <QString>

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

    // The minor-grid spacing (scene units) for a given view scale. Shared by the
    // background grid and grid snapping so both use the same, zoom-adaptive step.
    static double minorGridStep(double sceneScale);

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    void onEntityAdded(int id);
    void onEntityRemoved(int id);
    void onEntityAboutToChange(int id);
    void onEntityChanged(int id);
    void onLayerChanged(const QString& name);  // color/visibility of one layer
    void onLayersChanged();                     // layer set changed
    void applyLayerVisibility(int id, EntityItem* item);

    cad::CadDocument* m_document;
    std::unordered_map<int, EntityItem*> m_items;
};
