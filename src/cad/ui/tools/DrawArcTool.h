#pragma once

#include "cad/ui/tools/CadTool.h"

class QGraphicsScene;
class QGraphicsPathItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Draws an arc in center → start → end order: click the center, click the start
// point (sets radius + start angle), then click the end (sets the sweep, CCW).
// Esc / right-click cancels. (A 3-point mode can be added later.)
class DrawArcTool : public CadTool
{
    Q_OBJECT

public:
    DrawArcTool(CadDocument* document, QUndoStack* undoStack,
                QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

private:
    void updatePreview();
    void reset();
    void clearPreview();

    enum class Stage { Center, Start, End };

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    Stage m_stage = Stage::Center;
    QPointF m_center;
    double m_radius = 0.0;
    double m_startAngle = 0.0;
    // Accumulated signed sweep (deg) as the cursor is moved around, so arcs can
    // grow past 180 deg without jumping to the short side.
    double m_sweep = 0.0;
    double m_lastAngle = 0.0;
    QPointF m_cursor;
    QGraphicsPathItem* m_preview = nullptr;
};

} // namespace cad
