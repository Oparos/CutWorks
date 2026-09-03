#pragma once

#include "cnc/core/gcode/Toolpath.h"

#include <QGraphicsScene>

class QGraphicsPathItem;
class QGraphicsEllipseItem;

// Draws a toolpath: rapids (G0) dashed, cuts (G1/G2/G3) solid, plus a marker for
// the current tool position. Works on the parsed toolpath model, not on raw
// G-code.
class GCodeScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit GCodeScene(QObject* parent = nullptr);

    void setToolpath(const cnc::Toolpath& path);
    void setToolPosition(double x, double y);

private:
    QGraphicsPathItem* m_rapidItem = nullptr;
    QGraphicsPathItem* m_cutItem = nullptr;
    QGraphicsEllipseItem* m_toolItem = nullptr;
};
