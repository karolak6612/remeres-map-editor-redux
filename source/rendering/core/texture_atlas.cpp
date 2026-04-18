#include "rendering/core/texture_atlas.h"

#include <algorithm>
#include <cstring>

#include <spdlog/spdlog.h>

TextureAtlas::TextureAtlas() = default;

TextureAtlas::~TextureAtlas() {
	release();
}

TextureAtlas::TextureAtlas(TextureAtlas&& other) noexcept :
	texture_id_(std::move(other.texture_id_)),
	layer_count_(other.layer_count_),
	allocated_layers_(other.allocated_layers_),
	total_sprite_count_(other.total_sprite_count_),
	small_search_hint_(other.small_search_hint_),
	large_search_hint_(other.large_search_hint_),
	occupancy_(std::move(other.occupancy_)),
	free_slots_(std::move(other.free_slots_)),
	pbo_(std::move(other.pbo_)) {
	other.layer_count_ = 0;
	other.allocated_layers_ = 0;
	other.total_sprite_count_ = 0;
	other.small_search_hint_ = 0;
	other.large_search_hint_ = 0;
	other.pbo_.reset();
}

TextureAtlas& TextureAtlas::operator=(TextureAtlas&& other) noexcept {
	if (this != &other) {
		release();
		texture_id_ = std::move(other.texture_id_);
		layer_count_ = other.layer_count_;
		allocated_layers_ = other.allocated_layers_;
		total_sprite_count_ = other.total_sprite_count_;
		small_search_hint_ = other.small_search_hint_;
		large_search_hint_ = other.large_search_hint_;
		occupancy_ = std::move(other.occupancy_);
		free_slots_ = std::move(other.free_slots_);
		pbo_ = std::move(other.pbo_);
		other.layer_count_ = 0;
		other.allocated_layers_ = 0;
		other.total_sprite_count_ = 0;
		other.small_search_hint_ = 0;
		other.large_search_hint_ = 0;
		other.pbo_.reset();
	}
	return *this;
}

