#include "rendering/drawers/minimap_renderer.h"
#include "rendering/core/shared_geometry.h"
#include "map/map.h"
#include "map/tile.h"
#include "rendering/core/minimap_colors.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include "rendering/core/gl_scoped_state.h"

const char* minimap_vert = R"(
#version 450 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aDestRect; // x, y, w, h
layout (location = 3) in float aLayer;

out vec2 TexCoord;
out float Layer;
uniform mat4 uProjection;

void main() {
    vec2 pos = aDestRect.xy + aPos * aDestRect.zw;
    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
    TexCoord = aTexCoord;
    Layer = aLayer;
}
)";

const char* minimap_frag = R"(
#version 450 core
in vec2 TexCoord;
in float Layer;
out vec4 FragColor;

uniform usampler2DArray uMinimapTexture; // R8UI Array
uniform sampler1D uPaletteTexture;  // RGBA

void main() {
    uint colorIndex = texture(uMinimapTexture, vec3(TexCoord, Layer)).r;
    if (colorIndex == 0u) {
        discard; // Transparent
    }
    
    // Map 0-255 index to 0.0-1.0 UV
    float paletteUV = (float(colorIndex) + 0.5) / 256.0;
    FragColor = texture(uPaletteTexture, paletteUV);
}
)";

MinimapRenderer::MinimapRenderer() {
}

MinimapRenderer::~MinimapRenderer() {
	// RAII manages resources
}

bool MinimapRenderer::initialize() {
	shader_ = std::make_unique<ShaderProgram>();
	if (!shader_->Load(minimap_vert, minimap_frag)) {
		spdlog::error("MinimapRenderer: Failed to load shader");
		return false;
	}
	spdlog::info("MinimapRenderer: Shader loaded successfully");

	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	spdlog::info("MinimapRenderer: GL_MAX_TEXTURE_SIZE = {}", maxTextureSize);

	pbo_ = std::make_unique<PixelBufferObject>();
	// Initial PBO size for small chunk
	pbo_->initialize(1024 * 1024);

	createPaletteTexture();

	// Initialize Shared Geometry
	if (!SharedGeometry::Instance().initialize()) {
		spdlog::error("MinimapRenderer: Failed to initialize SharedGeometry");
		return false;
	}

	// Create VAO/VBO for fullscreen quad
	vao_ = std::make_unique<GLVertexArray>();
	instance_vbo_ = std::make_unique<GLBuffer>();

	// Pre-allocate instance buffer
	instance_vbo_capacity_ = 1024;
	glNamedBufferStorage(instance_vbo_->GetID(), instance_vbo_capacity_ * sizeof(InstanceData), nullptr, GL_DYNAMIC_STORAGE_BIT);

	// Setup VAO (DSA)
	GLuint vao = vao_->GetID();
	GLuint inst_vbo = instance_vbo_->GetID();

	// Binding 0: Quad Data from SharedGeometry
	glVertexArrayVertexBuffer(vao, 0, SharedGeometry::Instance().getQuadVBO(), 0, 4 * sizeof(float));
	glVertexArrayElementBuffer(vao, SharedGeometry::Instance().getQuadEBO());

	// Attribute 0: Pos
	glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 0, 0);

	// Attribute 1: TexCoord
	glEnableVertexArrayAttrib(vao, 1);
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexArrayAttribBinding(vao, 1, 0);

	// Binding 1: Instance Data
	glVertexArrayVertexBuffer(vao, 1, inst_vbo, 0, sizeof(InstanceData));
	glVertexArrayBindingDivisor(vao, 1, 1); // One instance per step

	// Attribute 2: DestRect (x, y, w, h)
	glEnableVertexArrayAttrib(vao, 2);
	glVertexArrayAttribFormat(vao, 2, 4, GL_FLOAT, GL_FALSE, offsetof(InstanceData, x));
	glVertexArrayAttribBinding(vao, 2, 1);

	// Attribute 3: Layer
	glEnableVertexArrayAttrib(vao, 3);
	glVertexArrayAttribFormat(vao, 3, 1, GL_FLOAT, GL_FALSE, offsetof(InstanceData, layer));
	glVertexArrayAttribBinding(vao, 3, 1);

	return true;
}

