#ifndef RME_RENDERING_CORE_LIGHT_BUFFER_H_
#define RME_RENDERING_CORE_LIGHT_BUFFER_H_

#include <vector>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include "rendering/core/sprite_light.h"
#include "app/definitions.h"

struct LightBuffer {
	struct Light {
		uint16_t map_x = 0;
		uint16_t map_y = 0;
		uint8_t color = 0;
		uint8_t intensity = 0;
	};

	std::vector<Light> lights;
	std::unordered_map<uint64_t, size_t> light_index_map;

	void AddLight(int map_x, int map_y, int map_z, const SpriteLight& light);
	void AddLight(int map_x, int map_y, const SpriteLight& light);
	void Clear();
};

#endif
