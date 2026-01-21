# Remaining Extraction Tasks

> **Goal**: Extract code FROM monolithic files INTO specialized components  
> **Status**: Infrastructure created, extraction NOT done  
> **Priority**: Step-by-step extraction to reduce file sizes

---

## Source Files to Decompose

| File | Current Lines | Target Lines | Functions |
|------|---------------|--------------|-----------|
| `map_drawer.cpp` | 2042 | ~150 (facade) | 48 |
| `map_display.cpp` | 2764 | ~200 (thin wrapper) | 77 |
| `graphics.cpp` | 1637 | ~100 (facade) | 77 |

---

## Phase A: map_drawer.cpp Extraction (2042 → ~150 lines)

### A.1 Extract to tile_renderer.cpp
- [ ] A.1.1 Move `DrawTile()` (lines 1482-1682, ~200 lines)
- [ ] A.1.2 Move `BlitItem()` both overloads (lines 1064-1248, ~185 lines)
- [ ] A.1.3 Move `BlitSpriteType()` both overloads (lines 1250-1287, ~38 lines)
- [ ] A.1.4 Replace with calls to `TileRenderer::render()`
- [ ] A.1.5 Verify build

### A.2 Extract to creature_renderer.cpp
- [ ] A.2.1 Move `BlitCreature()` both overloads (lines 1289-1351, ~63 lines)
- [ ] A.2.2 Replace with calls to `CreatureRenderer::render()`
- [ ] A.2.3 Verify build

### A.3 Extract to grid_renderer.cpp
- [ ] A.3.1 Move `DrawGrid()` (lines 511-530, ~20 lines)
- [ ] A.3.2 Replace with call to `GridRenderer::render()`
- [ ] A.3.3 Verify build

### A.4 Extract to brush_renderer.cpp
- [ ] A.4.1 Move `DrawBrush()` (lines 727-1062, ~335 lines)
- [ ] A.4.2 Move `DrawBrushIndicator()` (lines 1684-1732, ~49 lines)
- [ ] A.4.3 Move `DrawRawBrush()` (lines 1382-1421, ~40 lines)
- [ ] A.4.4 Move `DrawHookIndicator()` (lines 1734-1755, ~22 lines)
- [ ] A.4.5 Replace with calls to `BrushRenderer::render()`
- [ ] A.4.6 Verify build

### A.5 Extract to selection_renderer.cpp
- [ ] A.5.1 Move `DrawSelectionBox()` (lines 636-681, ~46 lines)
- [ ] A.5.2 Move `DrawDraggingShadow()` (lines 532-591, ~60 lines)
- [ ] A.5.3 Move `BlitSquare()` (lines 1353-1380, ~28 lines)
- [ ] A.5.4 Replace with calls to `SelectionRenderer::render()`
- [ ] A.5.5 Verify build

### A.6 Extract to ui_renderer.cpp
- [ ] A.6.1 Move `DrawIngameBox()` (lines 452-509, ~58 lines)
- [ ] A.6.2 Move `DrawLiveCursors()` (lines 683-725, ~43 lines)
- [ ] A.6.3 Replace with calls to `UIRenderer::render()`
- [ ] A.6.4 Verify build

### A.7 Extract to tooltip_renderer.cpp
- [ ] A.7.1 Move `DrawTooltips()` (lines 1757-1861, ~105 lines)
- [ ] A.7.2 Move `MakeTooltip()` (lines 1868-1876, ~9 lines)
- [ ] A.7.3 Move `WriteTooltip()` both overloads (lines 1423-1480, ~58 lines)
- [ ] A.7.4 Replace with calls to `TooltipRenderer::render()`
- [ ] A.7.5 Verify build

### A.8 Extract to light_renderer.cpp
- [ ] A.8.1 Move `DrawLight()` (lines 1863-1866, ~4 lines)
- [ ] A.8.2 Move `AddLight()` (lines 1878-1904, ~27 lines)
- [ ] A.8.3 Replace with calls to `LightRenderer::render()`
- [ ] A.8.4 Verify build

### A.9 Remaining in map_drawer.cpp (facade)
After extraction, map_drawer.cpp should contain only:
- `DrawingOptions` class (~70 lines)
- `MapDrawer` constructor/destructor (~10 lines)
- `SetupVars()` - delegates to RenderState (~35 lines)
- `SetupGL()` - delegates to GLContext (~22 lines)
- `Release()` - delegates (~16 lines)
- `Draw()` - orchestrates RenderCoordinator (~23 lines)
- `DrawMap()` - delegates to TileRenderer (~195 lines → ~20 after delegation)
- Helper functions: `getColor()`, `TakeScreenshot()`, `glBlitTexture()` etc.

