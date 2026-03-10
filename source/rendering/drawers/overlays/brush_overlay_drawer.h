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

class BrushOverlayDrawer {
public:
    BrushOverlayDrawer();
    ~BrushOverlayDrawer();

    void draw(
        const DrawContext& ctx, const BrushVisualSettings& visual, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer,
        CreatureDrawer* creature_drawer, BrushCursorDrawer* brush_cursor_drawer, Editor& editor, bool is_drawing_mode,
        Brush* current_brush, BrushShape brush_shape, int brush_size, bool is_dragging_draw, int last_click_map_x, int last_click_map_y
    );

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
    void drawDragging(
        const DrawContext& ctx, const BrushVisualSettings& visual, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer,
        Editor& editor, Brush* brush, const glm::vec4& brushColor, BrushShape brush_shape, int last_click_map_x, int last_click_map_y
    );
    void drawStationary(
        const DrawContext& ctx, const BrushVisualSettings& visual, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer,
        CreatureDrawer* creature_drawer, BrushCursorDrawer* brush_cursor_drawer, Editor& editor, Brush* brush,
        const glm::vec4& brushColor, BrushShape brush_shape, int brush_size
    );
    void drawStationaryWall(const DrawContext& ctx, Brush* brush, const glm::vec4& brushColor, int brush_size);
    void drawStationaryDoor(const DrawContext& ctx, Brush* brush, Editor& editor, const BrushVisualSettings& visual);
    void drawStationaryCreature(
        const DrawContext& ctx, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, Brush* brush, Editor& editor
    );
    void drawStationaryGeneric(
        const DrawContext& ctx, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, BrushCursorDrawer* brush_cursor_drawer,
        Editor& editor, Brush* brush, const glm::vec4& brushColor, BrushShape brush_shape, int brush_size,
        const BrushVisualSettings& visual
    );

    void get_color(Brush* brush, Editor& editor, const Position& position, uint8_t& r, uint8_t& g, uint8_t& b);

    glm::vec4 get_brush_color(BrushColor color, const BrushVisualSettings& visual);
    glm::vec4 get_check_color(Brush* brush, Editor& editor, const Position& pos, const BrushVisualSettings& visual);
};

#endif
