#include "cad/core/CadDocument.h"

namespace cad {

CadDocument::CadDocument(QObject* parent)
    : QObject(parent)
{
}

int CadDocument::addEntity(std::unique_ptr<CadEntity> entity)
{
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
    std::unique_ptr<CadEntity> entity = std::move(it->second);
    m_entities.erase(it);
    emit entityRemoved(id);
    return entity;
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
