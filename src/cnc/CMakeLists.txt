# CNC module: machine control (serial comm with GRBL, G-code, jog, DRO, THC).
# Currently a placeholder workspace widget. This module will later use
# Qt6::SerialPort for asynchronous communication with the controller.
add_library(cutworks_cnc STATIC
    CncModule.h
    CncModule.cpp
)

target_include_directories(cutworks_cnc PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(cutworks_cnc PUBLIC Qt6::Widgets)
