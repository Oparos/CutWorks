#include "cad/ui/tools/OffsetTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/ArcEntity.h"
#include "cad/core/entities/CircleEntity.h"
#include "cad/core/entities/LineEntity.h"
#include "cad/core/entities/PolylineEntity.h"
#include "cad/ui/render/EntityItem.h"
#include "cad/ui/tools/ToolPick.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>
#include <Qt>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace cad {

namespace {

constexpr double kEps = 1e-9;

double dot(const QPointF& a, const QPointF& b)
{
    return a.x() * b.x() + a.y() * b.y();
}

QPointF normalized(const QPointF& v)
{
    const double len = std::hypot(v.x(), v.y());
    return len < kEps ? QPointF(0, 0) : QPointF(v.x() / len, v.y() / len);
}

// Left-hand normal of the direction a->b.
QPointF leftNormal(const QPointF& a, const QPointF& b)
{
    const QPointF d = normalized(b - a);
    return QPointF(-d.y(), d.x());
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

// Signed distance from p to segment [a,b] (perpendicular, clamped to the segment).
double distanceToSegment(const QPointF& p, const QPointF& a, const QPointF& b)
{
    const QPointF ab = b - a;
    const double len2 = ab.x() * ab.x() + ab.y() * ab.y();
    double t = 0.0;
    if (len2 > kEps) {
        t = std::clamp(dot(p - a, ab) / len2, 0.0, 1.0);
    }
    return QLineF(p, QPointF(a.x() + t * ab.x(), a.y() + t * ab.y())).length();
}

// Offset a straight polyline (no bulges) to the side the cursor is on.
std::unique_ptr<CadEntity> offsetPolyline(const PolylineEntity& poly, const QPointF& cursor,
                                          double distance)
{
    const QVector<PolyVertex>& vs = poly.vertices();
    const int n = vs.size();
    if (n < 2) {
        return nullptr;
    }
    for (const PolyVertex& v : vs) {
        if (std::abs(v.bulge) > 1e-9) {
            return nullptr;  // curved polylines need proper (Clipper) offsetting
        }
    }
    const bool closed = poly.isClosed();
    const int segs = closed ? n : n - 1;

    // Handedness: which side of the nearest edge the cursor is on. All edges
    // shift to that same side, giving a consistent parallel offset.
    int nearest = 0;
    double nearestDist = 1e18;
    for (int i = 0; i < segs; ++i) {
        const double d = distanceToSegment(cursor, vs[i].pos, vs[(i + 1) % n].pos);
        if (d < nearestDist) {
            nearestDist = d;
            nearest = i;
        }
    }
    const QPointF a = vs[nearest].pos;
    const QPointF b = vs[(nearest + 1) % n].pos;
    const QPointF ln = leftNormal(a, b);
    const double h = (dot(ln, cursor - (a + b) / 2.0) >= 0.0) ? 1.0 : -1.0;

    // Each edge shifted by distance along h * its own left normal.
    std::vector<std::pair<QPointF, QPointF>> off(segs);
    for (int i = 0; i < segs; ++i) {
        const QPointF p1 = vs[i].pos;
        const QPointF p2 = vs[(i + 1) % n].pos;
        const QPointF shift = h * distance * leftNormal(p1, p2);
        off[i] = {p1 + shift, p2 + shift};
    }

    QVector<QPointF> result;
    if (closed) {
        for (int j = 0; j < n; ++j) {
            const int prev = (j - 1 + segs) % segs;
            QPointF pt;
            if (!infiniteLineIntersection(off[prev].first, off[prev].second, off[j].first,
                                          off[j].second, pt)) {
                pt = off[j].first;  // collinear edges: no miter needed
            }
            result.append(pt);
        }
    }
    else {
        result.append(off[0].first);
        for (int j = 1; j < segs; ++j) {
            QPointF pt;
            if (!infiniteLineIntersection(off[j - 1].first, off[j - 1].second, off[j].first,
                                          off[j].second, pt)) {
                pt = off[j].first;
            }
            result.append(pt);
        }
        result.append(off[segs - 1].second);
    }
    return std::make_unique<PolylineEntity>(result, closed);
}

// The offset copy of an entity toward `cursor`, or nullptr if not offsettable.
std::unique_ptr<CadEntity> computeOffset(const CadEntity& e, const QPointF& cursor, double distance)
{
    switch (e.type()) {
    case EntityType::Line: {
        const auto& l = static_cast<const LineEntity&>(e);
        const QPointF ln = leftNormal(l.p1(), l.p2());
        const double h = (dot(ln, cursor - l.p1()) >= 0.0) ? 1.0 : -1.0;
        const QPointF shift = h * distance * ln;
        return std::make_unique<LineEntity>(l.p1() + shift, l.p2() + shift);
    }
    case EntityType::Circle: {
        const auto& c = static_cast<const CircleEntity&>(e);
        const bool outside = QLineF(cursor, c.center()).length() > c.radius();
        const double r = c.radius() + (outside ? distance : -distance);
        return (r > kEps) ? std::make_unique<CircleEntity>(c.center(), r) : nullptr;
    }
    case EntityType::Arc: {
        const auto& a = static_cast<const ArcEntity&>(e);
        const bool outside = QLineF(cursor, a.center()).length() > a.radius();
        const double r = a.radius() + (outside ? distance : -distance);
        return (r > kEps) ? std::make_unique<ArcEntity>(a.center(), r, a.startAngle(), a.sweepAngle())
                          : nullptr;
    }
    case EntityType::Polyline:
        return offsetPolyline(static_cast<const PolylineEntity&>(e), cursor, distance);
    default:
        return nullptr;
    }
}

bool offsettable(const CadEntity& e)
{
    switch (e.type()) {
    case EntityType::Line:
    case EntityType::Circle:
    case EntityType::Arc:
    case EntityType::Polyline:
        return true;
    default:
        return false;
    }
}

} // namespace

OffsetTool::OffsetTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
                       QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void OffsetTool::onMouseMove(const QPointF& scenePos)
{
    if (m_entityId < 0) {
        return;
    }
    const CadEntity* e = m_document->entity(m_entityId);
    if (!e) {
        reset();
        return;
    }
    if (std::unique_ptr<CadEntity> result = computeOffset(*e, scenePos, m_distance)) {
        showPreview(result->path());
    }
    else {
        clearPreview();
    }
}

void OffsetTool::onMousePress(const QPointF& scenePos)
{
    if (m_entityId < 0) {
        if (EntityItem* item = pickEntityItem(m_scene, scenePos)) {
            const CadEntity* e = m_document->entity(item->entityId());
            if (e && offsettable(*e)) {
                m_entityId = item->entityId();
            }
        }
        return;
    }

    if (const CadEntity* e = m_document->entity(m_entityId)) {
        if (std::unique_ptr<CadEntity> result = computeOffset(*e, scenePos, m_distance)) {
            m_undoStack->push(new AddEntityCommand(m_document, std::move(result), tr("Offset")));
        }
    }
    reset();
}

void OffsetTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void OffsetTool::onCancel()
{
    reset();
}

void OffsetTool::deactivate()
{
    reset();
}

QList<CadTool::InputField> OffsetTool::inputFields() const
{
    return {{tr("Distance"), m_distance}};
}

void OffsetTool::applyInput(const QVector<double>& values)
{
    if (!values.isEmpty()) {
        m_distance = std::max(0.0, values[0]);  // sticky; the side click commits
        emit inputChanged();
    }
}

void OffsetTool::showPreview(const QPainterPath& path)
{
    if (!m_preview) {
        QPen pen(QColor(0xff, 0x9c, 0x33));
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_preview = m_scene->addPath(QPainterPath(), pen);
        m_preview->setZValue(1000);
    }
    m_preview->setPath(path);
}

void OffsetTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

void OffsetTool::reset()
{
    clearPreview();
    m_entityId = -1;
}

} // namespace cad
