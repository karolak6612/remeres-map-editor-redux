//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include <algorithm>
#undef min
#undef max

#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/render_list.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/core/drawing_options.h"
#include "rendering/utilities/pattern_calculator.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/items.h"
#include "game/complexitem.h"
#include "game/sprites.h"
#include "ui/gui.h"

static void ExtractItemData(BlitItemParams& params, const Tile* tile, Item* item) {
	params.item_id = item->getID();
	params.is_selected = item->isSelected();
	params.is_locked = item->isLocked();
	params.light = item->getLight();

	if (item->asPodium()) {
		Podium* podium = static_cast<Podium*>(item);
		params.is_podium = true;
		params.podium_show_platform = podium->hasShowPlatform();
		params.podium_show_outfit = podium->hasShowOutfit();
		params.podium_show_mount = podium->hasShowMount();
		params.podium_direction = podium->getDirection();
		params.podium_outfit = podium->getOutfit();
	}

	const ItemType& it = g_items[params.item_id];
	if (it.sprite) {
		params.patterns = PatternCalculator::Calculate(it.sprite, it, item, tile, params.pos);
		params.has_patterns = true;
	}
}

BlitItemParams::BlitItemParams(const Tile* t, Item* i, const DrawingOptions& o) : options(&o) {
	if (t) {
		pos = t->getPosition();
	}
	ExtractItemData(*this, t, i);
}

BlitItemParams::BlitItemParams(const Position& p, Item* i, const DrawingOptions& o) : pos(p), options(&o) {
	ExtractItemData(*this, nullptr, i);
}

ItemDrawer::ItemDrawer() {
}

ItemDrawer::~ItemDrawer() {
}

