#include "cad/ui/tools/TrimTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/commands/RemoveEntityCommand.h"
#include "cad/core/entities/ArcEntity.h"
#include "cad/core/entities/CircleEntity.h"
#include "cad/core/entities/LineEntity.h"
#include "cad/core/geometry/Intersections.h"
#include "cad/ui/render/EntityItem.h"
#include "cad/ui/tools/ToolPick.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace cad {

namespace {

constexpr double kTEps = 1e-6;          // tolerance on a 0..1 line parameter
constexpr double kAngleEps = 1e-4;      // tolerance on an angle (deg)

// What a trim would do: which piece is removed (for the red preview) and the
// pieces that stay (added back as new entities; empty = the entity is deleted).
struct TrimPlan
{
    bool valid = false;
    QPainterPath removed;
    std::vector<std::unique_ptr<CadEntity>> survivors;
};

int entityIdAt(QGraphicsScene* scene, const QPointF& pos)
{
    EntityItem* item = pickEntityItem(scene, pos);
    return item ? item->entityId() : -1;
}

// All points where the clicked entity is crossed by every other entity.
QVector<QPointF> crossings(CadDocument* document, int id, const CadEntity& clicked)
{
    QVector<QPointF> points;
    for (int otherId : document->entityIds()) {
        if (otherId == id) {
            continue;
        }
        if (const CadEntity* other = document->entity(otherId)) {
            points += geom::intersect(clicked, *other);
        }
    }
    return points;
}

void sortUnique(std::vector<double>& values, double eps)
{
    std::sort(values.begin(), values.end());
    std::vector<double> out;
    for (double v : values) {
        if (out.empty() || v - out.back() > eps) {
            out.push_back(v);
        }
    }
    values = std::move(out);
}

// Pick the interval [breaks[i], breaks[i+1]] that contains parameter c.
std::pair<double, double> bracket(const std::vector<double>& breaks, double c)
{
    for (std::size_t i = 0; i + 1 < breaks.size(); ++i) {
        if (c >= breaks[i] - kTEps && c <= breaks[i + 1] + kTEps) {
            return {breaks[i], breaks[i + 1]};
        }
    }
    return {breaks.front(), breaks.back()};
}

TrimPlan planTrimLine(const LineEntity& line, const QPointF& click, const QVector<QPointF>& cross)
{
    TrimPlan plan;
    const QPointF p1 = line.p1();
    const QPointF dir = line.p2() - p1;
    const double len2 = dir.x() * dir.x() + dir.y() * dir.y();
    if (len2 < 1e-12) {
        return plan;
    }
    const auto paramOf = [&](const QPointF& p) {
        return ((p.x() - p1.x()) * dir.x() + (p.y() - p1.y()) * dir.y()) / len2;
    };
    const auto pointAt = [&](double t) { return QPointF(p1.x() + t * dir.x(), p1.y() + t * dir.y()); };

    std::vector<double> ts{0.0, 1.0};
    for (const QPointF& pt : cross) {
        const double t = paramOf(pt);
        if (t > kTEps && t < 1.0 - kTEps) {
            ts.push_back(t);
        }
    }
    sortUnique(ts, kTEps);

    const auto [a, b] = bracket(ts, std::clamp(paramOf(click), 0.0, 1.0));
    if (a <= kTEps && b >= 1.0 - kTEps) {
        return plan;  // no cut brackets the cursor — nothing to trim (never delete whole)
    }

    plan.valid = true;
    plan.removed.moveTo(pointAt(a));
    plan.removed.lineTo(pointAt(b));
    if (a > kTEps) {
        plan.survivors.push_back(std::make_unique<LineEntity>(pointAt(0.0), pointAt(a)));
    }
    if (b < 1.0 - kTEps) {
        plan.survivors.push_back(std::make_unique<LineEntity>(pointAt(b), pointAt(1.0)));
    }
    return plan;
}

TrimPlan planTrimArc(const ArcEntity& arc, const QPointF& click, const QVector<QPointF>& cross)
{
    TrimPlan plan;
    const QPointF center = arc.center();
    const double r = arc.radius();
    const double start = arc.startAngle();
    const double sweep = arc.sweepAngle();
    const double span = std::abs(sweep);
    if (span < kAngleEps) {
        return plan;
    }
    const double sign = (sweep >= 0.0) ? 1.0 : -1.0;
    // Parameter = angular distance from the start along the sweep direction.
    const auto paramOf = [&](const QPointF& p) {
        return geom::normalizeDeg(sign * (geom::angleAtDeg(center, p) - start));
    };

    std::vector<double> thetas{0.0, span};
    for (const QPointF& pt : cross) {
        const double t = paramOf(pt);
        if (t > kAngleEps && t < span - kAngleEps) {
            thetas.push_back(t);
        }
    }
    sortUnique(thetas, kAngleEps);

    const auto [a, b] = bracket(thetas, std::clamp(paramOf(click), 0.0, span));
    if (a <= kAngleEps && b >= span - kAngleEps) {
        return plan;  // no cut brackets the cursor — nothing to trim (never delete whole)
    }

    plan.valid = true;
    plan.removed = ArcEntity(center, r, start + sign * a, sign * (b - a)).path();
    if (a > kAngleEps) {
        plan.survivors.push_back(std::make_unique<ArcEntity>(center, r, start, sign * a));
    }
    if (span - b > kAngleEps) {
        plan.survivors.push_back(
            std::make_unique<ArcEntity>(center, r, start + sign * b, sign * (span - b)));
    }
    return plan;
}

TrimPlan planTrimCircle(const CircleEntity& circle, const QPointF& click, const QVector<QPointF>& cross)
{
    TrimPlan plan;
    const QPointF center = circle.center();
    const double r = circle.radius();

    std::vector<double> angles;
    for (const QPointF& pt : cross) {
        angles.push_back(geom::angleAtDeg(center, pt));
    }
    sortUnique(angles, kAngleEps);

    if (angles.size() < 2) {
        // A circle needs at least two real crossings to cut a piece out. A lone
        // circle, or one only *touched* by a tangent (a single point), is left
        // alone — Trim never deletes a whole entity (use Delete for that).
        return plan;
    }

    const double clickAngle = geom::angleAtDeg(center, click);
    const int n = static_cast<int>(angles.size());
    double lo = angles[n - 1];
    double hi = angles[0] + 360.0;  // default: the wrap-around gap
    for (int i = 0; i < n; ++i) {
        const double a = angles[i];
        const double b = (i + 1 < n) ? angles[i + 1] : angles[0] + 360.0;
        double c = clickAngle;
        if (c < a) {
            c += 360.0;
        }
        if (c >= a - kAngleEps && c <= b + kAngleEps) {
            lo = a;
            hi = b;
            break;
        }
    }

    const double gap = hi - lo;
    plan.valid = true;
    plan.removed = ArcEntity(center, r, lo, gap).path();  // the clicked piece
    plan.survivors.push_back(
        std::make_unique<ArcEntity>(center, r, geom::normalizeDeg(hi), 360.0 - gap));
    return plan;
}

TrimPlan planTrim(CadDocument* document, int id, const QPointF& click)
{
    CadEntity* entity = document->entity(id);
    if (!entity) {
        return {};
    }
    const QVector<QPointF> cross = crossings(document, id, *entity);
    switch (entity->type()) {
    case EntityType::Line:
        return planTrimLine(static_cast<const LineEntity&>(*entity), click, cross);
    case EntityType::Arc:
        return planTrimArc(static_cast<const ArcEntity&>(*entity), click, cross);
    case EntityType::Circle:
        return planTrimCircle(static_cast<const CircleEntity&>(*entity), click, cross);
    default:
        return {};  // polyline/point trimming is a later increment
    }
}

} // namespace

