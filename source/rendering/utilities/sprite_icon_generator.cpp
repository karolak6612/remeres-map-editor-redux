//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include "app/settings.h"
#include "ui/gui.h"

namespace {
	// Helper for blending pixels manually
	// dest/src are 4-byte RGBA pointers
	inline void BlendPixel(uint8_t* dest, const uint8_t* src) {
		uint8_t sa = src[3];
		if (sa == 0) return; // Fully transparent source

		if (sa == 255) {
			// Fully opaque source replaces destination
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
			dest[3] = 255;
		} else {
			// Standard Alpha Blending
			// outA = srcA + dstA * (1 - srcA)
			// outRGB = (srcRGB * srcA + dstRGB * dstA * (1 - srcA)) / outA
			// Simplified assumption: pre-multiplied alpha or standard interpolation

			float a = sa / 255.0f;
			float inv_a = 1.0f - a;

			dest[0] = static_cast<uint8_t>(src[0] * a + dest[0] * inv_a);
			dest[1] = static_cast<uint8_t>(src[1] * a + dest[1] * inv_a);
			dest[2] = static_cast<uint8_t>(src[2] * a + dest[2] * inv_a);
			dest[3] = std::max(dest[3], sa); // Simple max alpha accumulation for sprite logic
		}
	}

	// Paste sprite data (32x32) into composite buffer
	void PasteSprite(uint8_t* composite, int compW, int compH, const uint8_t* spriteData, int posX, int posY) {
		if (!spriteData) return;

		for (int sy = 0; sy < 32; ++sy) {
			for (int sx = 0; sx < 32; ++sx) {
				int dy = posY + sy;
				int dx = posX + sx;

				if (dx < 0 || dx >= compW || dy < 0 || dy >= compH) continue;

				// Source is usually standard opaque-masked-by-magenta in old code,
				// but getRGBAData returns RGBA.
				// However, getRGBData returns 3-byte RGB (magenta masked).
				// We need to handle that if getRGBData is used.
				// BUT: getRGBAData (A) is preferred.

				// Assuming input spriteData is RGBA (4 bytes).
				// If the source was 3-byte RGB, we'd need conversion.
				// But getRGBData returns 3 bytes.
				// The original code used `getRGBData` and created wxImage with mask.
				// We should verify if we have getRGBAData available on Image/TemplateImage.
				// GameSprite::Image has pure virtual getRGBAData!

				// So we assume spriteData is 3-byte RGB (R,G,B).
				// Magenta (255, 0, 255) is transparent.

				int srcIdx = (sy * 32 + sx) * 3;
				int dstIdx = (dy * compW + dx) * 4;

				uint8_t r = spriteData[srcIdx + 0];
				uint8_t g = spriteData[srcIdx + 1];
				uint8_t b = spriteData[srcIdx + 2];

				if (r == 255 && g == 0 && b == 255) {
					// Transparent
					continue;
				}

				// Compose opaque
				uint8_t rgba[4] = { r, g, b, 255 };
				BlendPixel(&composite[dstIdx], rgba);
			}
		}
	}
}

wxBitmap SpriteIconGenerator::Generate(GameSprite* sprite, SpriteSize size, bool rescale) {
	return Generate(sprite, size, Outfit(), rescale);
}

wxBitmap SpriteIconGenerator::Generate(GameSprite* sprite, SpriteSize size, const Outfit& outfit, bool rescale, Direction direction) {
	auto rgba = GenerateRGBA(sprite, size, outfit, false, direction, false);
	if (!rgba) return wxNullBitmap;

	int dim = std::max<uint8_t>(sprite->width, sprite->height) * SPRITE_PIXELS;
	wxImage image(dim, dim, rgba.release(), true); // Takes ownership

	// Resizing if needed
	if (rescale && (size == SPRITE_SIZE_16x16 || size == SPRITE_SIZE_64x64 || image.GetWidth() > SPRITE_PIXELS || image.GetHeight() > SPRITE_PIXELS)) {
		int new_size = 32;
		if (size == SPRITE_SIZE_16x16) {
			new_size = 16;
		} else if (size == SPRITE_SIZE_64x64) {
			new_size = 64;
		}
		image.Rescale(new_size, new_size, wxIMAGE_QUALITY_HIGH);
	}

	return wxBitmap(image);
}

