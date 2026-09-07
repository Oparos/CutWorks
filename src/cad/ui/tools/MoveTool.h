#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QVector>

class QGraphicsScene;
class QGraphicsPathItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Moves the current selection. Click a base point, then a destination (or type
// dx / dy). Select entities first (with the Select tool); the selection is
// captured when the base point is placed. Esc / right-click cancels.
class MoveTool : public CadTool
{
    Q_OBJECT

public:
    MoveTool(CadDocument* document, QUndoStack* undoStack,
             QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    void captureSelection();
    void commitMove(const QPointF& delta);
    void updateGhost();
    void reset();
    void clearGhost();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    bool m_hasBase = false;
    QPointF m_base;
    QPointF m_cursor;
    QVector<int> m_ids;  // selection captured at the base point
    QGraphicsPathItem* m_ghost = nullptr;
};

} // namespace cad
