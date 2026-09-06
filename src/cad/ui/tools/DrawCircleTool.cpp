#include "cad/ui/tools/DrawCircleTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/CircleEntity.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QPen>
#include <QUndoStack>
#include <Qt>

#include <cmath>

namespace cad {

namespace {
QRectF circleRect(const QPointF& center, double radius)
{
    return QRectF(center.x() - radius, center.y() - radius, 2 * radius, 2 * radius);
}
}

DrawCircleTool::DrawCircleTool(CadDocument* document, QUndoStack* undoStack,
                               QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void DrawCircleTool::onMousePress(const QPointF& scenePos)
{
    if (!m_hasCenter) {
        m_center = scenePos;
        m_cursor = scenePos;
        m_hasCenter = true;

        QPen pen(QColor(0xff, 0x9c, 0x33));
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_preview = m_scene->addEllipse(circleRect(m_center, 0.0), pen);

        emit inputChanged();
        emit requestInputFocus();
    }
    else {
        const double radius = std::hypot(scenePos.x() - m_center.x(), scenePos.y() - m_center.y());
        commitCircle(radius);
    }
}

void DrawCircleTool::onMouseMove(const QPointF& scenePos)
{
    if (!m_hasCenter) {
        return;
    }
    m_cursor = scenePos;
    if (m_preview) {
        const double radius = std::hypot(m_cursor.x() - m_center.x(), m_cursor.y() - m_center.y());
        m_preview->setRect(circleRect(m_center, radius));
    }
    emit inputChanged();
}

void DrawCircleTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void DrawCircleTool::onCancel()
{
    reset();
}

void DrawCircleTool::deactivate()
{
    clearPreview();
    m_hasCenter = false;
}

QList<CadTool::InputField> DrawCircleTool::inputFields() const
{
    if (!m_hasCenter) {
        return {};
    }
    const double radius = std::hypot(m_cursor.x() - m_center.x(), m_cursor.y() - m_center.y());
    return {{tr("Radius"), radius}};
}

void DrawCircleTool::applyInput(const QVector<double>& values)
{
    if (m_hasCenter && !values.isEmpty()) {
        commitCircle(values[0]);
    }
}

void DrawCircleTool::commitCircle(double radius)
{
    if (radius > 0.0) {
        m_undoStack->push(new AddEntityCommand(
            m_document, std::make_unique<CircleEntity>(m_center, radius), tr("Draw circle")));
    }
    reset();
}

void DrawCircleTool::reset()
{
    clearPreview();
    m_hasCenter = false;
    emit inputChanged();
}

void DrawCircleTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

} // namespace cad