void ItemDrawer::BlitItem(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, int& draw_x, int& draw_y, const BlitItemParams& params) {
	const Position& pos = params.pos;
	const DrawingOptions& options = *params.options;
	bool ephemeral = params.ephemeral;
	int red = params.red;
	int green = params.green;
	int blue = params.blue;
	int alpha = params.alpha;

	ItemType& it = g_items[params.item_id];

	// Locked door indicator
	if (!options.ingame && options.highlight_locked_doors && it.isDoor()) {
		// Door orientation: horizontal wall -> West border (south=true), vertical wall -> North border (east=true)
		if (it.border_alignment == WALL_HORIZONTAL) {
			DrawDoorIndicator(params.is_locked, pos, true, false);
		} else if (it.border_alignment == WALL_VERTICAL) {
			DrawDoorIndicator(params.is_locked, pos, false, true);
		} else {
			// Center case for non-aligned doors
			DrawDoorIndicator(params.is_locked, pos, false, false);
		}
	}

	if (!options.ingame && !ephemeral && params.is_selected) {
		red /= 2;
		blue /= 2;
		green /= 2;
	}

	// item sprite
	GameSprite* spr = it.sprite;

	// Display invisible and invalid items
	// Ugly hacks. :)
	if (!options.ingame && options.show_tech_items) {
		// Red invalid client id
		if (it.id == 0) {
			sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(red, 0, 0, alpha));
			return;
		}

		switch (it.clientID) {
			// Yellow invisible stairs tile (459)
			case 469:
				sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(red, green, 0, (alpha * 171) >> 8));
				return;

			// Red invisible walkable tile (460)
			case 470:
			case 17970:
			case 20028:
			case 34168:
				sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(red, 0, 0, (alpha * 171) >> 8));
				return;

			// Cyan invisible wall (1548)
			case 2187:
				sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(0, green, blue, 80));
				return;

			default:
				break;
		}

		// primal light
		if (it.clientID >= 39092 && it.clientID <= 39100 || it.clientID == 39236 || it.clientID == 39367 || it.clientID == 39368) {
			spr = g_items[SPRITE_LIGHTSOURCE].sprite;
			red = 0;
			alpha = 180;
		}
	}

	// metaItem, sprite not found or not hidden
	if (it.isMetaItem() || spr == nullptr || !ephemeral && it.pickupable && !options.show_items) {
		return;
	}

	// int screenx = draw_x - spr->getDrawOffset().first;
	// int screeny = draw_y - spr->getDrawOffset().second;
	// The original code modified draw_x/draw_y AFTER calculating screenx/screeny using the original draw_x/draw_y
	// screenx use input draw_x
	int screenx = draw_x - spr->drawoffset_x;
	int screeny = draw_y - spr->drawoffset_y;

	// Set the newd drawing height accordingly
	draw_x -= spr->draw_height;
	draw_y -= spr->draw_height;

	SpritePatterns patterns = params.patterns;

	int subtype = patterns.subtype;
	int pattern_x = patterns.x;
	int pattern_y = patterns.y;
	int pattern_z = patterns.z;
	int frame = patterns.frame;

	if (!ephemeral && options.transparent_items && (!it.isGroundTile() || spr->width > 1 || spr->height > 1) && !it.isSplash() && (!it.isBorder || spr->width > 1 || spr->height > 1)) {
		alpha /= 2;
	}

	if (params.is_podium) {
		if (!params.podium_show_platform && !options.ingame) {
			if (options.show_tech_items) {
				alpha /= 2;
			} else {
				alpha = 0;
			}
		}
	}

	// Atlas-only rendering
	// g_gui.gfx.ensureAtlasManager();
	// BatchRenderer::SetAtlasManager(g_gui.gfx.getAtlasManager());

	if (spr->width == 1 && spr->height == 1 && spr->layers == 1) {
		const AtlasRegion* region;
		if (subtype == -1 && pattern_x == 0 && pattern_y == 0 && pattern_z == 0 && frame == 0) {
			region = spr->getAtlasRegion(0, 0, 0, -1, 0, 0, 0, 0);
		} else {
			region = spr->getAtlasRegion(0, 0, 0, subtype, pattern_x, pattern_y, pattern_z, frame);
		}

		if (region) {
#ifdef DEBUG
			// DEBUG: Check for mismatch on Item 369 using PRECISE sub-sprite ID
			if (params.item_id == 369) {
				// Use 0,0 as pattern coordinates for 1x1 items
				uint32_t precise_expected_id = spr->getSpriteId(frame, 0, 0);
				if (region->debug_sprite_id != 0 && precise_expected_id != 0 && region->debug_sprite_id != precise_expected_id) {
					spdlog::error("SPRITE MISMATCH DETECTED: Item 369 (Expected Sprite ID {}, Actual Region Owner {})", precise_expected_id, region->debug_sprite_id);
				}
			}
#endif
			sprite_drawer->glBlitAtlasQuad(sprite_batch, screenx, screeny, region, DrawColor(red, green, blue, alpha));
		}
	} else {
		for (int cx = 0; cx != spr->width; cx++) {
			for (int cy = 0; cy != spr->height; cy++) {
				for (int cf = 0; cf != spr->layers; cf++) {
					const AtlasRegion* region = spr->getAtlasRegion(cx, cy, cf, subtype, pattern_x, pattern_y, pattern_z, frame);
					if (region) {
						sprite_drawer->glBlitAtlasQuad(sprite_batch, screenx - cx * TILE_SIZE, screeny - cy * TILE_SIZE, region, DrawColor(red, green, blue, alpha));
					}
				}
			}
		}
	}

	if (params.is_podium) {
		Outfit outfit = params.podium_outfit;
		if (!params.podium_show_outfit) {
			if (params.podium_show_mount) {
				outfit.lookType = outfit.lookMount;
				outfit.lookHead = outfit.lookMountHead;
				outfit.lookBody = outfit.lookMountBody;
				outfit.lookLegs = outfit.lookMountLegs;
				outfit.lookFeet = outfit.lookMountFeet;
				outfit.lookAddon = 0;
				outfit.lookMount = 0;
			} else {
				outfit.lookType = 0;
			}
		}
		if (!params.podium_show_mount) {
			outfit.lookMount = 0;
		}

		creature_drawer->BlitCreature(sprite_batch, sprite_drawer, draw_x, draw_y, outfit, static_cast<Direction>(params.podium_direction), CreatureDrawOptions { .color = DrawColor(red, green, blue, alpha) });
	}

	// draw wall hook
	if (!options.ingame && options.show_hooks && (it.hookSouth || it.hookEast)) {
		DrawHookIndicator(it, pos);
	}

	// draw light color indicator
	if (!options.ingame && options.show_light_str) {
		const SpriteLight& light = params.light;
		if (light.intensity > 0) {
			wxColor lightColor = colorFromEightBit(light.color);
			uint8_t byteR = lightColor.Red();
			uint8_t byteG = lightColor.Green();
			uint8_t byteB = lightColor.Blue();
			uint8_t byteA = 255;

			int startOffset = std::max<int>(16, 32 - light.intensity);
			int sqSize = TILE_SIZE - startOffset;

			// We need to disable texture 2d for BlitSquare. SpriteDrawer::glBlitSquare does NOT disable texture 2d automatically?
			// SpriteDrawer::glBlitSquare internally uses BatchRenderer::DrawQuad which sets blank texture if needed.
			// So we don't need manual enable/disable here anymore.

			sprite_drawer->glBlitSquare(sprite_batch, draw_x + startOffset - 2, draw_y + startOffset - 2, DrawColor(0, 0, 0, byteA), sqSize + 2);
			sprite_drawer->glBlitSquare(sprite_batch, draw_x + startOffset - 1, draw_y + startOffset - 1, DrawColor(byteR, byteG, byteB, byteA), sqSize);
		}
	}
}

