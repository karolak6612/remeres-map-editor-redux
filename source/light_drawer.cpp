//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "light_drawer.h"

LightDrawer::LightDrawer() {
	texture = 0;
	global_color = wxColor(50, 50, 50, 255);
}

LightDrawer::~LightDrawer() {
	unloadGLTexture();

	lights.clear();
}

void LightDrawer::draw(int map_x, int map_y, int end_x, int end_y, int scroll_x, int scroll_y, bool fog) {
	if (texture == 0) {
		createGLTexture();
	}

	int w = end_x - map_x;
	int h = end_y - map_y;
	size_t buffer_size = static_cast<size_t>(w * h * PixelFormatRGBA);

	if (buffer.size() != buffer_size) {
		buffer.resize(buffer_size);
	}

	// Initialize buffer with global color
	uint8_t gr = global_color.Red();
	uint8_t gg = global_color.Green();
	uint8_t gb = global_color.Blue();
	uint8_t ga = 140; // global_color.Alpha();

	for (size_t i = 0; i < buffer_size; i += PixelFormatRGBA) {
		buffer[i] = gr;
		buffer[i + 1] = gg;
		buffer[i + 2] = gb;
		buffer[i + 3] = ga;
	}

	for (const auto& light : lights) {
		if (light.intensity == 0) {
			continue;
		}

		int radius = light.intensity;
		int lx_min = light.map_x - radius;
		int lx_max = light.map_x + radius;
		int ly_min = light.map_y - radius;
		int ly_max = light.map_y + radius;

		// Clip to view
		int start_x_rel = std::max(0, lx_min - map_x);
		int end_x_rel = std::min(w, lx_max - map_x + 1);
		int start_y_rel = std::max(0, ly_min - map_y);
		int end_y_rel = std::min(h, ly_max - map_y + 1);

		if (start_x_rel >= end_x_rel || start_y_rel >= end_y_rel) {
			continue;
		}

		wxColor light_color = colorFromEightBit(light.color);
		uint8_t lr = light_color.Red();
		uint8_t lg = light_color.Green();
		uint8_t lb = light_color.Blue();

		for (int y = start_y_rel; y < end_y_rel; ++y) {
			int my = map_y + y;
			int dy = my - light.map_y;
			int dy2 = dy * dy;
			size_t row_idx = y * w * PixelFormatRGBA;

			for (int x = start_x_rel; x < end_x_rel; ++x) {
				int mx = map_x + x;
				int dx = mx - light.map_x;
				int dist_sq = dx * dx + dy2;

				if (dist_sq > radius * radius) {
					continue;
				}

				float distance = std::sqrt(static_cast<float>(dist_sq));
				// Assuming calculateIntensity logic here to avoid function call overhead
				if (distance > MaxLightIntensity) {
					continue;
				}

				float intensity = (-distance + light.intensity) * 0.2f;
				if (intensity < 0.01f) {
					continue;
				}
				if (intensity > 1.f) {
					intensity = 1.f;
				}

				size_t idx = row_idx + x * PixelFormatRGBA;
				buffer[idx] = std::max(buffer[idx], static_cast<uint8_t>(lr * intensity));
				buffer[idx + 1] = std::max(buffer[idx + 1], static_cast<uint8_t>(lg * intensity));
				buffer[idx + 2] = std::max(buffer[idx + 2], static_cast<uint8_t>(lb * intensity));
			}
		}
	}

	const int draw_x = map_x * TileSize - scroll_x;
	const int draw_y = map_y * TileSize - scroll_y;
	int draw_width = w * TileSize;
	int draw_height = h * TileSize;

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

	if (!fog) {
		glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	}

	glColor4ub(255, 255, 255, 255); // reset color
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex2f(draw_x, draw_y);
	glTexCoord2f(1.f, 0.f);
	glVertex2f(draw_x + draw_width, draw_y);
	glTexCoord2f(1.f, 1.f);
	glVertex2f(draw_x + draw_width, draw_y + draw_height);
	glTexCoord2f(0.f, 1.f);
	glVertex2f(draw_x, draw_y + draw_height);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (fog) {
		glColor4ub(10, 10, 10, 80);
		glBegin(GL_QUADS);
		glVertex2f(draw_x, draw_y);
		glVertex2f(draw_x + draw_width, draw_y);
		glVertex2f(draw_x + draw_width, draw_y + draw_height);
		glVertex2f(draw_x, draw_y + draw_height);
		glEnd();
	}
}

void LightDrawer::setGlobalLightColor(uint8_t color) {
	global_color = colorFromEightBit(color);
}

void LightDrawer::addLight(int map_x, int map_y, int map_z, const SpriteLight& light) {
	if (map_z <= GROUND_LAYER) {
		map_x -= (GROUND_LAYER - map_z);
		map_y -= (GROUND_LAYER - map_z);
	}

	if (map_x <= 0 || map_x >= MAP_MAX_WIDTH || map_y <= 0 || map_y >= MAP_MAX_HEIGHT) {
		return;
	}

	uint8_t intensity = std::min(light.intensity, static_cast<uint8_t>(MaxLightIntensity));

	if (!lights.empty()) {
		Light& previous = lights.back();
		if (previous.map_x == map_x && previous.map_y == map_y && previous.color == light.color) {
			previous.intensity = std::max(previous.intensity, intensity);
			return;
		}
	}

	lights.push_back(Light { static_cast<uint16_t>(map_x), static_cast<uint16_t>(map_y), light.color, intensity });
}

void LightDrawer::clear() noexcept {
	lights.clear();
}

void LightDrawer::createGLTexture() {
	glGenTextures(1, &texture);
	ASSERT(texture == 0);
}

void LightDrawer::unloadGLTexture() {
	if (texture != 0) {
		glDeleteTextures(1, &texture);
	}
}
