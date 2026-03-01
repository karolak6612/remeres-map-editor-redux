#include "rendering/core/sprite_database.h"
#include "rendering/core/sprite_preloader.h"

void SpriteDatabase::clear() {
	SpritePreloader::get().clear();
	sprite_space.clear();
	image_space.clear();
	// editor_sprite_space is intentionally not cleared here, as editor sprites persist across map versions.
	item_count = 0;
	creature_count = 0;
}

Sprite* SpriteDatabase::getSprite(int id) {
	if (id < 0) {
		if (auto it = editor_sprite_space.find(id); it != editor_sprite_space.end()) {
			return it->second.get();
		}
		return nullptr;
	}
	if (static_cast<size_t>(id) >= sprite_space.size()) {
		return nullptr;
	}
	return sprite_space[id].get();
}

void SpriteDatabase::insertSprite(int id, std::unique_ptr<Sprite> sprite) {
	if (id < 0) {
		editor_sprite_space[id] = std::move(sprite);
	} else {
		if (static_cast<size_t>(id) >= sprite_space.size()) {
			sprite_space.resize(id + 1);
		}
		sprite_space[id] = std::move(sprite);
	}
}

GameSprite* SpriteDatabase::getCreatureSprite(int id) {
	if (id < 0) {
		return nullptr;
	}

	size_t target_id = static_cast<size_t>(id) + item_count;
	if (target_id >= sprite_space.size()) {
		return nullptr;
	}
	return static_cast<GameSprite*>(sprite_space[target_id].get());
}
