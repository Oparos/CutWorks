#include "cad/ui/render/CadView.h"

#include "cad/ui/tools/CadTool.h"

#include <QMouseEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QWheelEvent>

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
        event->accept();
        return;
    }
    if (m_tool && event->button() == Qt::RightButton) {
        m_tool->onCancel();  // right-click ends the current operation
        event->accept();
        return;
    }
    if (m_tool && event->button() == Qt::LeftButton) {
        m_tool->onMousePress(mapToScene(event->position().toPoint()));
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
        m_tool->onMouseMove(mapToScene(event->position().toPoint()));
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
    QGraphicsView::mouseReleaseEvent(event);
}

void CadView::keyPressEvent(QKeyEvent* event)
{
    if (m_tool) {
        m_tool->onKeyPress(event->key());
    }
    QGraphicsView::keyPressEvent(event);
}
