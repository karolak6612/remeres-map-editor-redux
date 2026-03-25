#ifndef RME_RENDERING_CORE_GRAPHICS_RUNTIME_CONFIG_H_
#define RME_RENDERING_CORE_GRAPHICS_RUNTIME_CONFIG_H_

#include "app/settings.h"

struct GraphicsRuntimeConfig {
	int texture_longevity = 60;
	bool use_memcached_sprites = false;
	int software_clean_threshold = 512;
	int software_clean_size = 128;
	bool texture_management = true;
	int texture_clean_threshold = 2048;
	int texture_clean_pulse = 5;
	int icon_background = 0;

	[[nodiscard]] static GraphicsRuntimeConfig FromSettings(const Settings& settings) {
		return GraphicsRuntimeConfig {
			.texture_longevity = settings.getInteger(Config::TEXTURE_LONGEVITY),
			.use_memcached_sprites = settings.getBoolean(Config::USE_MEMCACHED_SPRITES),
			.software_clean_threshold = settings.getInteger(Config::SOFTWARE_CLEAN_THRESHOLD),
			.software_clean_size = settings.getInteger(Config::SOFTWARE_CLEAN_SIZE),
			.texture_management = settings.getInteger(Config::TEXTURE_MANAGEMENT) != 0,
			.texture_clean_threshold = settings.getInteger(Config::TEXTURE_CLEAN_THRESHOLD),
			.texture_clean_pulse = settings.getInteger(Config::TEXTURE_CLEAN_PULSE),
			.icon_background = settings.getInteger(Config::ICON_BACKGROUND),
		};
	}
};

#endif
