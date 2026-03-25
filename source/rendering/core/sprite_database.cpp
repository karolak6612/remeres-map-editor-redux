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

#include "rendering/core/animator.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/image.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/sprite_icon_renderer.h"
#include "rendering/core/template_image.h"

#include <ranges>

namespace {
	[[nodiscard]] size_t spriteIndex(
		const SpriteMetadata& meta, int width, int height, int layer, int pattern_x, int pattern_y, int pattern_z, int frame
	) {
		if (meta.is_simple || meta.frames == 0) {
			return 0;
		}

		size_t idx = (meta.frames > 1) ? static_cast<size_t>(frame % meta.frames) : 0;
		idx = idx * static_cast<size_t>(meta.pattern_z) + static_cast<size_t>(pattern_z);
		idx = idx * static_cast<size_t>(meta.pattern_y) + static_cast<size_t>(pattern_y);
		idx = idx * static_cast<size_t>(meta.pattern_x) + static_cast<size_t>(pattern_x);
		idx = idx * static_cast<size_t>(meta.layers) + static_cast<size_t>(layer);
		idx = idx * static_cast<size_t>(meta.height) + static_cast<size_t>(height);
		idx = idx * static_cast<size_t>(meta.width) + static_cast<size_t>(width);
		return idx;
	}

	[[nodiscard]] uint32_t resolveAtlasSpriteIndex(
		const SpriteMetadata& meta, int x, int y, int layer, int subtype, int pattern_x, int pattern_y, int pattern_z, int frame
	) {
		if (subtype >= 0 && meta.height <= 1 && meta.width <= 1) {
			return static_cast<uint32_t>(subtype);
		}

		return static_cast<uint32_t>(
			((((((frame)*meta.pattern_y + pattern_y) * meta.pattern_x + pattern_x) * meta.layers + layer) * meta.height + y) * meta.width + x)
		);
	}
}

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

GameSprite* SpriteDatabase::getGameSprite(int id) {
	if (id < 0 || static_cast<size_t>(id) >= sprite_space_.size()) {
		return nullptr;
	}
	return dynamic_cast<GameSprite*>(sprite_space_[id].get());
}

const GameSprite* SpriteDatabase::getGameSprite(int id) const {
	if (id < 0 || static_cast<size_t>(id) >= sprite_space_.size()) {
		return nullptr;
	}
	return dynamic_cast<const GameSprite*>(sprite_space_[id].get());
}

void SpriteDatabase::syncComponents(int id, const GameSprite& sprite) {
	if (id < 0) {
		return;
	}

	const auto required_size = static_cast<size_t>(id) + 1;
	if (metadata_.size() < required_size) {
		metadata_.resize(required_size);
		atlas_caches_.resize(required_size);
		animations_.resize(required_size);
		icon_data_.resize(required_size);
	}

	metadata_[id] = sprite.meta;
	atlas_caches_[id] = sprite.atlas_cache;

	SpriteAnimationState mirrored_animation;
	mirrored_animation.sprite_id = sprite.animation.sprite_id;
	if (sprite.animation.animator) {
		mirrored_animation.animator = std::make_unique<Animator>(*sprite.animation.animator);
	}
	animations_[id] = std::move(mirrored_animation);

	auto& mirrored_icon_data = icon_data_[id];
	mirrored_icon_data.sprite_list = sprite.icon_data.sprite_list;
	mirrored_icon_data.instanced_templates.clear();
	if (!mirrored_icon_data.icon_renderer) {
		mirrored_icon_data.icon_renderer = std::make_unique<SpriteIconRenderer>();
	}
}

