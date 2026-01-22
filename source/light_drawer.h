#ifndef RME_LIGHDRAWER_H
#define RME_LIGHDRAWER_H

#include "graphics.h"
#include "position.h"
#include "gl_wrappers/gl_texture.h"
#include "gl_wrappers/gl_buffer.h"

class LightDrawer {
	struct Light {
		uint16_t map_x = 0;
		uint16_t map_y = 0;
		uint8_t color = 0;
		uint8_t intensity = 0;
	};

public:
	LightDrawer();
	virtual ~LightDrawer();

	void draw(int map_x, int map_y, int end_x, int end_y, int scroll_x, int scroll_y, bool fog);

	void setGlobalLightColor(uint8_t color);
	void addLight(int map_x, int map_y, int map_z, const SpriteLight& light);
	void clear() noexcept;

private:
	inline float calculateIntensity(int map_x, int map_y, const Light& light) {
		int dx = map_x - light.map_x;
		int dy = map_y - light.map_y;
		float distance = std::sqrt(dx * dx + dy * dy);
		if (distance > MaxLightIntensity) {
			return 0.f;
		}
		float intensity = (-distance + light.intensity) * 0.2f;
		if (intensity < 0.01f) {
			return 0.f;
		}
		return std::min(intensity, 1.f);
	}

	GLTexture texture;
	std::vector<Light> lights;
	std::vector<uint8_t> buffer;
	wxColor global_color;

	// Quad VBO
	GLBuffer vbo;
};

#endif
