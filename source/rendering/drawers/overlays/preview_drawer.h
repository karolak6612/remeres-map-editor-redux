#ifndef RME_PREVIEW_DRAWER_H_
#define RME_PREVIEW_DRAWER_H_

#include "rendering/core/render_view.h"
#include <cstdint>

struct RenderSettings;
struct ViewSnapshot;
class Editor;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class SpriteBatch;

class PrimitiveRenderer;

class PreviewDrawer {
public:
	PreviewDrawer();
	~PreviewDrawer();

	void draw(SpriteBatch& sprite_batch, const ViewSnapshot& snapshot, const ViewState& view, const FloorViewParams& floor_params, int map_z, const RenderSettings& settings, Editor& editor, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, uint32_t current_house_id);
};

#endif
