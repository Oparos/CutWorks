#pragma once

#include "cad/core/entities/CadEntity.h"

#include <QList>
#include <QWidget>

#include <memory>
#include <vector>

class QUndoStack;
class CadScene;
class CadView;
class ToolInputBar;

namespace cad {
class CadDocument;
class CadTool;
class SelectTool;
class MoveTool;
class RotateTool;
class MirrorTool;
class DrawLineTool;
class DrawCircleTool;
class DrawPointTool;
class DrawPolylineTool;
class DrawRectangleTool;
class DrawArcTool;
class DrawPolygonTool;
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
    void deleteSelection();
    void copySelection();
    void pasteClipboard();
    void arrayRectangular();
    void arrayPolar();
    QList<int> selectedEntityIds() const;

    cad::CadDocument* m_document = nullptr;
    QUndoStack* m_undoStack = nullptr;
    CadScene* m_scene = nullptr;
    CadView* m_view = nullptr;
    ToolInputBar* m_inputBar = nullptr;

    cad::SelectTool* m_selectTool = nullptr;
    cad::MoveTool* m_moveTool = nullptr;
    cad::RotateTool* m_rotateTool = nullptr;
    cad::MirrorTool* m_mirrorTool = nullptr;
    cad::DrawLineTool* m_lineTool = nullptr;
    cad::DrawCircleTool* m_circleTool = nullptr;
    cad::DrawPointTool* m_pointTool = nullptr;
    cad::DrawPolylineTool* m_polylineTool = nullptr;
    cad::DrawRectangleTool* m_rectangleTool = nullptr;
    cad::DrawArcTool* m_arcTool = nullptr;
    cad::DrawPolygonTool* m_polygonTool = nullptr;
    cad::CadTool* m_activeTool = nullptr;

    std::vector<std::unique_ptr<cad::CadEntity>> m_clipboard;
};
