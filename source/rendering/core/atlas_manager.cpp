#include "rendering/core/atlas_manager.h"
#include <iostream>
#include <algorithm>
#include <shared_mutex>
#include <spdlog/spdlog.h>

const AtlasRegion* AtlasManager::getRegionUnlocked(uint32_t sprite_id) const {
	if (sprite_id < DIRECT_LOOKUP_SIZE) {
		return direct_lookup_[sprite_id];
	}
	if (auto it = sprite_regions_.find(sprite_id); it != sprite_regions_.end()) {
		return it->second;
	}
	return nullptr;
}

bool AtlasManager::ensureInitializedUnlocked() {
	if (atlas_.isValid()) {
		return true;
	}

	// Pre-allocate 16 layers (16 * 16384 = 262K sprites capacity).
	// This fits in ~1GB VRAM and covers older clients/smaller Tibia 10+ datasets.
	// Larger datasets will dynamically expand this via addLayer().
	static constexpr int INITIAL_LAYERS = 16;

	if (!atlas_.initialize(INITIAL_LAYERS)) {
		spdlog::error("AtlasManager: Failed to initialize texture array");
		return false;
	}

	spdlog::info("AtlasManager: Texture array initialized ({}x{}, {} initial layers)", TextureAtlas::ATLAS_SIZE, TextureAtlas::ATLAS_SIZE, INITIAL_LAYERS);

	// Ensure white pixel exists (ID AtlasRegion::INVALID_SENTINEL)
	std::vector<uint8_t> white_data(32 * 32 * 4, 255);
	white_pixel_cache_ = addSpriteUnlocked(WHITE_PIXEL_ID, white_data.data());

	if (!white_pixel_cache_) {
		spdlog::error("AtlasManager: Failed to register white pixel sprite");
		return false;
	}

	return true;
}

bool AtlasManager::ensureInitialized() {
	std::unique_lock lock(atlas_mutex_);
	return ensureInitializedUnlocked();
}

const AtlasRegion* AtlasManager::addSpriteUnlocked(uint32_t sprite_id, const uint8_t* rgba_data) {
	// Fast check via direct lookup for common sprites
	if (const auto* region = getRegionUnlocked(sprite_id)) {
		return region;
	}

	if (!rgba_data) {
		spdlog::warn("AtlasManager::addSprite called with null data for sprite {}", sprite_id);
		return nullptr;
	}

	if (!ensureInitializedUnlocked()) {
		return nullptr;
	}

	// Add to texture array
	auto region = atlas_.addSprite(rgba_data);
	if (!region.has_value()) {
		spdlog::error("AtlasManager: Failed to add sprite {} to texture array", sprite_id);
		return nullptr;
	}

	// Store in stable deque
	region_storage_.push_back(*region);
	AtlasRegion* ptr = &region_storage_.back();

	// Store pointer in hash map
	ptr->debug_sprite_id = sprite_id;
	sprite_regions_.emplace(sprite_id, ptr);

	// Also store in direct lookup for O(1) access
	if (sprite_id < DIRECT_LOOKUP_SIZE) {
		direct_lookup_[sprite_id] = ptr;
	}

	return ptr;
}

const AtlasRegion* AtlasManager::addSprite(uint32_t sprite_id, const uint8_t* rgba_data) {
	std::unique_lock lock(atlas_mutex_);
	return addSpriteUnlocked(sprite_id, rgba_data);
}

void AtlasManager::removeSprite(uint32_t sprite_id) {
	std::unique_lock lock(atlas_mutex_);
	if (sprite_id == WHITE_PIXEL_ID) {
		white_pixel_cache_ = nullptr;
	}

	AtlasRegion* region = nullptr;

	if (sprite_id < DIRECT_LOOKUP_SIZE) {
		if (direct_lookup_[sprite_id] != nullptr) {
			region = direct_lookup_[sprite_id];
			direct_lookup_[sprite_id] = nullptr;
			sprite_regions_.erase(sprite_id);
		}
	} else {
		auto it = sprite_regions_.find(sprite_id);
		if (it != sprite_regions_.end()) {
			region = it->second;
			sprite_regions_.erase(it);
		}
	}

	if (region) {
		// 1. Free the slot in the texture array (requires valid UVs)
		atlas_.freeSlot(*region);

		// 2. MARK AS INVALID to trigger Self-Healing in stale GameSprites
		// This prevents "Double Allocation" visual bugs where a stale sprite references
		// this region object after the slot has been reused for a new sprite.
		region->debug_sprite_id = AtlasRegion::INVALID_SENTINEL;
		region->atlas_index = AtlasRegion::INVALID_SENTINEL;
	}
}

void AtlasManager::clearMapping(uint32_t sprite_id) {
	std::unique_lock lock(atlas_mutex_);
	if (sprite_id == WHITE_PIXEL_ID) {
		white_pixel_cache_ = nullptr;
	}

	if (sprite_id < DIRECT_LOOKUP_SIZE) {
		direct_lookup_[sprite_id] = nullptr;
	}
	sprite_regions_.erase(sprite_id);
}

const AtlasRegion* AtlasManager::getRegion(uint32_t sprite_id) const {
	std::shared_lock lock(atlas_mutex_);
	return getRegionUnlocked(sprite_id);
}

const AtlasRegion* AtlasManager::getWhitePixel() const {
	std::shared_lock lock(atlas_mutex_);
	if (white_pixel_cache_) {
		return white_pixel_cache_;
	}
	if (auto it = sprite_regions_.find(WHITE_PIXEL_ID); it != sprite_regions_.end()) {
		return it->second;
	}
	// Should have been initialized in ensureInitialized()
	return nullptr;
}

bool AtlasManager::hasSprite(uint32_t sprite_id) const {
	std::shared_lock lock(atlas_mutex_);
	return getRegionUnlocked(sprite_id) != nullptr;
}

void AtlasManager::bind(uint32_t slot) const {
	std::shared_lock lock(atlas_mutex_);
	atlas_.bind(slot);
}

GLuint AtlasManager::getTextureId() const {
	std::shared_lock lock(atlas_mutex_);
	return atlas_.id();
}

void AtlasManager::clear() {
	std::unique_lock lock(atlas_mutex_);
	atlas_.release();
	region_storage_.clear();
	sprite_regions_.clear();
	std::fill(direct_lookup_.begin(), direct_lookup_.end(), nullptr);
	white_pixel_cache_ = nullptr;
	spdlog::info("AtlasManager cleared");
}
