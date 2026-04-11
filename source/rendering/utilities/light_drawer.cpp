#include "app/main.h"
#include "rendering/utilities/light_drawer.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "rendering/core/drawing_options.h"
#include "rendering/core/gl_scoped_state.h"
#include "rendering/core/render_view.h"

namespace {
	[[nodiscard]] glm::vec3 normalizedPaletteColor(uint8_t color_index) {
		const wxColor color = colorFromEightBit(color_index);
		return {
			color.Red() / 255.0f,
			color.Green() / 255.0f,
			color.Blue() / 255.0f
		};
	}

	[[nodiscard]] glm::vec3 ambientColorForView(const RenderView& view, const DrawingOptions& options) {
		const bool above_ground = view.floor <= GROUND_LAYER;
		uint8_t ambient_color_index = above_ground ? options.server_light.color : static_cast<uint8_t>(215);
		if (ambient_color_index == 0) {
			ambient_color_index = 215;
		}
		const float server_intensity = above_ground ? options.server_light.intensity / 255.0f : 0.0f;
		const float ambient_intensity = std::max(options.minimum_ambient_light, server_intensity);
		return normalizedPaletteColor(ambient_color_index) * ambient_intensity;
	}
}

LightDrawer::LightDrawer() = default;

LightDrawer::~LightDrawer() = default;

void LightDrawer::markDirty() {
	dirty_ = true;
}

void LightDrawer::computeBrightness(const LightBuffer& light_buffer, const DrawingOptions& options) {
	const int tw = light_buffer.width;
	const int th = light_buffer.height;
	const int light_count = static_cast<int>(light_buffer.lights.size());

	// --- Initialize tile-resolution brightness buffer with ambient ---
	const size_t tile_pixels = static_cast<size_t>(tw) * static_cast<size_t>(th);
	tile_brightness_.resize(tile_pixels * 4);

	const uint8_t ambient_r = static_cast<uint8_t>(std::min(255.0f, options.minimum_ambient_light * 255.0f));
	const uint8_t ambient_g = static_cast<uint8_t>(std::min(255.0f, options.minimum_ambient_light * 255.0f));
	const uint8_t ambient_b = static_cast<uint8_t>(std::min(255.0f, options.minimum_ambient_light * 255.0f));

	for (size_t i = 0; i < tile_pixels; ++i) {
		const size_t idx = i * 4;
		tile_brightness_[idx]     = ambient_r;
		tile_brightness_[idx + 1] = ambient_g;
		tile_brightness_[idx + 2] = ambient_b;
		tile_brightness_[idx + 3] = 255;
	}

	// --- Per-light accumulation: iterate lights first, then only tiles in range ---
	for (int i = 0; i < light_count; ++i) {
		const auto& light = light_buffer.lights[static_cast<size_t>(i)];
		if (light.intensity == 0) continue;

		const float intensity_f = static_cast<float>(light.intensity);
		const int light_tx = light.tile_x - light_buffer.origin_x;
		const int light_ty = light.tile_y - light_buffer.origin_y;

		// Effective radius in tiles (OTClient formula: intensity is already in tile-equivalent units)
		const int radius = static_cast<int>(std::ceil(intensity_f));

		const int min_tx = std::max(0, light_tx - radius);
		const int max_tx = std::min(tw - 1, light_tx + radius);
		const int min_ty = std::max(0, light_ty - radius);
		const int max_ty = std::min(th - 1, light_ty + radius);

		const auto lc = normalizedPaletteColor(light.color);

		for (int ty = min_ty; ty <= max_ty; ++ty) {
			for (int tx = min_tx; tx <= max_tx; ++tx) {
				const size_t tile_idx = static_cast<size_t>(ty) * static_cast<size_t>(tw) + static_cast<size_t>(tx);
				const size_t start = light_buffer.tiles[tile_idx].start;

				// Occlusion gate: skip if this light is before the tile's start index
				if (static_cast<size_t>(i) < start) continue;

				// Distance in tile units
				const float dx = static_cast<float>(tx - light_tx);
				const float dy = static_cast<float>(ty - light_ty);
				const float dist = std::sqrt(dx * dx + dy * dy);

				// OTClient falloff formula
				float intensity = (-dist + intensity_f) * 0.2f;
				if (intensity < 0.01f) continue;
				if (intensity > 1.0f) intensity = 1.0f;

				const size_t buf_idx = tile_idx * 4;
				const uint8_t lr = static_cast<uint8_t>(std::min(255.0f, lc.r * intensity * 255.0f));
				const uint8_t lg = static_cast<uint8_t>(std::min(255.0f, lc.g * intensity * 255.0f));
				const uint8_t lb = static_cast<uint8_t>(std::min(255.0f, lc.b * intensity * 255.0f));

				// Per-channel MAX
				if (lr > tile_brightness_[buf_idx])     tile_brightness_[buf_idx]     = lr;
				if (lg > tile_brightness_[buf_idx + 1]) tile_brightness_[buf_idx + 1] = lg;
				if (lb > tile_brightness_[buf_idx + 2]) tile_brightness_[buf_idx + 2] = lb;
			}
		}
	}

	cached_tw_ = tw;
	cached_th_ = th;
	dirty_ = false;
}

