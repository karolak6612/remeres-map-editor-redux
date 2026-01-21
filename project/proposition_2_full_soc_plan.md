# Proposition 2: Full SoC Decomposition - Detailed Implementation Plan

> **Goal**: Complete SOLID-compliant refactoring of the rendering pipeline with GLAD integration.
> **Philosophy**: Full compliance, no simplification, proper refactoring.

---

## Overview

| Metric | Value |
|--------|-------|
| Total Phases | 8 |
| Estimated Duration | 4-6 weeks |
| New Files | ~35 |
| Modified Files | ~15 |
| Deleted Files | 0 (deprecated, kept for reference) |

---

## GLAD Integration Note

**What is GLAD?**
GLAD is an OpenGL loader generator. It replaces:
- Manual OpenGL function pointer loading
- The need for `<GL/gl.h>` includes
- Provides modern, clean OpenGL header

**GLAD vs freeglut:**
- freeglut: Window management + GLUT utility functions (bitmap fonts)
- GLAD: OpenGL function loading

**Our approach:**
1. Add GLAD for proper OpenGL function loading
2. Keep freeglut temporarily for `glutBitmapCharacter` (tooltip fonts)
3. Later phase: Replace GLUT bitmap fonts with texture-based fonts

---

## Directory Structure (Final State)

```
source/rendering/
├── glad/                           # GLAD loader (generated)
│   ├── glad.h
│   ├── glad.c
│   └── khrplatform.h
├── interfaces/
│   ├── i_renderer.h                # Base renderer interface
│   ├── i_sprite_provider.h         # Sprite access interface
│   ├── i_render_target.h           # Render target abstraction
│   ├── i_texture_cache.h           # Texture caching interface
│   └── i_font_renderer.h           # Font rendering interface
├── types/
│   ├── render_types.h              # Common types, enums
│   ├── color.h                     # Color utilities
│   ├── rect.h                      # Rectangle type
│   ├── viewport.h                  # Viewport calculations
│   └── drawing_options.h           # Render options
├── opengl/
│   ├── gl_context.h/cpp            # OpenGL context management
│   ├── gl_state.h/cpp              # State machine wrapper
│   ├── gl_texture.h/cpp            # Texture wrapper
│   ├── gl_primitives.h/cpp         # Quad, line drawing
│   └── gl_debug.h/cpp              # Debug utilities
├── texture/
│   ├── texture_manager.h/cpp       # High-level texture management
│   ├── texture_cache.h/cpp         # LRU cache implementation
│   ├── sprite_loader.h/cpp         # DAT/SPR loading
│   └── sprite_sheet.h/cpp          # Sprite data container
├── fonts/
│   ├── font_renderer.h/cpp         # Font rendering abstraction
│   └── bitmap_font.h/cpp           # GLUT bitmap wrapper (temporary)
├── renderers/
│   ├── renderer_base.h             # CRTP base
│   ├── tile_renderer.h/cpp         # Tile rendering
│   ├── item_renderer.h/cpp         # Item rendering
│   ├── creature_renderer.h/cpp     # Creature rendering
│   ├── selection_renderer.h/cpp    # Selection overlay
│   ├── brush_renderer.h/cpp        # Brush preview
│   ├── grid_renderer.h/cpp         # Grid overlay
│   ├── light_renderer.h/cpp        # Lighting system
│   ├── tooltip_renderer.h/cpp      # Tooltips
│   └── ui_renderer.h/cpp           # Ingame box, cursors
├── pipeline/
│   ├── render_pass.h               # Pass definition
│   ├── render_queue.h/cpp          # Draw command queue
│   └── render_coordinator.h/cpp    # Frame orchestration
├── input/
│   ├── input_types.h               # Input event types
│   ├── input_dispatcher.h/cpp      # Event routing
│   ├── coordinate_mapper.h/cpp     # Screen <-> World
│   └── mouse_handler.h/cpp         # Mouse state
├── canvas/
│   └── map_canvas.h/cpp            # Thin wxGLCanvas wrapper
└── legacy/                         # Old files (deprecated)
    ├── map_drawer_legacy.h/cpp
    ├── map_display_legacy.h/cpp
    └── graphics_legacy.h/cpp
```

---

## Phase 0: Foundation & GLAD Integration

**Goal**: Integrate GLAD, setup directory structure, ensure build works.

### Subtask 0.1: Generate and Add GLAD

| Step | Action | Files |
|------|--------|-------|
| 0.1.1 | Download GLAD from https://glad.dav1d.de/ (GL 2.1, Core, C/C++) | External |
| 0.1.2 | Create `source/rendering/glad/` directory | New directory |
| 0.1.3 | Add `glad.h`, `glad.c`, `khrplatform.h` | New files |
| 0.1.4 | Update CMakeLists.txt to include GLAD sources | `source/CMakeLists.txt` |
| 0.1.5 | Update vcpkg.json: add `glad` dependency (alternative) | `vcpkg.json` |

