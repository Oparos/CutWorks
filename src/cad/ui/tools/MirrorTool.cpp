#include "cad/ui/tools/MirrorTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/CadEntity.h"
#include "cad/ui/render/EntityItem.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>
#include <QTransform>
#include <Qt>
#include <QUndoStack>

#include <cmath>

namespace cad {

namespace {
// Reflection across the line through a at the a->b direction. Ordered so the
// point is un-translated/un-rotated, flipped in Y, then restored.
QTransform reflectionAcross(const QPointF& a, const QPointF& b)
{
    const double angle = std::atan2(b.y() - a.y(), b.x() - a.x()) * 180.0 / 3.14159265358979323846;
    QTransform t;
    t.translate(a.x(), a.y());
    t.rotate(angle);
    t.scale(1.0, -1.0);
    t.rotate(-angle);
    t.translate(-a.x(), -a.y());
    return t;
}
}

MirrorTool::MirrorTool(CadDocument* document, QUndoStack* undoStack,
                       QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void MirrorTool::onMousePress(const QPointF& scenePos)
{
    if (!m_hasAxisStart) {
        captureSelection();
        if (m_ids.isEmpty()) {
            return;  // select something first
        }
        m_axisA = scenePos;
        m_cursor = scenePos;
        m_hasAxisStart = true;
        updateGhost();
    }
    else {
        commitMirror(scenePos);
    }
}

void MirrorTool::onMouseMove(const QPointF& scenePos)
{
    if (!m_hasAxisStart) {
        return;
    }
    m_cursor = scenePos;
    updateGhost();
}

void MirrorTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void MirrorTool::onCancel()
{
    reset();
}

void MirrorTool::deactivate()
{
    reset();
}

void MirrorTool::captureSelection()
{
    m_ids.clear();
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            m_ids.append(entityItem->entityId());
        }
    }
}

void MirrorTool::commitMirror(const QPointF& axisB)
{
    if (axisB == m_axisA) {
        reset();
        return;  // degenerate axis
    }
    m_undoStack->beginMacro(tr("Mirror"));
    for (int id : m_ids) {
        if (CadEntity* e = m_document->entity(id)) {
            std::unique_ptr<CadEntity> copy = e->clone();
            copy->mirror(m_axisA, axisB);
            m_undoStack->push(new AddEntityCommand(m_document, std::move(copy), tr("Mirror copy")));
        }
    }
    m_undoStack->endMacro();
    reset();
}

void MirrorTool::updateGhost()
{
    const QTransform t = reflectionAcross(m_axisA, m_cursor);
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

void MirrorTool::reset()
{
    clearGhost();
    m_hasAxisStart = false;
    m_ids.clear();
}

void MirrorTool::clearGhost()
{
    if (m_ghost) {
        m_scene->removeItem(m_ghost);
        delete m_ghost;
        m_ghost = nullptr;
    }
}

} // namespace cad
