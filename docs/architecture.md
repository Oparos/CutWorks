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
torch height control, overrides, console). Uses **Qt6::SerialPort**.

The CNC module is the first to be split into a backend/UI boundary:

- `core/` (backend, Qt Core + SerialPort, **no widgets**):
  - `SerialLink` — asynchronous byte transport over `QSerialPort`; frames
    incoming bytes into whole lines. No GRBL semantics.
  - `grbl/GrblProtocol` + `grbl/GrblTypes` — pure functions and value types:
    classify a received line, parse status/setting lines, GRBL error/alarm text
    tables, and command-byte encoding (real-time bytes vs newline-terminated).
  - `GrblController` — the backend brain: owns the transport, polls status, and
    turns every incoming line into typed signals (`statusUpdated`,
    `errorReceived`, `alarmReceived`, `settingReceived`, `lineLogged`,
    `responseReceived`, `connectionChanged`). All GRBL knowledge lives here.
  - `JobStreamer` — streams a loaded G-code program with the simple
    send-one / wait-for-`ok` protocol (Idle/Running/Paused, dry-run, comment
    skipping). Decoupled from the controller: it emits `sendCommandRequested`
    and consumes `responseReceived`, so it is wired up by the UI, not hard-tied
    to the transport.
- `ui/` (Qt Widgets): presents the controller and forwards user intents. The UI
  never parses the protocol or talks to the port directly.

This replaces the legacy design where a single 700-line widget built the UI and
also held all protocol parsing, streaming and error tables.

*Implemented so far: the `core/` backend above, plus the first UI slice
(`ui/CncModule` composing `ConnectionWidget`, `ConsoleWidget`, `DroWidget`) wired
to the controller — connect to a port, see live status/DRO, send console
commands, zero/home/probe. Streaming, jog and THC are the next increments.*

Icons are provided as embedded SVG under `resources/icons/` (crisp at any DPI, no
downloads): refresh, play/pause/stop, and the 8 jog-pad direction arrows.

The jog step mode is stored as combo **item data** (a numeric mm value, or a
sentinel for continuous), never as display text, so continuous jog is detected
independently of the UI language — fixing a language-dependent bug in the legacy
jog. The backend exposes `jog(dx,dy,dz,distance,feed)` and `cancelJog()`; the UI
(buttons and keyboard) only chooses direction/step/feed.

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

## UI theming and high-DPI scaling

The application must look and scale correctly on any screen and OS. This is set
up once, at the application level, and every module is expected to follow the
same discipline:

- **High-DPI:** `main.cpp` sets `HighDpiScaleFactorRoundingPolicy::PassThrough`
  before constructing `QApplication`, giving smooth fractional scaling on 125% /
  150% / 175% displays.
- **Theme:** a single dark theme lives in `resources/theme.qss` and is embedded
  into the binary via `resources/app.qrc` (compiled by AUTORCC). It is loaded in
  `main.cpp`. Embedding (rather than reading a file from disk) means styling
  works on every machine with no external files.
- **Font:** no font family/size is hard-coded in the theme; the system font is
  used, so text respects the user's DPI and accessibility settings and stays
  portable across OSes.

**Rules for module UI (to keep scaling correct):**
- Always use layouts; never position widgets at fixed pixel coordinates.
- Avoid magic pixel sizes; prefer layout-driven and font-relative sizing.
- Put resizable content in scroll areas / splitters so it fits small screens.
- Style via the shared theme; avoid per-widget inline stylesheets.

## Migration status

| Area | Status |
|------|--------|
| Build system (CMake, presets) | ✅ done (skeleton) |
| Application shell + navigation | ✅ done |
| Theming + high-DPI foundation | ✅ done |
| CAD module | ⬜ placeholder only |
| CAM module | ⬜ placeholder only |
| CNC module — backend core (transport, GRBL protocol, controller) | ✅ done |
| CNC module — UI (connection, console, DRO) | ✅ done |
| CNC module — G-code streaming (load, play/pause/stop, dry-run) | ✅ done |
| CNC module — jog (buttons + keyboard, step & continuous) | ✅ done |
| CNC module — feed overrides + THC (torch height control) | ✅ done |
| CNC module — config ($$) / preview / run-from-line | ⬜ not started |
| CAD → CAM → CNC data flow | ⬜ not started |
