#include "cad/ui/tools/FilletChamferTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/commands/ModifyEntityCommand.h"
#include "cad/core/entities/ArcEntity.h"
#include "cad/core/entities/LineEntity.h"
#include "cad/core/entities/PolylineEntity.h"
#include "cad/core/geometry/Intersections.h"  // angleAtDeg / normalizeDeg
#include "cad/ui/render/EntityItem.h"
#include "cad/ui/tools/ToolPick.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>
#include <Qt>

#include <cmath>
#include <memory>

namespace cad {

namespace {

constexpr double kEps = 1e-9;
constexpr double kPi = 3.14159265358979323846;

using Mode = FilletChamferTool::Mode;
using Pick = FilletChamferTool::Pick;

double dot(const QPointF& a, const QPointF& b)
{
    return a.x() * b.x() + a.y() * b.y();
}

QPointF normalized(const QPointF& v)
{
    const double len = std::hypot(v.x(), v.y());
    return len < kEps ? QPointF(0, 0) : QPointF(v.x() / len, v.y() / len);
}

bool infiniteLineIntersection(const QPointF& a1, const QPointF& a2, const QPointF& b1,
                              const QPointF& b2, QPointF& out)
{
    const double den = (a1.x() - a2.x()) * (b1.y() - b2.y()) - (a1.y() - a2.y()) * (b1.x() - b2.x());
    if (std::abs(den) < kEps) {
        return false;
    }
    const double t =
        ((a1.x() - b1.x()) * (b1.y() - b2.y()) - (a1.y() - b1.y()) * (b1.x() - b2.x())) / den;
    out = QPointF(a1.x() + t * (a2.x() - a1.x()), a1.y() + t * (a2.y() - a1.y()));
    return true;
}

// Signed minor sweep (deg) from pStart to pEnd about center; the fillet arc.
double minorSweep(const QPointF& center, const QPointF& pStart, const QPointF& pEnd)
{
    double sweep = geom::normalizeDeg(geom::angleAtDeg(center, pEnd) - geom::angleAtDeg(center, pStart));
    if (sweep > 180.0) {
        sweep -= 360.0;
    }
    return sweep;
}

double bulgeForSweep(double sweepDeg)
{
    return std::tan(sweepDeg * kPi / 180.0 / 4.0);  // DXF bulge = tan(sweep/4)
}

// The corner geometry shared by both cases.
struct Corner
{
    bool valid = false;
    QPointF corner;         // where the two arms' lines meet
    QPointF tan1, tan2;     // tangent / setback points on arm 1 and arm 2
    QPointF far1, far2;     // each arm's endpoint on the kept (clicked) side
    QPointF center;         // fillet arc center (only when rounding with value > 0)
};

Corner computeCorner(const Pick& p1, const Pick& p2, double value, Mode mode)
{
    Corner c;
    if (!infiniteLineIntersection(p1.a, p1.b, p2.a, p2.b, c.corner)) {
        return c;  // parallel arms
    }

    // For each arm: the click only chooses which endpoint (side) to keep; the arm
    // DIRECTION comes from the true edge geometry, so the result never drifts with
    // the cursor inside the pick tolerance.
    const auto arm = [&](const Pick& p, QPointF& farOut, QPointF& dirOut) {
        const QPointF vClick = p.click - c.corner;
        farOut = (dot(p.a - c.corner, vClick) > dot(p.b - c.corner, vClick)) ? p.a : p.b;
        dirOut = normalized(farOut - c.corner);
        return std::hypot(dirOut.x(), dirOut.y()) > 0.5;
    };
    QPointF d1, d2;
    if (!arm(p1, c.far1, d1) || !arm(p2, c.far2, d2)) {
        return c;  // an arm collapses onto the corner
    }

    const double alpha = std::acos(std::clamp(dot(d1, d2), -1.0, 1.0));
    if (alpha < 1e-4 || alpha > kPi - 1e-4) {
        return c;  // collinear
    }

    const double setback = (mode == Mode::Fillet) ? value / std::tan(alpha / 2.0) : value;
    c.tan1 = c.corner + setback * d1;
    c.tan2 = c.corner + setback * d2;
    c.valid = true;

    if (mode == Mode::Fillet && value > kEps) {
        const QPointF bisector = normalized(d1 + d2);
        const double dist = value / std::sin(alpha / 2.0);
        c.center = c.corner + dist * bisector;
    }
    return c;
}

// What the operation will do: replace one polyline, or trim two lines and add a
// connector. Built once, used for both preview and commit.
struct Plan
{
    bool valid = false;

    bool polyReplace = false;
    int polyId = -1;
    QVector<PolyVertex> verts;
    bool closed = false;

