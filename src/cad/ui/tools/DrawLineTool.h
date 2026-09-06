#pragma once

#include "cad/ui/tools/CadTool.h"

class QGraphicsScene;
class QGraphicsLineItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Draws line segments. First click sets the start; the second click (or a typed
// Length + Angle) sets the end and commits an AddEntityCommand. Keeps drawing
// consecutive lines until another tool is chosen. Esc cancels the current line.
class DrawLineTool : public CadTool
{
    Q_OBJECT

public:
    DrawLineTool(CadDocument* document, QUndoStack* undoStack,
                 QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    void commitLine(const QPointF& end);
    void cancelChain();
    void clearPreview();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    bool m_hasStart = false;
    QPointF m_start;
    QPointF m_cursor;
    QGraphicsLineItem* m_preview = nullptr;
};

} // namespace cad
