#include "cad/core/commands/AddEntityCommand.h"

#include "cad/core/CadDocument.h"

namespace cad {

AddEntityCommand::AddEntityCommand(CadDocument* document,
                                   std::unique_ptr<CadEntity> entity,
                                   const QString& text)
    : QUndoCommand(text)
    , m_document(document)
    , m_entity(std::move(entity))
{
}

void AddEntityCommand::redo()
{
    if (!m_added) {
        // First time: let the document assign an id we remember for later.
        m_id = m_document->addEntity(std::move(m_entity));
        m_added = true;
    }
    else {
        // Re-doing after an undo: put the same entity back under the same id.
        m_document->addEntity(std::move(m_entity), m_id);
    }
}

void AddEntityCommand::undo()
{
    // Reclaim ownership so the entity survives (for a possible redo) and is
    // freed with this command if the command falls off the undo stack.
    m_entity = m_document->takeEntity(m_id);
}

} // namespace cad
