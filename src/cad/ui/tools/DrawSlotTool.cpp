#include "cad/ui/tools/DrawSlotTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/PolylineEntity.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>
#include <Qt>

#include <cmath>

namespace cad {

namespace {

constexpr double kEps = 1e-6;

double axisLength(const QPointF& a, const QPointF& b)
{
    return std::hypot(b.x() - a.x(), b.y() - a.y());
}

// Perpendicular distance from p to the line through a and b (the slot radius).
double perpDistance(const QPointF& a, const QPointF& b, const QPointF& p)
{
    const QPointF d = b - a;
    const double len = std::hypot(d.x(), d.y());
    if (len < kEps) {
        return std::hypot(p.x() - a.x(), p.y() - a.y());
    }
    return std::abs((p.x() - a.x()) * d.y() - (p.y() - a.y()) * d.x()) / len;
}

// The four obround vertices: two straight sides and two semicircle caps
// (bulge = -1 sweeps the cap outward, away from the axis).
QVector<PolyVertex> obroundVertices(const QPointF& a, const QPointF& b, double r)
{
    const QPointF d = b - a;
    const double len = std::hypot(d.x(), d.y());
    const QPointF u(d.x() / len, d.y() / len);
    const QPointF n(-u.y(), u.x());  // left normal
    const QPointF off = r * n;
    return {
        {a + off, 0.0},   // side A→B
        {b + off, -1.0},  // cap at B
        {b - off, 0.0},   // side B→A
        {a - off, -1.0},  // cap at A (wraps to the first vertex)
    };
}

} // namespace

DrawSlotTool::DrawSlotTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
                           QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

double DrawSlotTool::currentRadius() const
{
    return perpDistance(m_centerA, m_centerB, m_cursor);
}

void DrawSlotTool::onMousePress(const QPointF& scenePos)
{
    if (m_stage == 0) {
        m_centerA = scenePos;
        m_cursor = scenePos;
        m_stage = 1;
        updatePreview();
    }
    else if (m_stage == 1) {
        if (axisLength(m_centerA, scenePos) < kEps) {
            return;  // need two distinct end centers
        }
        m_centerB = scenePos;
        m_cursor = scenePos;
        m_stage = 2;
        updatePreview();
        emit inputChanged();
        emit requestInputFocus();
    }
    else {
        commit(currentRadius());
    }
}

void DrawSlotTool::onMouseMove(const QPointF& scenePos)
{
    if (m_stage == 0) {
        return;
    }
    m_cursor = scenePos;
    updatePreview();
    emit inputChanged();
}

void DrawSlotTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void DrawSlotTool::onCancel()
{
    reset();
}

void DrawSlotTool::deactivate()
{
    reset();
}

QList<CadTool::InputField> DrawSlotTool::inputFields() const
{
    if (m_stage != 2) {
        return {};
    }
    return {{tr("Radius"), currentRadius()}};
}

void DrawSlotTool::applyInput(const QVector<double>& values)
{
    if (m_stage == 2 && !values.isEmpty()) {
        commit(values[0]);
    }
}

void DrawSlotTool::updatePreview()
{
    QPainterPath path;
    if (m_stage == 1) {
        path.moveTo(m_centerA);  // just the axis while placing the second center
        path.lineTo(m_cursor);
    }
    else if (m_stage == 2) {
        const double r = currentRadius();
        if (r < kEps) {
            path.moveTo(m_centerA);
            path.lineTo(m_centerB);
        }
        else {
            path = PolylineEntity(obroundVertices(m_centerA, m_centerB, r), /*closed*/ true).path();
        }
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

void DrawSlotTool::commit(double radius)
{
    if (axisLength(m_centerA, m_centerB) >= kEps && radius >= kEps) {
        m_undoStack->push(new AddEntityCommand(
            m_document, std::make_unique<PolylineEntity>(obroundVertices(m_centerA, m_centerB, radius),
                                                         /*closed*/ true),
            tr("Draw slot")));
    }
    reset();
}

void DrawSlotTool::reset()
{
    clearPreview();
    m_stage = 0;
    emit inputChanged();
}

void DrawSlotTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

} // namespace cad
