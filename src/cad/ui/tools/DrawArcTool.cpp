#include "cad/ui/tools/DrawArcTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/ArcEntity.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>
#include <Qt>

#include <cmath>

namespace cad {

namespace {
constexpr double kRadToDeg = 180.0 / 3.14159265358979323846;

double angleDeg(const QPointF& from, const QPointF& to)
{
    return std::atan2(to.y() - from.y(), to.x() - from.x()) * kRadToDeg;
}

double distance(const QPointF& a, const QPointF& b)
{
    return std::hypot(b.x() - a.x(), b.y() - a.y());
}

// Shortest signed sweep from start to end, in (-180, 180]. Positive = CCW,
// negative = CW — so the arc follows the mouse the natural (short) way instead
// of always wrapping counter-clockwise.
double signedSweep(double startDeg, double endDeg)
{
    double sweep = endDeg - startDeg;
    while (sweep > 180.0) {
        sweep -= 360.0;
    }
    while (sweep <= -180.0) {
        sweep += 360.0;
    }
    return sweep;
}
}

DrawArcTool::DrawArcTool(CadDocument* document, QUndoStack* undoStack,
                         QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void DrawArcTool::onMousePress(const QPointF& scenePos)
{
    switch (m_stage) {
    case Stage::Center:
        m_center = scenePos;
        m_cursor = scenePos;
        m_stage = Stage::Start;
        updatePreview();
        break;
    case Stage::Start:
        m_radius = distance(m_center, scenePos);
        m_startAngle = angleDeg(m_center, scenePos);
        m_cursor = scenePos;
        m_sweep = 0.0;
        m_lastAngle = m_startAngle;  // cursor sits on the start point right now
        m_stage = Stage::End;
        updatePreview();
        break;
    case Stage::End:
        if (m_radius > 0.0 && m_sweep != 0.0) {
            m_undoStack->push(new AddEntityCommand(
                m_document, std::make_unique<ArcEntity>(m_center, m_radius, m_startAngle, m_sweep),
                tr("Draw arc")));
        }
        reset();
        break;
    }
}

void DrawArcTool::onMouseMove(const QPointF& scenePos)
{
    if (m_stage == Stage::Center) {
        return;
    }
    m_cursor = scenePos;
    if (m_stage == Stage::End) {
        // Follow how far the cursor travels around the center, so continuing to
        // circle grows the arc past 180 deg instead of snapping to the short side.
        const double current = angleDeg(m_center, scenePos);
        m_sweep += signedSweep(m_lastAngle, current);
        m_lastAngle = current;
    }
    updatePreview();
}

void DrawArcTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void DrawArcTool::onCancel()
{
    reset();
}

void DrawArcTool::deactivate()
{
    clearPreview();
    m_stage = Stage::Center;
}

void DrawArcTool::updatePreview()
{
    QPainterPath path;
    if (m_stage == Stage::Start) {
        // Show the circle the arc will lie on.
        const double r = distance(m_center, m_cursor);
        path.addEllipse(m_center, r, r);
    }
    else if (m_stage == Stage::End) {
        path = ArcEntity(m_center, m_radius, m_startAngle, m_sweep).path();
    }

    if (!m_preview) {
        QPen pen(QColor(0xff, 0x9c, 0x33));
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_preview = m_scene->addPath(path, pen);
    }
    else {
        m_preview->setPath(path);
    }
}

void DrawArcTool::reset()
{
    clearPreview();
    m_stage = Stage::Center;
    m_sweep = 0.0;
    m_lastAngle = 0.0;
}

void DrawArcTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

} // namespace cad
