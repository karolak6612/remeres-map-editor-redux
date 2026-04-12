//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

// glut include removed

#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "game/creature.h"
#include "ui/gui.h"
#include "game/sprites.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/animator.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/render_view.h"
#include <spdlog/spdlog.h>

CreatureDrawer::CreatureDrawer() {
}

CreatureDrawer::~CreatureDrawer() {
}

namespace {
	void registerCreatureSpriteLight(LightBuffer& light_buffer, const RenderView& view, const GameSprite& sprite, int screen_x, int screen_y, SpriteLight light, bool preview_local_player) {
		if (preview_local_player) {
			light.intensity = std::max<uint8_t>(light.intensity, 2);
			if (light.color == 0 || light.color > 215) {
				light.color = 215;
			}
		}

		if (light.intensity == 0) {
			return;
		}

		const int left = screen_x - sprite.getDrawOffset().first - (static_cast<int>(sprite.width) - 1) * TILE_SIZE;
		const int top = screen_y - sprite.getDrawOffset().second - (static_cast<int>(sprite.height) - 1) * TILE_SIZE;
		const int width = std::max(1, static_cast<int>(sprite.width) * TILE_SIZE);
		const int height = std::max(1, static_cast<int>(sprite.height) * TILE_SIZE);
		light_buffer.AddScreenLight(left + width / 2, top + height / 2, view, light);
	}

	void registerCreatureCenterLight(LightBuffer& light_buffer, const RenderView& view, int screen_x, int screen_y, const GameSprite* displacement_sprite, SpriteLight light, bool preview_local_player) {
		if (preview_local_player) {
			light.intensity = std::max<uint8_t>(light.intensity, 2);
			if (light.color == 0 || light.color > 215) {
				light.color = 215;
			}
		}

		if (light.intensity == 0) {
			return;
		}

		const int displacement_x = displacement_sprite ? displacement_sprite->getDrawOffset().first : 0;
		const int displacement_y = displacement_sprite ? displacement_sprite->getDrawOffset().second : 0;
		light_buffer.AddScreenLight(screen_x - displacement_x + TILE_SIZE / 2, screen_y - displacement_y + TILE_SIZE / 2, view, light);
	}
}

void CreatureDrawer::BlitCreature(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, const Creature* c, const CreatureDrawOptions& options) {
	CreatureDrawOptions local_opts = options;
	if (!local_opts.ingame && (c->isSelected() || (local_opts.transient_selection_bounds.has_value() && local_opts.transient_selection_bounds->contains(local_opts.map_pos.x, local_opts.map_pos.y)))) {
		local_opts.color.r /= 2;
		local_opts.color.g /= 2;
		local_opts.color.b /= 2;
	}
	BlitCreature(sprite_batch, sprite_drawer, screenx, screeny, c->getLookType(), c->getDirection(), local_opts);
}

void CreatureDrawer::BlitCreature(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, const Outfit& outfit, Direction dir, const CreatureDrawOptions& options) {
	if (outfit.lookItem != 0) {
		if (const auto definition = g_item_definitions.get(outfit.lookItem)) {
			GameSprite* spr = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(definition.clientId()));
			if (spr && options.light_buffer && options.view && spr->hasLight()) {
				registerCreatureSpriteLight(*options.light_buffer, *options.view, *spr, screenx, screeny, spr->getLight(), false);
			}
			sprite_drawer->BlitSprite(sprite_batch, screenx, screeny, spr, options.color);
			if (spr && options.light_buffer && options.view && (spr->hasLight() || options.preview_local_player)) {
				registerCreatureCenterLight(*options.light_buffer, *options.view, screenx, screeny, spr, spr->hasLight() ? spr->getLight() : SpriteLight {}, options.preview_local_player);
			}
		}
	} else {
		// get outfit sprite
		GameSprite* spr = g_gui.gfx.getCreatureSprite(outfit.lookType);
		if (!spr || outfit.lookType == 0) {
			return;
		}

		if (options.light_buffer && options.view && spr->hasLight()) {
			registerCreatureSpriteLight(*options.light_buffer, *options.view, *spr, screenx, screeny, spr->getLight(), false);
		}

		// Resolve animation frame for walk animation
		// For in-game preview: animationPhase controls walk animation
		// - When > 0: walking (use the provided animation phase)
		// - When == 0: standing idle (ALWAYS use frame 0, NOT the global animator)
		// The global animator is for idle creatures on the map, NOT for the player
		int resolvedFrame = options.animationPhase > 0 ? options.animationPhase : 0;

		// mount and addon drawing thanks to otc code
		// mount colors by Zbizu
		int pattern_z = 0;
		GameSprite* mountSpr = nullptr;
		if (outfit.lookMount != 0) {
			if ((mountSpr = g_gui.gfx.getCreatureSprite(outfit.lookMount))) {
				if (options.light_buffer && options.view && mountSpr->hasLight()) {
					registerCreatureSpriteLight(*options.light_buffer, *options.view, *mountSpr, screenx, screeny, mountSpr->getLight(), false);
				}

				// generate mount colors
				Outfit mountOutfit;
				mountOutfit.lookType = outfit.lookMount;
				mountOutfit.lookHead = outfit.lookMountHead;
				mountOutfit.lookBody = outfit.lookMountBody;
				mountOutfit.lookLegs = outfit.lookMountLegs;
				mountOutfit.lookFeet = outfit.lookMountFeet;

				for (int cx = 0; cx != mountSpr->width; ++cx) {
					for (int cy = 0; cy != mountSpr->height; ++cy) {
						const AtlasRegion* region = mountSpr->getAtlasRegion(cx, cy, static_cast<int>(dir), 0, 0, mountOutfit, resolvedFrame);
						if (region) {
							sprite_drawer->glBlitAtlasQuad(sprite_batch, screenx - cx * TILE_SIZE - mountSpr->getDrawOffset().first, screeny - cy * TILE_SIZE - mountSpr->getDrawOffset().second, region, options.color);
						}
					}
				}

				pattern_z = std::clamp(spr->pattern_z - 1, 0, 1);
			}
		}

		// pattern_y => creature addon
		for (int pattern_y = 0; pattern_y < spr->pattern_y; pattern_y++) {

			// continue if we dont have this addon
			if (pattern_y > 0) {
				if ((pattern_y - 1 >= 31) || !(outfit.lookAddon & (1 << (pattern_y - 1)))) {
					continue;
				}
			}

			for (int cx = 0; cx != spr->width; ++cx) {
				for (int cy = 0; cy != spr->height; ++cy) {
					const AtlasRegion* region = spr->getAtlasRegion(cx, cy, static_cast<int>(dir), pattern_y, pattern_z, outfit, resolvedFrame);
					if (region) {
						sprite_drawer->glBlitAtlasQuad(sprite_batch, screenx - cx * TILE_SIZE - spr->getDrawOffset().first, screeny - cy * TILE_SIZE - spr->getDrawOffset().second, region, options.color);
					}
				}
			}
		}

		if (options.light_buffer && options.view && (spr->hasLight() || options.preview_local_player)) {
			registerCreatureCenterLight(*options.light_buffer, *options.view, screenx, screeny, mountSpr ? mountSpr : spr, spr->hasLight() ? spr->getLight() : SpriteLight {}, options.preview_local_player);
		}
	}
}
