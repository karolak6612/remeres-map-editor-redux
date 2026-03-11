//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "rendering/ui/nvg_image_cache.h"
#include "rendering/core/graphics.h"
#include "rendering/core/normal_image.h"
#include "item_definitions/core/item_definition_store.h"
#include "game/sprites.h"
#include <nanovg.h>

NVGImageCache::~NVGImageCache() {
	invalidateAll();
}

void NVGImageCache::invalidateAll() {
	if (last_context_) {
		for (auto& [key, handle] : cache_) {
			if (handle > 0) {
				nvgDeleteImage(last_context_, handle);
			}
		}
	}
	cache_.clear();
}

int NVGImageCache::getSpriteImage(NVGcontext* vg, uint16_t itemId) {
	if (itemId == 0) {
		return 0;
	}

	// Detect context change and clear cache
	if (vg != last_context_) {
		invalidateAll();
		last_context_ = vg;
	}

	// Check cache first
	auto it = cache_.find(itemId);
	if (it != cache_.end()) {
		return it->second;
	}

	// Resolve item definition to GameSprite
	const auto definition = g_item_definitions.get(itemId);
	GameSprite* gameSprite = definition ? dynamic_cast<GameSprite*>(graphics_.getSprite(definition.clientId())) : nullptr;
	if (!gameSprite || gameSprite->icon_data.sprite_list.empty()) {
		return 0;
	}

	NormalImage* img = gameSprite->icon_data.sprite_list[0];
	if (!img) {
		return 0;
	}

	std::unique_ptr<uint8_t[]> rgba;

	// For legacy sprites (no transparency), use RGB + Magenta masking
	if (!graphics_.hasTransparency()) {
		std::unique_ptr<uint8_t[]> rgb = img->getRGBData();
		if (rgb) {
			rgba = std::make_unique<uint8_t[]>(32 * 32 * 4);
			for (int i = 0; i < 32 * 32; ++i) {
				uint8_t r = rgb[i * 3 + 0];
				uint8_t g = rgb[i * 3 + 1];
				uint8_t b = rgb[i * 3 + 2];

				if (r == 0xFF && g == 0x00 && b == 0xFF) {
					rgba[i * 4 + 0] = 0;
					rgba[i * 4 + 1] = 0;
					rgba[i * 4 + 2] = 0;
					rgba[i * 4 + 3] = 0;
				} else {
					rgba[i * 4 + 0] = r;
					rgba[i * 4 + 1] = g;
					rgba[i * 4 + 2] = b;
					rgba[i * 4 + 3] = 255;
				}
			}
		}
	}

	// Fallback/Standard path for alpha sprites
	if (!rgba) {
		rgba = img->getRGBAData();
	}

	if (!rgba) {
		return 0;
	}

	int image = nvgCreateImageRGBA(vg, 32, 32, 0, rgba.get());
	if (image > 0) {
		cache_[itemId] = image;
	}
	return image;
}