### Subtask 0.2: Update OpenGL Includes

| Step | Action | Files |
|------|--------|-------|
| 0.2.1 | Create `source/rendering/opengl/gl_includes.h` | New file |
| 0.2.2 | Make all rendering files include `gl_includes.h` instead of direct GL headers | Multiple |
| 0.2.3 | Call `gladLoadGL()` after context creation in application.cpp | `application.cpp` |
| 0.2.4 | Remove `glutInit()` call (not needed with wxWidgets) | `application.cpp` |

### Subtask 0.3: Create Directory Structure

| Step | Action |
|------|--------|
| 0.3.1 | Create `source/rendering/interfaces/` |
| 0.3.2 | Create `source/rendering/types/` |
| 0.3.3 | Create `source/rendering/opengl/` |
| 0.3.4 | Create `source/rendering/texture/` |
| 0.3.5 | Create `source/rendering/fonts/` |
| 0.3.6 | Create `source/rendering/renderers/` |
| 0.3.7 | Create `source/rendering/pipeline/` |
| 0.3.8 | Create `source/rendering/input/` |
| 0.3.9 | Create `source/rendering/canvas/` |
| 0.3.10 | Create `source/rendering/legacy/` |

### Compilation Checkpoint 0

```bash
cmake --build . --target rme
# Expected: Build succeeds with GLAD integrated
# Test: Application starts, map renders correctly
```

### Verification 0
- [ ] GLAD files present in build
- [ ] `gladLoadGL()` called successfully
- [ ] No changes to visual output
- [ ] No GL errors on startup

---

## Phase 1: Interfaces & Type System

**Goal**: Define all interfaces and common types. No implementation changes yet.

### Subtask 1.1: Create Render Types

**File**: `source/rendering/types/render_types.h`
```cpp
#pragma once
#include <cstdint>

namespace rme::render {

// Forward declarations
struct Viewport;
struct DrawingOptions;
class RenderState;

// Coordinate types
struct Point {
    int x = 0;
    int y = 0;
};

struct Size {
    int width = 0;
    int height = 0;
};

// Tile size constant
inline constexpr int kTileSize = 32;
inline constexpr int kGroundLayer = 7;
inline constexpr int kMaxLayer = 15;

// Render pass order
enum class RenderPass : uint8_t {
    Background = 0,
    Tiles,
    Selection,
    DraggingShadow,
    HigherFloors,
    Brush,
    Grid,
    Light,
    UI,
    Tooltips,
    Count
};

} // namespace rme::render
```

### Subtask 1.2: Create Color Type

**File**: `source/rendering/types/color.h`
```cpp
#pragma once
#include <cstdint>
#include <wx/colour.h>

namespace rme::render {

struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    explicit Color(const wxColour& wx)
        : r(wx.Red()), g(wx.Green()), b(wx.Blue()), a(wx.Alpha()) {}

    [[nodiscard]] wxColour toWx() const { return wxColour(r, g, b, a); }

    [[nodiscard]] Color withAlpha(uint8_t newAlpha) const {
        return Color(r, g, b, newAlpha);
    }

    [[nodiscard]] Color dimmed(uint8_t factor = 2) const {
        return Color(r / factor, g / factor, b / factor, a);
    }
};

// Predefined colors
namespace colors {
    inline constexpr Color White{255, 255, 255};
    inline constexpr Color Black{0, 0, 0};
    inline constexpr Color Red{255, 0, 0};
    inline constexpr Color Green{0, 255, 0};
    inline constexpr Color Blue{0, 0, 255};
    inline constexpr Color Transparent{0, 0, 0, 0};
}

} // namespace rme::render
```

### Subtask 1.3: Create Viewport Type

**File**: `source/rendering/types/viewport.h`
```cpp
#pragma once
#include "render_types.h"

namespace rme::render {

struct Viewport {
    int scrollX = 0;
    int scrollY = 0;
    int width = 0;
    int height = 0;
    int floor = kGroundLayer;
    float zoom = 1.0f;

    [[nodiscard]] int tileSize() const noexcept {
        return static_cast<int>(kTileSize / zoom);
    }

    [[nodiscard]] int startTileX() const noexcept {
        return scrollX / kTileSize;
    }

    [[nodiscard]] int startTileY() const noexcept {
        return scrollY / kTileSize;
    }

    [[nodiscard]] int endTileX() const noexcept {
        return startTileX() + width / tileSize() + 2;
    }

    [[nodiscard]] int endTileY() const noexcept {
        return startTileY() + height / tileSize() + 2;
    }

    [[nodiscard]] int floorAdjustment() const noexcept {
        if (floor > kGroundLayer) return 0;
        return kTileSize * (kGroundLayer - floor);
    }

    [[nodiscard]] Point screenToTile(int screenX, int screenY) const noexcept;
    [[nodiscard]] Point tileToScreen(int tileX, int tileY) const noexcept;
};

} // namespace rme::render
```

