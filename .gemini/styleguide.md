# RME Redux — Project Styleguide

## 🎯 CORE PRINCIPLES

| Priority | Principle | Meaning |
|---|---|---|
| 0 | **C++20/23** | **MANDATORY.** Every new and modified file MUST use modern C++20/23 features. No exceptions. |
| 1 | **DOD** | Data-Oriented Design — flat structs, contiguous storage, cache-friendly layouts |
| 2 | **SRP** | Single Responsibility — one reason to change per file/class/function |
| 3 | **KISS** | Keep It Simple — prefer simple solutions over clever abstractions |
| 4 | **DRY** | Don't Repeat Yourself — search before coding, reuse existing utilities |

> [!IMPORTANT]
> This is a **2D tile-based map editor for Tibia** (rewrite of Remere's Map Editor).
> **C++20/23 MANDATORY** · wxWidgets 3.3.x · OpenGL 4.5 · CMake
>
> Every line of code you write or modify **MUST** use C++20/23 features. If you see pre-C++20 patterns while editing a file, **upgrade them**. This is not optional — it is the #1 technical requirement.

---

## 🚫 CRITICAL RULES

1. **`application.cpp` is OFF-LIMITS** — Only app init and main loop. Business logic → proper module.
2. **SEARCH BEFORE CODING** — Check existing utilities, helpers, managers before creating new ones.
3. **SRP LIMITS** — File > 500 lines → split. Function > 50 lines → split. Class doing multiple things → split.
4. **NO FRUSTUM CULLING** — We use `SpatialHashGrid` with optimized dual-strategy `visitLeaves()`. Never add frustum.
5. **NO TOOLTIPS ON HOVER** — Tooltips are always-visible information panels. This is a map editor — quick info matters.
6. **NO STATUS BAR INFO / ON-MOUSE INFO** — Don't add new status bars or mouse-position info panels.

---

## 🏗️ DATA-ORIENTED DESIGN

### Value Types Over Pointers
```cpp
// ✅ Flat data struct
struct TileData {
    uint16_t ground_id;
    uint16_t mapflags;
    uint16_t statflags;
    uint8_t minimap_color;
};

// ❌ Pointer chasing
struct Tile {
    TileLocation* location;   // pointer chase
    std::unique_ptr<Item> ground;  // another chase
};
```

### Contiguous Storage
```cpp
// ✅ Cache-friendly iteration
std::vector<SpriteInstance> pending_sprites_;  // flat, contiguous

// ❌ Pointer-per-element
std::vector<std::unique_ptr<Item>> items;  // each element is a heap chase
```

### Separate Data From Behavior
```cpp
// ✅ Free functions operating on data
bool tile_has_ground(const TileData& data);
void tile_set_flag(TileData& data, TileFlag flag);

// ❌ God class with 40+ methods
class Tile {
    bool hasGround() const;
    bool hasBorders() const;
    bool hasTable() const;
    // ... 37 more methods
};
```

---

## 🔧 C++20/23 — MANDATORY STANDARD

> [!CAUTION]
> **C++20/23 is NOT optional.** Every new file, every modified function, every touched line MUST use modern C++. If you encounter legacy C++ while editing, you MUST upgrade it in the same commit. Pre-C++17 patterns are technical debt — eliminate on contact.

### Required C++20/23 Patterns (use these EVERYWHERE)
```cpp
// ✅ std::format over sprintf/wxString::Format
auto msg = std::format("Position: {}, {}", x, y);

// ✅ std::span for non-owning ranges
void process_tiles(std::span<Tile*> tiles);

// ✅ Concepts for templates
template<typename T> requires std::integral<T>
void snap_to_grid(T& value, T grid_size);

// ✅ Designated initializers
Position pos{.x = 10, .y = 20, .z = 7};

// ✅ Structured bindings
auto [x, y, z] = position.get_coords();

// ✅ enum class (not raw enum)
enum class TileState : uint16_t { None = 0, Selected = 0x01 };

// ✅ = delete over private copy ctors
MyClass(const MyClass&) = delete;

// ✅ C++23: std::expected for error handling
std::expected<Map, LoadError> load_map(const std::filesystem::path& path);

// ✅ C++23: std::print when available
std::print("Loaded {} tiles\n", count);

// ✅ constexpr everywhere possible
constexpr int CELL_SIZE = 1 << 6;
```

### Memory Management
```cpp
// ✅ Smart pointers for ownership
auto tile = std::make_unique<Tile>(...);

// ✅ Raw pointers for observation only (non-owning)
Tile* observed = container.get();

// ❌ BANNED
Tile* t = new Tile();  // raw new
delete t;              // raw delete
```

### Threading
```cpp
// ✅ Standard C++ threading
std::jthread worker([](std::stop_token st) { /* ... */ });

// ✅ UI updates from threads
wxGetApp().CallAfter([result]() { panel->Update(result); });

// ❌ BANNED
wxThread* thread = new MyThread();  // use std::thread/jthread
```

---

## 🖼️ wxWidgets Rules

### Event Handling
- **USE:** `Bind()` with lambdas or method pointers
- **BANNED:** `DECLARE_EVENT_TABLE`, `Connect()`

### Object Lifecycle
- **USE:** `window->Destroy()` for windows
- **BANNED:** `delete window`
- Parent owns children — trust the hierarchy.

### Strings
- **USE:** Standard literals `"text"`
- **BANNED:** `wxT("text")`, `L"text"`

### Layout
- **USE:** `wxSizer` + `wxSizerFlags` for all layouts
- **USE:** `FromDIP()` for any pixel values
- **USE:** `sizer->AddSpacer(n)` for spacing
- **BANNED:** Hardcoded `wxPoint`/`wxSize` pixels, empty `wxStaticText` for padding

### High DPI & Theming
- **USE:** `wxBitmapBundle` (SVG preferred)
- **USE:** `wxSystemSettings::GetColour()` for colors
- **USE:** `wxAutoBufferedPaintDC` for custom paint
- **BANNED:** `wxBitmap`/`wxIcon` directly, hardcoded colors (`*wxWHITE`, `*wxBLACK`), `wxPaintDC` without buffering

### Icons & Assets
- **USE:** `IMAGE_MANAGER.GetBitmap(ICON_*)` for loading icons
- **USE:** `wxBitmapBundle` for new icon integration
- **BANNED:** `wxEmbeddedFile`, loose file assumptions

### IDs
- **USE:** `wxID_ANY` for dynamic IDs
- **USE:** `wxID_OK`, `wxID_CANCEL`, `wxID_EXIT` for standard actions
- **BANNED:** Hardcoded magic numbers (`10001`)

### Containers
- **USE:** `std::vector`, `std::string`, `std::thread`
- **BANNED:** `wxList`, `wxArrayInt`, `wxThread` (prefer std equivalents)

### Data Validation
- **USE:** `wxTextValidator` for input filtering
- **BANNED:** Manual `OnChar` key filtering

### Logging
- **USE:** `wxLogMessage()`, `wxLogError()`
- **BANNED:** `std::cout`, `printf` in GUI code

---

## 🎮 RENDERING ARCHITECTURE

### Current System (Already Modernized)
The rendering pipeline uses **OpenGL 4.5** with:

| Component | File | Purpose |
|---|---|---|
| `SpriteBatch` | `rendering/core/sprite_batch.h` | Instanced rendering, MDI, RingBuffer (100k sprites, 6.4MB) |
| `TextureAtlas` | `rendering/core/texture_atlas.h` | Dynamic texture atlas with packing |
| `RingBuffer` | `rendering/core/ring_buffer.h` | Triple-buffered persistent mapping |
| `MultiDrawIndirectRenderer` | `rendering/core/multi_draw_indirect_renderer.h` | GL 4.3+ MDI batching |
| `GLResources` | `rendering/core/gl_resources.h` | RAII wrappers for VAO/VBO/FBO/textures |
| `ScopedGLState` | `rendering/core/gl_scoped_state.h` | Scoped GL state management |
| `SyncHandle` | `rendering/core/sync_handle.h` | Fence sync for ring buffer |
| `MapDrawer` | `rendering/map_drawer.h` | Orchestrates 18+ specialized drawers |

### Rendering Rules
- **USE:** `SpriteBatch` for all sprite rendering — never raw GL draw calls
- **USE:** RAII wrappers from `gl_resources.h` for all GL objects
- **USE:** `SpatialHashGrid::visitLeaves()` for visibility queries
- **BANNED:** `glBegin`/`glEnd` (immediate mode)
- **BANNED:** `glFinish()` in render loops
- **BANNED:** Per-tile texture binds — batch through atlas

---

## 📁 FILE ORGANIZATION

### Actual Structure
```
source/
├── app/            # Application, main, definitions
├── brushes/        # Brush implementations (15 subdirs by type)
│   ├── ground/     carpet/  table/  wall/  door/
│   ├── creature/   spawn/   doodad/ house/
│   └── managers/   raw/     eraser/ flag/  waypoint/
├── editor/         # Actions, selection, undo, copy, hotkeys
│   ├── operations/ # Editor operations (10 files)
│   └── persistence/
├── game/           # Items, creatures, houses, towns, materials
├── io/             # File I/O, loaders
│   ├── loaders/    # DAT/SPR loaders
│   └── otbm/       # OTBM serialization (decomposed)
├── map/            # Tiles, position, spatial hash grid, regions
├── net/            # Live editing / network
├── palette/        # Palette panels and managers
├── rendering/
│   ├── core/       # SpriteBatch, atlas, ring buffer, shaders, GL wrappers
│   ├── drawers/    # Specialized drawers (tiles, entities, overlays, minimap)
│   ├── postprocess/
│   ├── shaders/
│   ├── ui/         # Rendering UI components
│   └── utilities/
├── ui/             # wxWidgets UI (200+ files)
│   ├── controls/   dialogs/   menubar/   toolbar/
│   ├── properties/ tile_properties/  map/  replace_tool/
│   └── managers/
└── util/           # Common utilities, image manager
```

### File Naming
| Pattern | Location |
|---------|----------|
| `*_window.cpp` | `ui/` |
| `*_drawer.cpp` | `rendering/drawers/` |
| `*_brush.cpp` | `brushes/<type>/` |
| `*_serialization_otbm.cpp` | `io/otbm/` |

---

## ✅ PRE-COMMIT CHECKLIST

```
☐ No code added to application.cpp
☐ No duplicate code (searched first)
☐ Functions < 50 lines
☐ Files < 500 lines (or has splitting plan)
☐ Smart pointers for new allocations
☐ enum class (not raw enum) for new enums
☐ RAII for any new OpenGL resources
☐ Bind() for new event handling (no event tables)
☐ FromDIP() for any new pixel values
☐ wxBitmapBundle for new icons
☐ Builds clean with no new warnings
```

## 🔄 WHEN TOUCHING A FILE

Apply these incremental improvements:
1. `NULL` → `nullptr`
2. `auto` where type is obvious
3. Range-based `for` loops
4. `const` on non-mutating methods
5. C-style casts → `static_cast`/`dynamic_cast`
6. `override` on virtual functions
7. `= default` / `= delete` on special members
8. `enum` → `enum class`
9. Separate data structs from behavior methods

---

## 📌 THE MANTRA

**SEARCH → REUSE → FLATTEN → SIMPLIFY → IMPLEMENT**