std::unique_ptr<uint8_t[]> SpriteIconGenerator::GenerateRGBA(GameSprite* sprite, SpriteSize size, const Outfit& outfit, bool rescale, Direction direction, bool transparent_bg) {
	ASSERT(sprite->width >= 1 && sprite->height >= 1);

	const int bgshade = g_settings.getInteger(Config::ICON_BACKGROUND);

	int dim = std::max<uint8_t>(sprite->width, sprite->height) * SPRITE_PIXELS;
	size_t bufferSize = dim * dim * 4;
	auto composite = std::make_unique<uint8_t[]>(bufferSize);

	// Fill background
	if (transparent_bg) {
		std::fill(composite.get(), composite.get() + bufferSize, 0);
	} else {
		for (int i = 0; i < dim * dim; ++i) {
			composite[i * 4 + 0] = (bgshade >> 16) & 0xFF;
			composite[i * 4 + 1] = (bgshade >> 8) & 0xFF;
			composite[i * 4 + 2] = bgshade & 0xFF;
			composite[i * 4 + 3] = 255;
		}
	}

	int frame_index = 0;
	if (sprite->pattern_x == 4) {
		frame_index = direction;
	}

	// Mounts
	int pattern_z = 0;
	if (outfit.lookMount != 0) {
		if (GameSprite* mountSpr = g_gui.gfx.getCreatureSprite(outfit.lookMount)) {
			// Mount outfit
			Outfit mountOutfit;
			mountOutfit.lookType = outfit.lookMount;
			mountOutfit.lookHead = outfit.lookMountHead;
			mountOutfit.lookBody = outfit.lookMountBody;
			mountOutfit.lookLegs = outfit.lookMountLegs;
			mountOutfit.lookFeet = outfit.lookMountFeet;

			int mount_frame_index = 0;
			if (mountSpr->pattern_x == 4) {
				mount_frame_index = direction;
			}

			for (uint8_t l = 0; l < mountSpr->layers; l++) {
				for (uint8_t w = 0; w < mountSpr->width; w++) {
					for (uint8_t h = 0; h < mountSpr->height; h++) {
						std::unique_ptr<uint8_t[]> data = nullptr;
						if (mountSpr->layers == 2) {
							if (l == 1) continue;
							data = mountSpr->getTemplateImage(mountSpr->getIndex(w, h, 0, mount_frame_index, 0, 0, 0), mountOutfit)->getRGBData();
						} else {
							data = mountSpr->spriteList[mountSpr->getIndex(w, h, l, mount_frame_index, 0, 0, 0)]->getRGBData();
						}

						if (data) {
							int mount_x = (sprite->width - w - 1) * SPRITE_PIXELS - mountSpr->getDrawOffset().first;
							int mount_y = (sprite->height - h - 1) * SPRITE_PIXELS - mountSpr->getDrawOffset().second;
							PasteSprite(composite.get(), dim, dim, data.get(), mount_x, mount_y);
						}
					}
				}
			}
			pattern_z = std::min<int>(1, sprite->pattern_z - 1);
		}
	}

	for (int pattern_y = 0; pattern_y < sprite->pattern_y; pattern_y++) {
		if (pattern_y > 0) {
			if ((pattern_y - 1 >= 31) || !(outfit.lookAddon & (1 << (pattern_y - 1)))) {
				continue;
			}
		}

		for (uint8_t l = 0; l < sprite->layers; l++) {
			for (uint8_t w = 0; w < sprite->width; w++) {
				for (uint8_t h = 0; h < sprite->height; h++) {
					std::unique_ptr<uint8_t[]> data = nullptr;

					if (sprite->layers == 2) {
						if (l == 1) continue;
						data = sprite->getTemplateImage(sprite->getIndex(w, h, 0, frame_index, pattern_y, pattern_z, 0), outfit)->getRGBData();
					} else if (sprite->layers == 4) {
						if (l == 1 || l == 3) continue;
						if (l == 0) {
							data = sprite->getTemplateImage(sprite->getIndex(w, h, 0, frame_index, pattern_y, pattern_z, 0), outfit)->getRGBData();
						}
						if (l == 2) {
							data = sprite->getTemplateImage(sprite->getIndex(w, h, 2, frame_index, pattern_y, pattern_z, 0), outfit)->getRGBData();
						}
					} else {
						const int idx = sprite->getIndex(w, h, l, frame_index, pattern_y, pattern_z, 0);
						// Safety check for index
						if (idx >= 0 && (size_t)idx < sprite->spriteList.size()) {
							data = sprite->spriteList[idx]->getRGBData();
						}
					}

					if (data) {
						PasteSprite(composite.get(), dim, dim, data.get(), (sprite->width - w - 1) * SPRITE_PIXELS, (sprite->height - h - 1) * SPRITE_PIXELS);
					}
				}
			}
		}
	}

	// Resizing if requested (Naive/CPU - only if really needed for non-wx path)
	// For now, we only implement rescaling in the wx wrapper because high-quality scaling is complex.
	// If rescale is true in GenerateRGBA, we just warn or implement NN.
	// For this task, CreateGameSpriteTexture typically does NOT rescale (it wants full size).
	// CreatureSprite::GetRGBAData will use default (rescale=true) but maybe we should ignore it for texture uploads
	// and let GPU scale.

	return composite;
}
