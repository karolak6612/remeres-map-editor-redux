---
name: RME Image System (ImageManager)
description: How to add, organize, and use image assets in Remere's Map Editor via the centralized ImageManager. Covers wxWidgets bitmaps, NanoVG textures, tinting, and the asset macro convention.
---

# RME Image System — ImageManager

The `ImageManager` is the **single source of truth** for all image/icon assets in RME. It replaces the legacy `ArtProvider`, `pngfiles.h/cpp`, and inline XPM data. Every UI component — wxWidgets toolbars, NanoVG canvases, and OpenGL views — loads images through this one class.

## 1. Architecture Overview

```
source/assets/          ← Asset source files (copied to build dir)
├── png/                ← Raster images (brush shapes, zone icons, etc.)
├── svg/
│   ├── solid/          ← Font Awesome "solid" style SVGs
│   └── regular/        ← Font Awesome "regular" style SVGs
└── icon/               ← Application icons (ICO/XPM)

source/util/
├── image_manager.h     ← Class declaration + all asset path macros
└── image_manager.cpp   ← Implementation (path resolution, caching, loading)
```

### Build Pipeline
CMake copies `source/assets/` → `${CMAKE_BINARY_DIR}/Release/assets/` at build time. The `ImageManager` resolves paths relative to the executable's `assets/` directory at runtime.

## 2. Adding a New Image Asset

### Step 1 — Place the file
| Format | Where to put it | When to use |
|:-------|:----------------|:------------|
| **PNG** | `source/assets/png/my_icon.png` | Pixel-art brush shapes, zone overlays, editor sprites |
| **SVG** | `source/assets/svg/solid/my_icon.svg` or `svg/regular/` | Scalable toolbar/UI icons (prefer SVG for DPI) |

### Step 2 — Define a macro in `image_manager.h`
Add a `#define` at the bottom of `source/util/image_manager.h`, grouped with related assets:

```cpp
// In image_manager.h — use the path relative to the assets/ root
#define IMAGE_MY_FEATURE       "png/my_feature.png"
#define IMAGE_MY_FEATURE_SMALL "png/my_feature_small.png"
#define ICON_MY_ACTION         "svg/solid/my-action.svg"
```

### Step 3 — Use it in code
Include the header and call the appropriate getter:

```cpp
#include "util/image_manager.h"

// wxWidgets bitmap (toolbar, button, etc.)
wxBitmap bmp = IMAGE_MANAGER.GetBitmap(ICON_MY_ACTION, wxSize(16, 16));

// wxBitmapBundle (HiDPI support, wxAuiToolBar)
wxBitmapBundle bundle = IMAGE_MANAGER.GetBitmapBundle(IMAGE_MY_FEATURE);

// NanoVG texture (custom canvas rendering)
int nvgImg = IMAGE_MANAGER.GetNanoVGImage(vg, IMAGE_MY_FEATURE);
```

### Step 4 — Rebuild
Run `build_windows.bat`. CMake will copy the new asset to the build directory automatically.

> **No other steps are needed.** You do NOT need to edit `CMakeLists.txt`, create XPM data, or register anything. Just drop the file, define the macro, and use it.

## 3. API Reference

### `IMAGE_MANAGER` macro
```cpp
#define IMAGE_MANAGER ImageManager::GetInstance()
```
Convenience accessor for the singleton. Use this everywhere.

---

### `GetBitmap(assetPath, size, tint)`
Returns a `wxBitmap` at the requested size. Best for toolbar icons and buttons.

```cpp
wxBitmap bmp = IMAGE_MANAGER.GetBitmap(
    ICON_SAVE,                    // asset path macro
    wxSize(16, 16),               // desired size (wxDefaultSize = native)
    wxColour(200, 200, 200)       // optional tint (wxNullColour = no tint)
);
```

| Parameter | Type | Default | Notes |
|:----------|:-----|:--------|:------|
| `assetPath` | `const std::string&` | — | Use a macro from `image_manager.h` |
| `size` | `const wxSize&` | `wxDefaultSize` | Requested pixel size |
| `tint` | `const wxColour&` | `wxNullColour` | Multiplies RGB channels (good for white/gray icons) |

**Returns**: `wxNullBitmap` if the file could not be loaded.

---

### `GetBitmapBundle(assetPath, tint)`
Returns a `wxBitmapBundle` for HiDPI-aware controls. SVGs are rasterized on demand.

```cpp
wxBitmapBundle bundle = IMAGE_MANAGER.GetBitmapBundle(ICON_NEW);
toolbar->AddTool(wxID_NEW, "", bundle.GetBitmap(iconSize));
```

---

### `GetNanoVGImage(vg, assetPath)`
Returns a NanoVG image handle for custom OpenGL/NanoVG rendering.

```cpp
int img = IMAGE_MANAGER.GetNanoVGImage(vg, ICON_LOCATION_ARROW);
if (img > 0) {
    NVGpaint paint = nvgImagePattern(vg, x, y, w, h, 0, img, 1.0f);
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillPaint(vg, paint);
    nvgFill(vg);
}
```

- **SVGs** are rasterized to 128×128 via wxWidgets, then uploaded as RGBA textures.
- **PNGs** are loaded directly by NanoVG (`nvgCreateImage`).
- All images are cached by asset path — subsequent calls return the same handle.

