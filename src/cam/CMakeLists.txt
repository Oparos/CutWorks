# CAM module: toolpath generation (contours, leads, offsetting, post-processor).
# Currently a placeholder workspace widget. This module will later pull in
# Clipper2 (via FetchContent) for contour offsetting.
add_library(cutworks_cam STATIC
    CamModule.h
    CamModule.cpp
)

target_include_directories(cutworks_cam PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(cutworks_cam PUBLIC Qt6::Widgets)
