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

#include "app/main.h"
#include "rendering/core/texture_garbage_collector.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/texture_gc.h"
#include "rendering/io/sprite_loader.h"
#include "rendering/core/image.h"
#include "app/settings.h"
#include <algorithm>

TextureGarbageCollector::TextureGarbageCollector() :
	loaded_textures(0),
	lastclean(0) {
}

TextureGarbageCollector::~TextureGarbageCollector() {
}

void TextureGarbageCollector::Clear() {
	loaded_textures = 0;
	lastclean = time(nullptr);
	cleanup_list.clear();
}

void TextureGarbageCollector::NotifyTextureLoaded() {
	loaded_textures++;
}

void TextureGarbageCollector::NotifyTextureUnloaded() {
	loaded_textures--;
}

// Minimal threshold before we even consider reliable cleanup
const int MIN_CLEAN_THRESHOLD = 100;

void TextureGarbageCollector::AddSpriteToCleanup(GameSprite* spr) {
	cleanup_list.push_back(spr);
	// Clean if needed
	const size_t clean_threshold = static_cast<size_t>(std::max(MIN_CLEAN_THRESHOLD, g_settings.getInteger(Config::SOFTWARE_CLEAN_THRESHOLD)));
	if (cleanup_list.size() > clean_threshold) {
		const auto software_clean_size = std::max(0, g_settings.getInteger(Config::SOFTWARE_CLEAN_SIZE));
		const auto cleanup_count = std::min(cleanup_list.size(), static_cast<size_t>(software_clean_size));

		for (size_t i = 0; i < cleanup_count; ++i) {
			cleanup_list.front()->unloadDC();
			cleanup_list.pop_front();
		}
	}
}

void TextureGarbageCollector::GarbageCollect(std::vector<GameSprite*>& resident_game_sprites, std::vector<void*>& resident_images, time_t current_time) {
	if (g_settings.getInteger(Config::TEXTURE_MANAGEMENT)) {
		if (loaded_textures > g_settings.getInteger(Config::TEXTURE_CLEAN_THRESHOLD) && current_time - lastclean > g_settings.getInteger(Config::TEXTURE_CLEAN_PULSE)) {

			int longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);
			for (size_t i = resident_images.size(); i > 0; --i) {
				Image* img = static_cast<Image*>(resident_images[i - 1]);
				img->clean(current_time, longevity);

				if (!img->isGLLoaded) {
					// Image evicted itself during clean()
					if (i - 1 < resident_images.size() - 1) {
						resident_images[i - 1] = resident_images.back();
					}
					resident_images.pop_back();
				}
			}

			// 2. Clean GameSprites (Software caches/animators)
			for (size_t i = resident_game_sprites.size(); i > 0; --i) {
				GameSprite* gs = resident_game_sprites[i - 1];
				gs->clean(current_time, longevity);

				// Optional: Add logic to check if GameSprite is still "active"
				// For now, GameSprites stay resident if they have software DC/animator state
				// This part of the refactor is more subtle; we'll keep it simple first.
			}

			lastclean = current_time;
		}
	}
}

void TextureGarbageCollector::CleanSoftwareSprites(std::vector<std::unique_ptr<Sprite>>& sprite_space) {
	for (auto& sprite_ptr : sprite_space) {
		if (sprite_ptr) {
			sprite_ptr->unloadDC();
		}
	}
}
