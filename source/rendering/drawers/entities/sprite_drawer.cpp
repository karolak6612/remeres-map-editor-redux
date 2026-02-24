#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/core/graphics.h"
#include "game/sprites.h"
#include "game/items.h"

#include "ui/gui.h"
#include <spdlog/spdlog.h>
#include "rendering/core/sprite_batch.h"
#include "rendering/core/atlas_manager.h"

SpriteDrawer::SpriteDrawer() :
	last_bound_texture_(0) {
}

SpriteDrawer::~SpriteDrawer() {
}

void SpriteDrawer::ResetCache() {
	last_bound_texture_ = 0;
}

void SpriteDrawer::glBlitAtlasQuad(SpriteBatch& sprite_batch, int sx, int sy, const AtlasRegion* region, DrawColor color) {
	if (region) {
		float normalizedR = color.r / 255.0f;
		float normalizedG = color.g / 255.0f;
		float normalizedB = color.b / 255.0f;
		float normalizedA = color.a / 255.0f;

		sprite_batch.draw(
			static_cast<float>(sx), static_cast<float>(sy),
			static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE),
			*region,
			normalizedR, normalizedG, normalizedB, normalizedA
		);
	}
}

void SpriteDrawer::glBlitSquare(SpriteBatch& sprite_batch, int sx, int sy, DrawColor color, int size) {
	if (size == 0) {
		size = TILE_SIZE;
	}

	float normalizedR = color.r / 255.0f;
	float normalizedG = color.g / 255.0f;
	float normalizedB = color.b / 255.0f;
	float normalizedA = color.a / 255.0f;

	// Use Graphics::getAtlasManager() to get the atlas manager for white pixel access
	// This assumes Graphics and AtlasManager are available
	if (g_gui.gfx.hasAtlasManager()) {
		sprite_batch.drawRect(static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(size), static_cast<float>(size), glm::vec4(normalizedR, normalizedG, normalizedB, normalizedA), *g_gui.gfx.getAtlasManager());
	}
}

void SpriteDrawer::glDrawBox(SpriteBatch& sprite_batch, int sx, int sy, int width, int height, DrawColor color) {
	float normalizedR = color.r / 255.0f;
	float normalizedG = color.g / 255.0f;
	float normalizedB = color.b / 255.0f;
	float normalizedA = color.a / 255.0f;

	if (g_gui.gfx.hasAtlasManager()) {
		sprite_batch.drawRectLines(static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(width), static_cast<float>(height), glm::vec4(normalizedR, normalizedG, normalizedB, normalizedA), *g_gui.gfx.getAtlasManager());
	}
}

void SpriteDrawer::glSetColor(wxColor color) {
	// Not needed with BatchRenderer automatic color handling in DrawQuad,
	// but if used for stateful drawing elsewhere, we might need a state setter.
	// For now, ignoring as glBlitTexture/Square takes explicit color.
}

void SpriteDrawer::BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, uint32_t spriteid, DrawColor color) {
	GameSprite* spr = g_items[spriteid].sprite;
	if (spr == nullptr) {
		return;
	}
	// Call the pointer overload
	BlitSprite(sprite_batch, screenx, screeny, spr, color);
}

void SpriteDrawer::BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, GameSprite* spr, DrawColor color) {
	if (spr == nullptr) {
		return;
	}
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
					glBlitAtlasQuad(sprite_batch, screenx - cx * TILE_SIZE, screeny - cy * TILE_SIZE, region, color);
				}
				// No fallback - if region is null, sprite failed to load
			}
		}
	}
}
