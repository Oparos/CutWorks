#include "cad/ui/tools/DrawPolylineTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/PolylineEntity.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>
#include <Qt>

#include <cmath>

namespace cad {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr double kDegToRad = kPi / 180.0;
}

DrawPolylineTool::DrawPolylineTool(CadDocument* document, QUndoStack* undoStack,
                                   QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void DrawPolylineTool::onMousePress(const QPointF& scenePos)
{
    addVertex(scenePos);
}

void DrawPolylineTool::onMouseMove(const QPointF& scenePos)
{
    if (m_vertices.isEmpty()) {
        return;
    }
    m_cursor = scenePos;
    updatePreview();
    emit inputChanged();
}

void DrawPolylineTool::onKeyPress(int key)
{
    // Esc / Enter / right-click all end the polyline, committing it if it has at
    // least two vertices (finish() just clears the preview otherwise). 'C' closes.
    if (key == Qt::Key_Escape || key == Qt::Key_Return || key == Qt::Key_Enter) {
        finish(false);
    }
    else if (key == Qt::Key_C) {
        finish(true);
    }
}

void DrawPolylineTool::onCancel()
{
    finish(false);  // right-click ends the polyline (open)
}

void DrawPolylineTool::deactivate()
{
    discard();
}

QList<CadTool::InputField> DrawPolylineTool::inputFields() const
{
    if (m_vertices.isEmpty()) {
        return {};
    }
    const QPointF last = m_vertices.last();
    const double dx = m_cursor.x() - last.x();
    const double dy = m_cursor.y() - last.y();
    return {{tr("Length"), std::hypot(dx, dy)}, {tr("Angle"), std::atan2(dy, dx) * kRadToDeg}};
}

void DrawPolylineTool::applyInput(const QVector<double>& values)
{
    if (m_vertices.isEmpty() || values.size() < 2) {
        return;
    }
    const double length = values[0];
    const double angle = values[1] * kDegToRad;
    const QPointF last = m_vertices.last();
    addVertex(last + QPointF(length * std::cos(angle), length * std::sin(angle)));
    emit requestInputFocus();  // return focus to Length for the next segment
}

void DrawPolylineTool::addVertex(const QPointF& pos)
{
    const bool first = m_vertices.isEmpty();
    m_vertices.push_back(pos);
    m_cursor = pos;
    updatePreview();
    emit inputChanged();
    if (first) {
        emit requestInputFocus();
    }
}

void DrawPolylineTool::finish(bool closed)
{
    if (m_vertices.size() >= 2) {
        m_undoStack->push(new AddEntityCommand(
            m_document, std::make_unique<PolylineEntity>(m_vertices, closed), tr("Draw polyline")));
    }
    discard();
}

void DrawPolylineTool::discard()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
    m_vertices.clear();
    emit inputChanged();
}

void DrawPolylineTool::updatePreview()
{
    QPainterPath path;
    if (!m_vertices.isEmpty()) {
        path.moveTo(m_vertices.first());
        for (int i = 1; i < m_vertices.size(); ++i) {
            path.lineTo(m_vertices.at(i));
        }
        path.lineTo(m_cursor);  // rubber segment to the cursor
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

} // namespace cad
