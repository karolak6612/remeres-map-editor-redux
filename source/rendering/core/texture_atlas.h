#ifndef RME_RENDERING_CORE_TEXTURE_ATLAS_H_
#define RME_RENDERING_CORE_TEXTURE_ATLAS_H_

#include "app/main.h"
#include "rendering/core/vulkan_context.h"
#include "rendering/core/vulkan_resources.h"
#include <optional>
#include <cstdint>
#include <vector>
#include <memory>

struct AtlasRegion {
	uint32_t atlas_index = 0; // Layer in texture array
	float u_min = 0.0f; // UV left
	float v_min = 0.0f; // UV top
	float u_max = 1.0f; // UV right
	float v_max = 1.0f; // UV bottom
	uint32_t debug_sprite_id = 0; // DEBUG: Track which sprite ID owns this region
	int pixel_x = 0; // Pre-calculated pixel X in the atlas layer
	int pixel_y = 0; // Pre-calculated pixel Y in the atlas layer

	static constexpr uint32_t INVALID_SENTINEL = 0xFFFFFFFE;
};

class TextureAtlas {
public:
	static constexpr int ATLAS_SIZE = 4096;
	static constexpr int SPRITE_SIZE = 32;
	static constexpr int SPRITES_PER_ROW = ATLAS_SIZE / SPRITE_SIZE; // 128
	static constexpr int SPRITES_PER_LAYER = SPRITES_PER_ROW * SPRITES_PER_ROW; // 16384
	static constexpr int MAX_LAYERS = 64; // 64 * 16384 = 1M+ sprites

	TextureAtlas(VulkanContext* vkContext);
	~TextureAtlas();

	TextureAtlas(const TextureAtlas&) = delete;
	TextureAtlas& operator=(const TextureAtlas&) = delete;

	TextureAtlas(TextureAtlas&& other) noexcept;
	TextureAtlas& operator=(TextureAtlas&& other) noexcept;

	bool initialize(int initial_layers = 8);
	std::optional<AtlasRegion> addSprite(const uint8_t* rgba_data);
	void freeSlot(const AtlasRegion& region);

	int getLayerCount() const {
		return layer_count_;
	}

	VkImageView GetView() const {
		return texture_id_ ? texture_id_->GetView() : VK_NULL_HANDLE;
	}

    VkSampler GetSampler() const {
        return sampler_;
    }

	bool isValid() const {
		return texture_id_ != nullptr;
	}

	void release();

private:
	bool addLayer();
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, int x, int y, int layer);

    VulkanContext* vkContext_ = nullptr;
	std::unique_ptr<VulkanImage> texture_id_;
    VkSampler sampler_ = VK_NULL_HANDLE;

	int layer_count_ = 0;
	int allocated_layers_ = 0;
	int total_sprite_count_ = 0;
	int current_layer_ = 0;
	int next_x_ = 0; // Next slot X in grid
	int next_y_ = 0; // Next slot Y in grid

	struct FreeSlot {
		int pixel_x;
		int pixel_y;
		int layer;
	};
	std::vector<FreeSlot> free_slots_;
};

#endif
