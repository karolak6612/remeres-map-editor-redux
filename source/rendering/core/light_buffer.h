#ifndef RME_RENDERING_CORE_LIGHT_BUFFER_H_
#define RME_RENDERING_CORE_LIGHT_BUFFER_H_

#include <vector>
#include <cstdint>
#include <algorithm>
#include "rendering/core/sprite_light.h"
#include "rendering/core/light_sink.h"
#include "app/definitions.h"

struct LightBuffer : public ILightSink {
	struct Light {
		uint16_t map_x = 0;
		uint16_t map_y = 0;
		uint8_t color = 0;
		uint8_t intensity = 0;
	};

	std::vector<Light> lights;

	void AddLight(int map_x, int map_y, int map_z, const SpriteLight& light) override;
	void Clear();
};

#endif
