#include "rendering/core/texture_array.h"
#include "rendering/core/gl_resources.h"
#include <iostream>
#include <spdlog/spdlog.h>

TextureArray::TextureArray() {
}

TextureArray::~TextureArray() {
	cleanup();
}

bool TextureArray::initialize(int width, int height, int maxLayers) {
	if (initialized_) {
		spdlog::error("TextureArray: Already initialized");
		return true;
	}

	width_ = width;
	height_ = height;
	maxLayers_ = maxLayers;

	// Create texture array
	texture_ = std::make_unique<GLTextureResource>(GL_TEXTURE_2D_ARRAY);
	GLuint textureId_ = texture_->GetID();

	if (textureId_ == 0) {
		spdlog::error("TextureArray: Failed to create texture");
		texture_.reset();
		return false;
	}

	// Allocate immutable storage for all layers
	// Using RGBA8 format, no mipmaps (level 1)
	glTextureStorage3D(textureId_, 1, GL_RGBA8, width_, height_, maxLayers_);

	// Set texture parameters
	glTextureParameteri(textureId_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(textureId_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(textureId_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureId_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	allocatedLayers_ = 0;
	initialized_ = true;

	spdlog::info("TextureArray: Initialized {}x{} with {} layers", width_, height_, maxLayers_);

	return true;
}

bool TextureArray::uploadLayer(int layer, const uint8_t* rgbaData) {
	if (!initialized_ || !texture_) {
		spdlog::error("TextureArray: Not initialized");
		return false;
	}
	if (layer < 0 || layer >= maxLayers_) {
		spdlog::error("TextureArray: Layer {} out of range (max: {})", layer, maxLayers_);
		return false;
	}
	if (rgbaData == nullptr) {
		spdlog::error("TextureArray: Null data for layer {}", layer);
		return false;
	}

	// Upload to specific layer (z offset = layer, depth = 1 layer)
	glTextureSubImage3D(texture_->GetID(), 0, 0, 0, layer, width_, height_, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgbaData);

	return true;
}

int TextureArray::allocateLayer() {
	if (!initialized_) {
		return -1;
	}
	if (allocatedLayers_ >= maxLayers_) {
		spdlog::error("TextureArray: No more layers available (allocated: {})", allocatedLayers_);
		return -1;
	}
	return allocatedLayers_++;
}

void TextureArray::bind(int unit) const {
	if (initialized_ && texture_) {
		glBindTextureUnit(unit, texture_->GetID());
	}
}

GLuint TextureArray::getTextureId() const {
	return texture_ ? texture_->GetID() : 0;
}

void TextureArray::cleanup() {
	texture_.reset();
	initialized_ = false;
	allocatedLayers_ = 0;
}
