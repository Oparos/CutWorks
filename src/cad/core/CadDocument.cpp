#include "cad/core/CadDocument.h"

namespace cad {

CadDocument::CadDocument(QObject* parent)
    : QObject(parent)
{
}

int CadDocument::addEntity(std::unique_ptr<CadEntity> entity)
{
    // A freshly drawn entity carries no layer yet — put it on the active layer.
    // Copies (paste/array/mirror) and DXF imports already carry one, so keep it.
    if (entity->layer().isEmpty()) {
        entity->setLayer(m_layers.activeName());
    }
    const int id = m_nextId++;
    entity->setId(id);
    m_entities[id] = std::move(entity);
    emit entityAdded(id);
    return id;
}

void CadDocument::addEntity(std::unique_ptr<CadEntity> entity, int id)
{
    entity->setId(id);
    m_entities[id] = std::move(entity);
    if (id >= m_nextId) {
        m_nextId = id + 1;
    }
    emit entityAdded(id);
}

std::unique_ptr<CadEntity> CadDocument::takeEntity(int id)
{
    const auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return nullptr;
    }
    // Notify FIRST, while the entity is still present, so the view can remove its
    // item using valid geometry (otherwise the scene's spatial index keeps a
    // dangling pointer to the just-deleted item -> crash on the next query).
    emit entityRemoved(id);
    std::unique_ptr<CadEntity> entity = std::move(it->second);
    m_entities.erase(it);
    return entity;
}

std::unique_ptr<CadEntity> CadDocument::swapEntity(int id, std::unique_ptr<CadEntity> replacement)
{
    const auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return replacement;  // unknown id: hand it back, nothing changed
    }
    replacement->setId(id);
    // Notify the view BEFORE the bounds change so it can update its index while
    // the old geometry is still current.
    emit entityAboutToChange(id);
    std::unique_ptr<CadEntity> previous = std::move(it->second);
    it->second = std::move(replacement);
    emit entityChanged(id);
    return previous;
}

CadEntity* CadDocument::entity(int id) const
{
    const auto it = m_entities.find(id);
    return (it != m_entities.end()) ? it->second.get() : nullptr;
}

QList<int> CadDocument::entityIds() const
{
    QList<int> ids;
    ids.reserve(static_cast<int>(m_entities.size()));
    for (const auto& [id, entity] : m_entities) {
        ids.append(id);
    }
    return ids;
}

void CadDocument::notifyChanged(int id)
{
    if (m_entities.count(id) != 0) {
        emit entityChanged(id);
    }
}

} // namespace cad
