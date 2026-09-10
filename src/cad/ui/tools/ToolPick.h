#pragma once

#include <QPointF>

class QGraphicsScene;
class EntityItem;

namespace cad {

// Pick the topmost entity within a constant *screen* radius of scenePos. Unlike
// hitting an entity's own (thin, mm-sized) shape, this keeps the same easy-to-
// hit tolerance at any zoom, by converting `pixelTolerance` through the scene's
// view scale. Returns nullptr if nothing is close enough. Shared by the editing
// and construction tools so their picking feels the same.
EntityItem* pickEntityItem(QGraphicsScene* scene, const QPointF& scenePos,
                           double pixelTolerance = 6.0);

} // namespace cad