### Subtask 1.4: Create Drawing Options

**File**: `source/rendering/types/drawing_options.h`
```cpp
#pragma once

namespace rme::render {

struct DrawingOptions {
    // Floor rendering
    bool transparentFloors = false;
    bool transparentItems = false;
    bool showAllFloors = true;
    bool showShade = true;

    // Entity visibility
    bool showCreatures = true;
    bool showSpawns = true;
    bool showItems = true;
    bool showTechItems = true;
    bool showWaypoints = true;

    // Overlays
    bool showGrid = false;
    bool showLights = false;
    bool showLightStrength = true;
    bool showHouses = true;
    bool showSpecialTiles = true;
    bool showTowns = false;
    bool showBlocking = false;
    bool showTooltips = false;
    bool showPreview = false;
    bool showHooks = false;
    bool showIngameBox = false;

    // Highlighting
    bool highlightItems = false;
    bool highlightLockedDoors = true;

    // Display modes
    bool ingame = false;
    bool asMinimap = false;
    bool onlyColors = false;
    bool onlyModified = false;

    // Performance
    bool hideItemsWhenZoomed = true;

    // Experimental
    bool experimentalFog = false;
    bool extendedHouseShader = false;

    // Runtime state
    bool dragging = false;

    void setDefault();
    void setIngame();
    [[nodiscard]] bool shouldDrawLight() const noexcept { return showLights; }
};

} // namespace rme::render
```

### Subtask 1.5: Create Interfaces

**File**: `source/rendering/interfaces/i_renderer.h`
```cpp
#pragma once

namespace rme::render {

class RenderState;

class IRenderer {
public:
    virtual ~IRenderer() = default;

    [[nodiscard]] virtual bool isEnabled() const noexcept = 0;
    virtual void render(const RenderState& state) = 0;
};

} // namespace rme::render
```

**File**: `source/rendering/interfaces/i_sprite_provider.h`
```cpp
#pragma once
#include <cstdint>

class GameSprite;

namespace rme::render {

class ISpriteProvider {
public:
    virtual ~ISpriteProvider() = default;

    [[nodiscard]] virtual GameSprite* getSprite(uint32_t id) = 0;
    [[nodiscard]] virtual GameSprite* getCreatureSprite(uint32_t id) = 0;
    [[nodiscard]] virtual uint32_t getItemSpriteMaxID() const = 0;
    [[nodiscard]] virtual uint32_t getCreatureSpriteMaxID() const = 0;
};

} // namespace rme::render
```

**File**: `source/rendering/interfaces/i_texture_cache.h`
```cpp
#pragma once
#include <cstdint>

using GLuint = unsigned int;

namespace rme::render {

class ITextureCache {
public:
    virtual ~ITextureCache() = default;

    [[nodiscard]] virtual GLuint getTexture(uint32_t spriteId) = 0;
    virtual void bindTexture(GLuint textureId) = 0;
    virtual void garbageCollect() = 0;
};

} // namespace rme::render
```

### Subtask 1.6: Update CMakeLists.txt

Add new header files to `rme_H` list.

### Compilation Checkpoint 1

```bash
cmake --build . --target rme
# Expected: Build succeeds, interfaces compile
# Test: No runtime changes, application unchanged
```

### Verification 1
- [ ] All new headers compile with no errors
- [ ] No undefined references
- [ ] C++20 features work (`constexpr`, `[[nodiscard]]`)
- [ ] Namespace structure correct

---

## Phase 2: OpenGL Abstraction Layer

**Goal**: Create clean OpenGL wrappers, isolate all raw GL calls.

### Subtask 2.1: GL Includes Header

**File**: `source/rendering/opengl/gl_includes.h`
```cpp
#pragma once

// Order matters: GLAD must come before any GL headers
#include "../glad/glad.h"

// Platform-specific
#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    // GLAD provides gl.h functionality
#endif

// wxWidgets GL canvas (after GLAD)
#include <wx/glcanvas.h>
```

### Subtask 2.2: GL State Manager

**File**: `source/rendering/opengl/gl_state.h`
```cpp
#pragma once
#include "gl_includes.h"
#include "../types/color.h"

namespace rme::render::gl {

class GLState {
public:
    static GLState& instance();

    // Texture binding with caching
    void bindTexture2D(GLuint textureId);
    void unbindTexture2D();

    // State toggles
    void enableBlend();
    void disableBlend();
    void enableTexture2D();
    void disableTexture2D();
    void enableLineStipple();
    void disableLineStipple();

    // Blend modes
    void setBlendAlpha();
    void setBlendLight();

    // Color state
    void setColor(const Color& color);
    void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // Matrix operations
    void pushMatrix();
    void popMatrix();
    void loadIdentity();
    void translate(float x, float y, float z = 0.0f);

    // Reset all cached state
    void resetCache();

private:
    GLState() = default;

    // Cached state
    GLuint currentTexture_ = 0;
    bool blendEnabled_ = false;
    bool texture2DEnabled_ = false;
};

} // namespace rme::render::gl
```

