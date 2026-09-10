#pragma once

#include "cad/core/entities/CadEntity.h"

#include <QList>
#include <QObject>

#include <memory>
#include <vector>

class QGraphicsScene;
class QUndoStack;
class QWidget;

namespace cad {

class CadDocument;

// App-level edit operations on the current selection: delete, copy/paste and
// arrays. Keeps CadModule to composition + tool switching. Owns the clipboard;
// operates on the document through undo commands.
class EditController : public QObject
{
    Q_OBJECT

public:
    EditController(CadDocument* document, QUndoStack* undoStack,
                   QGraphicsScene* scene, QWidget* dialogParent, QObject* parent = nullptr);

    void deleteSelection();
    void copySelection();
    void paste();
    void arrayRectangular();
    void arrayPolar();

private:
    QList<int> selectedEntityIds() const;

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;
    QWidget* m_dialogParent;
    std::vector<std::unique_ptr<CadEntity>> m_clipboard;
};

} // namespace cad
