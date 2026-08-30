# Architecture

Living document describing the **current** state of CutWorks and the direction
of the ongoing rewrite. It is updated together with the code. Anything not yet
implemented is explicitly marked as *planned*.

## Purpose

CutWorks is a single desktop application covering the full workflow for a
GRBL-based CNC plasma cutter: design a part (CAD), generate a cutting toolpath
(CAM), and control the machine (CNC).

## Current state (skeleton)

The project currently builds and runs as an **application shell** with three
empty workspace modules. No domain logic has been migrated yet.

```text
                +------------------+
                |   CutWorks (exe) |   src/app  — MainWindow, navigation
                +---------+--------+
                          | links
        +-----------------+-----------------+
        v                 v                 v
  cutworks_cad      cutworks_cam      cutworks_cnc
  (src/cad)         (src/cam)         (src/cnc)
```

- `src/app/MainWindow` hosts a `QStackedWidget` and a navigation toolbar that
  switches between the three module widgets. It contains **no domain logic**.
- `src/cad`, `src/cam`, `src/cnc` are each a **static library** exposing a single
  placeholder workspace `QWidget`.

### Dependency direction

- The application shell depends on the three module libraries.
- The module libraries do **not** depend on the shell and do **not** depend on
  each other.
- Cross-module data flow (CAD → CAM → CNC) is coordinated by the shell / an
  application layer, so modules stay decoupled. *(Mechanism to be defined when
  the first real hand-off is migrated; the legacy app passed data as temporary
  DXF / G-code files.)*

## Modules and their responsibilities

These describe the **target** responsibility of each module, derived from the
reference application. Only the module shells exist today.

### CAD — geometry design
Draw and import 2D geometry: lines, arcs, circles, polylines. Editing tools
(move, rotate, trim, extend, fillet, chamfer, select), layers, snapping,
undo/redo, and DXF import/export.
Dependency to add during migration: **libdxfrw** (vendored) for DXF I/O.

### CAM — toolpath generation
Build contours from geometry, generate cutting toolpaths with lead-in/lead-out,
apply kerf offset, and post-process to G-code for GRBL-HAL. Cut/machine/path
settings.
Dependency to add during migration: **Clipper2** (via FetchContent) for offsetting.

### CNC — machine control
Asynchronous serial communication with a GRBL controller, G-code parsing and
streaming (job manager), simulation/preview, and machine controls (jog, DRO,
torch height control, overrides, console).
Dependency to add during migration: **Qt6::SerialPort**.

## Planned internal structure of a module

To keep the UI/backend boundary clear, each module will separate:

- `core/` — pure domain logic, ideally free of Qt Widgets (Qt Core allowed where
  it genuinely helps). Usable and testable without the GUI.
- `ui/` — Qt widgets and views (presentation and user interaction only).

These subfolders are introduced per module as real logic is migrated, not up
front, to avoid empty structure.

## Key architectural principles

- **UI/backend separation:** domain logic must not know about widgets/views;
  UI must not implement domain algorithms.
- **GRBL communication is asynchronous:** the serial layer never blocks the UI
  thread and does not depend on UI types.
- **Explicit, one-directional dependencies:** no cycles between modules.
- **Minimal dependencies:** prefer the standard library and CMake-native
  dependency handling over extra package managers.

## Migration status

| Area | Status |
|------|--------|
| Build system (CMake, presets) | ✅ done (skeleton) |
| Application shell + navigation | ✅ done |
| CAD module | ⬜ placeholder only |
| CAM module | ⬜ placeholder only |
| CNC module | ⬜ placeholder only |
| CAD → CAM → CNC data flow | ⬜ not started |
