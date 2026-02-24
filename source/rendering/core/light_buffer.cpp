#include "light_buffer.h"

void LightBuffer::AddLight(int map_x, int map_y, int map_z, const SpriteLight& light) {
	if (map_z <= GROUND_LAYER) {
		map_x -= (GROUND_LAYER - map_z);
		map_y -= (GROUND_LAYER - map_z);
	}
	AddLight(map_x, map_y, light);
}

void LightBuffer::AddLight(int map_x, int map_y, const SpriteLight& light) {
	if (map_x <= 0 || map_x >= MAP_MAX_WIDTH || map_y <= 0 || map_y >= MAP_MAX_HEIGHT) {
		return;
	}

	uint8_t intensity = light.intensity;

	// O(1) deduplication using hash map
	uint64_t key = (static_cast<uint64_t>(map_x) << 32) | (static_cast<uint64_t>(map_y) << 16) | static_cast<uint64_t>(light.color);
	auto it = light_index_map.find(key);
	if (it != light_index_map.end()) {
		Light& existing = lights[it->second];
		existing.intensity = std::max(existing.intensity, intensity);
	} else {
		light_index_map[key] = lights.size();
		lights.push_back(Light { static_cast<uint16_t>(map_x), static_cast<uint16_t>(map_y), light.color, intensity });
	}
}

void LightBuffer::Clear() {
	lights.clear();
	light_index_map.clear();
}