void ItemDrawer::DrawRawBrush(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, ItemType* itemType, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
	GameSprite* spr = itemType->sprite;
	uint16_t cid = itemType->clientID;

	switch (cid) {
		// Yellow invisible stairs tile
		case 469:
			b = 0;
			alpha = (alpha * 171) >> 8;
			spr = g_items[SPRITE_ZONE].sprite;
			break;

		// Red invisible walkable tile
		case 470:
			g = 0;
			b = 0;
			alpha = (alpha * 171) >> 8;
			spr = g_items[SPRITE_ZONE].sprite;
			break;

		// Cyan invisible wall
		case 2187:
			r = 0;
			alpha = alpha / 3;
			spr = g_items[SPRITE_ZONE].sprite;
			break;

		default:
			break;
	}

	// primal light
	if (cid >= 39092 && cid <= 39100 || cid == 39236 || cid == 39367 || cid == 39368) {
		spr = g_items[SPRITE_LIGHTSOURCE].sprite;
		r = 0;
		alpha = (alpha * 171) >> 8;
	}

	sprite_drawer->BlitSprite(sprite_batch, screenx, screeny, spr, DrawColor(r, g, b, alpha));
}

void ItemDrawer::BlitItem(RenderList& list, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, int& draw_x, int& draw_y, const BlitItemParams& params) {
	const Position& pos = params.pos;
	const DrawingOptions& options = *params.options;
	bool ephemeral = params.ephemeral;
	int red = params.red;
	int green = params.green;
	int blue = params.blue;
	int alpha = params.alpha;

	ItemType& it = g_items[params.item_id];

	if (!options.ingame && options.highlight_locked_doors && it.isDoor()) {
		if (it.border_alignment == WALL_HORIZONTAL) {
			DrawDoorIndicator(params.is_locked, pos, true, false);
		} else if (it.border_alignment == WALL_VERTICAL) {
			DrawDoorIndicator(params.is_locked, pos, false, true);
		} else {
			DrawDoorIndicator(params.is_locked, pos, false, false);
		}
	}

	if (!options.ingame && !ephemeral && params.is_selected) {
		red /= 2;
		blue /= 2;
		green /= 2;
	}

	GameSprite* spr = it.sprite;

	if (!options.ingame && options.show_tech_items) {
		if (it.id == 0) {
			sprite_drawer->glBlitSquare(list, draw_x, draw_y, DrawColor(red, 0, 0, alpha));
			return;
		}

		switch (it.clientID) {
			case 469:
				sprite_drawer->glBlitSquare(list, draw_x, draw_y, DrawColor(red, green, 0, (alpha * 171) >> 8));
				return;
			case 470:
			case 17970:
			case 20028:
			case 34168:
				sprite_drawer->glBlitSquare(list, draw_x, draw_y, DrawColor(red, 0, 0, (alpha * 171) >> 8));
				return;
			case 2187:
				sprite_drawer->glBlitSquare(list, draw_x, draw_y, DrawColor(0, green, blue, 80));
				return;
			default:
				break;
		}

		if (it.clientID >= 39092 && it.clientID <= 39100 || it.clientID == 39236 || it.clientID == 39367 || it.clientID == 39368) {
			spr = g_items[SPRITE_LIGHTSOURCE].sprite;
			red = 0;
			alpha = 180;
		}
	}

	if (it.isMetaItem() || spr == nullptr || !ephemeral && it.pickupable && !options.show_items) {
		return;
	}

	int screenx = draw_x - spr->drawoffset_x;
	int screeny = draw_y - spr->drawoffset_y;

	draw_x -= spr->draw_height;
	draw_y -= spr->draw_height;

	SpritePatterns patterns = params.patterns;

	int subtype = patterns.subtype;
	int pattern_x = patterns.x;
	int pattern_y = patterns.y;
	int pattern_z = patterns.z;
	int frame = patterns.frame;

	if (!ephemeral && options.transparent_items && (!it.isGroundTile() || spr->width > 1 || spr->height > 1) && !it.isSplash() && (!it.isBorder || spr->width > 1 || spr->height > 1)) {
		alpha /= 2;
	}

	if (params.is_podium) {
		if (!params.podium_show_platform && !options.ingame) {
			if (options.show_tech_items) {
				alpha /= 2;
			} else {
				alpha = 0;
			}
		}
	}

	if (spr->width == 1 && spr->height == 1 && spr->layers == 1) {
		const AtlasRegion* region;
		if (subtype == -1 && pattern_x == 0 && pattern_y == 0 && pattern_z == 0 && frame == 0) {
			region = spr->getAtlasRegion(0, 0, 0, -1, 0, 0, 0, 0);
		} else {
			region = spr->getAtlasRegion(0, 0, 0, subtype, pattern_x, pattern_y, pattern_z, frame);
		}

		if (region) {
#ifdef DEBUG
			if (params.item_id == 369) {
				uint32_t precise_expected_id = spr->getSpriteId(frame, 0, 0);
				if (region->debug_sprite_id != 0 && precise_expected_id != 0 && region->debug_sprite_id != precise_expected_id) {
					spdlog::error("SPRITE MISMATCH DETECTED: Item 369 (Expected Sprite ID {}, Actual Region Owner {})", precise_expected_id, region->debug_sprite_id);
				}
			}
#endif
			sprite_drawer->glBlitAtlasQuad(list, screenx, screeny, region, DrawColor(red, green, blue, alpha));
		}
	} else {
		for (int cx = 0; cx != spr->width; cx++) {
			for (int cy = 0; cy != spr->height; cy++) {
				for (int cf = 0; cf != spr->layers; cf++) {
					const AtlasRegion* region = spr->getAtlasRegion(cx, cy, cf, subtype, pattern_x, pattern_y, pattern_z, frame);
					if (region) {
						sprite_drawer->glBlitAtlasQuad(list, screenx - cx * TILE_SIZE, screeny - cy * TILE_SIZE, region, DrawColor(red, green, blue, alpha));
					}
				}
			}
		}
	}

	if (params.is_podium) {
		Outfit outfit = params.podium_outfit;
		if (!params.podium_show_outfit) {
			if (params.podium_show_mount) {
				outfit.lookType = outfit.lookMount;
				outfit.lookHead = outfit.lookMountHead;
				outfit.lookBody = outfit.lookMountBody;
				outfit.lookLegs = outfit.lookMountLegs;
				outfit.lookFeet = outfit.lookMountFeet;
				outfit.lookAddon = 0;
				outfit.lookMount = 0;
			} else {
				outfit.lookType = 0;
			}
		}
		if (!params.podium_show_mount) {
			outfit.lookMount = 0;
		}

		creature_drawer->BlitCreature(list, sprite_drawer, draw_x, draw_y, outfit, static_cast<Direction>(params.podium_direction), CreatureDrawOptions { .color = DrawColor(red, green, blue, alpha) });
	}

	if (!options.ingame && options.show_hooks && (it.hookSouth || it.hookEast)) {
		DrawHookIndicator(it, pos);
	}

	if (!options.ingame && options.show_light_str) {
		const SpriteLight& light = params.light;
		if (light.intensity > 0) {
			wxColor lightColor = colorFromEightBit(light.color);
			uint8_t byteR = lightColor.Red();
			uint8_t byteG = lightColor.Green();
			uint8_t byteB = lightColor.Blue();
			uint8_t byteA = 255;

			int startOffset = std::max<int>(16, 32 - light.intensity);
			int sqSize = TILE_SIZE - startOffset;

			sprite_drawer->glBlitSquare(list, draw_x + startOffset - 2, draw_y + startOffset - 2, DrawColor(0, 0, 0, byteA), sqSize + 2);
			sprite_drawer->glBlitSquare(list, draw_x + startOffset - 1, draw_y + startOffset - 1, DrawColor(byteR, byteG, byteB, byteA), sqSize);
		}
	}
}

