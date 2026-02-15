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
#include "rendering/utilities/light_calculator.h"
#include <glm/gtc/matrix_transform.hpp>
#include <format>
#include "map/tile.h"
#include "game/item.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/render_view.h"
#include "rendering/core/gl_scoped_state.h"

// GPULight struct moved to header

LightDrawer::LightDrawer() {
}

LightDrawer::~LightDrawer() {
	// Resources are RAII managed
}

void LightDrawer::InitFBO() {
	fbo = std::make_unique<GLFramebuffer>();
	fbo_texture = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);

	// Initial dummy size
	ResizeFBO(32, 32);

	glNamedFramebufferTexture(fbo->GetID(), GL_COLOR_ATTACHMENT0, fbo_texture->GetID(), 0);

	GLenum status = glCheckNamedFramebufferStatus(fbo->GetID(), GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		spdlog::error("LightDrawer FBO Incomplete: {}", status);
	}
}

void LightDrawer::ResizeFBO(int width, int height) {
	if (width == buffer_width && height == buffer_height) {
		return;
	}

	buffer_width = width;
	buffer_height = height;

	glTextureStorage2D(fbo_texture->GetID(), 1, GL_RGBA8, width, height);

	// Set texture parameters
	glTextureParameteri(fbo_texture->GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(fbo_texture->GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(fbo_texture->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(fbo_texture->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void LightDrawer::draw(const RenderView& view, bool fog, const LightBuffer& light_buffer, const wxColor& global_color, float light_intensity, float ambient_light_level) {
	if (!generation_shader || !composite_shader) {
		initRenderResources();
	}

	if (!fbo) {
		InitFBO();
	}

	int map_x = view.start_x;
	int map_y = view.start_y;
	int end_x = view.end_x;
	int end_y = view.end_y;

	int w = end_x - map_x;
	int h = end_y - map_y;

	if (w <= 0 || h <= 0) {
		return;
	}

	int TileSize = 32;
	int pixel_width = w * TileSize;
	int pixel_height = h * TileSize;

	// 1. Resize FBO if needed (ensure it covers the visible map area)
	if (buffer_width < pixel_width || buffer_height < pixel_height) {
		fbo_texture = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);
		ResizeFBO(std::max(buffer_width, pixel_width), std::max(buffer_height, pixel_height));
		glNamedFramebufferTexture(fbo->GetID(), GL_COLOR_ATTACHMENT0, fbo_texture->GetID(), 0);
	}

	// 2. Prepare Lights
	gpu_lights_.clear();
	gpu_lights_.reserve(light_buffer.lights.size());

	float map_draw_start_x = (float)(map_x * TileSize - view.view_scroll_x);
	float map_draw_start_y = (float)(map_y * TileSize - view.view_scroll_y);

	float fbo_origin_x = (float)(map_x * TileSize);
	float fbo_origin_y = (float)(map_y * TileSize);

	for (const auto& light : light_buffer.lights) {
		// Cull lights that are definitely out of FBO range
		int radius_px = light.intensity * TileSize + 16;
		int lx_px = light.map_x * TileSize + TileSize / 2;
		int ly_px = light.map_y * TileSize + TileSize / 2;

		if (lx_px + radius_px < fbo_origin_x || lx_px - radius_px > fbo_origin_x + pixel_width || ly_px + radius_px < fbo_origin_y || ly_px - radius_px > fbo_origin_y + pixel_height) {
			continue;
		}

		wxColor c = colorFromEightBit(light.color);

		float rel_x = lx_px - fbo_origin_x;
		float rel_y = ly_px - fbo_origin_y;

		gpu_lights_.push_back({ .position = { rel_x, rel_y }, .intensity = static_cast<float>(light.intensity), .padding = 0.0f,
								.color = { (c.Red() / 255.0f) * light_intensity, (c.Green() / 255.0f) * light_intensity, (c.Blue() / 255.0f) * light_intensity, 1.0f } });
	}

	// 3. Render to FBO
	{
		ScopedGLFramebuffer fboScope(GL_FRAMEBUFFER, fbo->GetID());
		ScopedGLViewport viewportScope(0, 0, buffer_width, buffer_height);

		// Clear to Ambient Color
		float ambient_r = (global_color.Red() / 255.0f) * ambient_light_level;
		float ambient_g = (global_color.Green() / 255.0f) * ambient_light_level;
		float ambient_b = (global_color.Blue() / 255.0f) * ambient_light_level;

		if (global_color.Red() == 0 && global_color.Green() == 0 && global_color.Blue() == 0) {
			ambient_r = 0.5f * ambient_light_level;
			ambient_g = 0.5f * ambient_light_level;
			ambient_b = 0.5f * ambient_light_level;
		}

		glClearColor(ambient_r, ambient_g, ambient_b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (!gpu_lights_.empty()) {
			// Cap lights to buffer capacity
			if (gpu_lights_.size() > light_ssbo->getMaxElements()) {
				gpu_lights_.resize(light_ssbo->getMaxElements());
			}

			// Map RingBuffer for persistent upload
			void* ptr = light_ssbo->waitAndMap(gpu_lights_.size());
			if (ptr) {
				std::memcpy(ptr, gpu_lights_.data(), gpu_lights_.size() * sizeof(GPULight));
				light_ssbo->finishWrite();

				generation_shader->Use();

				// Setup Projection for FBO: Ortho 0..buffer_width, buffer_height..0 (Y-down)
				glm::mat4 fbo_projection = glm::ortho(0.0f, (float)buffer_width, (float)buffer_height, 0.0f);
				generation_shader->SetMat4("uProjection", fbo_projection);
				generation_shader->SetFloat("uTileSize", (float)TileSize);

				// Bind RingBuffer range as SSBO
				// Using offset from RingBuffer to handle triple buffering
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, light_ssbo->getBufferId(), light_ssbo->getCurrentSectionOffset(), gpu_lights_.size() * sizeof(GPULight));

				glBindVertexArray(vao->GetID());

				{
					ScopedGLCapability blendCap(GL_BLEND);
					ScopedGLBlend blendState(GL_ONE, GL_ONE, GL_MAX);

					glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, (GLsizei)gpu_lights_.size());
				}

				glBindVertexArray(0);

				light_ssbo->signalFinished();
			}
		}
	}

	// 4. Composite FBO to Screen
	composite_shader->Use();

	// Bind FBO texture
	glBindTextureUnit(0, fbo_texture->GetID());
	composite_shader->SetInt("uTexture", 0);

	// Quad Transform for Screen
	float draw_dest_x = map_draw_start_x;
	float draw_dest_y = map_draw_start_y;
	float draw_dest_w = (float)pixel_width;
	float draw_dest_h = (float)pixel_height;

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(draw_dest_x, draw_dest_y, 0.0f));
	model = glm::scale(model, glm::vec3(draw_dest_w, draw_dest_h, 1.0f));
	composite_shader->SetMat4("uProjection", view.projectionMatrix * view.viewMatrix * model);

	float uv_w = (float)pixel_width / (float)buffer_width;
	float uv_h = (float)pixel_height / (float)buffer_height;

	composite_shader->SetVec2("uUVMin", glm::vec2(0.0f, 1.0f));
	composite_shader->SetVec2("uUVMax", glm::vec2(uv_w, 1.0f - uv_h));

	// Blending: Dst * Src
	{
		ScopedGLCapability blendCap(GL_BLEND);
		ScopedGLBlend blendState(GL_DST_COLOR, GL_ZERO);

		glBindVertexArray(vao->GetID());
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}
}

void LightDrawer::initRenderResources() {
	const char* gen_vs = R"(
		#version 450 core
		layout (location = 0) in vec2 aPos;
		
		uniform mat4 uProjection;
		uniform float uTileSize;

		struct Light {
			vec2 position; 
			float intensity;
			float padding;
			vec4 color;
		};
		layout(std430, binding = 0) buffer LightBlock {
			Light uLights[];
		};

		out vec2 TexCoord;
		out vec4 FragColor;

		void main() {
			Light l = uLights[gl_InstanceID];
			float radiusPx = l.intensity * uTileSize;
			float size = radiusPx * 2.0;
			vec2 center = l.position;
			vec2 localPos = (aPos - 0.5) * size;
			vec2 worldPos = center + localPos;
			gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);
			TexCoord = aPos - 0.5;
			FragColor = l.color;
		}
	)";

	const char* gen_fs = R"(
		#version 450 core
		in vec2 TexCoord;
		in vec4 FragColor;
		out vec4 OutColor;

		void main() {
			float dist = length(TexCoord) * 2.0;
			if (dist > 1.0) discard;
			float falloff = 1.0 - dist;
			OutColor = FragColor * falloff;
		}
	)";

	const char* comp_vs = R"(
		#version 450 core
		layout (location = 0) in vec2 aPos;
		
		uniform mat4 uProjection;
		uniform vec2 uUVMin;
		uniform vec2 uUVMax;

		out vec2 TexCoord;

		void main() {
			gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
			TexCoord = mix(uUVMin, uUVMax, aPos);
		}
	)";

	const char* comp_fs = R"(
		#version 450 core
		in vec2 TexCoord;
		uniform sampler2D uTexture;
		out vec4 OutColor;

		void main() {
			OutColor = texture(uTexture, TexCoord);
		}
	)";

	generation_shader = std::make_unique<ShaderProgram>();
	generation_shader->Load(gen_vs, gen_fs);

	composite_shader = std::make_unique<ShaderProgram>();
	composite_shader->Load(comp_vs, comp_fs);

	float vertices[] = {
		0.0f, 0.0f, // BL
		1.0f, 0.0f, // BR
		1.0f, 1.0f, // TR
		0.0f, 1.0f // TL
	};

	vao = std::make_unique<GLVertexArray>();
	vbo = std::make_unique<GLBuffer>();
	light_ssbo = std::make_unique<RingBuffer>();

	// Initialize persistent buffer for lights (8192 capacity)
	light_ssbo->initialize(sizeof(GPULight), 8192);

	glNamedBufferData(vbo->GetID(), sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexArrayVertexBuffer(vao->GetID(), 0, vbo->GetID(), 0, 2 * sizeof(float));

	glEnableVertexArrayAttrib(vao->GetID(), 0);
	glVertexArrayAttribFormat(vao->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao->GetID(), 0, 0);

	glBindVertexArray(0);
}
