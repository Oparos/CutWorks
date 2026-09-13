#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QVector>

class QGraphicsScene;
class QGraphicsPathItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Uniformly scales the current selection about a base point. Select entities
// first, then click the base point. Either type a Factor and press Enter, or
// click a reference point (its distance from the base becomes "1×") and then a
// target point — the factor is target-distance / reference-distance. Esc /
// right-click cancels.
class ScaleTool : public CadTool
{
    Q_OBJECT

public:
    ScaleTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
              QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    void captureSelection();
    double currentFactor() const;
    void commitScale(double factor);
    void updateGhost(double factor);
    void reset();
    void clearGhost();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    bool m_hasBase = false;
    bool m_hasReference = false;
    QPointF m_base;
    double m_referenceLen = 0.0;
    QPointF m_cursor;
    QVector<int> m_ids;
    QGraphicsPathItem* m_ghost = nullptr;
};

} // namespace cad
