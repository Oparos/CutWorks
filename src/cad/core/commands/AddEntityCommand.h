#pragma once

#include "cad/core/entities/CadEntity.h"

#include <QUndoCommand>

#include <memory>

namespace cad {

class CadDocument;

// Undoable "add an entity to the drawing". Ownership is explicit: while the
// entity is in the document, the document owns it; while it is undone, this
// command owns it (in m_entity) and frees it if the command is destroyed. No
// raw shared pointers, so nothing leaks or double-frees.
class AddEntityCommand : public QUndoCommand
{
public:
    AddEntityCommand(CadDocument* document, std::unique_ptr<CadEntity> entity,
                     const QString& text);

    void redo() override;
    void undo() override;

    // Valid after the command has been pushed/redone; used by paste to select
    // the newly added entities.
    int entityId() const { return m_id; }

private:
    CadDocument* m_document;
    std::unique_ptr<CadEntity> m_entity;  // held only while undone
    int m_id = 0;
    bool m_added = false;
};

} // namespace cad
