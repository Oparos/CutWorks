#pragma once

#include "cad/core/entities/CadEntity.h"

#include <QList>
#include <QObject>

#include <memory>
#include <unordered_map>

namespace cad {

// The drawing's data model and the single owner of every entity. Views observe
// its signals to stay in sync; undo commands transfer entity ownership in and
// out of it (add / take) so there is never a shared raw pointer to leak.
//
// It depends only on Qt Core (QObject) — no widgets, no QGraphicsItem.
class CadDocument : public QObject
{
    Q_OBJECT

public:
    explicit CadDocument(QObject* parent = nullptr);

    // Take ownership of a new entity and give it a fresh id (returned).
    int addEntity(std::unique_ptr<CadEntity> entity);

    // Re-insert an entity that already has an id (used when an undo is redone).
    void addEntity(std::unique_ptr<CadEntity> entity, int id);

    // Release ownership of an entity (used by undo). Returns nullptr if unknown.
    std::unique_ptr<CadEntity> takeEntity(int id);

    CadEntity* entity(int id) const;
    QList<int> entityIds() const;

    // Call after editing an entity in place so views repaint it.
    void notifyChanged(int id);

signals:
    void entityAdded(int id);
    void entityRemoved(int id);
    void entityChanged(int id);

private:
    std::unordered_map<int, std::unique_ptr<CadEntity>> m_entities;
    int m_nextId = 1;
};

} // namespace cad