void MinimapRenderer::createPaletteTexture() {
	palette_texture_id_ = std::make_unique<GLTextureResource>(GL_TEXTURE_1D);

	std::vector<uint8_t> palette(256 * 4);
	for (int i = 0; i < 256; ++i) {
		palette[i * 4 + 0] = minimap_color[i].red;
		palette[i * 4 + 1] = minimap_color[i].green;
		palette[i * 4 + 2] = minimap_color[i].blue;
		palette[i * 4 + 3] = 255;
	}
	// Index 0 is transparent
	palette[3] = 0;

	glTextureStorage1D(palette_texture_id_->GetID(), 1, GL_RGBA8, 256);
	glTextureSubImage1D(palette_texture_id_->GetID(), 0, 0, 256, GL_RGBA, GL_UNSIGNED_BYTE, palette.data());

	glTextureParameteri(palette_texture_id_->GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(palette_texture_id_->GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(palette_texture_id_->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
}

void MinimapRenderer::resize(int width, int height) {
	if (width_ == width && height_ == height && texture_id_) {
		return;
	}

	width_ = width;
	height_ = height;

	rows_ = (height + TILE_SIZE - 1) / TILE_SIZE;
	cols_ = (width + TILE_SIZE - 1) / TILE_SIZE;
	int num_layers = rows_ * cols_;

	texture_id_ = std::make_unique<GLTextureResource>(GL_TEXTURE_2D_ARRAY);
	glTextureStorage3D(texture_id_->GetID(), 1, GL_R8UI, TILE_SIZE, TILE_SIZE, num_layers);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		spdlog::error("MinimapRenderer: FAILED to create texture array {}x{}x{}! Error: {}", TILE_SIZE, TILE_SIZE, num_layers, error);
	}

	glTextureParameteri(texture_id_->GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture_id_->GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture_id_->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture_id_->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	spdlog::info("MinimapRenderer: Resized tiled texture to {}x{} ({}x{} tiles, {} layers)", width_, height_, cols_, rows_, num_layers);
}

void MinimapRenderer::updateRegion(const Map& map, int floor, int x, int y, int w, int h) {
	if (!texture_id_ || !pbo_) {
		return;
	}

	// Clamp to global bounds
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (x + w > width_) {
		w = width_ - x;
	}
	if (y + h > height_) {
		h = height_ - y;
	}
	if (w <= 0 || h <= 0) {
		return;
	}

	int start_col = x / TILE_SIZE;
	int end_col = (x + w - 1) / TILE_SIZE;
	int start_row = y / TILE_SIZE;
	int end_row = (y + h - 1) / TILE_SIZE;

	struct TileUpdate {
		int layer;
		int dest_x_in_tile, dest_y_in_tile;
		int map_start_x, map_start_y;
		int w, h;
		size_t pbo_offset;
	};
	std::vector<TileUpdate> pending_updates;
	pending_updates.reserve((end_row - start_row + 1) * (end_col - start_col + 1));
	size_t total_buffer_needed = 0;

	// 1. Calculate total buffer size needed
	for (int r = start_row; r <= end_row; ++r) {
		for (int c = start_col; c <= end_col; ++c) {
			int tile_x = c * TILE_SIZE;
			int tile_y = r * TILE_SIZE;

			int int_x = std::max(x, tile_x);
			int int_y = std::max(y, tile_y);
			int int_r = std::min(x + w, tile_x + TILE_SIZE);
			int int_b = std::min(y + h, tile_y + TILE_SIZE);

			int update_w = int_r - int_x;
			int update_h = int_b - int_y;

			if (update_w <= 0 || update_h <= 0) {
				continue;
			}

			int layer = r * cols_ + c;
			int offset_x = int_x - tile_x;
			int offset_y = int_y - tile_y;

			pending_updates.push_back({ .layer = layer, .dest_x_in_tile = offset_x, .dest_y_in_tile = offset_y, .map_start_x = int_x, .map_start_y = int_y, .w = update_w, .h = update_h, .pbo_offset = total_buffer_needed });
			total_buffer_needed += static_cast<size_t>(update_w) * update_h;
		}
	}

	if (pending_updates.empty()) {
		return;
	}

	// 2. Ensure PBO capacity
	if (pbo_->getSize() < total_buffer_needed) {
		pbo_->cleanup();
		pbo_->initialize(std::max(total_buffer_needed, pbo_->getSize() * 2)); // Growth strategy
	}

	// 3. Map PBO Once
	uint8_t* ptr = static_cast<uint8_t*>(pbo_->mapWrite());
	if (!ptr) {
		return;
	}

	// 4. Fill Data
	for (const auto& update : pending_updates) {
		uint8_t* dest = ptr + update.pbo_offset;
		int idx = 0;

		for (int dy = 0; dy < update.h; ++dy) {
			for (int dx = 0; dx < update.w; ++dx) {
				const Tile* tile = map.getTile(update.map_start_x + dx, update.map_start_y + dy, floor);
				if (tile) {
					dest[idx++] = tile->getMiniMapColor();
				} else {
					dest[idx++] = 0;
				}
			}
		}
	}

	// 5. Unmap and Upload
	pbo_->unmap();
	pbo_->bind();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (const auto& update : pending_updates) {
		glTextureSubImage3D(texture_id_->GetID(), 0, update.dest_x_in_tile, update.dest_y_in_tile, update.layer, update.w, update.h, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, reinterpret_cast<void*>(update.pbo_offset));
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	pbo_->unbind();
	pbo_->advance();
}

void MinimapRenderer::render(const glm::mat4& projection, int x, int y, int w, int h, float map_x, float map_y, float map_w, float map_h) {
	if (!texture_id_) {
		return;
	}

	// Constants
	float scale_x = (float)w / map_w;
	float scale_y = (float)h / map_h;

	// Determine visible tiles
	// Clamp to integer grid
	auto floor_to_int = [](float val) { return static_cast<int>(std::floor(val)); };
	int start_col = floor_to_int(map_x / TILE_SIZE);
	int end_col = floor_to_int((map_x + map_w) / TILE_SIZE);
	int start_row = floor_to_int(map_y / TILE_SIZE);
	int end_row = floor_to_int((map_y + map_h) / TILE_SIZE);

	start_col = std::max(0, start_col);
	start_row = std::max(0, start_row);
	end_col = std::min(cols_ - 1, end_col);
	end_row = std::min(rows_ - 1, end_row);

	// Collect instances
	instance_data_.clear();
	instance_data_.reserve((end_row - start_row + 1) * (end_col - start_col + 1));

	for (int r = start_row; r <= end_row; ++r) {
		for (int c = start_col; c <= end_col; ++c) {
			int tile_x = c * TILE_SIZE;
			int tile_y = r * TILE_SIZE;

			// Calculate screen position (dest rect)
			float screen_tile_x = x + (tile_x - map_x) * scale_x;
			float screen_tile_y = y + (tile_y - map_y) * scale_y;
			float screen_tile_w = TILE_SIZE * scale_x;
			float screen_tile_h = TILE_SIZE * scale_y;

			int layer = r * cols_ + c;

			instance_data_.push_back({ .x = screen_tile_x, .y = screen_tile_y, .w = screen_tile_w, .h = screen_tile_h, .layer = static_cast<float>(layer) });
		}
	}

	if (instance_data_.empty()) {
		return;
	}

	// Resize buffer if needed
	// Resize buffer if needed
	if (instance_data_.size() > instance_vbo_capacity_) {
		instance_vbo_capacity_ = instance_data_.size() * 2; // Grow strategy
		instance_vbo_ = std::make_unique<GLBuffer>();
		glNamedBufferStorage(instance_vbo_->GetID(), instance_vbo_capacity_ * sizeof(InstanceData), nullptr, GL_DYNAMIC_STORAGE_BIT);

		// Update VAO binding
		glVertexArrayVertexBuffer(vao_->GetID(), 1, instance_vbo_->GetID(), 0, sizeof(InstanceData));
	}

	// Upload data
	glNamedBufferSubData(instance_vbo_->GetID(), 0, instance_data_.size() * sizeof(InstanceData), instance_data_.data());

	// Render
	{
		ScopedGLCapability blendCap(GL_BLEND);
		ScopedGLBlend blendState(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		shader_->Use();
		shader_->SetMat4("uProjection", projection);

		// Bind textures
		glBindTextureUnit(0, texture_id_->GetID());
		shader_->SetInt("uMinimapTexture", 0);

		glBindTextureUnit(1, palette_texture_id_->GetID());
		shader_->SetInt("uPaletteTexture", 1);

		ScopedGLVertexArray vaoBind(vao_->GetID());
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instance_data_.size()));
	}
}
