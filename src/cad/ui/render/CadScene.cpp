#include "cad/ui/render/CadScene.h"

#include "cad/core/CadDocument.h"
#include "cad/core/entities/CadEntity.h"
#include "cad/ui/render/EntityItem.h"

#include <QPainter>

#include <cmath>

CadScene::CadScene(cad::CadDocument* document, QObject* parent)
    : QGraphicsScene(parent)
    , m_document(document)
{
    connect(m_document, &cad::CadDocument::entityAdded, this, &CadScene::onEntityAdded);
    connect(m_document, &cad::CadDocument::entityRemoved, this, &CadScene::onEntityRemoved);
    connect(m_document, &cad::CadDocument::entityAboutToChange, this, &CadScene::onEntityAboutToChange);
    connect(m_document, &cad::CadDocument::entityChanged, this, &CadScene::onEntityChanged);
    connect(&m_document->layers(), &cad::LayerTable::layerChanged, this, &CadScene::onLayerChanged);
    connect(&m_document->layers(), &cad::LayerTable::layersChanged, this, &CadScene::onLayersChanged);

    // Pick up anything already in the document.
    for (int id : m_document->entityIds()) {
        onEntityAdded(id);
    }
}

void CadScene::onEntityAdded(int id)
{
    if (m_items.count(id) != 0) {
        return;
    }
    auto* item = new EntityItem(m_document, id);
    addItem(item);
    m_items[id] = item;
    applyLayerVisibility(id, item);
}

void CadScene::onEntityRemoved(int id)
{
    const auto it = m_items.find(id);
    if (it != m_items.end()) {
        removeItem(it->second);
        delete it->second;  // the entity itself is owned elsewhere (document/undo)
        m_items.erase(it);
    }
}

void CadScene::onEntityAboutToChange(int id)
{
    const auto it = m_items.find(id);
    if (it != m_items.end()) {
        it->second->prepareForChange();
    }
}

void CadScene::onEntityChanged(int id)
{
    const auto it = m_items.find(id);
    if (it != m_items.end()) {
        it->second->refresh();
    }
}

double CadScene::minorGridStep(double sceneScale)
{
    // The spacing follows the zoom so lines stay a roughly constant distance
    // apart on screen. This bounds how many lines we draw (independent of zoom) —
    // a fixed spacing would draw tens of thousands of lines when zoomed out.
    if (sceneScale <= 0.0) {
        return 0.0;
    }
    constexpr double targetPx = 70.0;              // desired on-screen spacing
    const double raw = targetPx / sceneScale;      // that many scene mm
    const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
    const double residual = raw / magnitude;
    return (residual < 2.0 ? 1.0 : (residual < 5.0 ? 2.0 : 5.0)) * magnitude;
}

void CadScene::applyLayerVisibility(int id, EntityItem* item)
{
    const cad::CadEntity* e = m_document->entity(id);
    item->setVisible(!e || m_document->layers().isVisible(e->layer()));
}

void CadScene::onLayerChanged(const QString& name)
{
    // A layer's color or visibility changed: refresh every item on that layer.
    for (const auto& [id, item] : m_items) {
        const cad::CadEntity* e = m_document->entity(id);
        if (e && e->layer() == name) {
            item->setVisible(m_document->layers().isVisible(name));
            item->update();
        }
    }
}

void CadScene::onLayersChanged()
{
    for (const auto& [id, item] : m_items) {
        applyLayerVisibility(id, item);
    }
}

void CadScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, QColor(0x22, 0x22, 0x22));

    const double scale = std::abs(painter->worldTransform().m11());
    const double minor = minorGridStep(scale);
    if (minor <= 0.0) {
        return;
    }
    const double major = minor * 5.0;

    auto drawLines = [&](double step, const QColor& color) {
        if (step <= 0.0 || rect.width() / step > 400.0 || rect.height() / step > 400.0) {
            return;  // safety cap
        }
        QPen pen(color);
        pen.setCosmetic(true);
        painter->setPen(pen);
        for (double x = std::floor(rect.left() / step) * step; x <= rect.right(); x += step) {
            painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
        }
        for (double y = std::floor(rect.top() / step) * step; y <= rect.bottom(); y += step) {
            painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
        }
    };

    drawLines(minor, QColor(0x2b, 0x2b, 0x2b));  // faint minor grid
    drawLines(major, QColor(0x40, 0x40, 0x40));  // brighter major grid

    // Axes through the origin.
    QPen axisPen(QColor(0x60, 0x60, 0x60));
    axisPen.setCosmetic(true);
    painter->setPen(axisPen);
    painter->drawLine(QLineF(rect.left(), 0, rect.right(), 0));
    painter->drawLine(QLineF(0, rect.top(), 0, rect.bottom()));
}
