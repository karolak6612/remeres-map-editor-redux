#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/core/graphics.h"
#include "game/sprites.h"
#include "game/items.h"

#include "ui/gui.h"
#include <spdlog/spdlog.h>
#include "rendering/core/sprite_sink.h"
#include "rendering/core/atlas_manager.h"

SpriteDrawer::SpriteDrawer() :
	last_bound_texture_(0) {
}

SpriteDrawer::~SpriteDrawer() {
}

void SpriteDrawer::ResetCache() {
	last_bound_texture_ = 0;
}

void SpriteDrawer::glBlitAtlasQuad(ISpriteSink& sprite_sink, int sx, int sy, const AtlasRegion* region, int red, int green, int blue, int alpha) {
	if (region) {
		float normalizedR = red / 255.0f;
		float normalizedG = green / 255.0f;
		float normalizedB = blue / 255.0f;
		float normalizedA = alpha / 255.0f;

		sprite_sink.draw(
			(float)sx, (float)sy,
			(float)TILE_SIZE, (float)TILE_SIZE,
			*region,
			normalizedR, normalizedG, normalizedB, normalizedA
		);
	}
}

void SpriteDrawer::glBlitSquare(ISpriteSink& sprite_sink, int sx, int sy, int red, int green, int blue, int alpha, const AtlasRegion* white_pixel, int size) {
	if (size == 0) {
		size = TILE_SIZE;
	}

	float normalizedR = red / 255.0f;
	float normalizedG = green / 255.0f;
	float normalizedB = blue / 255.0f;
	float normalizedA = alpha / 255.0f;

	const AtlasRegion* wp = white_pixel;
	if (!wp && g_gui.gfx.hasAtlasManager()) {
		wp = g_gui.gfx.getAtlasManager()->getWhitePixel();
	}

	if (wp) {
		sprite_sink.drawRect((float)sx, (float)sy, (float)size, (float)size, glm::vec4(normalizedR, normalizedG, normalizedB, normalizedA), *wp);
	}
}

void SpriteDrawer::glDrawBox(ISpriteSink& sprite_sink, int sx, int sy, int width, int height, int red, int green, int blue, int alpha, const AtlasRegion* white_pixel) {
	float normalizedR = red / 255.0f;
	float normalizedG = green / 255.0f;
	float normalizedB = blue / 255.0f;
	float normalizedA = alpha / 255.0f;

	const AtlasRegion* wp = white_pixel;
	if (!wp && g_gui.gfx.hasAtlasManager()) {
		wp = g_gui.gfx.getAtlasManager()->getWhitePixel();
	}

	if (wp) {
		glm::vec4 color(normalizedR, normalizedG, normalizedB, normalizedA);
		// Top
		sprite_sink.drawRect((float)sx, (float)sy, (float)width, 1.0f, color, *wp);
		// Bottom
		sprite_sink.drawRect((float)sx, (float)sy + height - 1.0f, (float)width, 1.0f, color, *wp);
		// Left
		sprite_sink.drawRect((float)sx, (float)sy + 1.0f, 1.0f, (float)height - 2.0f, color, *wp);
		// Right
		sprite_sink.drawRect((float)sx + width - 1.0f, (float)sy + 1.0f, 1.0f, (float)height - 2.0f, color, *wp);
	}
}

void SpriteDrawer::glSetColor(wxColor color) {
	// Not needed with BatchRenderer automatic color handling in DrawQuad,
	// but if used for stateful drawing elsewhere, we might need a state setter.
	// For now, ignoring as glBlitTexture/Square takes explicit color.
}

void SpriteDrawer::BlitSprite(ISpriteSink& sprite_sink, int screenx, int screeny, uint32_t spriteid, int red, int green, int blue, int alpha) {
	GameSprite* spr = g_items[spriteid].sprite;
	if (spr == nullptr) {
		return;
	}
	// Call the pointer overload
	BlitSprite(sprite_sink, screenx, screeny, spr, red, green, blue, alpha);
}

void SpriteDrawer::BlitSprite(ISpriteSink& sprite_sink, int screenx, int screeny, GameSprite* spr, int red, int green, int blue, int alpha) {
	if (spr == nullptr) {
		return;
	}
	// Note: GameSprite is safe to read on threads if it doesn't trigger lazy loading
	// But getAtlasRegion might return nullptr if not loaded.

	screenx -= spr->getDrawOffset().first;
	screeny -= spr->getDrawOffset().second;

	int tme = 0; // GetTime() % itype->FPA;

	// Atlas-only rendering - ensure atlas is available
	// Note: ensureAtlasManager is called by MapDrawer at frame start usually, but we check here too if needed?
	// BatchRenderer::SetAtlasManager call removed. Use sprite_batch.

	for (int cx = 0; cx != spr->width; ++cx) {
		for (int cy = 0; cy != spr->height; ++cy) {
			for (int cf = 0; cf != spr->layers; ++cf) {
				const AtlasRegion* region = spr->getAtlasRegion(cx, cy, cf, -1, 0, 0, 0, tme);
				if (region) {
					glBlitAtlasQuad(sprite_sink, screenx - cx * TILE_SIZE, screeny - cy * TILE_SIZE, region, red, green, blue, alpha);
				} else {
					// Report missing
					uint32_t id = spr->getSpriteId(0, cx, cy);
					if (id != 0) {
						sprite_sink.reportMissingSprite(id);
					}
				}
			}
		}
	}
}
