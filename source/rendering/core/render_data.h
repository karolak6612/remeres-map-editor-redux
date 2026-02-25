#ifndef RME_RENDERING_CORE_RENDER_DATA_H_
#define RME_RENDERING_CORE_RENDER_DATA_H_

#include "app/definitions.h"
#include "map/position.h"
#include "map/tile.h"
#include "rendering/core/drawing_options.h"

// Forward declarations
class Item;
class Tile;
struct SpritePatterns;

struct RenderItem {
	const Tile* tile = nullptr;
	Position pos;
	Item* item = nullptr;
	const DrawingOptions* options = nullptr;
	const SpritePatterns* patterns = nullptr;

	// Default color/alpha
	int red = 255;
	int green = 255;
	int blue = 255;
	int alpha = 255;

	// Is this a temporary item?
	bool ephemeral = false;

	RenderItem() = default;

	RenderItem(const Tile* t, Item* i, const DrawingOptions& o) :
		tile(t), item(i), options(&o) {
		if (t) pos = t->getPosition();
	}

	RenderItem(const Position& p, Item* i, const DrawingOptions& o) :
		pos(p), item(i), options(&o) {
	}
};

#endif
