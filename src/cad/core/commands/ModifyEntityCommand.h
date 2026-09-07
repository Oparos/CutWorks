#pragma once

#include "cad/core/entities/CadEntity.h"

#include <QUndoCommand>

#include <memory>

namespace cad {

class CadDocument;

// Undoable "replace an entity with a modified version" — the general edit used
// by move, rotate, trim, fillet, etc. It holds the state that is NOT currently
// in the document and ping-pongs it with the document on each redo/undo, so no
// cloning and no leaks.
class ModifyEntityCommand : public QUndoCommand
{
public:
    // `newState` is the modified entity; the document still holds the original.
    ModifyEntityCommand(CadDocument* document, int id,
                        std::unique_ptr<CadEntity> newState, const QString& text);

    void redo() override;
    void undo() override;

private:
    void swap();

    CadDocument* m_document;
    int m_id;
    std::unique_ptr<CadEntity> m_stash;  // the state currently outside the document
};

} // namespace cad