void SpriteDatabase::insertSprite(int id, std::unique_ptr<Sprite> sprite) {
	if (id < 0) {
		editor_sprite_space_[id] = std::move(sprite);
	} else {
		if (static_cast<size_t>(id) >= sprite_space_.size()) {
			sprite_space_.resize(id + 1);
		}
		if (auto* game_sprite = dynamic_cast<GameSprite*>(sprite.get())) {
			syncComponents(id, *game_sprite);
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
	metadata_.clear();
	atlas_caches_.clear();
	animations_.clear();
	icon_data_.clear();
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
	metadata_.resize(sprite_size);
	atlas_caches_.resize(sprite_size);
	animations_.resize(sprite_size);
	icon_data_.resize(sprite_size);
}

const SpriteMetadata* SpriteDatabase::getMeta(int id) const {
	if (id < 0 || static_cast<size_t>(id) >= metadata_.size()) {
		return nullptr;
	}
	return &metadata_[id];
}

AtlasRegionCache* SpriteDatabase::getAtlasCache(int id) {
	if (id < 0 || static_cast<size_t>(id) >= atlas_caches_.size()) {
		return nullptr;
	}
	return &atlas_caches_[id];
}

const AtlasRegionCache* SpriteDatabase::getAtlasCache(int id) const {
	if (id < 0 || static_cast<size_t>(id) >= atlas_caches_.size()) {
		return nullptr;
	}
	return &atlas_caches_[id];
}

SpriteAnimationState* SpriteDatabase::getAnimation(int id) {
	if (id < 0 || static_cast<size_t>(id) >= animations_.size()) {
		return nullptr;
	}
	return &animations_[id];
}

const SpriteAnimationState* SpriteDatabase::getAnimation(int id) const {
	if (id < 0 || static_cast<size_t>(id) >= animations_.size()) {
		return nullptr;
	}
	return &animations_[id];
}

SpriteIconData* SpriteDatabase::getIconData(int id) {
	if (id < 0 || static_cast<size_t>(id) >= icon_data_.size()) {
		return nullptr;
	}
	return &icon_data_[id];
}

const SpriteIconData* SpriteDatabase::getIconData(int id) const {
	if (id < 0 || static_cast<size_t>(id) >= icon_data_.size()) {
		return nullptr;
	}
	return &icon_data_[id];
}

bool SpriteDatabase::isSimpleAndLoaded(int id) const {
	const auto* meta = getMeta(id);
	const auto* icon_data = getIconData(id);
	return meta && icon_data && meta->is_simple && !icon_data->sprite_list.empty() && icon_data->sprite_list.front() != nullptr
		&& icon_data->sprite_list.front()->isGLLoaded;
}

const AtlasRegion* SpriteDatabase::getItemAtlasRegion(
	int id, int x, int y, int layer, int subtype, int pattern_x, int pattern_y, int pattern_z, int frame
) {
	auto* meta = getMeta(id);
	auto* cache = getAtlasCache(id);
	auto* icon_data = getIconData(id);
	if (!meta || !cache || !icon_data || meta->numsprites == 0 || icon_data->sprite_list.empty()) {
		return nullptr;
	}

	if (subtype == -1 && meta->numsprites == 1 && meta->frames == 1 && meta->layers == 1 && meta->width == 1 && meta->height == 1
		&& x == 0 && y == 0 && layer == 0 && frame == 0 && pattern_x == 0 && pattern_y == 0 && pattern_z == 0) {
		if (cache->cached_default_region && icon_data->sprite_list[0]->isGLLoaded && cache->cached_generation_id == icon_data->sprite_list[0]->generation_id
			&& cache->cached_sprite_id == icon_data->sprite_list[0]->id) {
			return cache->cached_default_region;
		}

		const AtlasRegion* valid_region = icon_data->sprite_list[0]->getAtlasRegion();
		if (valid_region && icon_data->sprite_list[0]->isGLLoaded) {
			cache->cached_default_region = valid_region;
			cache->cached_generation_id = icon_data->sprite_list[0]->generation_id;
			cache->cached_sprite_id = icon_data->sprite_list[0]->id;
		} else {
			cache->invalidate();
		}
		return valid_region;
	}

	uint32_t resolved_index = resolveAtlasSpriteIndex(*meta, x, y, layer, subtype, pattern_x, pattern_y, pattern_z, frame);
	if (resolved_index >= meta->numsprites) {
		resolved_index = meta->numsprites == 1 ? 0 : resolved_index % meta->numsprites;
	}

	if (resolved_index >= icon_data->sprite_list.size() || icon_data->sprite_list[resolved_index] == nullptr) {
		return nullptr;
	}
	return icon_data->sprite_list[resolved_index]->getAtlasRegion();
}

const AtlasRegion* SpriteDatabase::getCreatureAtlasRegion(
	int id, int x, int y, int dir, int addon, int pattern_z, const Outfit& outfit, int frame
) {
	auto* meta = getMeta(id);
	auto* icon_data = getIconData(id);
	if (!meta || !icon_data || meta->numsprites == 0) {
		return nullptr;
	}

	uint32_t resolved_index = static_cast<uint32_t>(spriteIndex(*meta, x, y, 0, dir, addon, pattern_z, frame));
	if (resolved_index >= meta->numsprites) {
		resolved_index = meta->numsprites == 1 ? 0 : resolved_index % meta->numsprites;
	}

	if (meta->layers > 1) {
		auto* game_sprite = getGameSprite(id);
		if (!game_sprite) {
			return nullptr;
		}
		if (auto* image = game_sprite->getTemplateImage(static_cast<int>(resolved_index), outfit)) {
			return image->getAtlasRegion();
		}
		return nullptr;
	}

	if (resolved_index >= icon_data->sprite_list.size() || icon_data->sprite_list[resolved_index] == nullptr) {
		return nullptr;
	}
	return icon_data->sprite_list[resolved_index]->getAtlasRegion();
}
