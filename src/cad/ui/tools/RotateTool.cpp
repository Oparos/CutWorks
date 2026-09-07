#include "cad/ui/tools/RotateTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/ModifyEntityCommand.h"
#include "cad/core/entities/CadEntity.h"
#include "cad/ui/render/EntityItem.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>
#include <QTransform>
#include <QUndoStack>
#include <Qt>

#include <cmath>

namespace cad {

namespace {
QTransform rotationAround(const QPointF& pivot, double angleDeg)
{
    return QTransform()
        .translate(pivot.x(), pivot.y())
        .rotate(angleDeg)
        .translate(-pivot.x(), -pivot.y());
}
}

RotateTool::RotateTool(CadDocument* document, QUndoStack* undoStack,
                       QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

double RotateTool::currentAngle() const
{
    return std::atan2(m_cursor.y() - m_pivot.y(), m_cursor.x() - m_pivot.x())
           * 180.0 / 3.14159265358979323846;
}

void RotateTool::onMousePress(const QPointF& scenePos)
{
    if (!m_hasPivot) {
        captureSelection();
        if (m_ids.isEmpty()) {
            return;  // select something first
        }
        m_pivot = scenePos;
        m_cursor = scenePos;
        m_hasPivot = true;
        updateGhost();
        emit inputChanged();
        emit requestInputFocus();
    }
    else {
        commitRotate(currentAngle());
    }
}

void RotateTool::onMouseMove(const QPointF& scenePos)
{
    if (!m_hasPivot) {
        return;
    }
    m_cursor = scenePos;
    updateGhost();
    emit inputChanged();
}

void RotateTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void RotateTool::onCancel()
{
    reset();
}

void RotateTool::deactivate()
{
    reset();
}

QList<CadTool::InputField> RotateTool::inputFields() const
{
    if (!m_hasPivot) {
        return {};
    }
    return {{tr("Angle"), currentAngle()}};
}

void RotateTool::applyInput(const QVector<double>& values)
{
    if (m_hasPivot && !values.isEmpty()) {
        commitRotate(values[0]);
    }
}

void RotateTool::captureSelection()
{
    m_ids.clear();
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            m_ids.append(entityItem->entityId());
        }
    }
}

void RotateTool::commitRotate(double angleDeg)
{
    m_undoStack->beginMacro(tr("Rotate"));
    for (int id : m_ids) {
        if (CadEntity* e = m_document->entity(id)) {
            std::unique_ptr<CadEntity> rotated = e->clone();
            rotated->rotate(m_pivot, angleDeg);
            m_undoStack->push(new ModifyEntityCommand(m_document, id, std::move(rotated), tr("Rotate")));
        }
    }
    m_undoStack->endMacro();
    reset();
}

void RotateTool::updateGhost()
{
    const QTransform t = rotationAround(m_pivot, currentAngle());
    QPainterPath preview;
    for (int id : m_ids) {
        if (const CadEntity* e = m_document->entity(id)) {
            preview.addPath(t.map(e->path()));
        }
    }

    if (!m_ghost) {
        QPen pen(QColor(0xff, 0x9c, 0x33));
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_ghost = m_scene->addPath(preview, pen);
        m_ghost->setZValue(1000);
    }
    else {
        m_ghost->setPath(preview);
    }
}

void RotateTool::reset()
{
    clearGhost();
    m_hasPivot = false;
    m_ids.clear();
    emit inputChanged();
}

void RotateTool::clearGhost()
{
    if (m_ghost) {
        m_scene->removeItem(m_ghost);
        delete m_ghost;
        m_ghost = nullptr;
    }
}

} // namespace cad