### Subtask 2.3: GL Primitives

**File**: `source/rendering/opengl/gl_primitives.h`
```cpp
#pragma once
#include "gl_includes.h"
#include "../types/color.h"
#include "../types/render_types.h"

namespace rme::render::gl {

class Primitives {
public:
    // Textured quad
    static void drawTexturedQuad(int x, int y, int w, int h,
                                  float u0 = 0.0f, float v0 = 0.0f,
                                  float u1 = 1.0f, float v1 = 1.0f);

    // Colored quad
    static void drawFilledQuad(int x, int y, int w, int h, const Color& color);

    // Outline rectangle
    static void drawRect(int x, int y, int w, int h,
                         const Color& color, int lineWidth = 1);

    // Lines
    static void drawLine(int x1, int y1, int x2, int y2,
                         const Color& color, int width = 1);

    // Batch line drawing (for grid)
    static void beginLines();
    static void addLine(int x1, int y1, int x2, int y2);
    static void endLines();

    // Tile-sized sprite
    static void drawSprite(int x, int y, GLuint textureId,
                           const Color& tint = colors::White);
};

} // namespace rme::render::gl
```

### Subtask 2.4: GL Context Manager

**File**: `source/rendering/opengl/gl_context.h`
```cpp
#pragma once
#include "gl_includes.h"
#include "../types/viewport.h"

namespace rme::render::gl {

class GLContext {
public:
    // Initialize OpenGL for frame
    static void beginFrame(const Viewport& viewport);
    static void endFrame();

    // Viewport setup
    static void setupViewport(int width, int height);
    static void setupOrtho(int width, int height, float zoom);

    // Clear
    static void clear(const Color& color = colors::Black);

    // Context info
    [[nodiscard]] static const char* getVersion();
    [[nodiscard]] static const char* getRenderer();
    [[nodiscard]] static bool isInitialized();

    // Debug
    static void checkError(const char* location);
};

} // namespace rme::render::gl
```

### Subtask 2.5: Implement GL Wrappers

**Files**: Create `.cpp` implementations for all above headers.

### Subtask 2.6: Update Existing Code to Use Wrappers

| File | Change |
|------|--------|
| `map_drawer.cpp` | Replace raw `glColor4ub` with `GLState::setColor` |
| `map_drawer.cpp` | Replace raw `glBindTexture` with `GLState::bindTexture2D` |
| `light_drawer.cpp` | Use `gl::Primitives` for quad drawing |

### Compilation Checkpoint 2

```bash
cmake --build . --target rme
# Expected: Build succeeds with GL wrappers
# Test: Visual output unchanged, no GL errors
```

### Verification 2
- [ ] All GL calls go through wrappers (grep for raw glBegin)
- [ ] Texture binding cache works
- [ ] State changes minimized
- [ ] No visual regressions

---

## Phase 3: Texture & Sprite System Refactoring

**Goal**: Extract sprite loading and texture management into clean modules.

### Subtask 3.1: Sprite Loader

**File**: `source/rendering/texture/sprite_loader.h`
```cpp
#pragma once
#include <string>
#include <optional>

namespace rme::render {

struct SpriteMetadata;
struct SpritePixelData;

class SpriteLoader {
public:
    struct LoadResult {
        bool success = false;
        std::string error;
        uint32_t itemCount = 0;
        uint32_t creatureCount = 0;
    };

    [[nodiscard]] LoadResult loadMetadata(const std::string& datPath);
    [[nodiscard]] LoadResult loadPixelData(const std::string& sprPath);

    [[nodiscard]] std::optional<SpritePixelData>
        getPixelData(uint32_t spriteId) const;

private:
    // Implementation details
};

} // namespace rme::render
```

### Subtask 3.2: Texture Manager

**File**: `source/rendering/texture/texture_manager.h`
```cpp
#pragma once
#include "../interfaces/i_texture_cache.h"
#include "../interfaces/i_sprite_provider.h"
#include <memory>
#include <unordered_map>

namespace rme::render {

class TextureManager : public ITextureCache, public ISpriteProvider {
public:
    TextureManager();
    ~TextureManager() override;

    // ITextureCache
    [[nodiscard]] GLuint getTexture(uint32_t spriteId) override;
    void bindTexture(GLuint textureId) override;
    void garbageCollect() override;

    // ISpriteProvider
    [[nodiscard]] GameSprite* getSprite(uint32_t id) override;
    [[nodiscard]] GameSprite* getCreatureSprite(uint32_t id) override;
    [[nodiscard]] uint32_t getItemSpriteMaxID() const override;
    [[nodiscard]] uint32_t getCreatureSpriteMaxID() const override;

    // Loading
    bool loadFromFiles(const std::string& datPath, const std::string& sprPath);
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace rme::render
```

