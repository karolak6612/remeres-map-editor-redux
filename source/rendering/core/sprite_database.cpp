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
#include "rendering/core/sprite_database.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/image.h"

SpriteDatabase::SpriteDatabase() = default;
SpriteDatabase::~SpriteDatabase() = default;
SpriteDatabase::SpriteDatabase(SpriteDatabase&&) noexcept = default;
SpriteDatabase& SpriteDatabase::operator=(SpriteDatabase&&) noexcept = default;

Sprite* SpriteDatabase::getSprite(int id) const {
	if (id < 0) {
		if (auto it = editor_sprite_space_.find(id); it != editor_sprite_space_.end()) {
			return it->second.get();
		}
		return nullptr;
	}
	if (static_cast<size_t>(id) >= sprite_space_.size()) {
		return nullptr;
	}
	return sprite_space_[id].get();
}

void SpriteDatabase::insertSprite(int id, std::unique_ptr<Sprite> sprite) {
	if (id < 0) {
		editor_sprite_space_[id] = std::move(sprite);
	} else {
		if (static_cast<size_t>(id) >= sprite_space_.size()) {
			sprite_space_.resize(id + 1);
		}
		sprite_space_[id] = std::move(sprite);
	}
}

GameSprite* SpriteDatabase::getCreatureSprite(int id, uint16_t item_count) const {
	if (id < 0) {
		return nullptr;
	}
	size_t target_id = static_cast<size_t>(id) + item_count;
	if (target_id >= sprite_space_.size()) {
		return nullptr;
	}
	return static_cast<GameSprite*>(sprite_space_[target_id].get());
}

void SpriteDatabase::clear() {
	sprite_space_.clear();
	image_space_.clear();
	// editor_sprite_space_ persists across version changes
	resident_images_.clear();
	resident_game_sprites_.clear();
}

void SpriteDatabase::resize(size_t sprite_size, size_t image_size) {
	// If shrinking, clear resident tracking to avoid dangling pointers
	// to sprites/images that will be destroyed by resize.
	if (sprite_size < sprite_space_.size() || image_size < image_space_.size()) {
		resident_images_.clear();
		resident_game_sprites_.clear();
	}
	sprite_space_.resize(sprite_size);
	image_space_.resize(image_size);
}
