#pragma once

#include <QGraphicsView>
#include <QPoint>

namespace cad {
class CadTool;
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
    cad::CadTool* m_tool = nullptr;
    bool m_panning = false;
    bool m_firstShow = true;
    QPoint m_lastPanPos;
};
