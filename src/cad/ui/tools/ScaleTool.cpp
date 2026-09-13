#include "cad/ui/tools/ScaleTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/ModifyEntityCommand.h"
#include "cad/core/entities/CadEntity.h"
#include "cad/ui/render/EntityItem.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPainterPath>
#include <QPen>
#include <QTransform>
#include <QUndoStack>
#include <Qt>

#include <memory>
#include <utility>

namespace cad {

namespace {
constexpr double kEps = 1e-6;

QTransform scaleAbout(const QPointF& pivot, double factor)
{
    return QTransform()
        .translate(pivot.x(), pivot.y())
        .scale(factor, factor)
        .translate(-pivot.x(), -pivot.y());
}
}  // namespace

ScaleTool::ScaleTool(CadDocument* document, QUndoStack* undoStack, QGraphicsScene* scene,
                     QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void ScaleTool::onMousePress(const QPointF& scenePos)
{
    if (!m_hasBase) {
        captureSelection();
        if (m_ids.isEmpty()) {
            return;  // nothing selected — select first, then scale
        }
        m_base = scenePos;
        m_cursor = scenePos;
        m_hasBase = true;
        updateGhost(1.0);
        emit inputChanged();
        emit requestInputFocus();
    }
    else if (!m_hasReference) {
        const double len = QLineF(m_base, scenePos).length();
        if (len < kEps) {
            return;  // reference point must be away from the base
        }
        m_referenceLen = len;
        m_hasReference = true;
        m_cursor = scenePos;
    }
    else {
        commitScale(currentFactor());
    }
}

void ScaleTool::onMouseMove(const QPointF& scenePos)
{
    if (!m_hasBase) {
        return;
    }
    m_cursor = scenePos;
    if (m_hasReference) {
        updateGhost(currentFactor());
        emit inputChanged();
    }
}

void ScaleTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        reset();
    }
}

void ScaleTool::onCancel()
{
    reset();
}

void ScaleTool::deactivate()
{
    reset();
}

QList<CadTool::InputField> ScaleTool::inputFields() const
{
    if (!m_hasBase) {
        return {};
    }
    return {{tr("Factor"), currentFactor()}};
}

void ScaleTool::applyInput(const QVector<double>& values)
{
    if (m_hasBase && !values.isEmpty()) {
        commitScale(values[0]);  // typed factor: commit directly (no reference needed)
    }
}

void ScaleTool::captureSelection()
{
    m_ids.clear();
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            m_ids.append(entityItem->entityId());
        }
    }
}

double ScaleTool::currentFactor() const
{
    if (!m_hasReference || m_referenceLen < kEps) {
        return 1.0;
    }
    return QLineF(m_base, m_cursor).length() / m_referenceLen;
}

void ScaleTool::commitScale(double factor)
{
    if (factor > kEps) {
        m_undoStack->beginMacro(tr("Scale"));
        for (int id : m_ids) {
            if (CadEntity* e = m_document->entity(id)) {
                std::unique_ptr<CadEntity> scaled = e->clone();
                scaled->scale(m_base, factor);
                m_undoStack->push(new ModifyEntityCommand(m_document, id, std::move(scaled), tr("Scale")));
            }
        }
        m_undoStack->endMacro();
    }
    reset();
}

void ScaleTool::updateGhost(double factor)
{
    QPainterPath preview;
    const QTransform t = scaleAbout(m_base, factor);
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

void ScaleTool::reset()
{
    clearGhost();
    m_hasBase = false;
    m_hasReference = false;
    m_ids.clear();
    emit inputChanged();
}

void ScaleTool::clearGhost()
{
    if (m_ghost) {
        m_scene->removeItem(m_ghost);
        delete m_ghost;
        m_ghost = nullptr;
    }
}

} // namespace cad
