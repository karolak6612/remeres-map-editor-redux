#ifndef RME_RENDERING_CORE_LIGHT_BUFFER_H_
#define RME_RENDERING_CORE_LIGHT_BUFFER_H_

#include <vector>
#include <cstdint>
#include <algorithm>
#include "rendering/core/sprite_light.h"
#include "app/definitions.h"

// SoA (Structure of Arrays) layout for cache-friendly iteration
// in the light drawing pass. Each array is iterated independently,
// improving prefetch utilization on the GPU upload path.
struct LightBuffer {
	std::vector<uint16_t> map_x;
	std::vector<uint16_t> map_y;
	std::vector<uint8_t> color;
	std::vector<uint8_t> intensity;

	void AddLight(int map_x, int map_y, int map_z, const SpriteLight& light);
	void Clear();
	[[nodiscard]] size_t size() const { return map_x.size(); }

	void reserve(size_t capacity) {
		map_x.reserve(capacity);
		map_y.reserve(capacity);
		color.reserve(capacity);
		intensity.reserve(capacity);
	}
};

#endif
