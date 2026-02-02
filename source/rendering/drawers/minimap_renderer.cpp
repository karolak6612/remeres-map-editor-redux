#include "rendering/drawers/minimap_renderer.h"
#include "map/map.h"
#include "map/tile.h"
#include "rendering/core/minimap_colors.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>

const char* minimap_vert = R"(
#version 450 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aRect; // x, y, w, h
layout (location = 3) in float aLayer;

out vec2 TexCoord;
flat out float Layer;
uniform mat4 uProjection;

void main() {
    vec2 worldPos = aRect.xy + aPos * aRect.zw;
    gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);
    TexCoord = aTexCoord;
    Layer = aLayer;
}
)";

const char* minimap_frag = R"(
#version 450 core
in vec2 TexCoord;
flat in float Layer;
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

	// Create VAO/VBO for fullscreen quad
	vao_ = std::make_unique<GLVertexArray>();
	vbo_ = std::make_unique<GLBuffer>();
	instance_vbo_ = std::make_unique<GLBuffer>();

	// Allocate instance buffer (max 65536 instances)
	// struct { float x,y,w,h,layer; } = 5 floats
	glNamedBufferStorage(instance_vbo_->GetID(), 65536 * 5 * sizeof(float), nullptr, GL_DYNAMIC_STORAGE_BIT);

	float quad_vertices[] = {
		// pos      // tex
		0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f
	};

	glNamedBufferStorage(vbo_->GetID(), sizeof(quad_vertices), quad_vertices, 0);

	// DSA Setup for Quad
	glVertexArrayVertexBuffer(vao_->GetID(), 0, vbo_->GetID(), 0, 4 * sizeof(float));

	// Pos
	glEnableVertexArrayAttrib(vao_->GetID(), 0);
	glVertexArrayAttribFormat(vao_->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao_->GetID(), 0, 0);

	// Tex
	glEnableVertexArrayAttrib(vao_->GetID(), 1);
	glVertexArrayAttribFormat(vao_->GetID(), 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexArrayAttribBinding(vao_->GetID(), 1, 0);

	// DSA Setup for Instances (Binding 1)
	glVertexArrayVertexBuffer(vao_->GetID(), 1, instance_vbo_->GetID(), 0, 5 * sizeof(float));
	glVertexArrayBindingDivisor(vao_->GetID(), 1, 1);

	// Rect (Loc 2)
	glEnableVertexArrayAttrib(vao_->GetID(), 2);
	glVertexArrayAttribFormat(vao_->GetID(), 2, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao_->GetID(), 2, 1);

	// Layer (Loc 3)
	glEnableVertexArrayAttrib(vao_->GetID(), 3);
	glVertexArrayAttribFormat(vao_->GetID(), 3, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float));
	glVertexArrayAttribBinding(vao_->GetID(), 3, 1);

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
	if (!texture_id_) {
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

	// Max update size for PBO safety (likely just one tile usually)
	size_t max_tile_update = TILE_SIZE * TILE_SIZE;
	if (stage_buffer_.size() < max_tile_update) {
		stage_buffer_.resize(max_tile_update);
		if (pbo_->getSize() < max_tile_update) {
			pbo_->cleanup();
			pbo_->initialize(max_tile_update);
		}
	}

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

			// Fill buffer
			int idx = 0;
			for (int dy = 0; dy < update_h; ++dy) {
				for (int dx = 0; dx < update_w; ++dx) {
					const Tile* tile = map.getTile(int_x + dx, int_y + dy, floor);
					if (tile) {
						stage_buffer_[idx++] = tile->getMiniMapColor();
					} else {
						stage_buffer_[idx++] = 0;
					}
				}
			}

			// Upload to layer
			void* ptr = pbo_->mapWrite();
			if (ptr) {
				memcpy(ptr, stage_buffer_.data(), update_w * update_h);
				pbo_->unmap();
				pbo_->bind();
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				int layer = r * cols_ + c;
				int offset_x = int_x - tile_x;
				int offset_y = int_y - tile_y;

				glTextureSubImage3D(texture_id_->GetID(), 0, offset_x, offset_y, layer, update_w, update_h, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, 0);

				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
				pbo_->unbind();
				pbo_->advance();
			}
		}
	}
}

void MinimapRenderer::render(const glm::mat4& projection, int x, int y, int w, int h, float map_x, float map_y, float map_w, float map_h) {
	if (!texture_id_) {
		return;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader_->Use();

	// Bind textures
	glBindTextureUnit(0, texture_id_->GetID());
	shader_->SetInt("uMinimapTexture", 0);

	glBindTextureUnit(1, palette_texture_id_->GetID());
	shader_->SetInt("uPaletteTexture", 1);

	glBindVertexArray(vao_->GetID());

	// Constants
	float scale_x = (float)w / map_w;
	float scale_y = (float)h / map_h;

	// Determine visible tiles
	// Clamp to integer grid
	int start_col = (int)std::floor(map_x / TILE_SIZE);
	int end_col = (int)std::floor((map_x + map_w) / TILE_SIZE);
	int start_row = (int)std::floor(map_y / TILE_SIZE);
	int end_row = (int)std::floor((map_y + map_h) / TILE_SIZE);

	start_col = std::max(0, start_col);
	start_row = std::max(0, start_row);
	end_col = std::min(cols_ - 1, end_col);
	end_row = std::min(rows_ - 1, end_row);

	struct InstanceData {
		float x, y, w, h;
		float layer;
	};

	std::vector<InstanceData> instances;
	instances.reserve((end_row - start_row + 1) * (end_col - start_col + 1));

	for (int r = start_row; r <= end_row; ++r) {
		for (int c = start_col; c <= end_col; ++c) {
			int tile_x = c * TILE_SIZE;
			int tile_y = r * TILE_SIZE;

			// Calculate screen position
			float screen_tile_x = x + (tile_x - map_x) * scale_x;
			float screen_tile_y = y + (tile_y - map_y) * scale_y;
			float screen_tile_w = TILE_SIZE * scale_x;
			float screen_tile_h = TILE_SIZE * scale_y;

			int layer = r * cols_ + c;

			instances.push_back({ screen_tile_x, screen_tile_y, screen_tile_w, screen_tile_h, (float)layer });
		}
	}

	if (instances.empty()) {
		glBindVertexArray(0);
		return;
	}

	// Limit to buffer capacity (65536)
	if (instances.size() > 65536) {
		instances.resize(65536);
		static bool warned = false;
		if (!warned) {
			spdlog::warn("MinimapRenderer: Too many visible chunks (>65536), capped. This warning will appear once.");
			warned = true;
		}
	}

	glNamedBufferSubData(instance_vbo_->GetID(), 0, instances.size() * sizeof(InstanceData), instances.data());

	shader_->SetMat4("uProjection", projection);

	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, (GLsizei)instances.size());

	glBindVertexArray(0);
}