void LightDrawer::draw(const RenderView& view, const LightBuffer& light_buffer, const DrawingOptions& options) {
	if (!shader_) {
		initRenderResources();
	}

	const int screen_w = view.screensize_x;
	const int screen_h = view.screensize_y;
	if (screen_w <= 0 || screen_h <= 0) {
		return;
	}

	const int tw = light_buffer.width;
	const int th = light_buffer.height;
	if (tw <= 0 || th <= 0) {
		return;
	}

	// --- Recompute brightness only when dirty (map changed) ---
	// Auto-detect parameter changes (sliders, toggle) and light content changes
	if (!dirty_) {
		const float current_ambient = options.minimum_ambient_light;
		const uint8_t current_intensity = options.server_light.intensity;
		const uint8_t current_color = options.server_light.color;
		const bool current_show = options.show_lights;
		if (current_ambient != cached_ambient_ || current_intensity != cached_server_intensity_ ||
			current_color != cached_server_color_ || current_show != cached_show_lights_) {
			dirty_ = true;
		}
	}

	// Detect light content changes (count + simple position hash)
	if (!dirty_) {
		const size_t current_count = light_buffer.lights.size();
		size_t current_hash = current_count;
		// Simple hash: combine light positions
		const int sample_count = std::min(static_cast<int>(current_count), 100);
		for (int i = 0; i < sample_count; ++i) {
			const auto& light = light_buffer.lights[static_cast<size_t>(i)];
			current_hash ^= static_cast<size_t>(light.tile_x) * 31 + static_cast<size_t>(light.tile_y) * 17 + static_cast<size_t>(light.color) * 7 + static_cast<size_t>(light.intensity) * 3;
			current_hash = (current_hash << 5) | (current_hash >> (sizeof(size_t) * 8 - 5)); // rotate
		}
		if (current_count != cached_light_count_ || current_hash != cached_light_hash_) {
			dirty_ = true;
		}
	}

	if (dirty_ || cached_tw_ != tw || cached_th_ != th) {
		computeBrightness(light_buffer, options);
		cached_ambient_ = options.minimum_ambient_light;
		cached_server_intensity_ = options.server_light.intensity;
		cached_server_color_ = options.server_light.color;
		cached_show_lights_ = options.show_lights;
		cached_light_count_ = light_buffer.lights.size();
		cached_light_hash_ = 0;
		const int sample = std::min(static_cast<int>(light_buffer.lights.size()), 100);
		for (int i = 0; i < sample; ++i) {
			const auto& light = light_buffer.lights[static_cast<size_t>(i)];
			cached_light_hash_ ^= static_cast<size_t>(light.tile_x) * 31 + static_cast<size_t>(light.tile_y) * 17 + static_cast<size_t>(light.color) * 7 + static_cast<size_t>(light.intensity) * 3;
			cached_light_hash_ = (cached_light_hash_ << 5) | (cached_light_hash_ >> (sizeof(size_t) * 8 - 5));
		}
	}

	// --- Expand tile-resolution brightness to screen resolution with bilinear interpolation ---
	screen_pixels_.resize(static_cast<size_t>(screen_w) * static_cast<size_t>(screen_h) * 4);

	for (int sy = 0; sy < screen_h; ++sy) {
		const float tile_fy = (sy * view.zoom + static_cast<float>(view.view_scroll_y)) / static_cast<float>(TILE_SIZE) - static_cast<float>(light_buffer.origin_y);
		const int ty = static_cast<int>(std::clamp(std::floor(tile_fy), 0.0f, static_cast<float>(th - 1)));
		const int next_ty = std::min(ty + 1, th - 1);
		const float ty_frac = tile_fy - static_cast<float>(ty);

		for (int sx = 0; sx < screen_w; ++sx) {
			const float tile_fx = (sx * view.zoom + static_cast<float>(view.view_scroll_x)) / static_cast<float>(TILE_SIZE) - static_cast<float>(light_buffer.origin_x);
			const int tx = static_cast<int>(std::clamp(std::floor(tile_fx), 0.0f, static_cast<float>(tw - 1)));
			const int next_tx = std::min(tx + 1, tw - 1);
			const float tx_frac = tile_fx - static_cast<float>(tx);

			const size_t idx00 = (static_cast<size_t>(ty) * static_cast<size_t>(tw) + static_cast<size_t>(tx)) * 4;
			const size_t idx10 = (static_cast<size_t>(ty) * static_cast<size_t>(tw) + static_cast<size_t>(next_tx)) * 4;
			const size_t idx01 = (static_cast<size_t>(next_ty) * static_cast<size_t>(tw) + static_cast<size_t>(tx)) * 4;
			const size_t idx11 = (static_cast<size_t>(next_ty) * static_cast<size_t>(tw) + static_cast<size_t>(next_tx)) * 4;

			const size_t si = (static_cast<size_t>(sy) * static_cast<size_t>(screen_w) + static_cast<size_t>(sx)) * 4;

			for (int c = 0; c < 4; ++c) {
				const float top    = tile_brightness_[idx00 + c] * (1.0f - tx_frac) + tile_brightness_[idx10 + c] * tx_frac;
				const float bottom = tile_brightness_[idx01 + c] * (1.0f - tx_frac) + tile_brightness_[idx11 + c] * tx_frac;
				screen_pixels_[si + c] = static_cast<uint8_t>(top * (1.0f - ty_frac) + bottom * ty_frac);
			}
		}
	}

	// --- Upload screen-resolution texture ---
	if (!light_texture_ || tex_width_ != screen_w || tex_height_ != screen_h) {
		light_texture_ = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);
		tex_width_ = screen_w;
		tex_height_ = screen_h;
	}

	glTextureStorage2D(light_texture_->GetID(), 1, GL_RGBA8, screen_w, screen_h);
	glTextureParameteri(light_texture_->GetID(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(light_texture_->GetID(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(light_texture_->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(light_texture_->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureSubImage2D(light_texture_->GetID(), 0, 0, 0, screen_w, screen_h, GL_RGBA, GL_UNSIGNED_BYTE, screen_pixels_.data());

	// --- Composite ---
	glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(
		static_cast<float>(screen_w) * view.zoom,
		static_cast<float>(screen_h) * view.zoom,
		1.0f
	));

	shader_->Use();
	shader_->SetInt("uTexture", 0);
	shader_->SetMat4("uMVP", view.projectionMatrix * view.viewMatrix * model);

	glBindTextureUnit(0, light_texture_->GetID());

	{
		ScopedGLCapability blend_capability(GL_BLEND);
		ScopedGLBlend blend_state(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);

		glBindVertexArray(vao_->GetID());
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}
}

void LightDrawer::initRenderResources() {
	constexpr auto vertex_shader = R"(
		#version 450 core
		layout (location = 0) in vec2 aPos;

		uniform mat4 uMVP;
		out vec2 TexCoord;

		void main() {
			gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
			TexCoord = vec2(aPos.x, aPos.y);
		}
	)";

	constexpr auto fragment_shader = R"(
		#version 450 core
		in vec2 TexCoord;
		uniform sampler2D uTexture;
		out vec4 OutColor;

		void main() {
			OutColor = texture(uTexture, TexCoord);
		}
	)";

	shader_ = std::make_unique<ShaderProgram>();
	shader_->Load(vertex_shader, fragment_shader);

	constexpr float vertices[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	vao_ = std::make_unique<GLVertexArray>();
	vbo_ = std::make_unique<GLBuffer>();

	glNamedBufferStorage(vbo_->GetID(), sizeof(vertices), vertices, 0);
	glVertexArrayVertexBuffer(vao_->GetID(), 0, vbo_->GetID(), 0, 2 * sizeof(float));
	glEnableVertexArrayAttrib(vao_->GetID(), 0);
	glVertexArrayAttribFormat(vao_->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao_->GetID(), 0, 0);
	glBindVertexArray(0);
}