**Target: ~150 lines**

---

## Phase B: graphics.cpp Extraction (1637 → ~100 lines)

### B.1 Extract to sprite_loader.cpp
- [ ] B.1.1 Move `loadSpriteMetadata()` (lines 475-600, ~126 lines)
- [ ] B.1.2 Move `loadSpriteMetadataFlags()` (lines 602-785, ~184 lines)
- [ ] B.1.3 Move `loadSpriteData()` (lines 787-863, ~77 lines)
- [ ] B.1.4 Move `loadSpriteDump()` (lines 865-902, ~38 lines)
- [ ] B.1.5 Move `loadOTFI()` (lines 437-473, ~37 lines)
- [ ] B.1.6 Replace with calls to `SpriteLoader::load()`
- [ ] B.1.7 Verify build

### B.2 Extract to texture_manager.cpp
- [ ] B.2.1 Move sprite_space map management
- [ ] B.2.2 Move image_space map management
- [ ] B.2.3 Move `getSprite()` (lines 264-271)
- [ ] B.2.4 Move `getCreatureSprite()` (lines 273-284)
- [ ] B.2.5 Move `clear()` (lines 226-253)
- [ ] B.2.6 Move `cleanSoftwareSprites()` (lines 255-262)
- [ ] B.2.7 Replace with calls to `TextureManager::get()`
- [ ] B.2.8 Verify build

### B.3 Extract to texture_cache.cpp
- [ ] B.3.1 Move `addSpriteToCleanup()` (lines 904-919)
- [ ] B.3.2 Move `garbageCollection()` (lines 921-939)
- [ ] B.3.3 Move cleanup_list management
- [ ] B.3.4 Replace with calls to `TextureCache::cleanup()`
- [ ] B.3.5 Verify build

### B.4 Move GameSprite classes (lines 964-1500+)
- [ ] B.4.1 Create `sprite_types.cpp` for GameSprite implementation
- [ ] B.4.2 Move `GameSprite` class implementation (~300 lines)
- [ ] B.4.3 Move `GameSprite::NormalImage` class (~150 lines)
- [ ] B.4.4 Move `GameSprite::TemplateImage` class (~200 lines)
- [ ] B.4.5 Move `EditorSprite` class (~30 lines)
- [ ] B.4.6 Verify build

### B.5 Move Animator class
- [ ] B.5.1 Move `Animator` class to separate file (~150 lines)
- [ ] B.5.2 Move `FrameDuration` to same file
- [ ] B.5.3 Verify build

### B.6 Remaining in graphics.cpp (facade)
After extraction, graphics.cpp should contain only:
- TemplateOutfitLookupTable (data, keep here or move to types)
- `GraphicManager` constructor/destructor (delegates to subsystems)
- `loadEditorSprites()` (keep, ~130 lines)
- Accessor methods that delegate to TextureManager

**Target: ~100 lines + lookup table**

---

## Phase C: map_display.cpp Extraction (2764 → ~200 lines)

### C.1 Extract to coordinate_mapper.cpp
- [ ] C.1.1 Move `ScreenToMap()` (lines 377-403, ~27 lines)
- [ ] C.1.2 Move `GetScreenCenter()` (lines 405-409, ~5 lines)
- [ ] C.1.3 Move coordinate calculation logic
- [ ] C.1.4 Replace with calls to `CoordinateMapper::screenToMap()`
- [ ] C.1.5 Verify build

### C.2 Extract to mouse_handler.cpp
- [ ] C.2.1 Move core mouse state tracking (dragging, boundbox_selection, etc.)
- [ ] C.2.2 Move `OnMouseMove()` logic (lines 474-607, ~134 lines)
- [ ] C.2.3 Replace with calls to `MouseHandler::handleMove()`
- [ ] C.2.4 Verify build

### C.3 Extract drawing mode logic to drawing_tool_handler.cpp (NEW)
- [ ] C.3.1 Create `input/drawing_tool_handler.h/cpp`
- [ ] C.3.2 Move brush drawing logic from `OnMouseMove()` (~80 lines)
- [ ] C.3.3 Move `getTilesToDraw()` logic
- [ ] C.3.4 Move `floodFill()` logic
- [ ] C.3.5 Verify build

### C.4 Extract selection logic to selection_handler.cpp (NEW)
- [ ] C.4.1 Create `input/selection_handler.h/cpp`
- [ ] C.4.2 Move selection logic from `OnMouseActionClick()` (lines 686-966, ~280 lines)
- [ ] C.4.3 Move selection logic from `OnMouseActionRelease()` (lines 968-1260, ~293 lines)
- [ ] C.4.4 Move drag selection logic
- [ ] C.4.5 Verify build

