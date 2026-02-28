#include "rendering/core/template_image.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/outfit_colors.h"
#include "app/settings.h"
#include "ui/gui.h"
#include <atomic>
#include <spdlog/spdlog.h>

static std::atomic<uint32_t> template_id_generator(0x1000000);

TemplateImage::TemplateImage(GameSprite* parent, int v, const Outfit& outfit) :
	atlas_region(nullptr),
	texture_id(template_id_generator.fetch_add(1)), // Generate unique ID for Atlas
	parent(parent),
	sprite_index(v),
	lookHead(outfit.lookHead),
	lookBody(outfit.lookBody),
	lookLegs(outfit.lookLegs),
	lookFeet(outfit.lookFeet) {
}

TemplateImage::~TemplateImage() {
	if (isGLLoaded) {
		if (g_gui.gfx.hasAtlasManager()) {
			g_gui.gfx.getAtlasManager()->removeSprite(texture_id);
		}
	}
}

void TemplateImage::clean(time_t time, int longevity) {
	// Evict from atlas if expired
	if (longevity == -1) {
		longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);
	}
	if (isGLLoaded && time - static_cast<time_t>(lastaccess.load()) > longevity) {
		if (g_gui.gfx.hasAtlasManager()) {
			g_gui.gfx.getAtlasManager()->removeSprite(texture_id);
		}
		isGLLoaded = false;
		atlas_region = nullptr;
		generation_id++;
		g_gui.gfx.collector.NotifyTextureUnloaded();
	}
}

std::unique_ptr<uint8_t[]> TemplateImage::getRGBData() {
	if (!parent) {
		spdlog::warn("TemplateImage (texture_id={}): Invalid parent reference.", texture_id);
		return nullptr;
	}

	if (parent->width <= 0 || parent->height <= 0) {
		spdlog::warn("TemplateImage (texture_id={}): Invalid parent dimensions ({}x{})", texture_id, parent->width, parent->height);
		return nullptr;
	}

	size_t mask_index = sprite_index + parent->height * parent->width;
	if (sprite_index >= parent->spriteList.size() || mask_index >= parent->spriteList.size()) {
		spdlog::warn("TemplateImage (texture_id={}): Access index out of bounds (base_index={}, mask_index={}, list_size={})", texture_id, sprite_index, mask_index, parent->spriteList.size());
		return nullptr;
	}

	auto rgbdata = parent->spriteList[sprite_index]->getRGBData();
	auto template_rgbdata = parent->spriteList[mask_index]->getRGBData();

	if (!rgbdata) {
		// template_rgbdata auto-deleted
		return nullptr;
	}
	if (!template_rgbdata) {
		// rgbdata auto-deleted
		return nullptr;
	}

	if (lookHead > TemplateOutfitLookupTableSize) {
		lookHead = 0;
	}
	if (lookBody > TemplateOutfitLookupTableSize) {
		lookBody = 0;
	}
	if (lookLegs > TemplateOutfitLookupTableSize) {
		lookLegs = 0;
	}
	if (lookFeet > TemplateOutfitLookupTableSize) {
		lookFeet = 0;
	}

	GameSprite::ColorizeTemplatePixels(rgbdata.get(), template_rgbdata.get(), SPRITE_PIXELS * SPRITE_PIXELS, lookHead, lookBody, lookLegs, lookFeet, false);

	// template_rgbdata auto-deleted
	return rgbdata;
}

std::unique_ptr<uint8_t[]> TemplateImage::getRGBAData() {
	if (!parent) {
		spdlog::warn("TemplateImage (texture_id={}): Invalid parent reference.", texture_id);
		return nullptr;
	}

	if (parent->width <= 0 || parent->height <= 0) {
		spdlog::warn("TemplateImage (texture_id={}): Invalid parent dimensions ({}x{})", texture_id, parent->width, parent->height);
		return nullptr;
	}

	size_t mask_index = sprite_index + parent->height * parent->width;
	if (sprite_index >= parent->spriteList.size() || mask_index >= parent->spriteList.size()) {
		spdlog::warn("TemplateImage (texture_id={}): Access index out of bounds (base_index={}, mask_index={}, list_size={})", texture_id, sprite_index, mask_index, parent->spriteList.size());
		return nullptr;
	}

	auto rgbadata = parent->spriteList[sprite_index]->getRGBAData();
	auto template_rgbdata = parent->spriteList[mask_index]->getRGBData();

	if (!rgbadata) {
		spdlog::warn("TemplateImage: Failed to load BASE sprite data for sprite_index={} (template_id={}). Parent width={}, height={}", sprite_index, texture_id, parent->width, parent->height);
		// template_rgbdata auto-deleted
		return nullptr;
	}
	if (!template_rgbdata) {
		spdlog::warn("TemplateImage: Failed to load MASK sprite data for sprite_index={} (template_id={}) (mask_index={})", sprite_index, texture_id, sprite_index + parent->height * parent->width);
		// rgbadata auto-deleted
		return nullptr;
	}

	if (lookHead > TemplateOutfitLookupTableSize) {
		lookHead = 0;
	}
	if (lookBody > TemplateOutfitLookupTableSize) {
		lookBody = 0;
	}
	if (lookLegs > TemplateOutfitLookupTableSize) {
		lookLegs = 0;
	}
	if (lookFeet > TemplateOutfitLookupTableSize) {
		lookFeet = 0;
	}

	// Note: the base data is RGBA (4 channels) while the mask data is RGB (3 channels).
	GameSprite::ColorizeTemplatePixels(rgbadata.get(), template_rgbdata.get(), SPRITE_PIXELS * SPRITE_PIXELS, lookHead, lookBody, lookLegs, lookFeet, true);

	// template_rgbdata auto-deleted
	return rgbadata;
}

const AtlasRegion* TemplateImage::getAtlasRegion() {
	if (isGLLoaded && atlas_region) {
		// Self-Healing: Check for stale atlas region pointer
		if (atlas_region->debug_sprite_id == AtlasRegion::INVALID_SENTINEL || (atlas_region->debug_sprite_id != 0 && atlas_region->debug_sprite_id != texture_id)) {
			spdlog::warn("STALE ATLAS REGION DETECTED: TemplateImage {} held region owned by {}. Force reloading.", texture_id, atlas_region->debug_sprite_id);
			isGLLoaded = false;
			atlas_region = nullptr;
		} else {
			visit();
			return atlas_region;
		}
	}

	if (!isGLLoaded) {
		atlas_region = EnsureAtlasSprite(texture_id);
	}
	visit();
	return atlas_region;
}
