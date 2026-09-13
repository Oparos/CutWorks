#pragma once

#include <QWidget>

class QListWidget;

namespace cad {
class CadDocument;
}

// A small panel for managing the drawing's layers: pick the active layer, toggle
// visibility, set color, add and delete layers. It is a thin view over the
// document's LayerTable — it rebuilds from the table and pushes user actions
// back into it (deleting a layer moves its entities to "0" first). No layer logic
// of its own.
class LayersPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LayersPanel(cad::CadDocument* document, QWidget* parent = nullptr);

private:
    void rebuild();
    void addLayer();
    void deleteActiveLayer();

    cad::CadDocument* m_document;
    QListWidget* m_list;
};