### Subtask 3.3: Migrate GraphicManager Functions

| From `graphics.cpp` | To |
|---------------------|-----|
| `loadSpriteMetadata` | `SpriteLoader::loadMetadata` |
| `loadSpriteData` | `SpriteLoader::loadPixelData` |
| `getSprite` | `TextureManager::getSprite` |
| `garbageCollection` | `TextureManager::garbageCollect` |
| `getFreeTextureID` | Internal to TextureManager |

### Subtask 3.4: Update All Sprite Access

| File | Change |
|------|--------|
| `map_drawer.cpp` | Use `TextureManager` interface |
| `gui.cpp` | Access through `TextureManager` |
| `palette_*.cpp` | Access through `TextureManager` |

### Compilation Checkpoint 3

```bash
cmake --build . --target rme
# Expected: Build succeeds with new texture system
# Test: Map renders, sprites load correctly
```

### Verification 3
- [ ] All sprites load correctly
- [ ] Creature sprites work
- [ ] Texture garbage collection functions
- [ ] Memory usage similar to before

---

## Phase 4: Renderer Implementations

**Goal**: Create all individual renderers following SRP.

### Subtask 4.1: Renderer Base

**File**: `source/rendering/renderers/renderer_base.h`
```cpp
#pragma once
#include "../interfaces/i_renderer.h"
#include "../types/viewport.h"
#include "../types/drawing_options.h"

namespace rme::render {

class RenderState;

template<typename Derived>
class RendererBase : public IRenderer {
protected:
    // Access state
    [[nodiscard]] const Viewport& viewport() const noexcept;
    [[nodiscard]] const DrawingOptions& options() const noexcept;
    [[nodiscard]] const RenderState& state() const noexcept;

    // Helpers
    [[nodiscard]] int screenX(int tileX) const noexcept;
    [[nodiscard]] int screenY(int tileY) const noexcept;
    [[nodiscard]] bool isInView(int tileX, int tileY) const noexcept;

private:
    const RenderState* currentState_ = nullptr;
};

} // namespace rme::render
```

### Subtask 4.2: Tile Renderer

**File**: `source/rendering/renderers/tile_renderer.h`
```cpp
#pragma once
#include "renderer_base.h"

class TileLocation;
class Tile;
class Item;
class Creature;

namespace rme::render {

class TileRenderer : public RendererBase<TileRenderer> {
public:
    [[nodiscard]] bool isEnabled() const noexcept override;
    void render(const RenderState& state) override;

private:
    void renderTile(TileLocation* location);
    void renderGround(int x, int y, const Tile* tile);
    void renderItem(int& x, int& y, const Item* item, const Tile* tile);
    void renderCreature(int x, int y, const Creature* creature);
    void renderSpawn(int x, int y, const Tile* tile);
};

} // namespace rme::render
```

### Subtask 4.3: Item Renderer

**File**: `source/rendering/renderers/item_renderer.h`
```cpp
#pragma once
#include "renderer_base.h"
#include "../types/color.h"

class Item;
class Tile;
class GameSprite;

namespace rme::render {

class ItemRenderer : public RendererBase<ItemRenderer> {
public:
    void blitItem(int& drawX, int& drawY,
                  const Item* item,
                  const Tile* tile,
                  const Color& tint = colors::White,
                  bool ephemeral = false);

    void blitSprite(int x, int y,
                    GameSprite* sprite,
                    int frame,
                    const Color& tint = colors::White);

private:
    [[nodiscard]] Color calculateTint(const Item* item,
                                       const Tile* tile,
                                       bool ephemeral) const;
};

} // namespace rme::render
```

### Subtask 4.4: Creature Renderer

**File**: `source/rendering/renderers/creature_renderer.h`
```cpp
#pragma once
#include "renderer_base.h"

class Creature;
struct Outfit;
enum Direction : uint8_t;

namespace rme::render {

class CreatureRenderer : public RendererBase<CreatureRenderer> {
public:
    void renderCreature(int x, int y,
                        const Creature* creature,
                        const Color& tint = colors::White);

    void renderOutfit(int x, int y,
                      const Outfit& outfit,
                      Direction direction,
                      const Color& tint = colors::White);

private:
    void renderColoredLayers(int x, int y,
                             GameSprite* sprite,
                             const Outfit& outfit);
};

} // namespace rme::render
```