### C.5 Extract keyboard handling to keyboard_handler.cpp (NEW)
- [ ] C.5.1 Create `input/keyboard_handler.h/cpp`
- [ ] C.5.2 Move `OnKeyDown()` (lines 1575-1849, ~275 lines)
- [ ] C.5.3 Move `OnKeyUp()` (keep simple, ~3 lines)
- [ ] C.5.4 Replace with calls to `KeyboardHandler::onKeyDown()`
- [ ] C.5.5 Verify build

### C.6 Extract camera logic to camera_controller.cpp (NEW)
- [ ] C.6.1 Create `input/camera_controller.h/cpp`
- [ ] C.6.2 Move `OnMouseCameraClick()` (lines 1262-1277, ~16 lines)
- [ ] C.6.3 Move `OnMouseCameraRelease()` (lines 1279-1291, ~13 lines)
- [ ] C.6.4 Move `OnWheel()` zoom logic (lines 1505-1556, ~52 lines)
- [ ] C.6.5 Move `SetZoom()` (lines 165-185, ~21 lines)
- [ ] C.6.6 Replace with calls to `CameraController::handleWheel()`
- [ ] C.6.7 Verify build

### C.7 Extract properties popup to properties_handler.cpp (NEW)
- [ ] C.7.1 Create `input/properties_handler.h/cpp`
- [ ] C.7.2 Move `OnMousePropertiesClick()` (lines 1293-1352, ~60 lines)
- [ ] C.7.3 Move `OnMousePropertiesRelease()` (lines 1354-1503, ~150 lines)
- [ ] C.7.4 Move `OnMouseLeftDoubleClick()` properties logic (lines 617-652, ~36 lines)
- [ ] C.7.5 Replace with calls to `PropertiesHandler::onClick()`
- [ ] C.7.6 Verify build

### C.8 Menu commands (keep in place or separate)
- [ ] C.8.1 Consider extracting to `menu_command_handler.cpp` (~300 lines)
- [ ] C.8.2 Or keep in MapCanvas as thin dispatchers

### C.9 Remaining in map_display.cpp (thin wxGLCanvas wrapper)
After extraction, map_display.cpp should contain only:
- wxWidgets event table and bindings (~50 lines)
- `MapCanvas` constructor/destructor (~40 lines)
- `OnPaint()` - delegates to RenderCoordinator (~30 lines)
- `Refresh()` (~7 lines)
- Thin event handlers that delegate to extracted components
- `MapPopupMenu` class (~120 lines)
- `AnimationTimer` class (~40 lines)

**Target: ~200-300 lines**

---

## Phase D: Integration & Cleanup

### D.1 Update RenderCoordinator
- [ ] D.1.1 Wire all extracted renderers
- [ ] D.1.2 Implement proper pass execution
- [ ] D.1.3 Remove placeholder code
- [ ] D.1.4 Verify all render passes work

### D.2 Update InputDispatcher
- [ ] D.2.1 Wire all input handlers
- [ ] D.2.2 Implement proper event routing
- [ ] D.2.3 Remove placeholder code
- [ ] D.2.4 Verify all input events work

### D.3 Final Cleanup
- [ ] D.3.1 Move legacy files to legacy/ folder
- [ ] D.3.2 Update CMakeLists.txt to remove legacy
- [ ] D.3.3 Remove dead code
- [ ] D.3.4 Update includes
- [ ] D.3.5 Remove freeglut dependency
- [ ] D.3.6 Verify complete build

### D.4 Verification
- [ ] D.4.1 Manual visual testing
- [ ] D.4.2 All render passes work correctly
- [ ] D.4.3 All input events work correctly
- [ ] D.4.4 No visual regressions
- [ ] D.4.5 Performance check

---

## Execution Order

**Recommended order for minimal breakage:**

1. **Phase A first** - map_drawer.cpp renderers (most isolated)
2. **Phase B second** - graphics.cpp sprite system (used by renderers)
3. **Phase C third** - map_display.cpp input (depends on rendering)
4. **Phase D last** - integration and cleanup

**Within each phase, extract and verify one function at a time.**

---

## Summary

| Phase | Extractions | Estimated Effort |
|-------|-------------|------------------|
| A. map_drawer.cpp | 8 target files | 2-3 days |
| B. graphics.cpp | 5 target files | 2 days |
| C. map_display.cpp | 7 target files | 3-4 days |
| D. Integration | 4 cleanup steps | 1-2 days |
| **Total** | **24 extraction targets** | **8-11 days** |
