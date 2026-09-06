#pragma once

#include <QWidget>

class QUndoStack;
class CadScene;
class CadView;
class ToolInputBar;

namespace cad {
class CadDocument;
class CadTool;
class DrawLineTool;
}

// Entry-point widget for the CAD workspace. Owns the document, the rendering
// (scene/view), the tools and the parametric input bar, and wires them together.
class CadModule : public QWidget
{
    Q_OBJECT

public:
    explicit CadModule(QWidget* parent = nullptr);

private:
    void setActiveTool(cad::CadTool* tool);
    void refreshInputBar();

    cad::CadDocument* m_document = nullptr;
    QUndoStack* m_undoStack = nullptr;
    CadScene* m_scene = nullptr;
    CadView* m_view = nullptr;
    ToolInputBar* m_inputBar = nullptr;

    cad::DrawLineTool* m_lineTool = nullptr;
    cad::CadTool* m_activeTool = nullptr;
};
