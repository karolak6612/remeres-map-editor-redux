#include "app/main.h"
#include "rendering/utilities/light_drawer.h"

#include <algorithm>
#include <format>
#include <limits>

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
		const uint8_t ambient_color_index = above_ground ? options.server_light.color : 215;
		const float server_intensity = above_ground ? options.server_light.intensity / 255.0f : 0.0f;
		const float ambient_intensity = std::max(options.minimum_ambient_light, server_intensity);
		return normalizedPaletteColor(ambient_color_index) * ambient_intensity;
	}
}

LightDrawer::LightDrawer() = default;

LightDrawer::~LightDrawer() = default;

void LightDrawer::draw(const RenderView& view, const LightBuffer& light_buffer, const DrawingOptions& options) {
	if (!shader_) {
		initRenderResources();
	}

	if (!fbo_) {
		initFBO();
	}

	if (!shader_ || !fbo_ || light_buffer.width <= 0 || light_buffer.height <= 0) {
		return;
	}

	resizeFBO(light_buffer.width, light_buffer.height);

	gpu_lights_.clear();
	gpu_lights_.reserve(light_buffer.lights.size());
	for (const auto& light : light_buffer.lights) {
		gpu_lights_.push_back(GPULight {
			.tile_position = glm::vec2(static_cast<float>(light.tile_x), static_cast<float>(light.tile_y)),
			.intensity = static_cast<float>(light.intensity),
			.padding = 0.0f,
			.color = glm::vec4(normalizedPaletteColor(light.color), 1.0f)
		});
	}

	gpu_tiles_.clear();
	gpu_tiles_.reserve(light_buffer.tiles.size());
	for (const auto& tile : light_buffer.tiles) {
		gpu_tiles_.push_back(GPUTileLight {
			.start = tile.start,
			.color = tile.color
		});
	}

	uploadBuffer(light_ssbo_, light_ssbo_capacity_, gpu_lights_.data(), gpu_lights_.size(), sizeof(GPULight));
	uploadBuffer(tile_ssbo_, tile_ssbo_capacity_, gpu_tiles_.data(), gpu_tiles_.size(), sizeof(GPUTileLight));

	{
		ScopedGLFramebuffer framebuffer_scope(GL_FRAMEBUFFER, fbo_->GetID());
		ScopedGLViewport viewport_scope(0, 0, light_buffer.width, light_buffer.height);
		ScopedGLCapability blend_capability(GL_BLEND, false);

		shader_->Use();
		shader_->SetInt("uMode", 0);
		shader_->SetInt("uTileWidth", light_buffer.width);
		shader_->SetInt("uTileHeight", light_buffer.height);
		shader_->SetInt("uLightCount", static_cast<int>(gpu_lights_.size()));
		shader_->SetVec2("uTileOrigin", glm::vec2(static_cast<float>(light_buffer.origin_x), static_cast<float>(light_buffer.origin_y)));
		shader_->SetVec3("uAmbientColor", ambientColorForView(view, options));

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, light_ssbo_->GetID());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, tile_ssbo_->GetID());
		glBindVertexArray(vao_->GetID());
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

	const float draw_x = static_cast<float>(light_buffer.origin_x * TILE_SIZE - view.view_scroll_x) / view.zoom;
	const float draw_y = static_cast<float>(light_buffer.origin_y * TILE_SIZE - view.view_scroll_y) / view.zoom;
	const float draw_width = static_cast<float>(light_buffer.width * TILE_SIZE) / view.zoom;
	const float draw_height = static_cast<float>(light_buffer.height * TILE_SIZE) / view.zoom;

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(draw_x, draw_y, 0.0f));
	model = glm::scale(model, glm::vec3(draw_width, draw_height, 1.0f));

	shader_->Use();
	shader_->SetInt("uMode", 1);
	shader_->SetMat4("uCompositeMatrix", view.projectionMatrix * view.viewMatrix * model);
	shader_->SetInt("uTexture", 0);

	glBindTextureUnit(0, fbo_texture_->GetID());

	{
		ScopedGLCapability blend_capability(GL_BLEND);
		ScopedGLBlend blend_state(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);

		glBindVertexArray(vao_->GetID());
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

	shader_->SetInt("uMode", 0);
}

void LightDrawer::initFBO() {
	fbo_ = std::make_unique<GLFramebuffer>();
	fbo_texture_ = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);
	resizeFBO(1, 1);
	glNamedFramebufferTexture(fbo_->GetID(), GL_COLOR_ATTACHMENT0, fbo_texture_->GetID(), 0);

	const GLenum status = glCheckNamedFramebufferStatus(fbo_->GetID(), GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		spdlog::error("LightDrawer FBO incomplete: {}", status);
	}
}

