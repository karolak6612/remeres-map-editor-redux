#ifndef RME_RENDERING_TILE_RENDERER_H_
#define RME_RENDERING_TILE_RENDERER_H_

#include <memory>
#include <sstream>
#include <stdint.h>

#include "rendering/core/draw_context.h"

class TileLocation;
class Editor;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class CreatureNameDrawer;
class FloorDrawer;
class MarkerDrawer;
class TooltipDrawer;
struct LightBuffer;
class SpriteBatch;
class PrimitiveRenderer;
class ItemType;
struct SpritePatterns;

class TileRenderer {
public:
	TileRenderer(ItemDrawer* id, SpriteDrawer* sd, CreatureDrawer* cd, CreatureNameDrawer* cnd, FloorDrawer* fd, MarkerDrawer* md, TooltipDrawer* td, Editor* ed);

	void DrawTile(const DrawContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x = -1, int in_draw_y = -1, bool draw_lights = false);

private:
	void PreloadItem(const Tile* tile, Item* item, const ItemType& it, const SpritePatterns* patterns = nullptr);

	ItemDrawer* item_drawer;
	SpriteDrawer* sprite_drawer;
	CreatureDrawer* creature_drawer;
	FloorDrawer* floor_drawer;
	MarkerDrawer* marker_drawer;
	TooltipDrawer* tooltip_drawer;
	CreatureNameDrawer* creature_name_drawer;
	Editor* editor;
};

#endif
