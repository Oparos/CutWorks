# CAD module: geometry design (entities, tools, layers, snapping, DXF I/O).
#
# Split into:
#   core/ — domain: pure geometry entities + document model + undo commands.
#           Depends on Qt Gui value types (QPointF, QPainterPath) but NOT on
#           QGraphicsItem or any widget.
#   ui/   — rendering (QGraphicsScene/Item/View), tools and panels.
add_library(cutworks_cad STATIC
    # --- core (domain) ---
    core/geometry/Geometry.h
    core/entities/CadEntity.h
    core/entities/LineEntity.h
    core/entities/LineEntity.cpp
    core/entities/CircleEntity.h
    core/entities/CircleEntity.cpp
    core/entities/PointEntity.h
    core/entities/PointEntity.cpp
    core/entities/PolylineEntity.h
    core/entities/PolylineEntity.cpp
    core/entities/ArcEntity.h
    core/entities/ArcEntity.cpp
    core/CadDocument.h
    core/CadDocument.cpp
    core/commands/AddEntityCommand.h
    core/commands/AddEntityCommand.cpp
    core/commands/RemoveEntityCommand.h
    core/commands/RemoveEntityCommand.cpp
    core/commands/ModifyEntityCommand.h
    core/commands/ModifyEntityCommand.cpp

    # --- ui (presentation) ---
    ui/CadModule.h
    ui/CadModule.cpp
    ui/ToolInputBar.h
    ui/ToolInputBar.cpp
    ui/render/EntityItem.h
    ui/render/EntityItem.cpp
    ui/render/CadScene.h
    ui/render/CadScene.cpp
    ui/render/CadView.h
    ui/render/CadView.cpp
    ui/tools/CadTool.h
    ui/tools/SelectTool.h
    ui/tools/SelectTool.cpp
    ui/tools/MoveTool.h
    ui/tools/MoveTool.cpp
    ui/tools/RotateTool.h
    ui/tools/RotateTool.cpp
    ui/tools/MirrorTool.h
    ui/tools/MirrorTool.cpp
    ui/tools/DrawLineTool.h
    ui/tools/DrawLineTool.cpp
    ui/tools/DrawCircleTool.h
    ui/tools/DrawCircleTool.cpp
    ui/tools/DrawPointTool.h
    ui/tools/DrawPointTool.cpp
    ui/tools/DrawPolylineTool.h
    ui/tools/DrawPolylineTool.cpp
    ui/tools/DrawRectangleTool.h
    ui/tools/DrawRectangleTool.cpp
    ui/tools/DrawArcTool.h
    ui/tools/DrawArcTool.cpp
    ui/tools/DrawPolygonTool.h
    ui/tools/DrawPolygonTool.cpp
)

target_include_directories(cutworks_cad PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(cutworks_cad PUBLIC Qt6::Widgets)