### Subtask 4.5: Selection Renderer

**File**: `source/rendering/renderers/selection_renderer.h`
```cpp
#pragma once
#include "renderer_base.h"

namespace rme::render {

class SelectionRenderer : public RendererBase<SelectionRenderer> {
public:
    [[nodiscard]] bool isEnabled() const noexcept override;
    void render(const RenderState& state) override;

private:
    void renderSelectionBox();
    void renderDraggingShadow();
    void renderLiveCursors();
};

} // namespace rme::render
```

### Subtask 4.6: Brush Renderer

**File**: `source/rendering/renderers/brush_renderer.h`
```cpp
#pragma once
#include "renderer_base.h"

class Brush;

namespace rme::render {

class BrushRenderer : public RendererBase<BrushRenderer> {
public:
    [[nodiscard]] bool isEnabled() const noexcept override;
    void render(const RenderState& state) override;

private:
    void renderBrushPreview(const Brush* brush);
    void renderBrushIndicator(int x, int y, const Brush* brush);
    void renderDraggingBrush(const Brush* brush);

    [[nodiscard]] Color getBrushColor(const Brush* brush) const;
};

} // namespace rme::render
```

### Subtask 4.7: Grid Renderer

**File**: `source/rendering/renderers/grid_renderer.h`
```cpp
#pragma once
#include "renderer_base.h"

namespace rme::render {

class GridRenderer : public RendererBase<GridRenderer> {
public:
    [[nodiscard]] bool isEnabled() const noexcept override;
    void render(const RenderState& state) override;

private:
    Color gridColor_{255, 255, 255, 128};
};

} // namespace rme::render
```

### Subtask 4.8: Light Renderer

**File**: `source/rendering/renderers/light_renderer.h`
```cpp
#pragma once
#include "renderer_base.h"
#include <vector>

struct SpriteLight;

namespace rme::render {

class LightRenderer : public RendererBase<LightRenderer> {
public:
    LightRenderer();
    ~LightRenderer();

    [[nodiscard]] bool isEnabled() const noexcept override;
    void render(const RenderState& state) override;

    void addLight(int mapX, int mapY, int mapZ, const SpriteLight& light);
    void setGlobalLightColor(uint8_t color);
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace rme::render
```

### Subtask 4.9: Tooltip Renderer

**File**: `source/rendering/renderers/tooltip_renderer.h`
```cpp
#pragma once
#include "renderer_base.h"
#include <vector>
#include <string>

namespace rme::render {

class TooltipRenderer : public RendererBase<TooltipRenderer> {
public:
    [[nodiscard]] bool isEnabled() const noexcept override;
    void render(const RenderState& state) override;

    void addTooltip(int x, int y, const std::string& text, const Color& color);
    void clear();

private:
    struct Tooltip {
        int x, y;
        std::string text;
        Color color;
        bool ellipsis;
    };

    std::vector<Tooltip> tooltips_;
};

} // namespace rme::render
```

### Subtask 4.10: UI Renderer

**File**: `source/rendering/renderers/ui_renderer.h`
```cpp
#pragma once
#include "renderer_base.h"

namespace rme::render {

class UIRenderer : public RendererBase<UIRenderer> {
public:
    [[nodiscard]] bool isEnabled() const noexcept override;
    void render(const RenderState& state) override;

private:
    void renderIngameBox();
    void renderHigherFloors();
};

} // namespace rme::render
```

### Subtask 4.11: Implement All Renderers

Create `.cpp` files for all renderers, extracting code from `map_drawer.cpp`.

### Compilation Checkpoint 4

```bash
cmake --build . --target rme
# Expected: Build succeeds with all renderers
# Test: No runtime changes yet (renderers not wired)
```

### Verification 4
- [ ] All renderer headers compile
- [ ] All renderer implementations compile
- [ ] No circular dependencies
- [ ] Code extracted matches original functionality

---

## Phase 5: Pipeline Orchestration

**Goal**: Create render coordinator to manage passes.

### Subtask 5.1: Render State

