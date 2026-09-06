#pragma once

#include "cad/core/entities/CadEntity.h"

#include <QUndoCommand>

#include <memory>

namespace cad {

class CadDocument;

// Undoable "remove an entity". Mirror of AddEntityCommand: while removed, this
// command owns the entity (m_entity); redo takes it out of the document, undo
// puts it back under the same id. Ownership is always explicit.
class RemoveEntityCommand : public QUndoCommand
{
public:
    RemoveEntityCommand(CadDocument* document, int id, const QString& text);

    void redo() override;
    void undo() override;

private:
    CadDocument* m_document;
    std::unique_ptr<CadEntity> m_entity;  // held only while removed
    int m_id;
};

} // namespace cad
