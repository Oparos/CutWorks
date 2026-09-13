#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QPointF>

class QGraphicsScene;
class QGraphicsPathItem;
class QPainterPath;
class QUndoStack;

namespace cad {

class CadDocument;

// Creates a parallel copy of an entity at a fixed distance. Set the distance in
// the input bar (sticky), click the entity, then click the side you want the
// copy on — a live preview follows the cursor's side. Works on lines, circles,
// arcs and straight polylines (rectangles/polygons). Bulge polylines (slots,
// teardrops) are not offset yet. Esc / right-click starts over.
class OffsetTool : public CadTool
{
    Q_OBJECT

public:
    OffsetTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
               QObject* parent = nullptr);

    void onMouseMove(const QPointF& scenePos) override;
    void onMousePress(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    void showPreview(const QPainterPath& path);
    void clearPreview();
    void reset();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;
    double m_distance = 5.0;
    int m_entityId = -1;
    QGraphicsPathItem* m_preview = nullptr;
};

} // namespace cad
