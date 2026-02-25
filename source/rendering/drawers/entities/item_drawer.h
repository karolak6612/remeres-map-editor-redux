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

#include "game/sprites.h" // Needed for SpriteLight and Outfit

#include "rendering/utilities/pattern_calculator.h"

struct DrawingOptions;
class SpriteBatch;
class RenderList;

struct BlitItemParams {
	Position pos;
	const DrawingOptions* options = nullptr;
	SpritePatterns patterns;
	bool has_patterns = false;
	bool ephemeral = false;
	int red = 255;
	int green = 255;
	int blue = 255;
	int alpha = 255;

	// Extracted Data from Item
	uint16_t item_id = 0;
	bool is_selected = false;
	bool is_locked = false;

	// For Podium
	bool is_podium = false;
	bool podium_show_platform = true;
	bool podium_show_outfit = false;
	bool podium_show_mount = false;
	int podium_direction = 0;
	Outfit podium_outfit;

	// For Light
	SpriteLight light;

	BlitItemParams(const Tile* t, Item* i, const DrawingOptions& o);
	BlitItemParams(const Position& p, Item* i, const DrawingOptions& o);
};

class ItemDrawer {
public:
	ItemDrawer();
	~ItemDrawer();

	void BlitItem(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, int& draw_x, int& draw_y, const BlitItemParams& params);
	void DrawRawBrush(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, ItemType* itemType, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);

	void BlitItem(RenderList& list, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, int& draw_x, int& draw_y, const BlitItemParams& params);
	void DrawRawBrush(RenderList& list, SpriteDrawer* sprite_drawer, int screenx, int screeny, ItemType* itemType, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
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
