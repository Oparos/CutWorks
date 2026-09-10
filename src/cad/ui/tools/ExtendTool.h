#pragma once

#include "cad/ui/tools/CadTool.h"

class QGraphicsScene;
class QGraphicsPathItem;
class QPainterPath;
class QUndoStack;

namespace cad {

class CadDocument;

// Extend tool. Hovering an open entity (line, arc or open polyline) near one of
// its ends previews (in cyan) that end stretched along its own direction until
// it meets the next entity; clicking commits the extension. The end nearer the
// cursor is the one that moves. Entities with nothing ahead of them, and closed
// shapes (circle, closed polyline), are left unchanged.
class ExtendTool : public CadTool
{
    Q_OBJECT

public:
    ExtendTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
               QObject* parent = nullptr);

    void onMouseMove(const QPointF& scenePos) override;
    void onMousePress(const QPointF& scenePos) override;
    void deactivate() override;

private:
    void showPreview(const QPainterPath& result);
    void clearPreview();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;
    QGraphicsPathItem* m_preview = nullptr;
};

} // namespace cad
