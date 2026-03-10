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

#include "app/main.h"
#include "rendering/utilities/light_drawer.h"
#include "rendering/utilities/light_fbo.h"
#include "rendering/utilities/light_shader.h"
#include "rendering/utilities/light_calculator.h"
#include <glm/gtc/matrix_transform.hpp>
#include "rendering/core/drawing_options.h"
#include "rendering/core/render_view.h"
#include "rendering/core/gl_scoped_state.h"

LightDrawer::LightDrawer() = default;
LightDrawer::~LightDrawer() = default;

void LightDrawer::draw(const ViewState& view, bool fog, const LightBuffer& light_buffer, const DrawColor& global_color, float light_intensity, float ambient_light_level) {
	// Lazy-init resources on first draw
	if (!shader_) {
		shader_ = std::make_unique<LightShader>();
	}
	if (!fbo_) {
		fbo_ = std::make_unique<LightFBO>();
	}

	int buffer_w = view.screensize_x;
	int buffer_h = view.screensize_y;

	if (buffer_w <= 0 || buffer_h <= 0) {
		return;
	}

	// 1. Ensure FBO is large enough for the visible screen area
	fbo_->EnsureSize(buffer_w, buffer_h);

	// 2. Filter and convert lights to GPU format
	gpu_lights_.clear();
	gpu_lights_.reserve(light_buffer.lights.size());

	for (const auto& light : light_buffer.lights) {
		int lx_px = light.map_x * TILE_SIZE + TILE_SIZE / 2;
		int ly_px = light.map_y * TILE_SIZE + TILE_SIZE / 2;

		float map_pos_x = static_cast<float>(lx_px - view.view_scroll_x);
		float map_pos_y = static_cast<float>(ly_px - view.view_scroll_y);

		float screen_x = map_pos_x / view.zoom;
		float screen_y = map_pos_y / view.zoom;
		float screen_radius = (light.intensity * TILE_SIZE) / view.zoom;

		// Frustum culling
		if (screen_x + screen_radius < 0 || screen_x - screen_radius > buffer_w || screen_y + screen_radius < 0 || screen_y - screen_radius > buffer_h) {
			continue;
		}

		wxColor c = colorFromEightBit(light.color);

		gpu_lights_.push_back({ .position = { screen_x, screen_y }, .intensity = static_cast<float>(light.intensity), .padding = 0.0f,
								.color = { (c.Red() / 255.0f) * light_intensity, (c.Green() / 255.0f) * light_intensity, (c.Blue() / 255.0f) * light_intensity, 1.0f } });
	}

	// 3. Upload light data to SSBO
	if (!gpu_lights_.empty()) {
		shader_->Upload(gpu_lights_);
	}

	// 4. Render to FBO
	{
		ScopedGLFramebuffer fboScope(GL_FRAMEBUFFER, fbo_->GetFBOID());
		ScopedGLViewport viewportScope(0, 0, buffer_w, buffer_h);

		// Clear to ambient color
		float ambient_r = (global_color.r / 255.0f) * ambient_light_level;
		float ambient_g = (global_color.g / 255.0f) * ambient_light_level;
		float ambient_b = (global_color.b / 255.0f) * ambient_light_level;

		if (global_color.r == 0 && global_color.g == 0 && global_color.b == 0) {
			ambient_r = 0.5f * ambient_light_level;
			ambient_g = 0.5f * ambient_light_level;
			ambient_b = 0.5f * ambient_light_level;
		}

		glClearColor(ambient_r, ambient_g, ambient_b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (!gpu_lights_.empty()) {
			glm::mat4 fbo_projection = glm::ortho(0.0f, static_cast<float>(buffer_w), static_cast<float>(buffer_h), 0.0f);
			float tileSize = static_cast<float>(TILE_SIZE) / view.zoom;
			shader_->DrawLightPass(fbo_projection, tileSize, static_cast<int>(gpu_lights_.size()));
		}
	}

	// 5. Composite FBO to screen
	float draw_dest_w = static_cast<float>(buffer_w) * view.zoom;
	float draw_dest_h = static_cast<float>(buffer_h) * view.zoom;

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(draw_dest_w, draw_dest_h, 1.0f));
	glm::mat4 mvp = view.projectionMatrix * view.viewMatrix * model;

	float uv_w = static_cast<float>(buffer_w) / static_cast<float>(fbo_->GetWidth());
	float uv_h = static_cast<float>(buffer_h) / static_cast<float>(fbo_->GetHeight());

	shader_->DrawComposite(mvp, fbo_->GetTextureID(), glm::vec2(0.0f, uv_h), glm::vec2(uv_w, 0.0f));
}
