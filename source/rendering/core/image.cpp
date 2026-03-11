#include "rendering/core/image.h"

#include "rendering/core/graphics.h"

GraphicManager& Image::graphics() const {
	ASSERT(graphics_ != nullptr);
	return *graphics_;
}

Image::Image() :
	isGLLoaded(false),
	lastaccess(0) {
}

void Image::visit() const {
	lastaccess.store(static_cast<int64_t>(graphics().getCachedTime()), std::memory_order_relaxed);
}

void Image::clean(time_t time, int longevity) {
	static_cast<void>(time);
	static_cast<void>(longevity);
}

const AtlasRegion* Image::EnsureAtlasSprite(uint32_t sprite_id, std::unique_ptr<uint8_t[]> preloaded_data) {
	if (!graphics().ensureAtlasManager()) {
		spdlog::error("AtlasManager not available for sprite_id={}", sprite_id);
		return nullptr;
	}

	AtlasManager* atlas_mgr = graphics().getAtlasManager();

	const AtlasRegion* region = atlas_mgr->getRegion(sprite_id);
	if (region) {
		if (region->debug_sprite_id == AtlasRegion::INVALID_SENTINEL || (region->debug_sprite_id != 0 && region->debug_sprite_id != sprite_id)) {
			spdlog::warn("STALE/INVALID MAP ENTRY DETECTED: Sprite {} maps to region owned by {}. Clearing mapping.", sprite_id, region->debug_sprite_id);
			atlas_mgr->clearMapping(sprite_id);
			region = nullptr;
		} else {
			return region;
		}
	}

	std::unique_ptr<uint8_t[]> rgba = preloaded_data ? std::move(preloaded_data) : getRGBAData();
	if (!rgba) {
		constexpr int kSpriteDimension = 32;
		constexpr int kRgbaComponents = 4;
		rgba = std::make_unique<uint8_t[]>(kSpriteDimension * kSpriteDimension * kRgbaComponents);
		std::span<uint8_t> buffer(rgba.get(), kSpriteDimension * kSpriteDimension * kRgbaComponents);
		for (int i : std::views::iota(0, kSpriteDimension * kSpriteDimension)) {
			buffer[i * kRgbaComponents + 0] = 255;
			buffer[i * kRgbaComponents + 1] = 0;
			buffer[i * kRgbaComponents + 2] = 255;
			buffer[i * kRgbaComponents + 3] = 255;
		}
		spdlog::warn("getRGBAData returned null for sprite_id={} - using fallback", sprite_id);
	}

	region = atlas_mgr->addSprite(sprite_id, rgba.get());
	if (!region) {
		spdlog::warn("Atlas addSprite failed for sprite_id={}", sprite_id);
		return nullptr;
	}

	if (!isGLLoaded) {
		isGLLoaded = true;
		graphics().trackResidentImage(this);
	}
	graphics().notifyTextureLoaded();
	return region;
}
