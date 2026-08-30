# CutWorks

CAD/CAM/CNC desktop application for **GRBL-based CNC plasma cutters**.

CutWorks lets you design a part, generate a cutting toolpath from it, and drive
the CNC controller — all in one program:

1. **CAD** — draw / import the geometry of the part (lines, arcs, circles, polylines).
2. **CAM** — turn that geometry into a cutting toolpath and export G-code.
3. **CNC** — send the G-code to a GRBL controller and control the machine
   (jog, DRO, torch height control, overrides, console).

> **Status:** early rewrite. This is a clean, modular reimplementation of an
> existing application. Right now the project is a **buildable skeleton**: the
> application shell with three empty workspace modules. Functionality is being
> migrated incrementally. See [`docs/architecture.md`](docs/architecture.md).

---

## Technologies

- **C++20**
- **Qt 6** (Widgets; SerialPort will be added with the CNC module)
- **CMake** (>= 3.21)
- Target platforms: **Windows** and **Linux**

### Dependencies and how they are provided

The goal is that anyone can clone the public repository and build with as little
setup as possible. Dependencies are handled so that **the only thing you install
manually is Qt**:

| Dependency | How it is provided |
|------------|--------------------|
| **Qt 6**   | Installed by you (official Qt installer or your distro). Found by CMake via `CMAKE_PREFIX_PATH`. |
| **Clipper2** (contour offsetting) | Downloaded automatically by CMake `FetchContent` when the CAM module needs it. No manual install. |
| **libdxfrw** (DXF import/export)  | Vendored in `third_party/` — comes with the repo, nothing to install. |

No `vcpkg` or Conan is required.

> The skeleton currently links only **Qt Widgets**. Clipper2 and libdxfrw are
> wired in as their modules (CAM / CAD) are migrated.

---

## Requirements

- A C++20 compiler
  - Windows: **Visual Studio 2022** (MSVC v143)
  - Linux: GCC 11+ or Clang 14+
- **CMake** 3.21 or newer
- **Ninja** (recommended) or Visual Studio / Make
- **Qt 6** (developed against 6.7.x) with the **Widgets** module

---

## Building

### 1. Tell CMake where Qt is

Set the `QT_PREFIX` environment variable to your Qt kit, or pass
`-DCMAKE_PREFIX_PATH=...` on the command line.

Examples:

- Windows: `C:/Qt/6.7.3/msvc2022_64`
- Linux:   `~/Qt/6.7.3/gcc_64` (or `/usr` when Qt comes from the distro)

### 2. Configure and build

Windows (Visual Studio generator — finds the compiler automatically):

```bash
cmake --preset windows-vs
cmake --build --preset windows-vs
```

Linux / anywhere with a compiler on PATH (Ninja):

```bash
cmake --preset default
cmake --build --preset default
```

If you don't want to rely on `QT_PREFIX`, pass the path directly:

```bash
cmake --preset windows-vs -DCMAKE_PREFIX_PATH=C:/Qt/6.7.3/msvc2022_64
```

### 3. Run

The executable is placed in `build/bin/`. On Windows the Qt runtime is copied
next to it automatically (via `windeployqt`), so it can be launched directly.

### Per-developer paths (optional)

Instead of setting `QT_PREFIX` every time, you can create a
`CMakeUserPresets.json` next to `CMakePresets.json` with your local Qt path.
That file is git-ignored, so it never lands in the shared repo.

---

## Project layout

```text
CutWorks/
├── CMakeLists.txt          # top-level build
├── CMakePresets.json       # shared configure/build presets
├── README.md
├── docs/
│   └── architecture.md     # living architecture description
├── third_party/            # vendored dependencies (e.g. libdxfrw) — added when needed
└── src/
    ├── main.cpp
    ├── app/                # application shell (MainWindow, navigation)
    ├── cad/                # CAD module (library: cutworks_cad)
    ├── cam/                # CAM module (library: cutworks_cam)
    └── cnc/                # CNC module (library: cutworks_cnc)
```

Each module builds as its own static library; the `CutWorks` executable links
them together. See [`docs/architecture.md`](docs/architecture.md) for module
responsibilities and dependency direction.

---

## Developer notes

- New Qt `connect()` syntax only (function pointers, not `SIGNAL`/`SLOT`).
- Hardware communication (GRBL) must stay **asynchronous** — never block the UI thread.
- Domain logic should not depend on Qt widgets; UI should not contain domain logic.
- This is a rewrite: the previous application is a reference, not the code base.
