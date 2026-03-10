//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include <algorithm>
#undef min
#undef max

#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/core/graphics.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/special_client_ids.h"
#include "rendering/core/sprite_resolver.h"
#include "rendering/utilities/pattern_calculator.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/complexitem.h"
#include "game/sprites.h"
#include "ui/gui.h"

GameSprite* ItemDrawer::resolveSprite(const ItemDefinitionView& definition) const {
	if (!definition || !sprite_resolver) {
		return nullptr;
	}
	return sprite_resolver->getSprite(definition.clientId());
}

GameSprite* ItemDrawer::resolveSprite(ServerItemId item_id) const {
	return resolveSprite(g_item_definitions.get(item_id));
}

BlitItemParams::BlitItemParams(const Tile* t, Item* i, const RenderSettings& s, const FrameOptions& f) : tile(t), item(i), settings(&s), frame(&f) {
	if (t) {
		pos = t->getPosition();
	}
}

BlitItemParams::BlitItemParams(const Position& p, Item* i, const RenderSettings& s, const FrameOptions& f) : pos(p), item(i), settings(&s), frame(&f) {
}

ItemDrawer::ItemDrawer() {
}

ItemDrawer::~ItemDrawer() {
}

void ItemDrawer::BlitItem(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, int& draw_x, int& draw_y, const BlitItemParams& params) {
	const Position& pos = params.pos;
	Item* item = params.item;
	const Tile* tile = params.tile;
	const RenderSettings& settings = *params.settings;
	const FrameOptions& frame = *params.frame;
	bool ephemeral = params.ephemeral;
	int red = params.red;
	int green = params.green;
	int blue = params.blue;
	int alpha = params.alpha;
	const SpritePatterns* cached_patterns = params.patterns;

	const ItemDefinitionView it = params.item_definition ? params.item_definition : item->getDefinition();

	bool is_transient_selected = !ephemeral && frame.transient_selection_bounds && frame.transient_selection_bounds->contains(pos.x, pos.y);
	if (!settings.ingame && (item->isSelected() || is_transient_selected)) {
		red /= 2;
		blue /= 2;
		green /= 2;
	}

	// item sprite
	GameSprite* spr = resolveSprite(it);

	// Display invisible and invalid items
	// Ugly hacks. :)
	if (!settings.ingame && settings.show_tech_items) {
		// Red invalid client id
		if (!it) {
			sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(red, 0, 0, alpha));
			return;
		}

		switch (it.clientId()) {
			case SpecialClientId::INVISIBLE_STAIRS:
				sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(red, green, 0, (alpha * 171) >> 8));
				return;

			case SpecialClientId::INVISIBLE_WALKABLE_470:
			case SpecialClientId::INVISIBLE_WALKABLE_17970:
			case SpecialClientId::INVISIBLE_WALKABLE_20028:
			case SpecialClientId::INVISIBLE_WALKABLE_34168:
				sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(red, 0, 0, (alpha * 171) >> 8));
				return;

			case SpecialClientId::INVISIBLE_WALL:
				sprite_drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(0, green, blue, 80));
				return;

			default:
				break;
		}

		if (SpecialClientId::isPrimalLight(it.clientId())) {
			spr = resolveSprite(SPRITE_LIGHTSOURCE);
			red = 0;
			alpha = 180;
		}
	}

	// metaItem, sprite not found or not hidden
	if (it.isMetaItem() || spr == nullptr || !ephemeral && it.hasFlag(ItemFlag::Pickupable) && !settings.show_items) {
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

	SpritePatterns patterns;
	if (cached_patterns && spr == resolveSprite(it)) {
		patterns = *cached_patterns;
	} else {
		patterns = PatternCalculator::Calculate(spr, it, item, tile, pos);
	}

	int subtype = patterns.subtype;
	int pattern_x = patterns.x;
	int pattern_y = patterns.y;
	int pattern_z = patterns.z;
	int frame = patterns.frame;

	if (!ephemeral && settings.transparent_items && (!it.isGroundTile() || spr->width > 1 || spr->height > 1) && !it.isSplash() && (!it.hasFlag(ItemFlag::IsBorder) || spr->width > 1 || spr->height > 1)) {
		alpha /= 2;
	}

	if (it.isPodium()) {
		Podium* podium = static_cast<Podium*>(item);
		if (!podium->hasShowPlatform() && !settings.ingame) {
			if (settings.show_tech_items) {
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
			if (item->getID() == 369) {
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

	if (it.isPodium()) {
		Podium* podium = static_cast<Podium*>(item);
		Outfit outfit = podium->getOutfit();
		if (!podium->hasShowOutfit()) {
			if (podium->hasShowMount()) {
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
		if (!podium->hasShowMount()) {
			outfit.lookMount = 0;
		}

		creature_drawer->BlitCreature(sprite_batch, sprite_drawer, draw_x, draw_y, outfit, static_cast<Direction>(podium->getDirection()), CreatureDrawOptions { .color = DrawColor(red, green, blue, alpha) });
	}

	// draw light color indicator
	if (!settings.ingame && settings.show_light_str) {
		const SpriteLight& light = item->getLight();
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

void ItemDrawer::DrawRawBrush(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, ServerItemId item_id, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
	const auto definition = g_item_definitions.get(item_id);
	GameSprite* spr = resolveSprite(definition);
	uint16_t cid = definition ? definition.clientId() : 0;

	switch (cid) {
		case SpecialClientId::INVISIBLE_STAIRS:
			b = 0;
			alpha = (alpha * 171) >> 8;
			spr = resolveSprite(SPRITE_ZONE);
			break;

		case SpecialClientId::INVISIBLE_WALKABLE_470:
			g = 0;
			b = 0;
			alpha = (alpha * 171) >> 8;
			spr = resolveSprite(SPRITE_ZONE);
			break;

		case SpecialClientId::INVISIBLE_WALL:
			r = 0;
			alpha = alpha / 3;
			spr = resolveSprite(SPRITE_ZONE);
			break;

		default:
			break;
	}

	if (SpecialClientId::isPrimalLight(cid)) {
		spr = resolveSprite(SPRITE_LIGHTSOURCE);
		r = 0;
		alpha = (alpha * 171) >> 8;
	}

	if (spr) {
		sprite_drawer->BlitSprite(sprite_batch, screenx, screeny, spr, DrawColor(r, g, b, alpha));
	}
}
