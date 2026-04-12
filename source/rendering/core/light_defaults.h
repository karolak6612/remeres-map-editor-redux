#ifndef RME_RENDERING_CORE_LIGHT_DEFAULTS_H_
#define RME_RENDERING_CORE_LIGHT_DEFAULTS_H_

#include <cstdint>

namespace rme::lighting {
	inline constexpr uint8_t DEFAULT_SERVER_LIGHT_INTENSITY = 255;
	inline constexpr uint8_t DEFAULT_SERVER_LIGHT_COLOR = 215;
	inline constexpr float DEFAULT_MINIMUM_AMBIENT_LIGHT = 0.0f;
	inline constexpr uint8_t MAX_SERVER_LIGHT_COLOR = 215;
	inline constexpr int LIGHT_PALETTE_SIZE = MAX_SERVER_LIGHT_COLOR + 1;
}

#endif
