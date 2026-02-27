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
#include <limits>
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
	if (!shader) {
		initRenderResources();
	}

	if (!fbo) {
		InitFBO();
	}

	// NEW: Calculate dimensions based on screen viewport size, NOT map size.
	// This fixes the issue where huge maps cause huge FBOs that exceed max texture size.
	// We only need to render light for what's visible on screen (plus margin).

	// Logical viewport size (zoomed)
	// screensize_x is physical pixels.
	// logical width = screensize_x * zoom (if zoom < 1, logical is larger than screen)
	// logical width = screensize_x / zoom (wait, zoom is scaling factor)
	// render_view.cpp: projectionMatrix = glm::ortho(0.0f, width * zoom, height * zoom, ...)
	// So logical unit size is (width * zoom).

	// Let's use the RenderView's logical dimensions directly if available or recalculate.
	float logical_w = view.screensize_x * view.zoom;
	float logical_h = view.screensize_y * view.zoom;

	// Add margin for lights just off-screen
	// Max light radius is approx 10 tiles (intenstiy 8-10).
	// Let's add 16 tiles margin on each side to be safe.
	float margin = 16 * TILE_SIZE;

	// FBO dimensions (pixels) - keep it reasonably sized
	// We can't make the FBO arbitrarily huge.
	// If zoomed out extremely (zoom=10.0), logical_w = screen_w * 10.
	// If screen=1920, w=19200. This exceeds 16384 texture limit.
	// But `zoom` in RenderView seems to mean "How many screen pixels per map unit"? No.
	// In MapCanvas: SetZoom(1.0) -> 1:1. SetZoom(0.5) -> 50% size (zoomed out).
	// RenderView::Setup: zoom = canvas->GetZoom().
	// tile_size = TILE_SIZE / zoom? No, RenderView code: tile_size = TILE_SIZE / zoom (integer division?).
	// Wait, if zoom=0.5 (small tiles), view.zoom is 0.5?
	// RenderView::SetupGL: glOrtho(0, width * zoom, ...)
	// If zoom is 0.5, logical width is 0.5 * width? That means we see LESS?
	// Usually ortho(0, width/zoom) lets us see MORE.
	// Let's re-read RenderView::SetupGL:
	// projectionMatrix = glm::ortho(0.0f, width * zoom, height * zoom, 0.0f, -1.0f, 1.0f);
	// If zoom = 2.0 (magnified), ortho width is 2x screen width?
	// If I zoom IN (big tiles), I see FEWER tiles. Logical width should be SMALLER.
	// If `zoom` factor follows RME convention: 1.0 = 100%. 2.0 = 200% (Magnified). 0.5 = 50% (Minified).
	// If zoom=2.0, we want to map 0..Width to 0..Width/2 logical units?
	// Actually RME `zoom` variable in RenderView seems to be "Inverse Scale" or "View Scale"?
	// Let's trust logical_width from RenderView (cached).
	// view.logical_width = screensize_x * zoom.
	// If zoom=1.0, logical=screen.
	// If zoom=4.0 (zoomed out, tiny tiles?), logical=screen*4. This matches "seeing more".
	// So `zoom` here is essentially "How many map units fit in a screen unit" or similar.
	// If zoom > 8.0, texture size might explode.
	// BUT, we only draw lights if view.zoom <= 10.0 (MapLayerDrawer check).

	int fbo_w = static_cast<int>(logical_w + margin * 2);
	int fbo_h = static_cast<int>(logical_h + margin * 2);

	// Clamp to GL_MAX_TEXTURE_SIZE (e.g. 16384)
	GLint max_tex_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
	if (fbo_w > max_tex_size) fbo_w = max_tex_size;
	if (fbo_h > max_tex_size) fbo_h = max_tex_size;

	// Resize FBO if needed
	if (buffer_width < fbo_w || buffer_height < fbo_h) {
		// Re-create texture if we need to grow
		fbo_texture = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);
		ResizeFBO(std::max(buffer_width, fbo_w), std::max(buffer_height, fbo_h));
		glNamedFramebufferTexture(fbo->GetID(), GL_COLOR_ATTACHMENT0, fbo_texture->GetID(), 0);
	}

	// Calculate FBO origin in MAP PIXELS
	// We want FBO centered on the viewport.
	// Viewport origin in map pixels:
	float view_x = static_cast<float>(view.view_scroll_x);
	float view_y = static_cast<float>(view.view_scroll_y);

	// Note: view_scroll_x includes "offset" from floor adjustment?
	// RenderView::IsTileVisible uses: out_x = (map_x * TILE_SIZE) - view_scroll_x - offset;
	// offset depends on Z. LightBuffer stores absolute map X/Y.
	// MapLayerDrawer calculates offset per layer.
	// LightDrawer iterates ALL lights. They have Z.
	// LightBuffer::AddLight normalizes X/Y for underground layers (subtracting offset).
	// So light.map_x/y are in "Screen-aligned Map Grid" space relative to the current floor logic?
	// LightBuffer::AddLight: if (map_z <= GROUND_LAYER) map_x -= (GROUND_LAYER - map_z).
	// This matches the offset logic: offset = (GROUND_LAYER - map_z) * TILE_SIZE.
	// So LightBuffer coordinates are already adjusted to align with Floor 7 grid?
	// Actually:
	// RenderView Offset: (GROUND_LAYER - map_z) * TILE_SIZE.
	// Tile Draw X: map_x * 32 - scroll_x - offset.
	// = (map_x * 32 - offset) - scroll_x.
	// = (map_x - (GROUND_LAYER - map_z)) * 32 - scroll_x.
	// LightBuffer stores (map_x - (GROUND_LAYER - map_z)).
	// So Light Draw X = light.stored_x * 32 - scroll_x.
	// Matches!

	float fbo_origin_x = view_x - margin;
	float fbo_origin_y = view_y - margin;

	// Prepare Lights
	gpu_lights_.clear();
	gpu_lights_.reserve(light_buffer.lights.size());

	for (const auto& light : light_buffer.lights) {
		int radius_px = light.intensity * TILE_SIZE; // Tight radius
		float lx_px = light.map_x * TILE_SIZE + TILE_SIZE / 2.0f;
		float ly_px = light.map_y * TILE_SIZE + TILE_SIZE / 2.0f;

		// Cull lights outside FBO
		// FBO covers [fbo_origin_x, fbo_origin_x + fbo_w]
		if (lx_px + radius_px < fbo_origin_x || lx_px - radius_px > fbo_origin_x + fbo_w ||
			ly_px + radius_px < fbo_origin_y || ly_px - radius_px > fbo_origin_y + fbo_h) {
			continue;
		}

		wxColor c = colorFromEightBit(light.color);

		// Position relative to FBO
		float rel_x = lx_px - fbo_origin_x;
		float rel_y = ly_px - fbo_origin_y;

		gpu_lights_.push_back({ .position = { rel_x, rel_y }, .intensity = static_cast<float>(light.intensity), .padding = 0.0f,
								.color = { (c.Red() / 255.0f) * light_intensity, (c.Green() / 255.0f) * light_intensity, (c.Blue() / 255.0f) * light_intensity, 1.0f } });
	}

	if (gpu_lights_.empty()) {
		// Just render ambient? We still need to clear the FBO/screen area or simpy fill it.
		// If no lights, the overlay should just be ambient color.
	} else {
		// Upload Lights
		size_t needed_size = gpu_lights_.size() * sizeof(GPULight);
		if (needed_size > light_ssbo_capacity_) {
			light_ssbo_capacity_ = std::max(needed_size, static_cast<size_t>(light_ssbo_capacity_ * 1.5));
			if (light_ssbo_capacity_ < 1024) {
				light_ssbo_capacity_ = 1024;
			}
			glNamedBufferData(light_ssbo->GetID(), light_ssbo_capacity_, nullptr, GL_DYNAMIC_DRAW);
		}
		glNamedBufferSubData(light_ssbo->GetID(), 0, needed_size, gpu_lights_.data());
	}

	// 3. Render to FBO
	{
		ScopedGLFramebuffer fboScope(GL_FRAMEBUFFER, fbo->GetID());
		ScopedGLViewport viewportScope(0, 0, buffer_width, buffer_height);

		// Clear to Ambient Color
		float ambient_r = (global_color.Red() / 255.0f) * ambient_light_level;
		float ambient_g = (global_color.Green() / 255.0f) * ambient_light_level;
		float ambient_b = (global_color.Blue() / 255.0f) * ambient_light_level;

		// If global_color is (0,0,0) (not set), use a default dark ambient
		if (global_color.Red() == 0 && global_color.Green() == 0 && global_color.Blue() == 0) {
			ambient_r = 0.5f * ambient_light_level;
			ambient_g = 0.5f * ambient_light_level;
			ambient_b = 0.5f * ambient_light_level;
		}

		glClearColor(ambient_r, ambient_g, ambient_b, 1.0f);
		// Actually, for "Max" blending, we want to start with Ambient.
		glClear(GL_COLOR_BUFFER_BIT);

		if (!gpu_lights_.empty()) {
			shader->Use();

			// Setup Projection for FBO: Ortho 0..buffer_width, buffer_height..0 (Y-down)
			// This matches screen coordinate system and avoids flips
			glm::mat4 fbo_projection = glm::ortho(0.0f, static_cast<float>(buffer_width), static_cast<float>(buffer_height), 0.0f);
			shader->SetMat4("uProjection", fbo_projection);
			shader->SetFloat("uTileSize", static_cast<float>(TILE_SIZE));

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, light_ssbo->GetID());
			glBindVertexArray(vao->GetID());

			// Enable MAX blending
			{
				ScopedGLCapability blendCap(GL_BLEND);
				ScopedGLBlend blendState(GL_ONE, GL_ONE, GL_MAX); // Factors don't matter much for MAX, but usually 1,1 is safe

				if (gpu_lights_.size() > static_cast<size_t>(std::numeric_limits<GLsizei>::max())) {
					spdlog::error("Too many lights for glDrawArraysInstanced");
					return;
				}
				glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(gpu_lights_.size()));
			}

			glBindVertexArray(0);
		}
	}

	// 4. Composite FBO to Screen
	shader->Use();
	shader->SetInt("uMode", 1); // Composite Mode

	// Bind FBO texture
	glBindTextureUnit(0, fbo_texture->GetID());
	shader->SetInt("uTexture", 0);

	// Draw Quad at FBO Origin
	// fbo_origin_x/y is where the FBO starts in World (Screen) Coordinates.
	// fbo_w/h is the size we rendered to.
	float draw_dest_x = fbo_origin_x; // Relative to view_scroll_x, but projection matrix expects coordinates shifted by view_scroll_x?
	// RenderView::SetupGL:
	// projection = ortho(0, width*zoom, ...).
	// viewMatrix = translate(0.375, ...).
	// The View Matrix does NOT contain view_scroll_x.
	// Standard drawing in MapLayerDrawer:
	// out_x = (map_x * 32) - view_scroll_x - offset.
	// So vertex positions are "Screen Coordinates" relative to (0,0) top-left of viewport.
	// fbo_origin_x = view_scroll_x + ... No wait.
	// fbo_origin_x = view_x - margin = view_scroll_x - margin.
	// BUT "view_x" in my variable above `float view_x = static_cast<float>(view.view_scroll_x);`
	// So fbo_origin_x is in "Global Map Pixel Space".
	// To convert to "Screen Space", we must subtract view.view_scroll_x.

	// Screen X = Map X - Scroll X.
	// Draw X = fbo_origin_x - view.view_scroll_x
	//        = (view_scroll_x - margin) - view_scroll_x
	//        = -margin.
	// Correct! We draw the quad starting at -margin, -margin.

	float screen_x = fbo_origin_x - view.view_scroll_x;
	float screen_y = fbo_origin_y - view.view_scroll_y;

	// Apply Projection
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(screen_x, screen_y, 0.0f));
	model = glm::scale(model, glm::vec3((float)fbo_w, (float)fbo_h, 1.0f));
	shader->SetMat4("uProjection", view.projectionMatrix * view.viewMatrix * model);

	// UVs: We used fbo_w/fbo_h pixels of the texture.
	// Texture is buffer_width x buffer_height.
	float uv_w = static_cast<float>(fbo_w) / static_cast<float>(buffer_width);
	float uv_h = static_cast<float>(fbo_h) / static_cast<float>(buffer_height);

	// Y-flip handled as before: Map Top (Y=0 local) -> V=1. Map Bottom (Y=fbo_h) -> V=1-uv_h.
	shader->SetVec2("uUVMin", glm::vec2(0.0f, 1.0f));
	shader->SetVec2("uUVMax", glm::vec2(uv_w, 1.0f - uv_h));

	// Blending: Dst * Src
	{
		ScopedGLCapability blendCap(GL_BLEND);
		ScopedGLBlend blendState(GL_DST_COLOR, GL_ZERO);

		glBindVertexArray(vao->GetID());
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

	shader->SetInt("uMode", 0); // Reset
}

