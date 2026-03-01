#include "app/main.h"
#include "rendering/core/texture_gc.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/image.h"
#include "rendering/core/game_sprite.h"
#include "app/settings.h"
#include <algorithm>

TextureGC::TextureGC() {
	animation_timer = std::make_unique<RenderTimer>();
	animation_timer->Start();
}

TextureGC::~TextureGC() = default;

void TextureGC::clear() {
	resident_images.clear();
	resident_game_sprites.clear();
	cleanup_list.clear();
	loaded_textures = 0;
	lastclean = std::time(nullptr);
	cached_time_ = 0;
}

void TextureGC::updateTime() {
	cached_time_ = std::time(nullptr);
}

void TextureGC::addResidentImage(Image* img) {
	std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
	resident_images.push_back(img);
	loaded_textures++;
}

void TextureGC::removeResidentImage(Image* img) {
	std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
	auto it = std::find(resident_images.begin(), resident_images.end(), img);
	if (it != resident_images.end()) {
		resident_images.erase(it);
		if (loaded_textures > 0) {
			loaded_textures--;
		}
	}
}

bool TextureGC::containsResidentImage(Image* img) const {
	std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
	return std::find(resident_images.begin(), resident_images.end(), img) != resident_images.end();
}

void TextureGC::addResidentGameSprite(GameSprite* gs) {
	std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
	resident_game_sprites.push_back(gs);
	gs->is_resident = true;
}

void TextureGC::removeResidentGameSprite(GameSprite* gs) {
	std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
	auto it = std::find(resident_game_sprites.begin(), resident_game_sprites.end(), gs);
	if (it != resident_game_sprites.end()) {
		resident_game_sprites.erase(it);
		gs->is_resident = false;
	}
}

bool TextureGC::containsResidentGameSprite(GameSprite* gs) const {
	std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
	return std::find(resident_game_sprites.begin(), resident_game_sprites.end(), gs) != resident_game_sprites.end();
}

constexpr int MIN_CLEAN_THRESHOLD = 100;

void TextureGC::addSpriteToCleanup(GameSprite* spr) {
	cleanup_list.push_back(spr);
	const size_t clean_threshold = static_cast<size_t>(std::max(MIN_CLEAN_THRESHOLD, g_settings.getInteger(Config::SOFTWARE_CLEAN_THRESHOLD)));
	if (cleanup_list.size() > clean_threshold) {
		int software_clean_size = g_settings.getInteger(Config::SOFTWARE_CLEAN_SIZE);
		const size_t cleanup_count = (software_clean_size <= 0) ? cleanup_list.size() : std::min(cleanup_list.size(), static_cast<size_t>(software_clean_size));

		for (size_t i = 0; i < cleanup_count; ++i) {
			cleanup_list.front()->unloadDC();
			cleanup_list.pop_front();
		}
	}
}

void TextureGC::garbageCollection(SpriteDatabase& db) {
	if (g_settings.getInteger(Config::TEXTURE_MANAGEMENT)) {
		if (loaded_textures > g_settings.getInteger(Config::TEXTURE_CLEAN_THRESHOLD) && cached_time_ - lastclean > g_settings.getInteger(Config::TEXTURE_CLEAN_PULSE)) {

			int longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);
			
			// We iterate backwards and handle the fact that img->clean() might call removeResidentImage()
			// which would remove the element from the vector.
			{
				std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
				for (size_t i = resident_images.size(); i > 0; --i) {
					Image* img = resident_images[i - 1];
					img->clean(cached_time_, longevity);

					// If the image chose to unload, it already removed itself from resident_images
					// via the recursive removeResidentImage call. We don't need to do anything else
					// but the loop continues safely because its size-based and we are going backwards.
				}

				// Call clean on resident_game_sprites to preserve software cache/animator state.
				for (size_t i = resident_game_sprites.size(); i > 0; --i) {
					GameSprite* gs = resident_game_sprites[i - 1];
					gs->clean(cached_time_, longevity);
				}
			}
			
			// Clean software sprites occasionally
			if (++clean_software_counter >= 10) {
				cleanSoftwareSprites(db);
				clean_software_counter = 0;
			}

			lastclean = cached_time_;
		}
	}
}

void TextureGC::cleanSoftwareSprites(SpriteDatabase& db) {
	for (auto& sprite_ptr : db.getSpriteSpace()) {
		if (sprite_ptr) {
			if (auto* gs = dynamic_cast<GameSprite*>(sprite_ptr.get())) {
				if (!gs->is_resident) {
					gs->unloadDC();
				}
			} else {
				sprite_ptr->unloadDC();
			}
		}
	}
}
