#include "cad/ui/tools/DrawTangentTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/ArcEntity.h"
#include "cad/core/entities/CircleEntity.h"
#include "cad/core/entities/LineEntity.h"
#include "cad/core/geometry/Tangents.h"
#include "cad/ui/render/EntityItem.h"
#include "cad/ui/tools/ToolPick.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>
#include <QUndoStack>

#include <memory>

namespace cad {

DrawTangentTool::DrawTangentTool(CadDocument* document, QUndoStack* undoStack,
                                 QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

DrawTangentTool::Circle DrawTangentTool::circleAt(const QPointF& scenePos) const
{
    EntityItem* entityItem = pickEntityItem(m_scene, scenePos);
    if (!entityItem) {
        return {};
    }
    CadEntity* entity = m_document->entity(entityItem->entityId());
    if (!entity) {
        return {};
    }
    if (entity->type() == EntityType::Circle) {
        const auto& c = static_cast<const CircleEntity&>(*entity);
        return {entityItem->entityId(), c.center(), c.radius()};
    }
    if (entity->type() == EntityType::Arc) {
        const auto& a = static_cast<const ArcEntity&>(*entity);
        return {entityItem->entityId(), a.center(), a.radius()};
    }
    return {};
}

void DrawTangentTool::onMouseMove(const QPointF& scenePos)
{
    if (m_first.id < 0) {
        return;  // nothing to preview until the first circle is chosen
    }
    const Circle second = circleAt(scenePos);
    if (second.id < 0 || second.id == m_first.id) {
        clearPreview();
        return;
    }
    showPreview(geom::externalTangents(m_first.center, m_first.radius, second.center, second.radius));
}

void DrawTangentTool::onMousePress(const QPointF& scenePos)
{
    const Circle hit = circleAt(scenePos);
    if (m_first.id < 0) {
        if (hit.id >= 0) {
            m_first = hit;
        }
        return;
    }
    if (hit.id < 0 || hit.id == m_first.id) {
        return;  // need a different circle to define the tangents
    }

    const QVector<QLineF> tangents =
        geom::externalTangents(m_first.center, m_first.radius, hit.center, hit.radius);
    if (tangents.isEmpty()) {
        return;  // no external tangent (e.g. one circle inside the other); keep the first pick
    }

    m_undoStack->beginMacro(tr("Tangent lines"));
    for (const QLineF& t : tangents) {
        m_undoStack->push(
            new AddEntityCommand(m_document, std::make_unique<LineEntity>(t.p1(), t.p2()), tr("Tangent")));
    }
    m_undoStack->endMacro();

    reset();
}

void DrawTangentTool::onCancel()
{
    reset();
}

void DrawTangentTool::deactivate()
{
    reset();
}

void DrawTangentTool::showPreview(const QVector<QLineF>& lines)
{
    if (lines.isEmpty()) {
        clearPreview();
        return;
    }
    QPainterPath path;
    for (const QLineF& line : lines) {
        path.moveTo(line.p1());
        path.lineTo(line.p2());
    }
    if (!m_preview) {
        QPen pen(QColor(0x33, 0xcc, 0x55));  // green: construction preview
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        m_preview = m_scene->addPath(QPainterPath(), pen);
        m_preview->setZValue(1000);
    }
    m_preview->setPath(path);
}

void DrawTangentTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

void DrawTangentTool::reset()
{
    clearPreview();
    m_first = Circle{};
}

} // namespace cad
