# CAD module: geometry design (entities, tools, layers, snapping, DXF I/O).
# Currently a placeholder workspace widget; domain logic is migrated later into
# a Qt-free core/ subfolder, with Qt widgets kept under ui/.
add_library(cutworks_cad STATIC
    CadModule.h
    CadModule.cpp
)

target_include_directories(cutworks_cad PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(cutworks_cad PUBLIC Qt6::Widgets)
