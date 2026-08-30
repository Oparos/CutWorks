# CNC module: machine control (serial comm with GRBL, G-code, jog, DRO, THC).
#
# Split into:
#   core/ — backend: async serial transport + all GRBL protocol knowledge.
#           Depends on Qt Core + SerialPort only, NEVER on Qt Widgets.
#   ui/   — Qt widgets that present the backend and forward user intents.
#
# (Kept as a single library for now; core files must not include any widget.)
add_library(cutworks_cnc STATIC
    # --- core (backend) ---
    core/SerialPortInfo.h
    core/SerialLink.h
    core/SerialLink.cpp
    core/GrblController.h
    core/GrblController.cpp
    core/JobStreamer.h
    core/JobStreamer.cpp
    core/grbl/GrblTypes.h
    core/grbl/GrblProtocol.h
    core/grbl/GrblProtocol.cpp

    # --- ui (presentation) ---
    ui/CncModule.h
    ui/CncModule.cpp
    ui/ConnectionWidget.h
    ui/ConnectionWidget.cpp
    ui/ConsoleWidget.h
    ui/ConsoleWidget.cpp
    ui/DroWidget.h
    ui/DroWidget.cpp
    ui/GCodeEditorWidget.h
    ui/GCodeEditorWidget.cpp
    ui/JogWidget.h
    ui/JogWidget.cpp
    ui/OverrideWidget.h
    ui/OverrideWidget.cpp
    ui/ThcWidget.h
    ui/ThcWidget.cpp
)

target_include_directories(cutworks_cnc PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(cutworks_cnc PUBLIC
    Qt6::Widgets
    Qt6::SerialPort
)
