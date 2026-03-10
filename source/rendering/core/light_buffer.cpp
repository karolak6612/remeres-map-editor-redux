#include "light_buffer.h"

void LightBuffer::AddLight(int in_map_x, int in_map_y, int map_z, const SpriteLight& light) {
	if (map_z <= GROUND_LAYER) {
		in_map_x -= (GROUND_LAYER - map_z);
		in_map_y -= (GROUND_LAYER - map_z);
	}

	if (in_map_x <= 0 || in_map_x >= MAP_MAX_WIDTH || in_map_y <= 0 || in_map_y >= MAP_MAX_HEIGHT) {
		return;
	}

	uint8_t in_intensity = std::min(light.intensity, static_cast<uint8_t>(255));

	// Merge with previous light at same position and color
	if (!map_x.empty()) {
		size_t last = map_x.size() - 1;
		if (map_x[last] == in_map_x && map_y[last] == in_map_y && color[last] == light.color) {
			intensity[last] = std::max(intensity[last], in_intensity);
			return;
		}
	}

	map_x.push_back(static_cast<uint16_t>(in_map_x));
	map_y.push_back(static_cast<uint16_t>(in_map_y));
	color.push_back(light.color);
	intensity.push_back(in_intensity);
}

void LightBuffer::Clear() {
	map_x.clear();
	map_y.clear();
	color.clear();
	intensity.clear();
}