---

### `ClearCache()`
Releases all cached bitmaps, bundles, NanoVG images, and GL textures. Call on major state changes (e.g., theme switch).

```cpp
IMAGE_MANAGER.ClearCache();
```

## 4. Macro Naming Convention

| Prefix | Format | Example | Used for |
|:-------|:-------|:--------|:---------|
| `ICON_` | SVG icons | `ICON_SAVE`, `ICON_UNDO` | Toolbar buttons, menu items |
| `IMAGE_` | PNG rasters | `IMAGE_CIRCULAR_1`, `IMAGE_ERASER` | Brush shapes, zone overlays |

### Small Variants
For items that need both a large (32×32) and small (16×16) variant, define both:
```cpp
#define IMAGE_DOOR_NORMAL       "png/door_normal.png"       // 32×32
#define IMAGE_DOOR_NORMAL_SMALL "png/door_normal_small.png" // 16×16
```

## 5. Real-World Usage Examples

### Toolbar (wxAuiToolBar)
```cpp
// source/ui/toolbar/standard_toolbar.cpp
#include "util/image_manager.h"

wxSize icon_size = FROM_DIP(parent, wxSize(16, 16));
wxBitmap new_bitmap = IMAGE_MANAGER.GetBitmap(ICON_NEW, icon_size);
wxBitmap save_bitmap = IMAGE_MANAGER.GetBitmap(ICON_SAVE, icon_size);

toolbar->AddTool(wxID_NEW, "", new_bitmap, wxNullBitmap, wxITEM_NORMAL,
                 "New Map (Ctrl+N)", "Create a new empty map", nullptr);
```

### Brush Size Selector
```cpp
// source/ui/toolbar/size_toolbar.cpp
wxBitmap circ1 = IMAGE_MANAGER.GetBitmap(IMAGE_CIRCULAR_1_SMALL, icon_size);
toolbar->AddTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, "",
    IMAGE_MANAGER.GetBitmap(IMAGE_OPTIONAL_BORDER_SMALL, icon_size));
```

### NanoVG Custom Canvas (In-Game Preview)
```cpp
// source/ingame_preview/ingame_preview_window.cpp
int sunIcon = IMAGE_MANAGER.GetNanoVGImage(vg, ICON_SUNNY);
NVGpaint iconPaint = nvgImagePattern(vg, x, y, 24, 24, 0, sunIcon, 1.0f);
nvgBeginPath(vg);
nvgRoundedRect(vg, x, y, 24, 24, 4.0f);
nvgFillPaint(vg, iconPaint);
nvgFill(vg);
```

### Tinted Icons
```cpp
// White icon tinted to accent blue
wxBitmap tinted = IMAGE_MANAGER.GetBitmap(
    ICON_LOCATION, wxSize(16, 16), wxColour(0, 120, 215));
```

## 6. Troubleshooting

| Symptom | Likely Cause | Fix |
|:--------|:-------------|:----|
| Blank toolbar buttons | File not found at runtime | Check console for `ImageManager: Asset file not found:` errors |
| Wrong path (missing subdirectory) | Using `wxFileName(dir, file)` constructor | Use string concatenation: `assetsRoot + separator + assetPath` |
| SVG renders as black square | SVG uses unsupported features | Simplify SVG or convert to PNG |
| NanoVG image returns 0 | GL context not active | Ensure `GetNanoVGImage` is called within a paint handler or `ScopedGLContext` |
| Image not updated after change | Cached from previous load | Call `IMAGE_MANAGER.ClearCache()` or restart the editor |

### Debug Logging
Set spdlog to debug level to see all path resolution:
```cpp
spdlog::set_level(spdlog::level::debug);
// Output: "ImageManager: Resolving png/eraser.png -> C:\...\assets\png\eraser.png"
```

## 7. Key Files

| File | Purpose |
|:-----|:--------|
| [image_manager.h](source/util/image_manager.h) | Class declaration, all `ICON_*` and `IMAGE_*` macros |
| [image_manager.cpp](source/util/image_manager.cpp) | Path resolution, loading, caching, tinting |
| [source/assets/](source/assets/) | All asset source files (auto-copied to build) |
| [standard_toolbar.cpp](source/ui/toolbar/standard_toolbar.cpp) | Reference: wxWidgets toolbar usage |
| [size_toolbar.cpp](source/ui/toolbar/size_toolbar.cpp) | Reference: brush PNG usage |
| [ingame_preview_window.cpp](source/ingame_preview/ingame_preview_window.cpp) | Reference: NanoVG usage |

## 8. Checklist — Adding a New Asset

- [ ] File placed in `source/assets/png/` or `source/assets/svg/solid/` (or `regular/`)
- [ ] Macro defined in `source/util/image_manager.h` with correct relative path
- [ ] Macro follows naming convention (`ICON_` for SVG, `IMAGE_` for PNG)
- [ ] Small variant defined if needed (`_SMALL` suffix)
- [ ] Code uses `IMAGE_MANAGER.GetBitmap()` or `GetNanoVGImage()` with the macro
- [ ] Build succeeds and asset appears in `build/Release/assets/`
