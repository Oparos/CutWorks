#include "cad/core/commands/RemoveEntityCommand.h"

#include "cad/core/CadDocument.h"

namespace cad {

RemoveEntityCommand::RemoveEntityCommand(CadDocument* document, int id, const QString& text)
    : QUndoCommand(text)
    , m_document(document)
    , m_id(id)
{
}

void RemoveEntityCommand::redo()
{
    m_entity = m_document->takeEntity(m_id);
}

void RemoveEntityCommand::undo()
{
    if (m_entity) {
        m_document->addEntity(std::move(m_entity), m_id);
    }
}

} // namespace cad
