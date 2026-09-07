#pragma once

#include "cad/ui/tools/CadTool.h"

class QGraphicsScene;
class QGraphicsRectItem;

namespace cad {

// Selection tool. Click an entity to select it; drag a box on empty space to
// select several. Dragging left→right is a "window" (only fully-enclosed
// entities, blue box); right→left is a "crossing" (anything touched, green
// dashed box) — the usual CAD convention.
class SelectTool : public CadTool
{
    Q_OBJECT

public:
    explicit SelectTool(QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onMouseRelease(const QPointF& scenePos) override;
    void onCancel() override;
    void deactivate() override;

private:
    void clearBand();

    QGraphicsScene* m_scene;
    bool m_banding = false;
    bool m_additive = false;  // Ctrl held: add to the current selection
    QPointF m_pressPos;
    QGraphicsRectItem* m_band = nullptr;
};

} // namespace cad
