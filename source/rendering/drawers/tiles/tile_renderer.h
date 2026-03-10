#ifndef RME_RENDERING_TILE_RENDERER_H_
#define RME_RENDERING_TILE_RENDERER_H_

#include <memory>
#include <sstream>
#include <stdint.h>

class TileLocation;
class Tile;
class Item;
struct DrawContext;
class Editor;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class CreatureNameDrawer;
class MarkerDrawer;
class TooltipCollector;
class SpriteBatch;
struct SpritePatterns;
class ItemDefinitionView;

struct TileRenderDeps {
	ItemDrawer* item_drawer = nullptr;
	SpriteDrawer* sprite_drawer = nullptr;
	CreatureDrawer* creature_drawer = nullptr;
	CreatureNameDrawer* creature_name_drawer = nullptr;
	MarkerDrawer* marker_drawer = nullptr;
	TooltipCollector* tooltip_collector = nullptr;
	Editor* editor = nullptr;
};

class TileRenderer {
public:
	explicit TileRenderer(const TileRenderDeps& deps);

	void DrawTile(const DrawContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x = -1, int in_draw_y = -1, bool draw_lights = false);

private:
	void PreloadItem(const Tile* tile, Item* item, const ItemDefinitionView& definition, const SpritePatterns* patterns = nullptr);

	ItemDrawer* item_drawer;
	SpriteDrawer* sprite_drawer;
	CreatureDrawer* creature_drawer;
	MarkerDrawer* marker_drawer;
	TooltipCollector* tooltip_collector;
	CreatureNameDrawer* creature_name_drawer;
	Editor* editor;
};

#endif
