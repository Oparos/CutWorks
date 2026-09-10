#include "cad/ui/tools/ToolPick.h"

#include "cad/ui/render/EntityItem.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QRectF>
#include <QTransform>

#include <cmath>
#include <limits>

namespace cad {

EntityItem* pickEntityItem(QGraphicsScene* scene, const QPointF& scenePos, double pixelTolerance)
{
    // Convert the pixel tolerance to scene units via the current view scale, so
    // the hit area is a constant size on screen regardless of zoom.
    double scale = 1.0;
    QTransform deviceTransform;
    if (!scene->views().isEmpty()) {
        const QGraphicsView* view = scene->views().first();
        deviceTransform = view->viewportTransform();
        const double m = std::abs(view->transform().m11());
        if (m > 1e-9) {
            scale = m;
        }
    }

    const double tol = pixelTolerance / scale;
    const QRectF hitBox(scenePos.x() - tol, scenePos.y() - tol, 2.0 * tol, 2.0 * tol);
    const QList<QGraphicsItem*> hits =
        scene->items(hitBox, Qt::IntersectsItemShape, Qt::DescendingOrder, deviceTransform);

    // Among the candidates in range, choose the one whose geometry is actually
    // nearest the cursor — not just the topmost. This matters where entities
    // meet (e.g. a tangent line touching a circle): the pick follows the
    // crosshair instead of whichever was drawn last.
    EntityItem* nearest = nullptr;
    double nearestDist = std::numeric_limits<double>::max();
    for (QGraphicsItem* item : hits) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            const double d = entityItem->distanceTo(scenePos);
            if (d < nearestDist) {
                nearestDist = d;
                nearest = entityItem;
            }
        }
    }
    return nearest;
}

} // namespace cad
