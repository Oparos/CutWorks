#include "cnc/ui/render/GCodeView.h"

#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>

GCodeView::GCodeView(QWidget* parent)
    : QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setBackgroundBrush(QColor(0x1e, 0x1e, 0x1e));

    // Flip Y so the machine's Y axis points up on screen.
    scale(1.0, -1.0);
}

void GCodeView::zoomToFit()
{
    if (!scene()) {
        return;
    }
    const QRectF rect = scene()->itemsBoundingRect();
    if (!rect.isEmpty()) {
        fitInView(rect.adjusted(-10, -10, 10, 10), Qt::KeepAspectRatio);
    }
}

void GCodeView::wheelEvent(QWheelEvent* event)
{
    const double factor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
    scale(factor, factor);
}

void GCodeView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void GCodeView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        const QPoint delta = event->pos() - m_lastPanPos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        m_lastPanPos = event->pos();
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void GCodeView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}
