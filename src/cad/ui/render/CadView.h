#pragma once

#include <QGraphicsView>
#include <QPoint>

class QGraphicsPathItem;

namespace cad {
class CadTool;
class SnapEngine;
}

// Pan/zoom viewport for the CAD scene. Y points up (CAD convention). It converts
// mouse events to scene coordinates and forwards them to the active tool (which
// it does not own). With no tool, it behaves as a normal selecting view.
class CadView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CadView(QWidget* parent = nullptr);

    void setTool(cad::CadTool* tool);
    void setSnapEngine(cad::SnapEngine* snap) { m_snap = snap; }

signals:
    // Tab was pressed while a tool is active — the UI should move keyboard focus
    // into the parametric input bar.
    void focusInputRequested();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool focusNextPrevChild(bool next) override;

private:
    // Snap the raw cursor position to nearby geometry, updating the marker.
    QPointF applySnap(const QPointF& scenePos);
    void hideSnapMarker();

    cad::CadTool* m_tool = nullptr;
    cad::SnapEngine* m_snap = nullptr;
    QGraphicsPathItem* m_snapMarker = nullptr;
    bool m_panning = false;
    bool m_firstShow = true;
    QPoint m_lastPanPos;
};
