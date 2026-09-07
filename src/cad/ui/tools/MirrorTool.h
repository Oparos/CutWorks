#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QVector>

class QGraphicsScene;
class QGraphicsPathItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Mirrors the current selection across an axis defined by two clicked points,
// adding a reflected copy (the original is kept — handy for nesting). Select
// entities first. Esc / right-click cancels.
class MirrorTool : public CadTool
{
    Q_OBJECT

public:
    MirrorTool(CadDocument* document, QUndoStack* undoStack,
               QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

private:
    void captureSelection();
    void commitMirror(const QPointF& axisB);
    void updateGhost();
    void reset();
    void clearGhost();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    bool m_hasAxisStart = false;
    QPointF m_axisA;
    QPointF m_cursor;
    QVector<int> m_ids;
    QGraphicsPathItem* m_ghost = nullptr;
};

} // namespace cad
