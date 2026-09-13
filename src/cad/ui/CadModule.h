#pragma once

#include "cad/core/snap/SnapEngine.h"

#include <QWidget>

class QUndoStack;
class CadScene;
class CadView;
class ToolInputBar;

namespace cad {
class CadDocument;
class CadTool;
class EditController;
class SelectTool;
class MoveTool;
class RotateTool;
class MirrorTool;
class TrimTool;
class ExtendTool;
class FilletChamferTool;
class ScaleTool;
class OffsetTool;
class DrawLineTool;
class DrawPolylineTool;
class DrawRectangleTool;
class DrawCircleTool;
class DrawArcTool;
class DrawTangentTool;
class DrawSlotTool;
class DrawTeardropTool;
class DrawPolygonTool;
class DrawPointTool;
}

// Entry-point widget for the CAD workspace. Its job is composition and tool
// switching: it builds the UI, owns the document / rendering / tools, and routes
// user intent. Edit operations (delete/copy/paste/array) live in EditController.
class CadModule : public QWidget
{
    Q_OBJECT

public:
    explicit CadModule(QWidget* parent = nullptr);

private:
    void setActiveTool(cad::CadTool* tool);
    void refreshInputBar();

    cad::CadDocument* m_document = nullptr;
    cad::SnapEngine m_snap;
    QUndoStack* m_undoStack = nullptr;
    CadScene* m_scene = nullptr;
    CadView* m_view = nullptr;
    ToolInputBar* m_inputBar = nullptr;
    cad::EditController* m_edit = nullptr;

    cad::SelectTool* m_selectTool = nullptr;
    cad::MoveTool* m_moveTool = nullptr;
    cad::RotateTool* m_rotateTool = nullptr;
    cad::MirrorTool* m_mirrorTool = nullptr;
    cad::TrimTool* m_trimTool = nullptr;
    cad::ExtendTool* m_extendTool = nullptr;
    cad::FilletChamferTool* m_filletTool = nullptr;
    cad::FilletChamferTool* m_chamferTool = nullptr;
    cad::ScaleTool* m_scaleTool = nullptr;
    cad::OffsetTool* m_offsetTool = nullptr;
    cad::DrawLineTool* m_lineTool = nullptr;
    cad::DrawPolylineTool* m_polylineTool = nullptr;
    cad::DrawRectangleTool* m_rectangleTool = nullptr;
    cad::DrawCircleTool* m_circleTool = nullptr;
    cad::DrawArcTool* m_arcTool = nullptr;
    cad::DrawTangentTool* m_tangentTool = nullptr;
    cad::DrawSlotTool* m_slotTool = nullptr;
    cad::DrawTeardropTool* m_teardropTool = nullptr;
    cad::DrawPolygonTool* m_polygonTool = nullptr;
    cad::DrawPointTool* m_pointTool = nullptr;
    cad::CadTool* m_activeTool = nullptr;
};
