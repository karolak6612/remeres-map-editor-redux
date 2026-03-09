//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_DRAG_SHADOW_DRAWER_H_
#define RME_RENDERING_DRAG_SHADOW_DRAWER_H_

struct DrawContext;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
struct Position;

class DragShadowDrawer {
public:
	DragShadowDrawer();
	~DragShadowDrawer();

	void draw(const DrawContext& ctx, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, const Position& drag_start);
};

#endif
