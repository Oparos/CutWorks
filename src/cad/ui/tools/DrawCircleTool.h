#pragma once

#include "cad/ui/tools/CadTool.h"

class QGraphicsScene;
class QGraphicsEllipseItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Draws circles: first click sets the center, the second click (or a typed
// Radius) sets the radius and commits. Esc / right-click cancels.
class DrawCircleTool : public CadTool
{
    Q_OBJECT

public:
    DrawCircleTool(CadDocument* document, QUndoStack* undoStack,
                   QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    void commitCircle(double radius);
    void reset();
    void clearPreview();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    bool m_hasCenter = false;
    QPointF m_center;
    QPointF m_cursor;
    QGraphicsEllipseItem* m_preview = nullptr;
};

} // namespace cad