TrimTool::TrimTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
                   QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void TrimTool::onMouseMove(const QPointF& scenePos)
{
    const int id = entityIdAt(m_scene, scenePos);
    if (id < 0) {
        clearPreview();
        return;
    }
    const TrimPlan plan = planTrim(m_document, id, scenePos);
    if (plan.valid) {
        showPreview(plan.removed);
    }
    else {
        clearPreview();
    }
}

void TrimTool::onMousePress(const QPointF& scenePos)
{
    const int id = entityIdAt(m_scene, scenePos);
    if (id < 0) {
        return;
    }
    TrimPlan plan = planTrim(m_document, id, scenePos);
    if (!plan.valid) {
        return;
    }

    m_undoStack->beginMacro(tr("Trim"));
    m_undoStack->push(new RemoveEntityCommand(m_document, id, tr("Trim")));
    for (auto& survivor : plan.survivors) {
        m_undoStack->push(new AddEntityCommand(m_document, std::move(survivor), tr("Trim piece")));
    }
    m_undoStack->endMacro();

    clearPreview();
}

void TrimTool::deactivate()
{
    clearPreview();
}

void TrimTool::showPreview(const QPainterPath& removed)
{
    if (!m_preview) {
        QPen pen(QColor(0xff, 0x44, 0x44));  // red: "this will be removed"
        pen.setCosmetic(true);
        pen.setWidth(2);
        m_preview = m_scene->addPath(QPainterPath(), pen);
        m_preview->setZValue(1000);
    }
    m_preview->setPath(removed);
}

void TrimTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

} // namespace cad
