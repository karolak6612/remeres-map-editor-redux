//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_BRUSH_OVERLAY_DRAWER_H_
#define RME_RENDERING_BRUSH_OVERLAY_DRAWER_H_

#include "brushes/brush_enums.h"
#include "rendering/core/brush_visual_settings.h"
#include "map/position.h"
#include <memory>

struct DrawContext;
class AtlasManager;
class Brush;
class IMapAccess;

class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class BrushCursorDrawer;
class DraggingOverlayDrawer;
class WallOverlayDrawer;
class DoorOverlayDrawer;
class CreatureOverlayDrawer;
class GenericOverlayDrawer;

// Bundles all parameters for BrushOverlayDrawer::draw(), reducing the
// 13-parameter call to a single struct passed from MapDrawer.
struct BrushOverlayContext {
    ItemDrawer* item_drawer = nullptr;
    SpriteDrawer* sprite_drawer = nullptr;
    CreatureDrawer* creature_drawer = nullptr;
    BrushCursorDrawer* brush_cursor_drawer = nullptr;
    IMapAccess* map_access = nullptr;
    const BrushVisualSettings* visual = nullptr;
    Brush* current_brush = nullptr;
    BrushShape brush_shape = BRUSHSHAPE_SQUARE;
    int brush_size = 0;
    bool is_drawing_mode = false;
    bool is_dragging_draw = false;
    int last_click_map_x = 0;
    int last_click_map_y = 0;
};

class BrushOverlayDrawer {
public:
    BrushOverlayDrawer();
    ~BrushOverlayDrawer();

    void draw(const DrawContext& ctx, const BrushOverlayContext& overlay);

private:
    std::unique_ptr<DraggingOverlayDrawer> dragging_;
    std::unique_ptr<WallOverlayDrawer> wall_;
    std::unique_ptr<DoorOverlayDrawer> door_;
    std::unique_ptr<CreatureOverlayDrawer> creature_;
    std::unique_ptr<GenericOverlayDrawer> generic_;
};

#endif
