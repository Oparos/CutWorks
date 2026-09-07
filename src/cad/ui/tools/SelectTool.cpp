#include "cad/ui/tools/SelectTool.h"

#include "cad/ui/render/EntityItem.h"

#include <QBrush>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGuiApplication>
#include <QPen>

namespace cad {

namespace {
EntityItem* entityItemAt(QGraphicsScene* scene, const QPointF& pos)
{
    const QList<QGraphicsItem*> hits = scene->items(pos);
    for (QGraphicsItem* item : hits) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            return entityItem;
        }
    }
    return nullptr;
}
}

SelectTool::SelectTool(QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_scene(scene)
{
}

void SelectTool::onMousePress(const QPointF& scenePos)
{
    m_pressPos = scenePos;
    const bool ctrl = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);

    if (EntityItem* hit = entityItemAt(m_scene, scenePos)) {
        if (ctrl) {
            hit->setSelected(!hit->isSelected());  // toggle, keep the rest
        }
        else {
            m_scene->clearSelection();
            hit->setSelected(true);
        }
        m_banding = false;
    }
    else {
        // Empty space: start a selection box. Without Ctrl this replaces the
        // current selection; with Ctrl it adds to it.
        m_additive = ctrl;
        if (!m_additive) {
            m_scene->clearSelection();
        }
        m_banding = true;
    }
}

void SelectTool::onMouseMove(const QPointF& scenePos)
{
    if (!m_banding) {
        return;
    }

    const bool window = scenePos.x() >= m_pressPos.x();  // left→right = window
    const QColor color = window ? QColor(0x5c, 0x8a, 0xff) : QColor(0x33, 0xcc, 0x55);

    QPen pen(color);
    pen.setCosmetic(true);
    pen.setStyle(window ? Qt::SolidLine : Qt::DashLine);
    QColor fill = color;
    fill.setAlpha(40);

    if (!m_band) {
        m_band = m_scene->addRect(QRectF(), pen, QBrush(fill));
        m_band->setZValue(1000);
    }
    else {
        m_band->setPen(pen);
        m_band->setBrush(QBrush(fill));
    }
    m_band->setRect(QRectF(m_pressPos, scenePos).normalized());
}

void SelectTool::onMouseRelease(const QPointF& scenePos)
{
    if (!m_banding) {
        return;
    }

    const bool window = scenePos.x() >= m_pressPos.x();
    const QRectF rect = QRectF(m_pressPos, scenePos).normalized();
    const Qt::ItemSelectionMode mode =
        window ? Qt::ContainsItemShape : Qt::IntersectsItemShape;

    // Non-additive already cleared on press; here we only add the box's items.
    for (QGraphicsItem* item : m_scene->items(rect, mode)) {
        if (dynamic_cast<EntityItem*>(item)) {
            item->setSelected(true);
        }
    }

    clearBand();
    m_banding = false;
}

void SelectTool::onCancel()
{
    clearBand();
    m_banding = false;
}

void SelectTool::deactivate()
{
    clearBand();
    m_banding = false;
}

void SelectTool::clearBand()
{
    if (m_band) {
        m_scene->removeItem(m_band);
        delete m_band;
        m_band = nullptr;
    }
}

} // namespace cad