void LightDrawer::initRenderResources() {
	// Modes: 0 = Light Generation (Instanced), 1 = Composite (Simple Texture)
	const char* vs = R"(
		#version 450 core
		layout (location = 0) in vec2 aPos; // 0..1 Quad
		
		uniform int uMode;
		uniform mat4 uProjection;
		uniform float uTileSize;
		uniform vec2 uUVMin;
		uniform vec2 uUVMax;

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
		out vec4 FragColor; // For Mode 0

		void main() {
			if (uMode == 0) {
				// LIGHT GENERATION
				Light l = uLights[gl_InstanceID];
				
				// Calculate quad size/pos in FBO space
				// Radius spans 0..1 in distance math
				// Quad size should cover the light radius
				// light.intensity is in 'tiles'. 
				// The falloff is 1.0 at center, 0.0 at radius = intensity * TILE_SIZE.
				float radiusPx = l.intensity * uTileSize;
				float size = radiusPx * 2.0;
				
				// Center position
				vec2 center = l.position;
				
				// Vertex Pos (0..1) -> Local Pos (-size/2 .. +size/2) -> World Pos
				vec2 localPos = (aPos - 0.5) * size;
				vec2 worldPos = center + localPos;
				
				gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);
				
				// Pass data to fragment
				TexCoord = aPos - 0.5; // -0.5 to 0.5
				FragColor = l.color;
			} else {
				// COMPOSITE
				gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
				TexCoord = mix(uUVMin, uUVMax, aPos); 
			}
		}
	)";

	const char* fs = R"(
		#version 450 core
		in vec2 TexCoord;
		in vec4 FragColor; // From VS
		
		uniform int uMode;
		uniform sampler2D uTexture;

		out vec4 OutColor;

		void main() {
			if (uMode == 0) {
				// Light Falloff
				// TexCoord is -0.5 to 0.5
				float dist = length(TexCoord) * 2.0; // 0.0 to 1.0 (at edge of quad)
				if (dist > 1.0) discard;
				
				float falloff = 1.0 - dist;
				// Smooth it a bit?
				// falloff = offset - (distance * 0.2)? Legacy formula:
				// float intensity = (-distance + light.intensity) * 0.2f;
				// Here we just use linear falloff for simplicity or match formula
				// Visual approximation is fine for now.
				
				OutColor = FragColor * falloff;
			} else {
				// Texture fetch
				OutColor = texture(uTexture, TexCoord);
			}
		}
	)";

	shader = std::make_unique<ShaderProgram>();
	shader->Load(vs, fs);

	float vertices[] = {
		0.0f, 0.0f, // BL
		1.0f, 0.0f, // BR
		1.0f, 1.0f, // TR
		0.0f, 1.0f // TL
	};

	vao = std::make_unique<GLVertexArray>();
	vbo = std::make_unique<GLBuffer>();
	light_ssbo = std::make_unique<GLBuffer>();

	glNamedBufferData(vbo->GetID(), sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexArrayVertexBuffer(vao->GetID(), 0, vbo->GetID(), 0, 2 * sizeof(float));

	glEnableVertexArrayAttrib(vao->GetID(), 0);
	glVertexArrayAttribFormat(vao->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao->GetID(), 0, 0);

	glBindVertexArray(0);
}

