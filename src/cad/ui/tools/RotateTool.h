#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QVector>

class QGraphicsScene;
class QGraphicsPathItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Rotates the current selection. Click a pivot, then a point giving the angle
// (or type Angle). Select entities first; the selection is captured at the
// pivot. Esc / right-click cancels.
class RotateTool : public CadTool
{
    Q_OBJECT

public:
    RotateTool(CadDocument* document, QUndoStack* undoStack,
               QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    double currentAngle() const;  // degrees, pivot -> cursor
    void captureSelection();
    void commitRotate(double angleDeg);
    void updateGhost();
    void reset();
    void clearGhost();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    bool m_hasPivot = false;
    QPointF m_pivot;
    QPointF m_cursor;
    QVector<int> m_ids;
    QGraphicsPathItem* m_ghost = nullptr;
};

} // namespace cad