bool TextureAtlas::initialize(int initial_layers) {
	if (texture_id_) {
		return true;
	}

	initial_layers = std::clamp(initial_layers, 1, MAX_LAYERS);

	texture_id_ = std::make_unique<GLTextureResource>(GL_TEXTURE_2D_ARRAY);

	glTextureParameteri(texture_id_->GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture_id_->GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture_id_->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture_id_->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage3D(texture_id_->GetID(), 1, GL_RGBA8, ATLAS_SIZE, ATLAS_SIZE, initial_layers);

	allocated_layers_ = initial_layers;
	layer_count_ = 1;
	small_search_hint_ = 0;
	large_search_hint_ = 0;
	occupancy_.assign(static_cast<size_t>(initial_layers) * SLOTS_PER_LAYER, 0);

#ifdef USE_PBO_FOR_SPRITE_UPLOAD
	pbo_ = std::make_unique<PixelBufferObject>();
	if (!pbo_->initialize(MAX_SPRITE_SIZE * MAX_SPRITE_SIZE * 4)) {
		spdlog::error("TextureAtlas: Failed to initialize PBO");
		return false;
	}
#endif

	spdlog::info("TextureAtlas created: {}x{} x {} layers, id={}", ATLAS_SIZE, ATLAS_SIZE, initial_layers, texture_id_->GetID());
	return true;
}

bool TextureAtlas::addLayer() {
	if (layer_count_ >= MAX_LAYERS) {
		spdlog::error("TextureAtlas: Max layers ({}) reached", MAX_LAYERS);
		return false;
	}

	if (layer_count_ >= allocated_layers_) {
		const int new_allocated = std::min(allocated_layers_ + 4, MAX_LAYERS);
		spdlog::info("TextureAtlas: Expanding {} -> {} layers", allocated_layers_, new_allocated);

		auto new_texture = std::make_unique<GLTextureResource>(GL_TEXTURE_2D_ARRAY);
		glTextureParameteri(new_texture->GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(new_texture->GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(new_texture->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(new_texture->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureStorage3D(new_texture->GetID(), 1, GL_RGBA8, ATLAS_SIZE, ATLAS_SIZE, new_allocated);

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			spdlog::error("TextureAtlas: glTextureStorage3D failed during expansion (err={}). VRAM might be full.", err);
			return false;
		}

		glCopyImageSubData(texture_id_->GetID(), GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, new_texture->GetID(), GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, ATLAS_SIZE, ATLAS_SIZE, allocated_layers_);
		err = glGetError();
		if (err != GL_NO_ERROR) {
			spdlog::error("TextureAtlas: glCopyImageSubData failed (err={}). Texture data lost!", err);
			return false;
		}

		texture_id_ = std::move(new_texture);
		allocated_layers_ = new_allocated;
		occupancy_.resize(static_cast<size_t>(new_allocated) * SLOTS_PER_LAYER, 0);
	}

	layer_count_++;
	return true;
}

size_t TextureAtlas::occupancyIndex(int layer, int slot_x, int slot_y) const {
	return static_cast<size_t>(layer) * SLOTS_PER_LAYER + static_cast<size_t>(slot_y) * SLOTS_PER_ROW + static_cast<size_t>(slot_x);
}

bool TextureAtlas::normalizePixelDimension(int& pixel_width, int& pixel_height) {
	if (pixel_width <= 0 || pixel_height <= 0) {
		return false;
	}

	auto normalize = [](int value) {
		if (value <= BASE_SLOT_SIZE) {
			return BASE_SLOT_SIZE;
		}
		if (value <= MAX_SPRITE_SIZE) {
			return MAX_SPRITE_SIZE;
		}
		return 0;
	};

	pixel_width = normalize(pixel_width);
	pixel_height = normalize(pixel_height);
	return pixel_width != 0 && pixel_height != 0;
}

bool TextureAtlas::isAreaFree(int layer, int slot_x, int slot_y, int slot_width, int slot_height) const {
	if (layer < 0 || layer >= layer_count_) {
		return false;
	}
	if (slot_x < 0 || slot_y < 0) {
		return false;
	}
	if (slot_x + slot_width > SLOTS_PER_ROW || slot_y + slot_height > SLOTS_PER_ROW) {
		return false;
	}

	for (int y = 0; y < slot_height; ++y) {
		for (int x = 0; x < slot_width; ++x) {
			if (occupancy_[occupancyIndex(layer, slot_x + x, slot_y + y)] != 0) {
				return false;
			}
		}
	}

	return true;
}

void TextureAtlas::setAreaOccupied(int layer, int slot_x, int slot_y, int slot_width, int slot_height, bool occupied) {
	for (int y = 0; y < slot_height; ++y) {
		for (int x = 0; x < slot_width; ++x) {
			occupancy_[occupancyIndex(layer, slot_x + x, slot_y + y)] = occupied ? 1u : 0u;
		}
	}
}

std::optional<TextureAtlas::Placement> TextureAtlas::findPlacement(int slot_width, int slot_height) {
	const bool single_slot = slot_width == 1 && slot_height == 1;
	if (single_slot) {
		while (!free_slots_.empty()) {
			const auto slot = free_slots_.back();
			free_slots_.pop_back();
			if (isAreaFree(slot.layer, slot.slot_x, slot.slot_y, 1, 1)) {
				return Placement {
					.layer = slot.layer,
					.slot_x = slot.slot_x,
					.slot_y = slot.slot_y,
				};
			}
		}
	}

	size_t& search_hint = single_slot ? small_search_hint_ : large_search_hint_;
	for (;;) {
		const size_t search_limit = static_cast<size_t>(layer_count_) * SLOTS_PER_LAYER;
		for (size_t linear_index = search_hint; linear_index < search_limit; ++linear_index) {
			const int layer = static_cast<int>(linear_index / SLOTS_PER_LAYER);
			const int slot_index = static_cast<int>(linear_index % SLOTS_PER_LAYER);
			const int slot_y = slot_index / SLOTS_PER_ROW;
			const int slot_x = slot_index % SLOTS_PER_ROW;

			if (isAreaFree(layer, slot_x, slot_y, slot_width, slot_height)) {
				search_hint = linear_index + 1;
				return Placement {
					.layer = layer,
					.slot_x = slot_x,
					.slot_y = slot_y,
				};
			}
		}

		if (!addLayer()) {
			return std::nullopt;
		}
	}
}

std::optional<AtlasRegion> TextureAtlas::addSprite(const uint8_t* rgba_data, int pixel_width, int pixel_height) {
	if (!isValid()) {
		spdlog::error("TextureAtlas::addSprite called on uninitialized atlas");
		return std::nullopt;
	}
	if (!rgba_data) {
		spdlog::error("TextureAtlas::addSprite called with null data");
		return std::nullopt;
	}
	if (!normalizePixelDimension(pixel_width, pixel_height)) {
		spdlog::error("TextureAtlas::addSprite called with unsupported sprite size {}x{}", pixel_width, pixel_height);
		return std::nullopt;
	}

	const int slot_width = pixel_width / BASE_SLOT_SIZE;
	const int slot_height = pixel_height / BASE_SLOT_SIZE;
	const auto placement = findPlacement(slot_width, slot_height);
	if (!placement) {
		return std::nullopt;
	}

	const int pixel_x = placement->slot_x * BASE_SLOT_SIZE;
	const int pixel_y = placement->slot_y * BASE_SLOT_SIZE;
	const int layer = placement->layer;
	setAreaOccupied(layer, placement->slot_x, placement->slot_y, slot_width, slot_height, true);
	total_sprite_count_++;

	bool uploaded = false;
	if (pbo_) {
		void* ptr = pbo_->mapWrite();
		if (ptr) {
			std::memcpy(ptr, rgba_data, static_cast<size_t>(pixel_width) * pixel_height * 4);
			pbo_->unmap();
			pbo_->bind();
			glTextureSubImage3D(texture_id_->GetID(), 0, pixel_x, pixel_y, layer, pixel_width, pixel_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			pbo_->unbind();
			pbo_->advance();
			uploaded = true;
		}
	}

	if (!uploaded) {
		glTextureSubImage3D(texture_id_->GetID(), 0, pixel_x, pixel_y, layer, pixel_width, pixel_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);
	}

	const float texel_size = 1.0f / static_cast<float>(ATLAS_SIZE);
	const float half_texel = texel_size * 0.5f;

	AtlasRegion region;
	region.atlas_index = static_cast<uint32_t>(layer);
	region.pixel_x = pixel_x;
	region.pixel_y = pixel_y;
	region.pixel_width = pixel_width;
	region.pixel_height = pixel_height;
	region.slot_width = slot_width;
	region.slot_height = slot_height;
	region.u_min = static_cast<float>(pixel_x) / ATLAS_SIZE + half_texel;
	region.v_min = static_cast<float>(pixel_y) / ATLAS_SIZE + half_texel;
	region.u_max = static_cast<float>(pixel_x + pixel_width) / ATLAS_SIZE - half_texel;
	region.v_max = static_cast<float>(pixel_y + pixel_height) / ATLAS_SIZE - half_texel;
	return region;
}

void TextureAtlas::freeSlot(const AtlasRegion& region) {
	const int slot_x = region.pixel_x / BASE_SLOT_SIZE;
	const int slot_y = region.pixel_y / BASE_SLOT_SIZE;
	const int layer = static_cast<int>(region.atlas_index);
	const int slot_width = std::max(1, region.slot_width);
	const int slot_height = std::max(1, region.slot_height);

	for (int y = 0; y < slot_height; ++y) {
		for (int x = 0; x < slot_width; ++x) {
			if (isAreaFree(layer, slot_x + x, slot_y + y, 1, 1)) {
				spdlog::warn("TextureAtlas: Double free detected for slot [x={}, y={}, layer={}] - ignoring", slot_x + x, slot_y + y, layer);
				return;
			}
		}
	}

	setAreaOccupied(layer, slot_x, slot_y, slot_width, slot_height, false);
	for (int y = 0; y < slot_height; ++y) {
		for (int x = 0; x < slot_width; ++x) {
			free_slots_.push_back(FreeSlot {
				.slot_x = slot_x + x,
				.slot_y = slot_y + y,
				.layer = layer,
			});
		}
	}

	total_sprite_count_ = std::max(0, total_sprite_count_ - 1);
}

void TextureAtlas::bind(uint32_t slot) const {
	glBindTextureUnit(slot, texture_id_->GetID());
}

void TextureAtlas::unbind(uint32_t slot) const {
	glBindTextureUnit(slot, 0);
}

void TextureAtlas::release() {
	if (texture_id_) {
		spdlog::info("TextureAtlas releasing resources [ID={}]", texture_id_->GetID());
	}
	texture_id_.reset();
	layer_count_ = 0;
	allocated_layers_ = 0;
	total_sprite_count_ = 0;
	small_search_hint_ = 0;
	large_search_hint_ = 0;
	occupancy_.clear();
	free_slots_.clear();
}
