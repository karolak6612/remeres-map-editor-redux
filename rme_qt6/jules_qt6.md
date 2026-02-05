# Jules Qt6 Refactor Summary

## Phase 1: Initialization & Core Foundation (Completed)
**Goal:** Establish a buildable Qt6 project structure and replace the application entry point.

*   **Project Structure:** Created `rme_qt6/` with a standalone `CMakeLists.txt`.
*   **Dependencies:** Configured CMake to use system-installed Qt6 (Core, Gui, Widgets, OpenGL), Boost, GLM, spdlog, LibArchive, ZLIB, and nlohmann_json.
*   **Core Headers:** Ported `definitions.h` and `main.h` to remove all wxWidgets references.
*   **Application Loop:** Implemented `Application` (inheriting `QApplication`) and `MainWindow` (inheriting `QMainWindow`) to replace `wxApp` and `wxFrame`.
*   **Utilities:** Ported `common.h` and `common.cpp` to use `QString` and standard C++ string manipulation instead of `wxString`.
*   **Libraries:** Integrated `nanovg` and `pugixml` directly into the new source tree.

## Phase 2: Map Data & Logic (Next Steps)
**Goal:** Port the non-GUI map logic to enable map loading and manipulation.

*   **File I/O:** Port `FileHandle` and file loading utilities to use `QFile` / `std::fstream`.
*   **Map Structure:** Port `Map`, `Tile`, `Item`, `Creature`, `Spawn`, `House`, `Town` classes.
*   **Loaders:** Port `OTBM`, `OTXML`, and `DAT/SPR` loaders.
*   **Dependencies:** Ensure these classes rely on `main.h` (Qt/Std) and do not pull in legacy wx headers.

## Phase 3: Rendering Engine
**Goal:** get the map drawing on screen.

*   **OpenGL Context:** Replace `wxGLCanvas` with `QOpenGLWidget`.
*   **Renderer:** Port `MapDrawer`, `LightDrawer`, and sprite rendering logic to work with the Qt OpenGL context.
*   **Resources:** Ensure `SpriteManager` and `TextureAtlas` work with the new loaders.

## Phase 4: User Interface
**Goal:** Rebuild the editor tools and windows.

*   **Palettes:** Reimplement the Item/Terrain/Creature palettes using `QListView` or custom `QAbstractItemView`.
*   **Dialogs:** Port configuration dialogs, object properties, and search windows to `QDialog`.
*   **Menus/Toolbars:** Recreate the main menu and toolbar actions using `QAction`.

## Phase 5: Interaction & Polish
**Goal:** Make it interactive.

*   **Input Handling:** Port mouse and keyboard event handling from wx events to Qt events (`QMouseEvent`, `QKeyEvent`).
*   **Tools:** Port the Brush system (`Brush`, `TerrainBrush`, etc.) to interact with the Qt input system.
*   **Clipboard:** Port copy/paste logic to `QClipboard`.
