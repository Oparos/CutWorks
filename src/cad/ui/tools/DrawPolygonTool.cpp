#include "cad/ui/tools/DrawPolygonTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/PolylineEntity.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>
#include <Qt>

#include <algorithm>
#include <cmath>

namespace cad {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

DrawPolygonTool::DrawPolygonTool(CadDocument* document, QUndoStack* undoStack,
                                 QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

QVector<QPointF> DrawPolygonTool::polygonPoints(double radius) const
{
    QVector<QPointF> pts;
    const int n = std::max(3, m_sides);
    pts.reserve(n);
    for (int i = 0; i < n; ++i) {
        // Start at the top (90 deg) and go around; vertices lie on the radius.
        const double a = kPi / 2.0 + (2.0 * kPi * i) / n;
        pts.push_back(m_center + QPointF(radius * std::cos(a), radius * std::sin(a)));
    }
    return pts;
}

void DrawPolygonTool::onMousePress(const QPointF& scenePos)
{
    if (!m_hasCenter) {
        m_center = scenePos;
        m_cursor = scenePos;
        m_hasCenter = true;

        QPen pen(QColor(0xff, 0x9c, 0x33));
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_preview = m_scene->addPath(QPainterPath(), pen);

        emit inputChanged();
        emit requestInputFocus();
    }
    else {
        commitPolygon(std::hypot(scenePos.x() - m_center.x(), scenePos.y() - m_center.y()));
    }
}

void DrawPolygonTool::onMouseMove(const QPointF& scenePos)
{
    if (!m_hasCenter) {
        return;
    }
    m_cursor = scenePos;
    if (m_preview) {
        const double radius = std::hypot(m_cursor.x() - m_center.x(), m_cursor.y() - m_center.y());
        QPainterPath path;
        const QVector<QPointF> pts = polygonPoints(radius);
        if (!pts.isEmpty()) {
            path.moveTo(pts.first());
            for (int i = 1; i < pts.size(); ++i) {
                path.lineTo(pts.at(i));
            }
            path.closeSubpath();
        }
        m_preview->setPath(path);
    }
    emit inputChanged();
}

void DrawPolygonTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void DrawPolygonTool::onCancel()
{
    reset();
}

void DrawPolygonTool::deactivate()
{
    clearPreview();
    m_hasCenter = false;
}

QList<CadTool::InputField> DrawPolygonTool::inputFields() const
{
    if (!m_hasCenter) {
        return {};
    }
    const double radius = std::hypot(m_cursor.x() - m_center.x(), m_cursor.y() - m_center.y());
    return {{tr("Sides"), static_cast<double>(m_sides)}, {tr("Radius"), radius}};
}

void DrawPolygonTool::applyInput(const QVector<double>& values)
{
    if (!m_hasCenter || values.size() < 2) {
        return;
    }
    m_sides = std::max(3, static_cast<int>(std::lround(values[0])));
    commitPolygon(values[1]);
}

void DrawPolygonTool::commitPolygon(double radius)
{
    if (radius > 0.0) {
        m_undoStack->push(new AddEntityCommand(
            m_document, std::make_unique<PolylineEntity>(polygonPoints(radius), /*closed*/ true),
            tr("Draw polygon")));
    }
    reset();
}

void DrawPolygonTool::reset()
{
    clearPreview();
    m_hasCenter = false;
    emit inputChanged();
}

void DrawPolygonTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

} // namespace cad
