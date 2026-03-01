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
}

void TextureGC::updateTime() {
	cached_time_ = std::time(nullptr);
}

void TextureGC::notifyTextureLoaded() {
	loaded_textures++;
}

void TextureGC::notifyTextureUnloaded() {
	if (loaded_textures > 0) {
		loaded_textures--;
	}
}

constexpr int MIN_CLEAN_THRESHOLD = 100;

void TextureGC::addSpriteToCleanup(GameSprite* spr) {
	cleanup_list.push_back(spr);
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

void TextureGC::garbageCollection(SpriteDatabase& db) {
	if (g_settings.getInteger(Config::TEXTURE_MANAGEMENT)) {
		if (loaded_textures > g_settings.getInteger(Config::TEXTURE_CLEAN_THRESHOLD) && cached_time_ - lastclean > g_settings.getInteger(Config::TEXTURE_CLEAN_PULSE)) {

			int longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);
			std::lock_guard<std::mutex> lock(resident_images_mutex_);
			for (size_t i = resident_images.size(); i > 0; --i) {
				Image* img = resident_images[i - 1];
				img->clean(cached_time_, longevity);

				if (!img->isGLLoaded) {
					if (i - 1 < resident_images.size() - 1) {
						resident_images[i - 1] = resident_images.back();
					}
					resident_images.pop_back();
				}
			}

			// Call clean on resident_game_sprites to preserve software cache/animator state.
			// Memory bounds are managed elsewhere, ensuring they remain resident.
			for (size_t i = resident_game_sprites.size(); i > 0; --i) {
				GameSprite* gs = resident_game_sprites[i - 1];
				gs->clean(cached_time_, longevity);
			}
			
			// Clean software sprites
			cleanSoftwareSprites(db);

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
