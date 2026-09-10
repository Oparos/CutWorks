#include "cad/ui/tools/ExtendTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/ModifyEntityCommand.h"
#include "cad/core/entities/ArcEntity.h"
#include "cad/core/entities/LineEntity.h"
#include "cad/core/entities/PolylineEntity.h"
#include "cad/core/geometry/Intersections.h"
#include "cad/ui/render/EntityItem.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>

#include <cmath>
#include <limits>
#include <memory>

namespace cad {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kDistEps = 1e-6;   // must extend at least this far forward (mm)
constexpr double kAngleEps = 1e-4;  // angular tolerance (deg)

int entityIdAt(QGraphicsScene* scene, const QPointF& pos)
{
    for (QGraphicsItem* item : scene->items(pos)) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            return entityItem->entityId();
        }
    }
    return -1;
}

double distance(const QPointF& a, const QPointF& b)
{
    return QLineF(a, b).length();
}

QPointF pointOnCircle(const QPointF& center, double r, double angleDeg)
{
    return QPointF(center.x() + r * std::cos(angleDeg * kDegToRad),
                   center.y() + r * std::sin(angleDeg * kDegToRad));
}

// Nearest point in front of `endPt` where the infinite line through anchor/endPt
// meets another entity. "In front" = further along the anchor->endPt direction.
bool nearestForwardHit(CadDocument* document, int id, const QPointF& anchor,
                       const QPointF& endPt, QPointF& out)
{
    QPointF dir = endPt - anchor;
    const double len = std::hypot(dir.x(), dir.y());
    if (len < kDistEps) {
        return false;
    }
    dir /= len;

    double best = std::numeric_limits<double>::max();
    for (int otherId : document->entityIds()) {
        if (otherId == id) {
            continue;
        }
        const CadEntity* other = document->entity(otherId);
        if (!other) {
            continue;
        }
        for (const QPointF& p : geom::supportLineVsEntity(anchor, endPt, *other)) {
            const double proj = (p.x() - endPt.x()) * dir.x() + (p.y() - endPt.y()) * dir.y();
            if (proj > kDistEps && proj < best) {
                best = proj;
                out = p;
            }
        }
    }
    return best != std::numeric_limits<double>::max();
}

std::unique_ptr<CadEntity> extendLine(CadDocument* document, int id, const LineEntity& line,
                                      const QPointF& click)
{
    const QPointF p1 = line.p1();
    const QPointF p2 = line.p2();
    const bool moveP1 = distance(click, p1) < distance(click, p2);
    const QPointF endPt = moveP1 ? p1 : p2;
    const QPointF anchor = moveP1 ? p2 : p1;

    QPointF hit;
    if (!nearestForwardHit(document, id, anchor, endPt, hit)) {
        return nullptr;
    }
    return moveP1 ? std::make_unique<LineEntity>(hit, p2)
                  : std::make_unique<LineEntity>(p1, hit);
}

std::unique_ptr<CadEntity> extendPolyline(CadDocument* document, int id,
                                          const PolylineEntity& poly, const QPointF& click)
{
    if (poly.isClosed()) {
        return nullptr;
    }
    QVector<PolyVertex> verts = poly.vertices();
    const int n = verts.size();
    if (n < 2) {
        return nullptr;
    }
    const bool moveFirst = distance(click, verts.front().pos) < distance(click, verts.back().pos);
    const int endIdx = moveFirst ? 0 : n - 1;
    const int neighborIdx = moveFirst ? 1 : n - 2;

    QPointF hit;
    if (!nearestForwardHit(document, id, verts[neighborIdx].pos, verts[endIdx].pos, hit)) {
        return nullptr;
    }
    verts[endIdx].pos = hit;
    return std::make_unique<PolylineEntity>(verts, poly.isClosed());
}

std::unique_ptr<CadEntity> extendArc(CadDocument* document, int id, const ArcEntity& arc,
                                     const QPointF& click)
{
    const QPointF center = arc.center();
    const double r = arc.radius();
    const double start = arc.startAngle();
    const double sweep = arc.sweepAngle();
    const double span = std::abs(sweep);
    const double maxGrowth = 360.0 - span;
    if (maxGrowth < kAngleEps) {
        return nullptr;  // already a full turn — nothing to extend into
    }
    const double sign = (sweep >= 0.0) ? 1.0 : -1.0;
    const double endAngle = start + sweep;

    const bool moveStart = distance(click, pointOnCircle(center, r, start))
                           < distance(click, pointOnCircle(center, r, endAngle));

    double bestGrowth = std::numeric_limits<double>::max();
    for (int otherId : document->entityIds()) {
        if (otherId == id) {
            continue;
        }
        const CadEntity* other = document->entity(otherId);
        if (!other) {
            continue;
        }
        for (const QPointF& p : geom::supportCircleVsEntity(center, r, *other)) {
            const double phi = geom::angleAtDeg(center, p);
            // Growth = how far the chosen end has to travel (in its own
            // direction) to reach this point, wrapped into [0, 360).
            const double growth = moveStart ? geom::normalizeDeg(sign * (start - phi))
                                            : geom::normalizeDeg(sign * (phi - endAngle));
            if (growth > kAngleEps && growth <= maxGrowth + kAngleEps && growth < bestGrowth) {
                bestGrowth = growth;
            }
        }
    }
    if (bestGrowth == std::numeric_limits<double>::max()) {
        return nullptr;
    }

    const double newStart = moveStart ? (start - sign * bestGrowth) : start;
    const double newSweep = sweep + sign * bestGrowth;
    return std::make_unique<ArcEntity>(center, r, newStart, newSweep);
}

std::unique_ptr<CadEntity> planExtend(CadDocument* document, int id, const QPointF& click)
{
    CadEntity* entity = document->entity(id);
    if (!entity) {
        return nullptr;
    }
    switch (entity->type()) {
    case EntityType::Line:
        return extendLine(document, id, static_cast<const LineEntity&>(*entity), click);
    case EntityType::Arc:
        return extendArc(document, id, static_cast<const ArcEntity&>(*entity), click);
    case EntityType::Polyline:
        return extendPolyline(document, id, static_cast<const PolylineEntity&>(*entity), click);
    default:
        return nullptr;  // circles and points have no end to extend
    }
}

} // namespace

ExtendTool::ExtendTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
                       QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void ExtendTool::onMouseMove(const QPointF& scenePos)
{
    const int id = entityIdAt(m_scene, scenePos);
    if (id < 0) {
        clearPreview();
        return;
    }
    if (std::unique_ptr<CadEntity> result = planExtend(m_document, id, scenePos)) {
        showPreview(result->path());
    }
    else {
        clearPreview();
    }
}

void ExtendTool::onMousePress(const QPointF& scenePos)
{
    const int id = entityIdAt(m_scene, scenePos);
    if (id < 0) {
        return;
    }
    std::unique_ptr<CadEntity> result = planExtend(m_document, id, scenePos);
    if (!result) {
        return;
    }
    m_undoStack->push(new ModifyEntityCommand(m_document, id, std::move(result), tr("Extend")));
    clearPreview();
}

void ExtendTool::deactivate()
{
    clearPreview();
}

void ExtendTool::showPreview(const QPainterPath& result)
{
    if (!m_preview) {
        QPen pen(QColor(0x33, 0xcc, 0xcc));  // cyan: the extended result
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_preview = m_scene->addPath(QPainterPath(), pen);
        m_preview->setZValue(1000);
    }
    m_preview->setPath(result);
}

void ExtendTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

} // namespace cad
