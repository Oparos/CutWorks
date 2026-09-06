# CAD module: geometry design (entities, tools, layers, snapping, DXF I/O).
#
# Split into:
#   core/ — domain: pure geometry entities + document model + undo commands.
#           Depends on Qt Gui value types (QPointF, QPainterPath) but NOT on
#           QGraphicsItem or any widget.
#   ui/   — rendering (QGraphicsScene/Item/View), tools and panels.
add_library(cutworks_cad STATIC
    # --- core (domain) ---
    core/entities/CadEntity.h
    core/entities/LineEntity.h
    core/entities/LineEntity.cpp
    core/CadDocument.h
    core/CadDocument.cpp
    core/commands/AddEntityCommand.h
    core/commands/AddEntityCommand.cpp

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
    ui/tools/DrawLineTool.h
    ui/tools/DrawLineTool.cpp
)

target_include_directories(cutworks_cad PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(cutworks_cad PUBLIC Qt6::Widgets)
