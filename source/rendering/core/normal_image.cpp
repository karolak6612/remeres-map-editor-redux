#include "rendering/core/normal_image.h"
#include "rendering/core/game_sprite.h"
#include "app/settings.h"
#include "ui/gui.h"
#include <spdlog/spdlog.h>

constexpr int RGB_COMPONENTS = 3;

NormalImage::NormalImage() :
	id(0),
	atlas_region(nullptr),
	size(0),
	dump(nullptr) {
}

NormalImage::~NormalImage() {
	// dump auto-deleted
	unloadGL();
}

void NormalImage::unloadGL() {
	if (isGLLoaded) {
		if (g_gui.atlas.hasAtlasManager()) {
			g_gui.atlas.getAtlasManager()->removeSprite(id);
		}
		
		isGLLoaded = false;
		atlas_region = nullptr;
		generation_id++;

		g_gui.gc.removeResidentImage(this);
	}
}

void NormalImage::fulfillPreload(std::unique_ptr<uint8_t[]> data) {
	atlas_region = EnsureAtlasSprite(id, std::move(data));
}

void NormalImage::clean(time_t time, int longevity) {
	// Evict from atlas if expired
	if (longevity == -1) {
		longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);
	}
	if (isGLLoaded && time - static_cast<time_t>(lastaccess.load(std::memory_order_relaxed)) > longevity) {
		if (parent) {
			parent->invalidateCache(atlas_region);
		}
		unloadGL();
	}

	if (time - static_cast<time_t>(lastaccess.load(std::memory_order_relaxed)) > 5 && !g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) { // We keep dumps around for 5 seconds.
		dump.reset();
	}
}

bool NormalImage::ensureDumpLoaded() {
	if (!dump) {
		if (g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
			return false;
		}

		if (!g_gui.loader.loadSpriteDump(dump, size, id)) {
			return false;
		}
	}
	return true;
}

std::unique_ptr<uint8_t[]> NormalImage::getRGBData() {
	if (id == 0) {
		const int pixels_data_size = SPRITE_PIXELS * SPRITE_PIXELS * RGB_COMPONENTS;
		return std::make_unique<uint8_t[]>(pixels_data_size); // Value-initialized (zeroed)
	}

	if (!ensureDumpLoaded()) {
		return nullptr;
	}

	const int pixels_data_size = SPRITE_PIXELS * SPRITE_PIXELS * RGB_COMPONENTS;
	auto data = std::make_unique<uint8_t[]>(pixels_data_size);
	uint8_t bpp = g_gui.loader.hasTransparency() ? 4 : RGB_COMPONENTS;
	size_t write = 0;
	size_t read = 0;

	// decompress pixels
	while (read < size && write < static_cast<size_t>(pixels_data_size)) {
		if (read + 1 >= size) {
			spdlog::warn("NormalImage::getRGBData: Transparency header truncated (read={}, size={})", read, size);
			break;
		}
		int transparent = dump[read] | dump[read + 1] << 8;
		read += 2;
		for (int cnt = 0; cnt < transparent && write < static_cast<size_t>(pixels_data_size); ++cnt) {
			data[write + 0] = 0xFF; // red
			data[write + 1] = 0x00; // green
			data[write + 2] = 0xFF; // blue
			write += RGB_COMPONENTS;
		}

		if (read + 1 >= size) {
			spdlog::warn("NormalImage::getRGBData: Colored header truncated (read={}, size={})", read, size);
			break;
		}

		int colored = dump[read] | dump[read + 1] << 8;
		read += 2;

		if (read + static_cast<size_t>(colored) * bpp > size) {
			spdlog::warn("NormalImage::getRGBData: Read buffer overrun (colored={}, bpp={}, read={}, size={})", colored, bpp, read, size);
			break;
		}

		for (int cnt = 0; cnt < colored && write < static_cast<size_t>(pixels_data_size); ++cnt) {
			data[write + 0] = dump[read + 0]; // red
			data[write + 1] = dump[read + 1]; // green
			data[write + 2] = dump[read + 2]; // blue
			write += RGB_COMPONENTS;
			read += bpp;
		}
	}

	// fill remaining pixels
	while (write < static_cast<size_t>(pixels_data_size)) {
		data[write + 0] = 0xFF; // red
		data[write + 1] = 0x00; // green
		data[write + 2] = 0xFF; // blue
		write += RGB_COMPONENTS;
	}
	return data;
}

std::unique_ptr<uint8_t[]> NormalImage::getRGBAData() {
	// Robust ID 0 handling
	if (id == 0) {
		const int pixels_data_size = SPRITE_PIXELS_SIZE * 4;
		return std::make_unique<uint8_t[]>(pixels_data_size); // Value-initialized (zeroed)
	}

	if (!ensureDumpLoaded()) {
		// This is the only case where we return nullptr for non-zero ID
		// effectively warning the caller that the sprite is missing from file
		return nullptr;
	}

	return GameSprite::Decompress(std::span { dump.get(), size }, g_gui.loader.hasTransparency(), id);
}

const AtlasRegion* NormalImage::getAtlasRegion() {
	if (isGLLoaded && atlas_region) {
		// Self-Healing: Check for stale atlas region pointer (e.g. from memory reuse)
		// Force reload if Owner is INVALID or DOES NOT MATCH
		if (atlas_region->debug_sprite_id == AtlasRegion::INVALID_SENTINEL || (atlas_region->debug_sprite_id != 0 && atlas_region->debug_sprite_id != id)) {
			spdlog::warn("STALE ATLAS REGION DETECTED: NormalImage {} held region owned by {}. Force reloading.", id, atlas_region->debug_sprite_id);
			isGLLoaded = false;
			atlas_region = nullptr;
		} else {
			visit();
			return atlas_region;
		}
	}

	if (!isGLLoaded) {
		atlas_region = EnsureAtlasSprite(id);
	}
	visit();
	return atlas_region;
}
