#include "cnc/ui/render/GCodeScene.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPen>

#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kToolRadiusMm = 2.5;

// Append an arc to the path by sampling it into short line segments — robust
// regardless of orientation, which is what a preview needs.
void appendArc(QPainterPath& path, const cnc::PathSegment& seg)
{
    const double dx = seg.start.x - seg.center.x;
    const double dy = seg.start.y - seg.center.y;
    const double radius = std::hypot(dx, dy);
    if (radius <= 0.0) {
        path.lineTo(seg.end.x, seg.end.y);
        return;
    }

    const double a0 = std::atan2(dy, dx);
    const double a1 = std::atan2(seg.end.y - seg.center.y, seg.end.x - seg.center.x);

    double sweep = seg.clockwise ? (a0 - a1) : (a1 - a0);
    while (sweep <= 1e-9) {
        sweep += 2.0 * kPi;
    }
    const double signedSweep = seg.clockwise ? -sweep : sweep;

    const int steps = std::max(2, static_cast<int>(std::abs(signedSweep) / (kPi / 90.0)));
    for (int i = 1; i <= steps; ++i) {
        const double a = a0 + signedSweep * (static_cast<double>(i) / steps);
        path.lineTo(seg.center.x + radius * std::cos(a),
                    seg.center.y + radius * std::sin(a));
    }
}

} // namespace

GCodeScene::GCodeScene(QObject* parent)
    : QGraphicsScene(parent)
{
    QPen rapidPen(QColor(0xe0, 0x55, 0x55));
    rapidPen.setCosmetic(true);
    rapidPen.setStyle(Qt::DashLine);
    m_rapidItem = addPath(QPainterPath(), rapidPen);

    QPen cutPen(QColor(0x4e, 0xc9, 0xb0));
    cutPen.setCosmetic(true);
    cutPen.setWidth(2);
    m_cutItem = addPath(QPainterPath(), cutPen);

    m_toolItem = addEllipse(-kToolRadiusMm, -kToolRadiusMm,
                            2 * kToolRadiusMm, 2 * kToolRadiusMm,
                            QPen(Qt::NoPen), QBrush(QColor(0xff, 0xcc, 0x33)));
    m_toolItem->setZValue(10);  // keep the tool marker above the path
}

void GCodeScene::setToolpath(const cnc::Toolpath& path)
{
    QPainterPath rapidPath;
    QPainterPath cutPath;

    for (const cnc::PathSegment& seg : path) {
        QPainterPath& target = (seg.move == cnc::MoveType::Rapid) ? rapidPath : cutPath;
        target.moveTo(seg.start.x, seg.start.y);
        if (seg.shape == cnc::SegmentShape::Arc) {
            appendArc(target, seg);
        }
        else {
            target.lineTo(seg.end.x, seg.end.y);
        }
    }

    m_rapidItem->setPath(rapidPath);
    m_cutItem->setPath(cutPath);
}

void GCodeScene::setToolPosition(double x, double y)
{
    m_toolItem->setPos(x, y);
}
