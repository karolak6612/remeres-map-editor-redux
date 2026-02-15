#ifndef RME_RENDERING_UTILITIES_PATTERN_CALCULATOR_H_
#define RME_RENDERING_UTILITIES_PATTERN_CALCULATOR_H_

#include "rendering/core/game_sprite.h"
#include "game/items.h"
#include "game/item.h"
#include "map/tile.h"

struct SpritePatterns {
	int x = 0;
	int y = 0;
	int z = 0;
	int frame = 0;
	int subtype = -1;
};

class PatternCalculator {
public:
	static SpritePatterns Calculate(const GameSprite* spr, const ItemType& it, const Item* item, const Tile* tile, const Position& pos) {
		SpritePatterns patterns;

		if (!spr) {
			return patterns;
		}

		patterns.x = (spr->pattern_x > 1) ? pos.x % spr->pattern_x : 0;
		patterns.y = (spr->pattern_y > 1) ? pos.y % spr->pattern_y : 0;
		patterns.z = (spr->pattern_z > 1) ? pos.z % spr->pattern_z : 0;
		patterns.frame = (spr->animator) ? spr->animator->getFrame() : 0;

		if (it.isSplash() || it.isFluidContainer()) {
			patterns.subtype = item->getSubtype();
		} else if (it.isHangable) {
			if (tile && tile->hasHookSouth()) {
				patterns.x = 1;
			} else if (tile && tile->hasHookEast()) {
				patterns.x = 2;
			} else {
				patterns.x = 0;
			}
		} else if (it.stackable) {
			uint16_t itemSubtype = item->getSubtype();
			if (itemSubtype <= 1) {
				patterns.subtype = 0;
			} else if (itemSubtype <= 2) {
				patterns.subtype = 1;
			} else if (itemSubtype <= 3) {
				patterns.subtype = 2;
			} else if (itemSubtype <= 4) {
				patterns.subtype = 3;
			} else if (itemSubtype < 10) {
				patterns.subtype = 4;
			} else if (itemSubtype < 25) {
				patterns.subtype = 5;
			} else if (itemSubtype < 50) {
				patterns.subtype = 6;
			} else {
				patterns.subtype = 7;
			}
		}

		return patterns;
	}
};

#endif
