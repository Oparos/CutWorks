#pragma once

#include "cad/ui/tools/CadTool.h"

class QGraphicsScene;
class QGraphicsPathItem;
class QPainterPath;
class QUndoStack;

namespace cad {

class CadDocument;

// Trim tool. Every other entity acts as an implicit cutting edge: hovering an
// entity highlights (in red) the piece between the two cuts around the cursor,
// and clicking removes that piece. An entity with no cuts is removed whole.
// The surviving pieces are added back as new entities (a line/arc may split in
// two; a circle becomes the remaining arc). Right-click / Esc do nothing here —
// the tool is stateless between clicks.
class TrimTool : public CadTool
{
    Q_OBJECT

public:
    TrimTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
             QObject* parent = nullptr);

    void onMouseMove(const QPointF& scenePos) override;
    void onMousePress(const QPointF& scenePos) override;
    void deactivate() override;

private:
    void showPreview(const QPainterPath& removed);
    void clearPreview();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;
    QGraphicsPathItem* m_preview = nullptr;
};

} // namespace cad