void LightDrawer::resizeFBO(int width, int height) {
	if (width == buffer_width_ && height == buffer_height_) {
		return;
	}

	buffer_width_ = width;
	buffer_height_ = height;
	fbo_texture_ = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);
	glTextureStorage2D(fbo_texture_->GetID(), 1, GL_RGBA8, width, height);
	glTextureParameteri(fbo_texture_->GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(fbo_texture_->GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(fbo_texture_->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(fbo_texture_->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glNamedFramebufferTexture(fbo_->GetID(), GL_COLOR_ATTACHMENT0, fbo_texture_->GetID(), 0);
}

void LightDrawer::uploadBuffer(std::unique_ptr<GLBuffer>& buffer, size_t& capacity, const void* data, size_t count, size_t stride) {
	const size_t required_bytes = count * stride;
	if (!buffer) {
		buffer = std::make_unique<GLBuffer>();
	}

	if (required_bytes == 0) {
		if (capacity == 0) {
			capacity = stride;
			glNamedBufferStorage(buffer->GetID(), capacity, nullptr, GL_DYNAMIC_STORAGE_BIT);
		}
		return;
	}

	if (required_bytes > capacity) {
		capacity = std::max(required_bytes, std::max(stride, capacity * 2));
		buffer = std::make_unique<GLBuffer>();
		glNamedBufferStorage(buffer->GetID(), capacity, nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

	glNamedBufferSubData(buffer->GetID(), 0, required_bytes, data);
}

void LightDrawer::initRenderResources() {
	constexpr auto vertex_shader = R"(
		#version 450 core
		layout (location = 0) in vec2 aPos;

		uniform int uMode;
		uniform mat4 uCompositeMatrix;

		out vec2 TexCoord;

		void main() {
			if (uMode == 0) {
				vec2 ndc = vec2(aPos.x * 2.0 - 1.0, 1.0 - aPos.y * 2.0);
				gl_Position = vec4(ndc, 0.0, 1.0);
			} else {
				gl_Position = uCompositeMatrix * vec4(aPos, 0.0, 1.0);
			}
			TexCoord = vec2(aPos.x, 1.0 - aPos.y);
		}
	)";

	constexpr auto fragment_shader = R"(
		#version 450 core
		layout(std430, binding = 0) readonly buffer LightBlock {
			struct GPULight {
				vec2 tile_position;
				float intensity;
				float padding;
				vec4 color;
			};
			GPULight uLights[];
		};

		layout(std430, binding = 1) readonly buffer TileBlock {
			struct GPUTileLight {
				uint start;
				uint color;
			};
			GPUTileLight uTiles[];
		};

		in vec2 TexCoord;

		uniform int uMode;
		uniform sampler2D uTexture;
		uniform int uTileWidth;
		uniform int uTileHeight;
		uniform int uLightCount;
		uniform vec2 uTileOrigin;
		uniform vec3 uAmbientColor;

		out vec4 OutColor;

		void main() {
			if (uMode == 0) {
				int tile_x = int(gl_FragCoord.x);
				int tile_y = uTileHeight - 1 - int(gl_FragCoord.y);
				int tile_index = tile_y * uTileWidth + tile_x;

				vec3 color = uAmbientColor;
				vec2 tile_position = uTileOrigin + vec2(float(tile_x), float(tile_y));
				uint light_start = uTiles[tile_index].start;

				for (uint i = light_start; i < uint(uLightCount); ++i) {
					float distance_to_light = distance(tile_position, uLights[i].tile_position);
					float intensity = (-distance_to_light + uLights[i].intensity) * 0.2;
					if (intensity < 0.01) {
						continue;
					}
					intensity = min(intensity, 1.0);
					color = max(color, uLights[i].color.rgb * intensity);
				}

				OutColor = vec4(color, 1.0);
				return;
			}

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
	light_ssbo_ = std::make_unique<GLBuffer>();
	tile_ssbo_ = std::make_unique<GLBuffer>();

	glNamedBufferStorage(vbo_->GetID(), sizeof(vertices), vertices, 0);
	glVertexArrayVertexBuffer(vao_->GetID(), 0, vbo_->GetID(), 0, 2 * sizeof(float));
	glEnableVertexArrayAttrib(vao_->GetID(), 0);
	glVertexArrayAttribFormat(vao_->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao_->GetID(), 0, 0);
	glBindVertexArray(0);
}
