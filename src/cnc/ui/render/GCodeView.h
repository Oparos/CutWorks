#pragma once

#include <QGraphicsView>
#include <QPoint>

// Pan/zoom viewport for the toolpath. Y points up (machine convention): wheel
// zooms toward the cursor, the middle mouse button pans.
class GCodeView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit GCodeView(QWidget* parent = nullptr);

    void zoomToFit();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool m_panning = false;
    QPoint m_lastPanPos;
};
