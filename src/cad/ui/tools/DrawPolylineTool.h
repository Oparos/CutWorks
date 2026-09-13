#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QVector>

class QGraphicsScene;
class QGraphicsPathItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Draws a polyline: each click adds a vertex (or type Length + Angle). Enter or
// right-click finishes it open; 'C' closes it; Esc discards.
class DrawPolylineTool : public CadTool
{
    Q_OBJECT

public:
    DrawPolylineTool(CadDocument* document, QUndoStack* undoStack,
                     QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;
    std::optional<QPointF> referencePoint() const override
    {
        return m_vertices.isEmpty() ? std::nullopt : std::optional<QPointF>(m_vertices.last());
    }

private:
    void addVertex(const QPointF& pos);
    void finish(bool closed);
    void discard();
    void updatePreview();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    QVector<QPointF> m_vertices;
    QPointF m_cursor;
    QGraphicsPathItem* m_preview = nullptr;
};

} // namespace cad
