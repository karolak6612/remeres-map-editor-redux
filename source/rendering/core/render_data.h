#ifndef RME_RENDERING_CORE_RENDER_DATA_H_
#define RME_RENDERING_CORE_RENDER_DATA_H_

#include "app/definitions.h"
#include "map/position.h"
#include "game/sprites.h" // For SpriteLight, Outfit

struct DrawingOptions;
class Tile;
class Item;

struct RenderItem {
	const Tile* tile = nullptr;
	const Item* item = nullptr;
	const DrawingOptions* options = nullptr;

	Position pos;
	int screen_x = 0;
	int screen_y = 0;

	SpritePatterns patterns;
	bool has_patterns = false;

	uint8_t r = 255;
	uint8_t g = 255;
	uint8_t b = 255;
	uint8_t a = 255;

	// Constructor helper
	RenderItem(const Tile* t, const Item* i, const DrawingOptions& o, int x, int y) :
		tile(t), item(i), options(&o), pos(t->getPosition()), screen_x(x), screen_y(y) {}
};

#endif
