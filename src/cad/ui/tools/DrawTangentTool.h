#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QLineF>
#include <QPointF>
#include <QVector>

class QGraphicsScene;
class QGraphicsPathItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Draws the two external tangent lines shared by two circles (or arcs, taken as
// their full circle) — the skeleton of a teardrop / slot. Click the first
// circle, then the second; a green preview shows the tangents before the second
// click commits them. Right-click / Esc starts over.
class DrawTangentTool : public CadTool
{
    Q_OBJECT

public:
    DrawTangentTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
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
    void showPreview(const QVector<QLineF>& lines);
    void clearPreview();
    void reset();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;
    QGraphicsPathItem* m_preview = nullptr;
    Circle m_first;
};

} // namespace cad
