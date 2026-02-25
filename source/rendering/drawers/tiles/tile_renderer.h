#ifndef RME_RENDERING_TILE_RENDERER_H_
#define RME_RENDERING_TILE_RENDERER_H_

#include <memory>
#include <sstream>
#include <stdint.h>

class TileLocation;
struct RenderView;
struct DrawingOptions;
struct MarkerFlags;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class CreatureNameDrawer;
class FloorDrawer;
class MarkerDrawer;
class TooltipDrawer;
struct LightBuffer;
class SpriteBatch;
class RenderList;
class PrimitiveRenderer;
class ItemType;
struct SpritePatterns;

class TileRenderer {
public:
	TileRenderer(ItemDrawer* id, SpriteDrawer* sd, CreatureDrawer* cd, CreatureNameDrawer* cnd, FloorDrawer* fd, MarkerDrawer* md);

	void DrawTile(SpriteBatch& sprite_batch, TileLocation* location, const RenderView& view, const DrawingOptions& options, uint32_t current_house_id, const MarkerFlags& marker_flags, int in_draw_x = -1, int in_draw_y = -1);
	void DrawTile(RenderList& list, TileLocation* location, const RenderView& view, const DrawingOptions& options, uint32_t current_house_id, const MarkerFlags& marker_flags, int in_draw_x = -1, int in_draw_y = -1);
	void AddLight(TileLocation* location, const RenderView& view, const DrawingOptions& options, LightBuffer& light_buffer);

private:
	void PreloadItem(const Tile* tile, Item* item, const ItemType& it, const SpritePatterns* patterns = nullptr);

	ItemDrawer* item_drawer;
	SpriteDrawer* sprite_drawer;
	CreatureDrawer* creature_drawer;
	FloorDrawer* floor_drawer;
	MarkerDrawer* marker_drawer;
	CreatureNameDrawer* creature_name_drawer;
};

#endif
