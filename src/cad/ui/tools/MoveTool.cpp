#include "cad/ui/tools/MoveTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/ModifyEntityCommand.h"
#include "cad/core/entities/CadEntity.h"
#include "cad/ui/render/EntityItem.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>
#include <Qt>

namespace cad {

MoveTool::MoveTool(CadDocument* document, QUndoStack* undoStack,
                   QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void MoveTool::onMousePress(const QPointF& scenePos)
{
    if (!m_hasBase) {
        captureSelection();
        if (m_ids.isEmpty()) {
            return;  // nothing selected — select first, then move
        }
        m_base = scenePos;
        m_cursor = scenePos;
        m_hasBase = true;
        updateGhost();
        emit inputChanged();
        emit requestInputFocus();
    }
    else {
        commitMove(scenePos - m_base);
    }
}

void MoveTool::onMouseMove(const QPointF& scenePos)
{
    if (!m_hasBase) {
        return;
    }
    m_cursor = scenePos;
    updateGhost();
    emit inputChanged();
}

void MoveTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void MoveTool::onCancel()
{
    reset();
}

void MoveTool::deactivate()
{
    reset();
}

QList<CadTool::InputField> MoveTool::inputFields() const
{
    if (!m_hasBase) {
        return {};
    }
    return {{tr("dX"), m_cursor.x() - m_base.x()}, {tr("dY"), m_cursor.y() - m_base.y()}};
}

void MoveTool::applyInput(const QVector<double>& values)
{
    if (m_hasBase && values.size() >= 2) {
        commitMove(QPointF(values[0], values[1]));
    }
}

void MoveTool::captureSelection()
{
    m_ids.clear();
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            m_ids.append(entityItem->entityId());
        }
    }
}

void MoveTool::commitMove(const QPointF& delta)
{
    m_undoStack->beginMacro(tr("Move"));
    for (int id : m_ids) {
        if (CadEntity* e = m_document->entity(id)) {
            std::unique_ptr<CadEntity> moved = e->clone();
            moved->translate(delta);
            m_undoStack->push(new ModifyEntityCommand(m_document, id, std::move(moved), tr("Move")));
        }
    }
    m_undoStack->endMacro();
    reset();
}

void MoveTool::updateGhost()
{
    QPainterPath preview;
    const QPointF delta = m_cursor - m_base;
    for (int id : m_ids) {
        if (const CadEntity* e = m_document->entity(id)) {
            preview.addPath(e->path().translated(delta));
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

void MoveTool::reset()
{
    clearGhost();
    m_hasBase = false;
    m_ids.clear();
    emit inputChanged();
}

void MoveTool::clearGhost()
{
    if (m_ghost) {
        m_scene->removeItem(m_ghost);
        delete m_ghost;
        m_ghost = nullptr;
    }
}

} // namespace cad