**File**: `source/rendering/pipeline/render_state.h`
```cpp
#pragma once
#include "../types/viewport.h"
#include "../types/drawing_options.h"

class Editor;
class Map;
class Brush;

namespace rme::render {

class TextureManager;

class RenderState {
public:
    // Setup for frame
    void setupForFrame(int floor, float zoom,
                       int scrollX, int scrollY,
                       int screenWidth, int screenHeight);

    // Accessors
    [[nodiscard]] const Viewport& viewport() const noexcept { return viewport_; }
    [[nodiscard]] const DrawingOptions& options() const noexcept { return options_; }
    [[nodiscard]] DrawingOptions& options() noexcept { return options_; }

    // Context
    [[nodiscard]] Editor* editor() const noexcept { return editor_; }
    [[nodiscard]] Map* map() const noexcept;
    [[nodiscard]] Brush* currentBrush() const noexcept;
    [[nodiscard]] TextureManager* textures() const noexcept { return textures_; }

    // Mouse state
    [[nodiscard]] int mouseMapX() const noexcept { return mouseMapX_; }
    [[nodiscard]] int mouseMapY() const noexcept { return mouseMapY_; }

    void setEditor(Editor* editor) { editor_ = editor; }
    void setTextureManager(TextureManager* textures) { textures_ = textures; }
    void setMousePosition(int mapX, int mapY);

private:
    Viewport viewport_;
    DrawingOptions options_;
    Editor* editor_ = nullptr;
    TextureManager* textures_ = nullptr;
    int mouseMapX_ = 0;
    int mouseMapY_ = 0;
};

} // namespace rme::render
```

### Subtask 5.2: Render Coordinator

**File**: `source/rendering/pipeline/render_coordinator.h`
```cpp
#pragma once
#include "render_state.h"
#include "../types/render_types.h"
#include <memory>
#include <array>

namespace rme::render {

class IRenderer;

class RenderCoordinator {
public:
    RenderCoordinator();
    ~RenderCoordinator();

    // Frame execution
    void beginFrame(const RenderState& state);
    void executeFrame();
    void endFrame();

    // Renderer access
    template<typename T>
    [[nodiscard]] T* getRenderer();

    // State
    [[nodiscard]] RenderState& state() noexcept { return state_; }
    [[nodiscard]] const RenderState& state() const noexcept { return state_; }

private:
    RenderState state_;
    std::array<std::unique_ptr<IRenderer>,
               static_cast<size_t>(RenderPass::Count)> renderers_;
};

} // namespace rme::render
```

### Subtask 5.3: Implement Coordinator

**File**: `source/rendering/pipeline/render_coordinator.cpp`

Wire all renderers in correct order.

### Compilation Checkpoint 5

```bash
cmake --build . --target rme
# Expected: Build succeeds with coordinator
# Test: No runtime changes yet
```

### Verification 5
- [ ] Coordinator compiles
- [ ] All renderers instantiated
- [ ] Pass order correct

---

## Phase 6: Input Separation

**Goal**: Extract input handling from MapCanvas.

### Subtask 6.1: Input Types

**File**: `source/rendering/input/input_types.h`
```cpp
#pragma once
#include <cstdint>

namespace rme::render::input {

enum class MouseButton : uint8_t {
    Left,
    Middle,
    Right
};

enum class KeyModifier : uint8_t {
    None = 0,
    Shift = 1 << 0,
    Ctrl = 1 << 1,
    Alt = 1 << 2
};

struct MouseEvent {
    int screenX = 0;
    int screenY = 0;
    int mapX = 0;
    int mapY = 0;
    MouseButton button = MouseButton::Left;
    uint8_t modifiers = 0;
    bool isDown = false;
    bool isDoubleClick = false;
    int wheelDelta = 0;
};

struct KeyEvent {
    int keyCode = 0;
    uint8_t modifiers = 0;
    bool isDown = false;
};

} // namespace rme::render::input
```

### Subtask 6.2: Coordinate Mapper

**File**: `source/rendering/input/coordinate_mapper.h`
```cpp
#pragma once
#include "../types/viewport.h"

namespace rme::render::input {

class CoordinateMapper {
public:
    void setViewport(const Viewport& viewport);

    [[nodiscard]] Point screenToMap(int screenX, int screenY) const;
    [[nodiscard]] Point mapToScreen(int mapX, int mapY) const;
    [[nodiscard]] Point getScreenCenter() const;

private:
    Viewport viewport_;
};

} // namespace rme::render::input
```

### Subtask 6.3: Mouse Handler

**File**: `source/rendering/input/mouse_handler.h`
```cpp
#pragma once
#include "input_types.h"
#include "coordinate_mapper.h"

namespace rme::render::input {

class MouseHandler {
public:
    void setCoordinateMapper(const CoordinateMapper& mapper);

    void onMouseMove(int x, int y);
    void onMouseDown(MouseButton button, int x, int y, uint8_t modifiers);
    void onMouseUp(MouseButton button, int x, int y, uint8_t modifiers);
    void onMouseWheel(int delta, int x, int y);

    [[nodiscard]] int cursorX() const noexcept { return cursorX_; }
    [[nodiscard]] int cursorY() const noexcept { return cursorY_; }
    [[nodiscard]] int mapX() const noexcept { return mapX_; }
    [[nodiscard]] int mapY() const noexcept { return mapY_; }

    [[nodiscard]] bool isDragging() const noexcept { return dragging_; }
    [[nodiscard]] bool isScreenDragging() const noexcept { return screenDragging_; }

private:
    CoordinateMapper mapper_;
    int cursorX_ = 0;
    int cursorY_ = 0;
    int mapX_ = 0;
    int mapY_ = 0;
    bool dragging_ = false;
    bool screenDragging_ = false;
};

} // namespace rme::render::input
```

