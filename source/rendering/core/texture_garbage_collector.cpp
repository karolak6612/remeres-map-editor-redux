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
#include "rendering/core/graphics.h"
#include "rendering/core/image.h"
#include "app/settings.h"
#include <algorithm>

TextureGarbageCollector::TextureGarbageCollector() :
	loaded_textures(0),
	image_gc_index(0),
	sprite_gc_index(0),
	gc_phase(0) {
}

TextureGarbageCollector::~TextureGarbageCollector() {
}

void TextureGarbageCollector::Clear() {
	loaded_textures = 0;
	image_gc_index = 0;
	sprite_gc_index = 0;
	gc_phase = 0;
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
	if (!g_settings.getInteger(Config::TEXTURE_MANAGEMENT)) {
		return;
	}

	if (loaded_textures <= g_settings.getInteger(Config::TEXTURE_CLEAN_THRESHOLD)) {
		return;
	}

	// 1.0ms total budget per frame
	constexpr double MAX_GC_TIME_MS = 1.0;
	constexpr size_t CHECK_INTERVAL = 256;

	auto start_time = std::chrono::high_resolution_clock::now();
	int longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);

	size_t checks_since_time_read = 0;

	// 1. Clean Images
	if (gc_phase == 0) {
		if (!resident_images.empty()) {
			if (image_gc_index >= resident_images.size()) {
				image_gc_index = 0;
			}

			size_t items_checked = 0;
			size_t current_size = resident_images.size();

			while (items_checked < current_size && !resident_images.empty()) {
				if (image_gc_index >= resident_images.size()) {
					image_gc_index = 0;
				}

				Image* img = static_cast<Image*>(resident_images[image_gc_index]);
				img->clean(current_time, longevity);

				if (!img->isGLLoaded) {
					// Image evicted itself during clean()
					if (image_gc_index < resident_images.size() - 1) {
						resident_images[image_gc_index] = resident_images.back();
					}
					resident_images.pop_back();

					// Don't increment index, because we just swapped a new element into this slot.
					// However, we MUST increment items_checked to ensure we eventually exit the loop.
				} else {
					image_gc_index++;
				}

				items_checked++;

				checks_since_time_read++;
				if (checks_since_time_read >= CHECK_INTERVAL) {
					checks_since_time_read = 0;
					auto now = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double, std::milli> elapsed = now - start_time;
					if (elapsed.count() >= MAX_GC_TIME_MS) {
						return;
					}
				}
			}
		}
		gc_phase = 1; // Move to phase 1 (GameSprites) if we finished Phase 0 without hitting the time limit
	}

	// 2. Clean GameSprites (Software caches/animators)
	if (gc_phase == 1) {
		if (!resident_game_sprites.empty()) {
			if (sprite_gc_index >= resident_game_sprites.size()) {
				sprite_gc_index = 0;
			}

			size_t items_checked = 0;
			size_t current_size = resident_game_sprites.size();

			while (items_checked < current_size && !resident_game_sprites.empty()) {
				if (sprite_gc_index >= resident_game_sprites.size()) {
					sprite_gc_index = 0;
				}

				GameSprite* gs = resident_game_sprites[sprite_gc_index];
				gs->clean(current_time, longevity);

				// We don't remove GameSprites from resident_game_sprites here currently
				// If we ever do, we'd use the same swap-and-pop logic as above
				sprite_gc_index++;
				items_checked++;

				checks_since_time_read++;
				if (checks_since_time_read >= CHECK_INTERVAL) {
					checks_since_time_read = 0;
					auto now = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double, std::milli> elapsed = now - start_time;
					if (elapsed.count() >= MAX_GC_TIME_MS) {
						return;
					}
				}
			}
		}
		gc_phase = 0; // Reset back to phase 0 if we finished Phase 1 without hitting the time limit
	}
}

void TextureGarbageCollector::CleanSoftwareSprites(std::vector<std::unique_ptr<Sprite>>& sprite_space) {
	for (auto& sprite_ptr : sprite_space) {
		if (sprite_ptr) {
			sprite_ptr->unloadDC();
		}
	}
}
