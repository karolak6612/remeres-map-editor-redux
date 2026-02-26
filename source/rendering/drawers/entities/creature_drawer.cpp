//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "game/creature.h"
#include "ui/gui.h"
#include "game/items.h"
#include "game/sprites.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/animator.h"
#include "rendering/core/render_list.h"
#include <spdlog/spdlog.h>
#include <algorithm>

CreatureDrawer::CreatureDrawer(SpriteDrawer* sd) : sprite_drawer(sd) {
}

CreatureDrawer::~CreatureDrawer() {
}

void CreatureDrawer::BlitCreature(SpriteBatch& sprite_batch, int screenx, int screeny, const Creature* c, const CreatureDrawOptions& options) {
	CreatureDrawOptions local_opts = options;
	if (!local_opts.ingame && c->isSelected()) {
		local_opts.color.r /= 2;
		local_opts.color.g /= 2;
		local_opts.color.b /= 2;
	}
	BlitCreature(sprite_batch, screenx, screeny, c->getOutfit(), c->getDirection(), local_opts);
}

void CreatureDrawer::BlitCreature(SpriteBatch& sprite_batch, int screenx, int screeny, const Outfit& outfit, Direction dir, const CreatureDrawOptions& options) {
	if (outfit.lookItem != 0) {
		ItemType& it = g_items[outfit.lookItem];
		sprite_drawer->BlitSprite(sprite_batch, screenx, screeny, it.sprite, options.color);
	} else {
		// get outfit sprite
		GameSprite* spr = g_gui.gfx.getCreatureSprite(outfit.lookType);
		if (!spr || outfit.lookType == 0) {
			return;
		}

		int resolvedFrame = options.animationPhase > 0 ? options.animationPhase : 0;

		int pattern_z = 0;
		if (outfit.lookMount != 0) {
			GameSprite* mountSpr = g_gui.gfx.getCreatureSprite(outfit.lookMount);
			if (mountSpr) {
				Outfit mountOutfit;
				mountOutfit.lookType = outfit.lookMount;
				mountOutfit.lookHead = outfit.lookMountHead;
				mountOutfit.lookBody = outfit.lookMountBody;
				mountOutfit.lookLegs = outfit.lookMountLegs;
				mountOutfit.lookFeet = outfit.lookMountFeet;

				for (int cx = 0; cx < mountSpr->width; ++cx) {
					for (int cy = 0; cy < mountSpr->height; ++cy) {
						int draw_x = screenx - (mountSpr->width - 1 - cx) * TILE_SIZE - mountSpr->getDrawOffset().first;
						int draw_y = screeny - (mountSpr->height - 1 - cy) * TILE_SIZE - mountSpr->getDrawOffset().second;

						const AtlasRegion* region = mountSpr->getAtlasRegion(cx, cy, static_cast<int>(dir), 0, 0, mountOutfit, resolvedFrame);
						if (region) {
							sprite_drawer->glBlitAtlasQuad(sprite_batch, draw_x, draw_y, region, options.color);
						}
					}
				}
				pattern_z = std::clamp(spr->pattern_z - 1, 0, 1);
			}
		}

		for (int pattern_y = 0; pattern_y < spr->pattern_y; pattern_y++) {
			if (pattern_y > 0) {
				if ((pattern_y - 1 >= 31) || !(outfit.lookAddon & (1 << (pattern_y - 1)))) {
					continue;
				}
			}

			for (int cx = 0; cx < spr->width; ++cx) {
				for (int cy = 0; cy < spr->height; ++cy) {
					int draw_x = screenx - (spr->width - 1 - cx) * TILE_SIZE - spr->getDrawOffset().first;
					int draw_y = screeny - (spr->height - 1 - cy) * TILE_SIZE - spr->getDrawOffset().second;

					const AtlasRegion* region = spr->getAtlasRegion(cx, cy, static_cast<int>(dir), pattern_y, pattern_z, outfit, resolvedFrame);
					if (region) {
						sprite_drawer->glBlitAtlasQuad(sprite_batch, draw_x, draw_y, region, options.color);
					}
				}
			}
		}
	}
}

void CreatureDrawer::BlitCreature(RenderList& list, int screenx, int screeny, const Creature* c, const CreatureDrawOptions& options) {
	CreatureDrawOptions local_opts = options;
	if (!local_opts.ingame && c->isSelected()) {
		local_opts.color.r /= 2;
		local_opts.color.g /= 2;
		local_opts.color.b /= 2;
	}
	BlitCreature(list, screenx, screeny, c->getOutfit(), c->getDirection(), local_opts);
}

void CreatureDrawer::BlitCreature(RenderList& list, int screenx, int screeny, const Outfit& outfit, Direction dir, const CreatureDrawOptions& options) {
	if (outfit.lookItem != 0) {
		ItemType& it = g_items[outfit.lookItem];
		sprite_drawer->BlitSprite(list, screenx, screeny, it.sprite, options.color);
	} else {
		GameSprite* spr = g_gui.gfx.getCreatureSprite(outfit.lookType);
		if (!spr || outfit.lookType == 0) {
			return;
		}

		int resolvedFrame = options.animationPhase > 0 ? options.animationPhase : 0;

		int pattern_z = 0;
		if (outfit.lookMount != 0) {
			GameSprite* mountSpr = g_gui.gfx.getCreatureSprite(outfit.lookMount);
			if (mountSpr) {
				Outfit mountOutfit;
				mountOutfit.lookType = outfit.lookMount;
				mountOutfit.lookHead = outfit.lookMountHead;
				mountOutfit.lookBody = outfit.lookMountBody;
				mountOutfit.lookLegs = outfit.lookMountLegs;
				mountOutfit.lookFeet = outfit.lookMountFeet;

				for (int cx = 0; cx < mountSpr->width; ++cx) {
					for (int cy = 0; cy < mountSpr->height; ++cy) {
						int draw_x = screenx - (mountSpr->width - 1 - cx) * TILE_SIZE - mountSpr->getDrawOffset().first;
						int draw_y = screeny - (mountSpr->height - 1 - cy) * TILE_SIZE - mountSpr->getDrawOffset().second;

						const AtlasRegion* region = mountSpr->getAtlasRegion(cx, cy, static_cast<int>(dir), 0, 0, mountOutfit, resolvedFrame);
						if (region) {
							sprite_drawer->glBlitAtlasQuad(list, draw_x, draw_y, region, options.color);
						}
					}
				}
				pattern_z = std::clamp(spr->pattern_z - 1, 0, 1);
			}
		}

		for (int pattern_y = 0; pattern_y < spr->pattern_y; pattern_y++) {
			if (pattern_y > 0) {
				if ((pattern_y - 1 >= 31) || !(outfit.lookAddon & (1 << (pattern_y - 1)))) {
					continue;
				}
			}

			for (int cx = 0; cx < spr->width; ++cx) {
				for (int cy = 0; cy < spr->height; ++cy) {
					int draw_x = screenx - (spr->width - 1 - cx) * TILE_SIZE - spr->getDrawOffset().first;
					int draw_y = screeny - (spr->height - 1 - cy) * TILE_SIZE - spr->getDrawOffset().second;

					const AtlasRegion* region = spr->getAtlasRegion(cx, cy, static_cast<int>(dir), pattern_y, pattern_z, outfit, resolvedFrame);
					if (region) {
						sprite_drawer->glBlitAtlasQuad(list, draw_x, draw_y, region, options.color);
					}
				}
			}
		}
	}
}
