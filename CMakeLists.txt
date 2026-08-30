cmake_minimum_required(VERSION 3.21)

project(CutWorks
    VERSION 0.1.0
    DESCRIPTION "CAD/CAM/CNC application for GRBL-based plasma cutters"
    LANGUAGES CXX)

# --- Language standard ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Qt automatic tooling ---
# AUTOMOC: run moc on files with Q_OBJECT
# AUTORCC: compile .qrc resource files
# AUTOUIC: compile .ui form files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Output layout ---
# Keep all executables in one place so Qt runtime/plugins resolve at run time.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Dependencies ---
# Qt is provided by the developer's Qt installation (see README).
# Point CMAKE_PREFIX_PATH at the Qt kit, e.g. C:/Qt/6.7.3/msvc2022_64.
find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

qt_standard_project_setup()

# --- Source tree ---
add_subdirectory(src)
