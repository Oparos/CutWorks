#include "cad/core/commands/ModifyEntityCommand.h"

#include "cad/core/CadDocument.h"

namespace cad {

ModifyEntityCommand::ModifyEntityCommand(CadDocument* document, int id,
                                         std::unique_ptr<CadEntity> newState,
                                         const QString& text)
    : QUndoCommand(text)
    , m_document(document)
    , m_id(id)
    , m_stash(std::move(newState))
{
}

void ModifyEntityCommand::redo()
{
    swap();
}

void ModifyEntityCommand::undo()
{
    swap();
}

void ModifyEntityCommand::swap()
{
    // Put the stashed state into the document and keep whatever was there.
    m_stash = m_document->swapEntity(m_id, std::move(m_stash));
}

} // namespace cad