### Subtask 6.4: Implement Input Classes

Create `.cpp` implementations.

### Subtask 6.5: Integrate with MapCanvas

Update `map_display.cpp` to use new input classes.

### Compilation Checkpoint 6

```bash
cmake --build . --target rme
# Expected: Build succeeds with input separation
# Test: Input handling works correctly
```

### Verification 6
- [ ] Mouse movement updates correctly
- [ ] Click handling works
- [ ] Coordinate conversion accurate
- [ ] Dragging works

---

## Phase 7: Canvas Refactoring

**Goal**: Create thin MapCanvas wrapper.

### Subtask 7.1: New MapCanvas

**File**: `source/rendering/canvas/map_canvas.h`
```cpp
#pragma once
#include "../opengl/gl_includes.h"
#include "../input/mouse_handler.h"
#include "../pipeline/render_coordinator.h"
#include <wx/glcanvas.h>

class Editor;
class MapWindow;

namespace rme::render {

class MapCanvas : public wxGLCanvas {
public:
    MapCanvas(MapWindow* parent, Editor& editor, int* attribList);
    ~MapCanvas() override;

    void refresh();
    void setZoom(double value);
    void changeFloor(int newFloor);

    [[nodiscard]] int floor() const noexcept { return floor_; }
    [[nodiscard]] double zoom() const noexcept { return zoom_; }
    [[nodiscard]] Position cursorPosition() const;

    void takeScreenshot(const wxFileName& path, const wxString& format);

protected:
    void OnPaint(wxPaintEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

private:
    Editor& editor_;
    RenderCoordinator coordinator_;
    input::MouseHandler mouseHandler_;
    int floor_ = 7;
    double zoom_ = 1.0;

    wxDECLARE_EVENT_TABLE();
};

} // namespace rme::render
```

### Subtask 7.2: Implement New MapCanvas

Wire coordinator, implement event handlers.

### Subtask 7.3: Update External References

| File | Change |
|------|--------|
| `map_window.h/cpp` | Use new `MapCanvas` |
| `editor.cpp` | Use new canvas interface |
| `gui.cpp` | Update references |

### Compilation Checkpoint 7

```bash
cmake --build . --target rme
# Expected: Full build with new canvas
# Test: Full application functionality
```

### Verification 7
- [ ] Map renders correctly
- [ ] All interactions work
- [ ] Zoom and floor changes work
- [ ] Screenshots work

---

## Phase 8: Final Integration & Cleanup

**Goal**: Remove deprecated code, final polish.

### Subtask 8.1: Move Legacy Files

| File | Action |
|------|--------|
| `map_drawer.cpp` → `legacy/map_drawer_legacy.cpp` | Move |
| `map_display.cpp` → `legacy/map_display_legacy.cpp` | Move |
| `graphics.cpp` → `legacy/graphics_legacy.cpp` | Move |

### Subtask 8.2: Update CMakeLists.txt

Remove legacy files from build, add all new files.

### Subtask 8.3: Remove Unused Code

- Remove deprecated functions
- Remove unused includes
- Clean up comments

### Subtask 8.4: Remove freeglut Dependency

| Step | Action |
|------|--------|
| 8.4.1 | Replace `glutBitmapCharacter` with custom font renderer |
| 8.4.2 | Remove `freeglut` from vcpkg.json |
| 8.4.3 | Remove `#include <GL/glut.h>` |

### Subtask 8.5: Documentation

- Update inline documentation
- Add file headers to all new files

### Final Compilation Checkpoint

```bash
cmake --build . --target rme
# Expected: Clean build with no legacy code
# Test: Full visual verification
```

### Final Verification
- [ ] No legacy code in build
- [ ] No freeglut dependency
- [ ] All features work
- [ ] Code follows SOLID
- [ ] File sizes reasonable (<500 lines each)
- [ ] No circular dependencies
- [ ] C++20 features used correctly

---

## Success Metrics

| Metric | Target |
|--------|--------|
| Max file size | <500 lines |
| Interface count | ≥5 |
| Renderer count | 9 |
| Circular dependencies | 0 |
| Raw GL calls outside `opengl/` | 0 |
| Build warnings | 0 |
| Visual regressions | 0 |

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Breaking existing functionality | Compilation checkpoints after each phase |
| Performance regression | Benchmark before/after |
| Merge conflicts | Work on feature branch |
| Scope creep | Strict phase boundaries |

---

## Next Steps

After approval:
1. Create feature branch `refactor/rendering-soc`
2. Begin Phase 0 (GLAD integration)
3. Update task.md as progress is made
