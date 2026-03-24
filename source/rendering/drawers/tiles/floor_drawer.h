//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_FLOOR_DRAWER_H_
#define RME_RENDERING_FLOOR_DRAWER_H_

class MapDrawer;
struct ViewState;
struct RenderSettings;
struct FrameOptions;
class Editor;

class ItemDrawer;
class SpriteDrawer;

class CreatureDrawer;
class SpriteBatch;
class PrimitiveRenderer;

class FloorDrawer {
public:
	FloorDrawer();
	~FloorDrawer();

	void draw(SpriteBatch& sprite_batch, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, const ViewState& view, const RenderSettings& settings, const FrameOptions& frame, Editor& editor);
};

#endif
