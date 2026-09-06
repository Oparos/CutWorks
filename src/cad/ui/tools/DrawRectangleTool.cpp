#include "cad/ui/tools/DrawRectangleTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/PolylineEntity.h"

#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QPen>
#include <QUndoStack>
#include <Qt>

namespace cad {

DrawRectangleTool::DrawRectangleTool(CadDocument* document, QUndoStack* undoStack,
                                     QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void DrawRectangleTool::onMousePress(const QPointF& scenePos)
{
    if (!m_hasCorner1) {
        m_corner1 = scenePos;
        m_cursor = scenePos;
        m_hasCorner1 = true;

        QPen pen(QColor(0xff, 0x9c, 0x33));
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_preview = m_scene->addRect(QRectF(m_corner1, m_corner1), pen);

        emit inputChanged();
        emit requestInputFocus();
    }
    else {
        commitRect(scenePos);
    }
}

void DrawRectangleTool::onMouseMove(const QPointF& scenePos)
{
    if (!m_hasCorner1) {
        return;
    }
    m_cursor = scenePos;
    if (m_preview) {
        m_preview->setRect(QRectF(m_corner1, m_cursor).normalized());
    }
    emit inputChanged();
}

void DrawRectangleTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void DrawRectangleTool::onCancel()
{
    reset();
}

void DrawRectangleTool::deactivate()
{
    clearPreview();
    m_hasCorner1 = false;
}

QList<CadTool::InputField> DrawRectangleTool::inputFields() const
{
    if (!m_hasCorner1) {
        return {};
    }
    return {{tr("Width"), m_cursor.x() - m_corner1.x()},
            {tr("Height"), m_cursor.y() - m_corner1.y()}};
}

void DrawRectangleTool::applyInput(const QVector<double>& values)
{
    if (m_hasCorner1 && values.size() >= 2) {
        commitRect(m_corner1 + QPointF(values[0], values[1]));
    }
}

void DrawRectangleTool::commitRect(const QPointF& corner2)
{
    if (!qFuzzyCompare(corner2.x(), m_corner1.x()) && !qFuzzyCompare(corner2.y(), m_corner1.y())) {
        const QVector<QPointF> corners = {
            m_corner1,
            QPointF(corner2.x(), m_corner1.y()),
            corner2,
            QPointF(m_corner1.x(), corner2.y()),
        };
        m_undoStack->push(new AddEntityCommand(
            m_document, std::make_unique<PolylineEntity>(corners, /*closed*/ true),
            tr("Draw rectangle")));
    }
    reset();
}

void DrawRectangleTool::reset()
{
    clearPreview();
    m_hasCorner1 = false;
    emit inputChanged();
}

void DrawRectangleTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

} // namespace cad