void ItemDrawer::DrawRawBrush(RenderList& list, SpriteDrawer* sprite_drawer, int screenx, int screeny, ItemType* itemType, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
	GameSprite* spr = itemType->sprite;
	uint16_t cid = itemType->clientID;

	switch (cid) {
		case 469:
			b = 0;
			alpha = (alpha * 171) >> 8;
			spr = g_items[SPRITE_ZONE].sprite;
			break;
		case 470:
			g = 0;
			b = 0;
			alpha = (alpha * 171) >> 8;
			spr = g_items[SPRITE_ZONE].sprite;
			break;
		case 2187:
			r = 0;
			alpha = alpha / 3;
			spr = g_items[SPRITE_ZONE].sprite;
			break;
		default:
			break;
	}

	if (cid >= 39092 && cid <= 39100 || cid == 39236 || cid == 39367 || cid == 39368) {
		spr = g_items[SPRITE_LIGHTSOURCE].sprite;
		r = 0;
		alpha = (alpha * 171) >> 8;
	}

	sprite_drawer->BlitSprite(list, screenx, screeny, spr, DrawColor(r, g, b, alpha));
}

void ItemDrawer::DrawHookIndicator(const ItemType& type, const Position& pos) {
	if (hook_indicator_drawer) {
		hook_indicator_drawer->addHook(pos, type.hookSouth, type.hookEast);
	}
}

void ItemDrawer::DrawDoorIndicator(bool locked, const Position& pos, bool south, bool east) {
	if (door_indicator_drawer) {
		door_indicator_drawer->addDoor(pos, locked, south, east);
	}
}
