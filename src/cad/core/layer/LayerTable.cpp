#include "cad/core/layer/LayerTable.h"

#include <algorithm>

namespace cad {

LayerTable::LayerTable(QObject* parent)
    : QObject(parent)
{
    m_layers.push_back(Layer{QStringLiteral("0"), QColor(0xe6, 0xe6, 0xe6), true});
    m_active = QStringLiteral("0");
}

Layer* LayerTable::find(const QString& name)
{
    const auto it = std::find_if(m_layers.begin(), m_layers.end(),
                                 [&](const Layer& l) { return l.name == name; });
    return it == m_layers.end() ? nullptr : &*it;
}

const Layer* LayerTable::find(const QString& name) const
{
    const auto it = std::find_if(m_layers.begin(), m_layers.end(),
                                 [&](const Layer& l) { return l.name == name; });
    return it == m_layers.end() ? nullptr : &*it;
}

void LayerTable::addLayer(const QString& name)
{
    if (name.isEmpty() || find(name)) {
        return;
    }
    m_layers.push_back(Layer{name, QColor(0xe6, 0xe6, 0xe6), true});
    emit layersChanged();
}

void LayerTable::removeLayer(const QString& name)
{
    if (name == QStringLiteral("0") || m_layers.size() <= 1) {
        return;  // keep the default layer and never remove the last one
    }
    const auto it = std::find_if(m_layers.begin(), m_layers.end(),
                                 [&](const Layer& l) { return l.name == name; });
    if (it == m_layers.end()) {
        return;
    }
    m_layers.erase(it);
    emit layersChanged();
    if (m_active == name) {
        m_active = QStringLiteral("0");
        emit activeChanged(m_active);
    }
}

void LayerTable::setColor(const QString& name, const QColor& color)
{
    if (Layer* l = find(name); l && l->color != color) {
        l->color = color;
        emit layerChanged(name);
    }
}

void LayerTable::setVisible(const QString& name, bool visible)
{
    if (Layer* l = find(name); l && l->visible != visible) {
        l->visible = visible;
        emit layerChanged(name);
    }
}

const Layer* LayerTable::layer(const QString& name) const
{
    return find(name);
}

QColor LayerTable::colorOf(const QString& name) const
{
    const Layer* l = find(name);
    return l ? l->color : QColor(0xe6, 0xe6, 0xe6);
}

bool LayerTable::isVisible(const QString& name) const
{
    const Layer* l = find(name);
    return l ? l->visible : true;
}

void LayerTable::setActive(const QString& name)
{
    if (find(name) && m_active != name) {
        m_active = name;
        emit activeChanged(name);
    }
}

} // namespace cad
