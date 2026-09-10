#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QPointF>

class QGraphicsScene;
class QGraphicsPathItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Draws an obround (rounded slot) as one closed bulge polyline: two straight
// sides capped by exact semicircles. Click the first end center, then the second
// (this fixes the axis and length), then move the mouse to set the radius
// (perpendicular half-width) and click — or type Radius and press Enter.
// Esc / right-click cancels.
class DrawSlotTool : public CadTool
{
    Q_OBJECT

public:
    DrawSlotTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
                 QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    double currentRadius() const;
    void updatePreview();
    void commit(double radius);
    void reset();
    void clearPreview();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    int m_stage = 0;  // 0: pick A, 1: pick B (axis), 2: set radius
    QPointF m_centerA;
    QPointF m_centerB;
    QPointF m_cursor;
    QGraphicsPathItem* m_preview = nullptr;
};

} // namespace cad
