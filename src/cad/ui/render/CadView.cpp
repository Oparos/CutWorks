#include "cad/ui/render/CadView.h"

#include "cad/core/snap/SnapEngine.h"
#include "cad/ui/render/CadScene.h"
#include "cad/ui/tools/CadTool.h"

#include <QGraphicsPathItem>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QScrollBar>
#include <QShowEvent>
#include <QWheelEvent>

#include <cmath>

namespace {

// Marker glyph in screen pixels (the item ignores the view transform), one shape
// per snap kind — the usual CAD convention.
QPainterPath snapMarkerPath(cad::SnapType type)
{
    constexpr double r = 5.0;
    QPainterPath path;
    switch (type) {
    case cad::SnapType::Endpoint:  // square
        path.addRect(-r, -r, 2 * r, 2 * r);
        break;
    case cad::SnapType::Midpoint:  // triangle
        path.moveTo(0, -r);
        path.lineTo(r, r);
        path.lineTo(-r, r);
        path.closeSubpath();
        break;
    case cad::SnapType::Center:  // circle
        path.addEllipse(QPointF(0, 0), r, r);
        break;
    case cad::SnapType::Intersection:  // cross
        path.moveTo(-r, -r);
        path.lineTo(r, r);
        path.moveTo(-r, r);
        path.lineTo(r, -r);
        break;
    case cad::SnapType::Perpendicular:  // right-angle symbol
        path.moveTo(-r, -r);
        path.lineTo(-r, r);
        path.lineTo(r, r);
        path.moveTo(-r, 0);
        path.lineTo(0, 0);
        path.lineTo(0, r);
        break;
    case cad::SnapType::Tangent:  // circle with a tangent line on top
        path.addEllipse(QPointF(0, 0), r * 0.7, r * 0.7);
        path.moveTo(-r, -r * 0.7);
        path.lineTo(r, -r * 0.7);
        break;
    case cad::SnapType::Grid:  // plus
        path.moveTo(-r, 0);
        path.lineTo(r, 0);
        path.moveTo(0, -r);
        path.lineTo(0, r);
        break;
    case cad::SnapType::None:
        break;
    }
    return path;
}

} // namespace

CadView::CadView(QWidget* parent)
    : QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setSceneRect(-1000000, -1000000, 2000000, 2000000);

    scale(1.0, -1.0);   // Y up
}

void CadView::showEvent(QShowEvent* event)
{
    QGraphicsView::showEvent(event);
    if (m_firstShow) {
        m_firstShow = false;
        // Place the origin near the bottom-left corner (Y up), so the drawing
        // area reads like a natural first quadrant. Scale is 1 => 1 mm ≈ 1 px.
        const double margin = 40.0;
        centerOn(viewport()->width() / 2.0 - margin, viewport()->height() / 2.0 - margin);
    }
}

bool CadView::focusNextPrevChild(bool next)
{
    // Intercept Tab so it jumps into the parametric input bar instead of the
    // usual focus cycling, but only while a tool is active.
    if (next && m_tool) {
        emit focusInputRequested();
        return true;
    }
    return QGraphicsView::focusNextPrevChild(next);
}

void CadView::setTool(cad::CadTool* tool)
{
    m_tool = tool;
    // A drawing tool handles clicks itself; without one we allow rubber-band select.
    setDragMode(tool ? QGraphicsView::NoDrag : QGraphicsView::RubberBandDrag);
    hideSnapMarker();
}

QPointF CadView::applySnap(const QPointF& scenePos)
{
    if (!m_snap) {
        return scenePos;
    }
    const double scale = std::abs(transform().m11());
    const double tolerance = (scale > 1e-9) ? 10.0 / scale : 10.0;  // ~10 px, in scene units
    const double gridStep = CadScene::minorGridStep(scale);
    const std::optional<QPointF> reference = m_tool ? m_tool->referencePoint() : std::nullopt;

    const cad::SnapResult result = m_snap->snap(scenePos, tolerance, gridStep, reference);
    if (!result.hit) {
        hideSnapMarker();
        return scenePos;
    }

    if (!m_snapMarker) {
        m_snapMarker = new QGraphicsPathItem();
        m_snapMarker->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        QPen pen(QColor(0x33, 0xcc, 0xff));  // cyan snap marker
        pen.setCosmetic(true);
        pen.setWidthF(1.4);
        m_snapMarker->setPen(pen);
        m_snapMarker->setBrush(Qt::NoBrush);
        m_snapMarker->setZValue(10000);
        scene()->addItem(m_snapMarker);
    }
    m_snapMarker->setPath(snapMarkerPath(result.type));
    m_snapMarker->setPos(result.point);
    m_snapMarker->show();
    return result.point;
}

void CadView::hideSnapMarker()
{
    if (m_snapMarker) {
        m_snapMarker->hide();
    }
}

void CadView::wheelEvent(QWheelEvent* event)
{
    const QPointF beforePos = mapToScene(event->position().toPoint());
    const double factor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
    scale(factor, factor);
    // Keep the point under the cursor fixed.
    const QPointF afterPos = mapToScene(event->position().toPoint());
    const QPointF delta = afterPos - beforePos;
    translate(delta.x(), delta.y());
}

void CadView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPos = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
        hideSnapMarker();
        event->accept();
        return;
    }
    if (m_tool && event->button() == Qt::RightButton) {
        m_tool->onCancel();  // right-click ends the current operation
        event->accept();
        return;
    }
    if (m_tool && event->button() == Qt::LeftButton) {
        m_tool->onMousePress(applySnap(mapToScene(event->position().toPoint())));
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void CadView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        const QPoint delta = event->position().toPoint() - m_lastPanPos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        m_lastPanPos = event->position().toPoint();
        event->accept();
        return;
    }
    if (m_tool) {
        m_tool->onMouseMove(applySnap(mapToScene(event->position().toPoint())));
    }
    QGraphicsView::mouseMoveEvent(event);
}

void CadView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    if (m_tool && event->button() == Qt::LeftButton) {
        m_tool->onMouseRelease(applySnap(mapToScene(event->position().toPoint())));
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void CadView::keyPressEvent(QKeyEvent* event)
{
    if (m_tool) {
        m_tool->onKeyPress(event->key());
    }
    QGraphicsView::keyPressEvent(event);
}