    bool twoLines = false;
    int id1 = -1, id2 = -1;
    QPointF l1a, l1b, l2a, l2b;
    bool hasArc = false;
    QPointF center;
    double radius = 0.0, startDeg = 0.0, sweepDeg = 0.0;
    bool hasLine = false;
    QPointF cp, cq;
};

Plan buildPolylinePlan(const PolylineEntity& poly, const Pick& p1, const Pick& p2,
                       const Corner& c, double value, Mode mode)
{
    const QVector<PolyVertex>& vs = poly.vertices();
    const int n = vs.size();
    const bool closed = poly.isClosed();
    const int s1 = p1.segment;
    const int s2 = p2.segment;

    // Which segment precedes the shared corner vertex, and which follows it.
    int before = -1, after = -1;
    if (s1 + 1 == s2) {
        before = s1;
        after = s2;
    }
    else if (s2 + 1 == s1) {
        before = s2;
        after = s1;
    }
    else if (closed && s1 == n - 1 && s2 == 0) {
        before = s1;
        after = s2;
    }
    else if (closed && s2 == n - 1 && s1 == 0) {
        before = s2;
        after = s1;
    }
    else {
        return {};  // segments don't share a corner
    }
    const int sharedIdx = after;  // the corner vertex is where the "after" segment starts

    // Only straight corner arms are handled (a curved arm needs a different fix).
    if (std::abs(vs[s1].bulge) > 1e-9 || std::abs(vs[s2].bulge) > 1e-9) {
        return {};
    }

    const QPointF tanBefore = (before == s1) ? c.tan1 : c.tan2;
    const QPointF tanAfter = (after == s1) ? c.tan1 : c.tan2;
    const double insertBulge = (mode == Mode::Fillet && value > kEps)
                                   ? bulgeForSweep(minorSweep(c.center, tanBefore, tanAfter))
                                   : 0.0;

    Plan plan;
    plan.valid = true;
    plan.polyReplace = true;
    plan.polyId = p1.id;
    plan.closed = closed;
    for (int i = 0; i < n; ++i) {
        if (i == sharedIdx) {
            plan.verts.push_back({tanBefore, insertBulge});   // arm in + fillet arc
            plan.verts.push_back({tanAfter, vs[i].bulge});    // arm out (keeps the old outgoing bulge)
        }
        else {
            plan.verts.push_back(vs[i]);
        }
    }
    return plan;
}

Plan buildPlan(CadDocument* document, const Pick& p1, const Pick& p2, double value, Mode mode)
{
    if (p1.id < 0 || p2.id < 0) {
        return {};
    }
    const Corner c = computeCorner(p1, p2, value, mode);
    if (!c.valid) {
        return {};
    }

    if (p1.id == p2.id) {
        // Same entity: only a polyline, two different segments (a corner).
        if (!p1.isPolyline || p1.segment == p2.segment) {
            return {};
        }
        const auto* poly = dynamic_cast<const PolylineEntity*>(document->entity(p1.id));
        return poly ? buildPolylinePlan(*poly, p1, p2, c, value, mode) : Plan{};
    }

    // Different entities: only two separate straight lines are handled cleanly.
    if (p1.isPolyline || p2.isPolyline) {
        return {};
    }

    Plan plan;
    plan.valid = true;
    plan.twoLines = true;
    plan.id1 = p1.id;
    plan.id2 = p2.id;
    plan.l1a = c.far1;
    plan.l1b = c.tan1;
    plan.l2a = c.far2;
    plan.l2b = c.tan2;
    if (value > kEps) {
        if (mode == Mode::Fillet) {
            plan.hasArc = true;
            plan.center = c.center;
            plan.radius = value;
            plan.startDeg = geom::angleAtDeg(c.center, c.tan1);
            plan.sweepDeg = minorSweep(c.center, c.tan1, c.tan2);
        }
        else {
            plan.hasLine = true;
            plan.cp = c.tan1;
            plan.cq = c.tan2;
        }
    }
    return plan;
}

// Just the picked arm's segment, to show it is selected / hovered.
QPainterPath armPath(const Pick& p)
{
    QPainterPath path;
    if (p.id >= 0) {
        path.moveTo(p.a);
        path.lineTo(p.b);
    }
    return path;
}

QPainterPath planPreview(const Plan& plan)
{
    QPainterPath path;
    if (plan.polyReplace) {
        path = PolylineEntity(plan.verts, plan.closed).path();
    }
    else if (plan.twoLines) {
        path.addPath(LineEntity(plan.l1a, plan.l1b).path());
        path.addPath(LineEntity(plan.l2a, plan.l2b).path());
        if (plan.hasArc) {
            path.addPath(ArcEntity(plan.center, plan.radius, plan.startDeg, plan.sweepDeg).path());
        }
        if (plan.hasLine) {
            path.addPath(LineEntity(plan.cp, plan.cq).path());
        }
    }
    return path;
}

// Nearest polyline segment index to a scene point (by perpendicular distance).
int nearestSegment(const PolylineEntity& poly, const QPointF& p)
{
    const QVector<PolyVertex>& vs = poly.vertices();
    const int n = vs.size();
    if (n < 2) {
        return -1;
    }
    const int count = poly.isClosed() ? n : n - 1;
    int best = -1;
    double bestDist = 1e18;
    for (int i = 0; i < count; ++i) {
        const QPointF a = vs[i].pos;
        const QPointF b = vs[(i + 1) % n].pos;
        const double len2 = (a.x() - b.x()) * (a.x() - b.x()) + (a.y() - b.y()) * (a.y() - b.y());
        double t = 0.0;
        if (len2 > 1e-9) {
            t = std::clamp(((p.x() - a.x()) * (b.x() - a.x()) + (p.y() - a.y()) * (b.y() - a.y())) / len2,
                           0.0, 1.0);
        }
        const QPointF proj(a.x() + t * (b.x() - a.x()), a.y() + t * (b.y() - a.y()));
        const double dist = QLineF(p, proj).length();
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

} // namespace

FilletChamferTool::FilletChamferTool(Mode mode, CadDocument* document, QUndoStack* undoStack,
                                     QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_mode(mode)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

FilletChamferTool::Pick FilletChamferTool::pickArm(const QPointF& scenePos) const
{
    EntityItem* entityItem = pickEntityItem(m_scene, scenePos);
    if (!entityItem) {
        return {};
    }
    CadEntity* entity = m_document->entity(entityItem->entityId());
    if (!entity) {
        return {};
    }
    if (entity->type() == EntityType::Line) {
        const auto& l = static_cast<const LineEntity&>(*entity);
        return {entityItem->entityId(), -1, false, l.p1(), l.p2(), scenePos};
    }
    if (entity->type() == EntityType::Polyline) {
        const auto& poly = static_cast<const PolylineEntity&>(*entity);
        const int seg = nearestSegment(poly, scenePos);
        if (seg < 0) {
            return {};
        }
        const QVector<PolyVertex>& vs = poly.vertices();
        const int n = vs.size();
        return {entityItem->entityId(), seg, true, vs[seg].pos, vs[(seg + 1) % n].pos, scenePos};
    }
    return {};  // circles/arcs: fillet with arcs is a later increment
}

void FilletChamferTool::onMouseMove(const QPointF& scenePos)
{
    const Pick hovered = pickArm(scenePos);

    if (m_first.id < 0) {
        // Before the first pick: highlight the arm the cursor is over.
        showPreview(armPath(hovered));
        return;
    }

    // After the first pick: always show the picked arm; when a valid second arm
    // is under the cursor, show the full fillet/chamfer result instead.
    const Plan plan = buildPlan(m_document, m_first, hovered, m_value, m_mode);
    showPreview(plan.valid ? planPreview(plan) : armPath(m_first));
}

void FilletChamferTool::onMousePress(const QPointF& scenePos)
{
    const Pick hit = pickArm(scenePos);
    if (m_first.id < 0) {
        if (hit.id >= 0) {
            m_first = hit;
            showPreview(armPath(m_first));  // immediate feedback that it was picked
        }
        return;
    }

    const Plan plan = buildPlan(m_document, m_first, hit, m_value, m_mode);
    if (!plan.valid) {
        return;
    }

    const QString label = (m_mode == Mode::Fillet) ? tr("Fillet") : tr("Chamfer");
    if (plan.polyReplace) {
        m_undoStack->push(new ModifyEntityCommand(
            m_document, plan.polyId, std::make_unique<PolylineEntity>(plan.verts, plan.closed), label));
    }
    else if (plan.twoLines) {
        m_undoStack->beginMacro(label);
        m_undoStack->push(new ModifyEntityCommand(
            m_document, plan.id1, std::make_unique<LineEntity>(plan.l1a, plan.l1b), label));
        m_undoStack->push(new ModifyEntityCommand(
            m_document, plan.id2, std::make_unique<LineEntity>(plan.l2a, plan.l2b), label));
        if (plan.hasArc) {
            m_undoStack->push(new AddEntityCommand(
                m_document,
                std::make_unique<ArcEntity>(plan.center, plan.radius, plan.startDeg, plan.sweepDeg),
                label));
        }
        if (plan.hasLine) {
            m_undoStack->push(new AddEntityCommand(
                m_document, std::make_unique<LineEntity>(plan.cp, plan.cq), label));
        }
        m_undoStack->endMacro();
    }

    reset();
}

void FilletChamferTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void FilletChamferTool::onCancel()
{
    reset();
}

void FilletChamferTool::deactivate()
{
    reset();
}

QList<CadTool::InputField> FilletChamferTool::inputFields() const
{
    return {{m_mode == Mode::Fillet ? tr("Radius") : tr("Distance"), m_value}};
}

void FilletChamferTool::applyInput(const QVector<double>& values)
{
    if (!values.isEmpty()) {
        m_value = std::max(0.0, values[0]);  // sticky; does not commit
        emit inputChanged();
    }
}

void FilletChamferTool::showPreview(const QPainterPath& path)
{
    if (path.isEmpty()) {
        clearPreview();
        return;
    }
    if (!m_preview) {
        QPen pen(QColor(0xff, 0x9c, 0x33));
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_preview = m_scene->addPath(QPainterPath(), pen);
        m_preview->setZValue(1000);
    }
    m_preview->setPath(path);
}

void FilletChamferTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

void FilletChamferTool::reset()
{
    clearPreview();
    m_first = Pick{};
}

} // namespace cad
