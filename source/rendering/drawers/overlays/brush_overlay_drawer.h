//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_BRUSH_OVERLAY_DRAWER_H_
#define RME_RENDERING_BRUSH_OVERLAY_DRAWER_H_

#include "brushes/brush_enums.h"
#include "rendering/core/brush_visual_settings.h"
#include "map/position.h"
#include <glm/glm.hpp>

struct DrawContext;
class AtlasManager;
class Brush;
class Editor;

class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class BrushCursorDrawer;

// Bundles all parameters for BrushOverlayDrawer::draw(), reducing the
// 13-parameter call to a single struct passed from MapDrawer.
struct BrushOverlayContext {
    ItemDrawer* item_drawer = nullptr;
    SpriteDrawer* sprite_drawer = nullptr;
    CreatureDrawer* creature_drawer = nullptr;
    BrushCursorDrawer* brush_cursor_drawer = nullptr;
    Editor* editor = nullptr;
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
    enum BrushColor {
        COLOR_BRUSH,
        COLOR_HOUSE_BRUSH,
        COLOR_FLAG_BRUSH,
        COLOR_SPAWN_BRUSH,
        COLOR_ERASER,
        COLOR_VALID,
        COLOR_INVALID,
        COLOR_BLANK,
    };

    // Sub-methods for draw()
    void drawDragging(const DrawContext& ctx, const BrushOverlayContext& overlay, const glm::vec4& brushColor);
    void drawStationary(const DrawContext& ctx, const BrushOverlayContext& overlay, const glm::vec4& brushColor);
    void drawStationaryWall(const DrawContext& ctx, const BrushOverlayContext& overlay, const glm::vec4& brushColor);
    void drawStationaryDoor(const DrawContext& ctx, const BrushOverlayContext& overlay);
    void drawStationaryCreature(const DrawContext& ctx, const BrushOverlayContext& overlay);
    void drawStationaryGeneric(const DrawContext& ctx, const BrushOverlayContext& overlay, const glm::vec4& brushColor);

    void get_color(Brush* brush, Editor& editor, const Position& position, uint8_t& r, uint8_t& g, uint8_t& b);

    glm::vec4 get_brush_color(BrushColor color, const BrushVisualSettings& visual);
    glm::vec4 get_check_color(Brush* brush, Editor& editor, const Position& pos, const BrushVisualSettings& visual);
};

#endif
