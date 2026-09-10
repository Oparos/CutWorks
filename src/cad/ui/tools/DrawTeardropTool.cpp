#include "cad/ui/tools/DrawTeardropTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/commands/RemoveEntityCommand.h"
#include "cad/core/entities/CircleEntity.h"
#include "cad/core/entities/PolylineEntity.h"
#include "cad/core/geometry/Intersections.h"  // angleAtDeg / normalizeDeg
#include "cad/core/geometry/Tangents.h"
#include "cad/ui/render/EntityItem.h"
#include "cad/ui/tools/ToolPick.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>

#include <cmath>
#include <memory>

namespace cad {

namespace {

constexpr double kDegToRad = 0.017453292519943295;

// Smallest angular gap between two headings (degrees, 0..180).
double angularGap(double x, double y)
{
    const double d = std::fmod(std::abs(x - y), 360.0);
    return d > 180.0 ? 360.0 - d : d;
}

// Signed sweep (degrees) from pStart to pEnd along the arc that stays on the
// `awayDir` side of the center — i.e. the outer arc of a teardrop lobe.
double outerSweep(const QPointF& center, const QPointF& pStart, const QPointF& pEnd,
                  const QPointF& awayDir)
{
    const double a0 = geom::angleAtDeg(center, pStart);
    const double a1 = geom::angleAtDeg(center, pEnd);
    const double awayAngle = geom::angleAtDeg(center, center + awayDir);

    const double sweepCcw = geom::normalizeDeg(a1 - a0);           // (0, 360)
    const double midCcw = geom::normalizeDeg(a0 + sweepCcw / 2.0);
    // Take whichever direction's midpoint sits on the outer (away) side.
    if (angularGap(midCcw, awayAngle) <= angularGap(geom::normalizeDeg(midCcw + 180.0), awayAngle)) {
        return sweepCcw;
    }
    return sweepCcw - 360.0;
}

double bulgeForSweep(double sweepDeg)
{
    return std::tan(sweepDeg * kDegToRad / 4.0);  // DXF bulge = tan(sweep/4)
}

// The closed teardrop as bulge-polyline vertices, or empty if the circles have
// no external tangent.
QVector<PolyVertex> teardropVertices(const QPointF& cA, double rA, const QPointF& cB, double rB)
{
    const QVector<QLineF> tangents = geom::externalTangents(cA, rA, cB, rB);
    if (tangents.size() != 2) {
        return {};
    }
    // Each tangent runs from its touch point on A (p1) to the one on B (p2).
    const QPointF pA0 = tangents[0].p1();
    const QPointF pB0 = tangents[0].p2();
    const QPointF pA1 = tangents[1].p1();
    const QPointF pB1 = tangents[1].p2();

    const QPointF axis = cB - cA;
    const double len = std::hypot(axis.x(), axis.y());
    const QPointF u(axis.x() / len, axis.y() / len);  // toward B

    // Outer arc on B faces away from A (+u); outer arc on A faces away from B (-u).
    const double bulgeB = bulgeForSweep(outerSweep(cB, pB0, pB1, u));
    const double bulgeA = bulgeForSweep(outerSweep(cA, pA1, pA0, QPointF(-u.x(), -u.y())));

    return {
        {pA0, 0.0},     // tangent A→B
        {pB0, bulgeB},  // outer arc on B
        {pB1, 0.0},     // tangent B→A
        {pA1, bulgeA},  // outer arc on A (wraps to the first vertex)
    };
}

} // namespace

DrawTeardropTool::DrawTeardropTool(CadDocument* document, QUndoStack* undoStack,
                                   QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

DrawTeardropTool::Circle DrawTeardropTool::circleAt(const QPointF& scenePos) const
{
    EntityItem* entityItem = pickEntityItem(m_scene, scenePos);
    if (!entityItem) {
        return {};
    }
    CadEntity* entity = m_document->entity(entityItem->entityId());
    if (entity && entity->type() == EntityType::Circle) {
        const auto& c = static_cast<const CircleEntity&>(*entity);
        return {entityItem->entityId(), c.center(), c.radius()};
    }
    return {};
}

void DrawTeardropTool::onMouseMove(const QPointF& scenePos)
{
    if (m_first.id < 0) {
        return;
    }
    const Circle second = circleAt(scenePos);
    if (second.id < 0 || second.id == m_first.id) {
        clearPreview();
        return;
    }
    const QVector<PolyVertex> verts =
        teardropVertices(m_first.center, m_first.radius, second.center, second.radius);
    if (verts.isEmpty()) {
        clearPreview();
        return;
    }
    showPreview(PolylineEntity(verts, /*closed*/ true).path());
}

void DrawTeardropTool::onMousePress(const QPointF& scenePos)
{
    const Circle hit = circleAt(scenePos);
    if (m_first.id < 0) {
        if (hit.id >= 0) {
            m_first = hit;
        }
        return;
    }
    if (hit.id < 0 || hit.id == m_first.id) {
        return;  // need a different circle
    }

    const QVector<PolyVertex> verts =
        teardropVertices(m_first.center, m_first.radius, hit.center, hit.radius);
    if (verts.isEmpty()) {
        return;  // no external tangent; keep the first pick
    }

    m_undoStack->beginMacro(tr("Teardrop"));
    m_undoStack->push(new RemoveEntityCommand(m_document, m_first.id, tr("Teardrop")));
    m_undoStack->push(new RemoveEntityCommand(m_document, hit.id, tr("Teardrop")));
    m_undoStack->push(new AddEntityCommand(
        m_document, std::make_unique<PolylineEntity>(verts, /*closed*/ true), tr("Teardrop")));
    m_undoStack->endMacro();

    reset();
}

void DrawTeardropTool::onCancel()
{
    reset();
}

void DrawTeardropTool::deactivate()
{
    reset();
}

void DrawTeardropTool::showPreview(const QPainterPath& path)
{
    if (!m_preview) {
        QPen pen(QColor(0x33, 0xcc, 0x55));  // green: construction preview
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_preview = m_scene->addPath(QPainterPath(), pen);
        m_preview->setZValue(1000);
    }
    m_preview->setPath(path);
}

void DrawTeardropTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

void DrawTeardropTool::reset()
{
    clearPreview();
    m_first = Circle{};
}

} // namespace cad
