#pragma once

#include "cad/ui/tools/CadTool.h"

class QGraphicsScene;
class QGraphicsRectItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Draws an axis-aligned rectangle as a closed polyline: first click sets one
// corner, the second click (or typed Width + Height) sets the opposite corner.
class DrawRectangleTool : public CadTool
{
    Q_OBJECT

public:
    DrawRectangleTool(CadDocument* document, QUndoStack* undoStack,
                      QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    void commitRect(const QPointF& corner2);
    void reset();
    void clearPreview();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    bool m_hasCorner1 = false;
    QPointF m_corner1;
    QPointF m_cursor;
    QGraphicsRectItem* m_preview = nullptr;
};

} // namespace cad
