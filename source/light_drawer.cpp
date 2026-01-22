#include "main.h"
#include "light_drawer.h"

LightDrawer::LightDrawer() :
	vbo(GLBuffer::VERTEX) {
	global_color = wxColor(50, 50, 50, 255);
}

LightDrawer::~LightDrawer() {
	lights.clear();
}

void LightDrawer::draw(int map_x, int map_y, int end_x, int end_y, int scroll_x, int scroll_y, bool fog) {
	int w = end_x - map_x;
	int h = end_y - map_y;

	buffer.resize(static_cast<size_t>(w * h * PixelFormatRGBA));

	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			int mx = (map_x + x);
			int my = (map_y + y);
			int index = (y * w + x);
			int color_index = index * PixelFormatRGBA;

			buffer[color_index] = global_color.Red();
			buffer[color_index + 1] = global_color.Green();
			buffer[color_index + 2] = global_color.Blue();
			buffer[color_index + 3] = 140; // global_color.Alpha();

			for (auto& light : lights) {
				float intensity = calculateIntensity(mx, my, light);
				if (intensity == 0.f) {
					continue;
				}
				wxColor light_color = colorFromEightBit(light.color);
				uint8_t red = static_cast<uint8_t>(light_color.Red() * intensity);
				uint8_t green = static_cast<uint8_t>(light_color.Green() * intensity);
				uint8_t blue = static_cast<uint8_t>(light_color.Blue() * intensity);
				buffer[color_index] = std::max(buffer[color_index], red);
				buffer[color_index + 1] = std::max(buffer[color_index + 1], green);
				buffer[color_index + 2] = std::max(buffer[color_index + 2], blue);
			}
		}
	}

	const float draw_x = (float)(map_x * TileSize - scroll_x);
	const float draw_y = (float)(map_y * TileSize - scroll_y);
	const float draw_width = (float)(w * TileSize);
	const float draw_height = (float)(h * TileSize);

	texture.bind();
	texture.setFilter(GL_LINEAR, GL_LINEAR);
	texture.setWrap(0x812F, 0x812F); // GL_CLAMP_TO_EDGE
	texture.upload(w, h, buffer.data());

	if (!fog) {
		glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	}

	glColor4ub(255, 255, 255, 255); // reset color
	glEnable(GL_TEXTURE_2D);

	struct QuadVert {
		float x, y;
		float u, v;
	};

	QuadVert verts[4] = {
		{ draw_x, draw_y, 0.f, 0.f },
		{ draw_x + draw_width, draw_y, 1.f, 0.f },
		{ draw_x + draw_width, draw_y + draw_height, 1.f, 1.f },
		{ draw_x, draw_y + draw_height, 0.f, 1.f }
	};

	vbo.setData(verts, sizeof(verts), GL_DYNAMIC_DRAW);
	vbo.bind();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(2, GL_FLOAT, sizeof(QuadVert), (const void*)offsetof(QuadVert, x));
	glTexCoordPointer(2, GL_FLOAT, sizeof(QuadVert), (const void*)offsetof(QuadVert, u));

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	vbo.unbind();
	texture.unbind();

	glDisable(GL_TEXTURE_2D);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (fog) {
		glColor4ub(10, 10, 10, 80);

		QuadVert fogVerts[4] = {
			{ draw_x, draw_y, 0.f, 0.f },
			{ draw_x + draw_width, draw_y, 0.f, 0.f },
			{ draw_x + draw_width, draw_y + draw_height, 0.f, 0.f },
			{ draw_x, draw_y + draw_height, 0.f, 0.f }
		};
		vbo.setData(fogVerts, sizeof(fogVerts), GL_DYNAMIC_DRAW);
		vbo.bind();

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof(QuadVert), (const void*)offsetof(QuadVert, x));

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
		vbo.unbind();
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
