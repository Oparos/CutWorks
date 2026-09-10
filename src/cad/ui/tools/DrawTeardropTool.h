#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QPointF>

class QGraphicsScene;
class QGraphicsPathItem;
class QPainterPath;
class QUndoStack;

namespace cad {

class CadDocument;

// Forms a teardrop contour from two circles: their two outer arcs joined by the
// external tangents, as one closed bulge polyline. Click the first circle, then
// the second; the two source circles are replaced by the contour (one undoable
// step). A green preview shows the result before the second click. If the
// circles have no external tangent (one inside the other), nothing happens.
// Right-click / Esc starts over.
class DrawTeardropTool : public CadTool
{
    Q_OBJECT

public:
    DrawTeardropTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
                     QObject* parent = nullptr);

    void onMouseMove(const QPointF& scenePos) override;
    void onMousePress(const QPointF& scenePos) override;
    void onCancel() override;
    void deactivate() override;

private:
    struct Circle
    {
        int id = -1;
        QPointF center;
        double radius = 0.0;
    };

    Circle circleAt(const QPointF& scenePos) const;
    void showPreview(const QPainterPath& path);
    void clearPreview();
    void reset();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;
    QGraphicsPathItem* m_preview = nullptr;
    Circle m_first;
};

} // namespace cad
