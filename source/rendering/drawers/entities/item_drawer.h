//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_ITEM_DRAWER_H_
#define RME_RENDERING_ITEM_DRAWER_H_

#include "app/definitions.h"
#include "map/position.h"

// Forward declarations
class SpriteDrawer;
class CreatureDrawer;
class Tile;
class Item;
class ItemType;
class HookIndicatorDrawer;
class DoorIndicatorDrawer;

#include "rendering/core/draw_context.h"
struct SpritePatterns;

struct BlitItemParams {
	const Tile* tile = nullptr;
	Position pos;
	Item* item = nullptr;
	const ItemType* item_type = nullptr;
	const DrawingOptions* options = nullptr;
	const SpritePatterns* patterns = nullptr;
	bool ephemeral = false;
	int red = 255;
	int green = 255;
	int blue = 255;
	int alpha = 255;

	BlitItemParams(const Tile* t, Item* i, const DrawingOptions& o);
	BlitItemParams(const Position& p, Item* i, const DrawingOptions& o);
};

class ItemDrawer {
public:
	ItemDrawer();
	~ItemDrawer();

	void BlitItem(const DrawContext& ctx, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, int& draw_x, int& draw_y, const BlitItemParams& params);

	void DrawRawBrush(const DrawContext& ctx, SpriteDrawer* sprite_drawer, int screenx, int screeny, ItemType* itemType, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
	void DrawHookIndicator(const ItemType& type, const Position& pos);
	void DrawDoorIndicator(bool locked, const Position& pos, bool south, bool east);

	void SetHookIndicatorDrawer(HookIndicatorDrawer* drawer) {
		hook_indicator_drawer = drawer;
	}
	void SetDoorIndicatorDrawer(DoorIndicatorDrawer* drawer) {
		door_indicator_drawer = drawer;
	}

private:
	HookIndicatorDrawer* hook_indicator_drawer = nullptr;
	DoorIndicatorDrawer* door_indicator_drawer = nullptr;
};

#endif
