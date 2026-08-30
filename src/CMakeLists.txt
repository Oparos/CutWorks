# Each module is its own static library so dependencies stay explicit and
# one-directional: the application shell depends on the modules, never the
# other way around.
add_subdirectory(cad)
add_subdirectory(cam)
add_subdirectory(cnc)

# --- Application shell (executable) ---
add_executable(CutWorks
    main.cpp
    app/MainWindow.h
    app/MainWindow.cpp
)

target_include_directories(CutWorks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(CutWorks PRIVATE
    Qt6::Widgets
    cutworks_cad
    cutworks_cam
    cutworks_cnc
)

# On Windows, copy the Qt runtime next to the executable so it can be launched
# directly (no need to add Qt's bin folder to PATH).
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET CutWorks POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --no-translations --no-compiler-runtime
                    "$<TARGET_FILE:CutWorks>"
            COMMENT "Deploying Qt runtime next to CutWorks")
    endif()
endif()
