//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_BRUSH_OVERLAY_DRAWER_H_
#define RME_RENDERING_BRUSH_OVERLAY_DRAWER_H_

#include "map/position.h"
#include <wx/colour.h>
#include <glm/glm.hpp>

struct DrawContext;
class Brush;
class Editor;

class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class BrushCursorDrawer;
class PrimitiveRenderer;

class BrushOverlayDrawer {
public:
	BrushOverlayDrawer();
	~BrushOverlayDrawer();

	void draw(const DrawContext& ctx, PrimitiveRenderer& primitive_renderer, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, BrushCursorDrawer* brush_cursor_drawer, bool is_dragging_draw, int last_click_map_x, int last_click_map_y);

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

	void get_color(Brush* brush, Editor& editor, const Position& position, uint8_t& r, uint8_t& g, uint8_t& b);

	glm::vec4 get_brush_color(BrushColor color);
	glm::vec4 get_check_color(Brush* brush, Editor& editor, const Position& pos);
};

#endif
