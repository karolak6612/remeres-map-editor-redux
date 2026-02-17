//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_UTILITIES_SPRITE_ICON_GENERATOR_H_
#define RME_RENDERING_UTILITIES_SPRITE_ICON_GENERATOR_H_

#include "rendering/core/graphics.h"
#include "game/creature.h"

class SpriteIconGenerator {
public:
	static wxBitmap Generate(GameSprite* sprite, SpriteSize size, bool rescale = true);
	static wxBitmap Generate(GameSprite* sprite, SpriteSize size, const Outfit& outfit, bool rescale = true, Direction direction = SOUTH);

	/**
	 * @brief Generates raw RGBA pixel data for a sprite (including outfit composition).
	 *
	 * This method performs the core composition logic without relying on wxImage/wxBitmap,
	 * suitable for direct upload to OpenGL/NanoVG textures.
	 *
	 * @param sprite The source GameSprite.
	 * @param size Target size (16x16, 32x32, 64x64).
	 * @param outfit Outfit configuration (optional).
	 * @param rescale Whether to resize the output to match `size` standard dimensions.
	 * @param direction Direction for creatures.
	 * @param transparent_bg If true, initializes buffer with 0 (transparent) instead of Config::ICON_BACKGROUND.
	 * @return Unique pointer to the RGBA buffer. Use `size` to determine dimensions (e.g. 32*32*4).
	 */
	static std::unique_ptr<uint8_t[]> GenerateRGBA(GameSprite* sprite, SpriteSize size, const Outfit& outfit, bool rescale = true, Direction direction = SOUTH, bool transparent_bg = false);
};

#endif
