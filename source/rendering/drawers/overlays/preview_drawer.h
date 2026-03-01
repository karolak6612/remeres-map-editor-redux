#ifndef RME_PREVIEW_DRAWER_H_
#define RME_PREVIEW_DRAWER_H_

#include "rendering/core/draw_context.h"
#include "rendering/core/floor_view_params.h"
#include <cstdint>

class MapCanvas;
class Editor;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;

class PreviewDrawer {
public:
	PreviewDrawer();
	~PreviewDrawer();

	void draw(const DrawContext& ctx, const FloorViewParams& floor_params, MapCanvas& canvas, int map_z, Editor& editor, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, uint32_t current_house_id);
};

#endif
