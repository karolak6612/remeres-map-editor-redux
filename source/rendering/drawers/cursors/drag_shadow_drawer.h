//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_DRAG_SHADOW_DRAWER_H_
#define RME_RENDERING_DRAG_SHADOW_DRAWER_H_

#include "map/position.h"

struct DrawContext;
class IMapAccess;

class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;

class DragShadowDrawer {
public:
	DragShadowDrawer();
	~DragShadowDrawer();

	void draw(
		const DrawContext& ctx, IMapAccess& map_access, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer,
		const Position& drag_start
	);
};

#endif
