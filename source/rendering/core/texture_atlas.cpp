#include "rendering/core/texture_atlas.h"
#include <iostream>
#include <algorithm>
#include <spdlog/spdlog.h>

TextureAtlas::TextureAtlas(VulkanContext* vkContext) : vkContext_(vkContext) {}

TextureAtlas::~TextureAtlas() {
    release();
}

TextureAtlas::TextureAtlas(TextureAtlas&& other) noexcept
    :
    vkContext_(other.vkContext_),
    texture_id_(std::move(other.texture_id_)),
    sampler_(std::exchange(other.sampler_, VK_NULL_HANDLE)),
    layer_count_(other.layer_count_),
    allocated_layers_(other.allocated_layers_),
    total_sprite_count_(other.total_sprite_count_),
    current_layer_(other.current_layer_), next_x_(other.next_x_),
    next_y_(other.next_y_),
    free_slots_(std::move(other.free_slots_)) {
    other.layer_count_ = 0;
    other.allocated_layers_ = 0;
    other.total_sprite_count_ = 0;
    other.current_layer_ = 0;
    other.next_x_ = 0;
    other.next_y_ = 0;
}

TextureAtlas& TextureAtlas::operator=(TextureAtlas&& other) noexcept {
    if (this != &other) {
        release();
        vkContext_ = other.vkContext_;
        texture_id_ = std::move(other.texture_id_);
        sampler_ = std::exchange(other.sampler_, VK_NULL_HANDLE);
        layer_count_ = other.layer_count_;
        allocated_layers_ = other.allocated_layers_;
        total_sprite_count_ = other.total_sprite_count_;
        current_layer_ = other.current_layer_;
        next_x_ = other.next_x_;
        next_y_ = other.next_y_;
        free_slots_ = std::move(other.free_slots_);
        other.layer_count_ = 0;
        other.allocated_layers_ = 0;
        other.total_sprite_count_ = 0;
        other.current_layer_ = 0;
        other.next_x_ = 0;
        other.next_y_ = 0;
    }
    return *this;
}

bool TextureAtlas::initialize(int initial_layers) {
    if (texture_id_) {
        return true; // Already initialized
    }

    if (initial_layers < 1) {
        initial_layers = 1;
    }
    if (initial_layers > MAX_LAYERS) {
        initial_layers = MAX_LAYERS;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = ATLAS_SIZE;
    imageInfo.extent.height = ATLAS_SIZE;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = MAX_LAYERS; // Allocate max upfront or resize later, choosing max for simplicity here
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    texture_id_ = std::make_unique<VulkanImage>(vkContext_->GetAllocator(), vkContext_->GetDevice(), imageInfo, VMA_MEMORY_USAGE_GPU_ONLY);
    texture_id_->CreateView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, MAX_LAYERS);

    // Transition image to TRANSFER_DST_OPTIMAL
    VkCommandBuffer cmdBuffer = vkContext_->BeginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture_id_->GetImage();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = MAX_LAYERS;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkContext_->EndSingleTimeCommands(cmdBuffer);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(vkContext_->GetDevice(), &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
        spdlog::error("Failed to create texture sampler!");
        return false;
    }

    allocated_layers_ = initial_layers;
    layer_count_ = 1; // Start with one active layer
    current_layer_ = 0;
    next_x_ = 0;
    next_y_ = 0;

    spdlog::info("TextureAtlas created: {}x{} x {} layers", ATLAS_SIZE, ATLAS_SIZE, MAX_LAYERS);
    return true;
}

bool TextureAtlas::addLayer() {
    if (layer_count_ >= MAX_LAYERS) {
        spdlog::error("TextureAtlas: Max layers ({}) reached", MAX_LAYERS);
        return false;
    }

    layer_count_++;
    current_layer_ = layer_count_ - 1;
    next_x_ = 0;
    next_y_ = 0;
    spdlog::info("TextureAtlas: Added layer {}", current_layer_);
    return true;
}

std::optional<AtlasRegion> TextureAtlas::addSprite(const uint8_t* rgba_data) {
    int px = -1, py = -1, pl = -1;

    if (!free_slots_.empty()) {
        FreeSlot slot = free_slots_.back();
        free_slots_.pop_back();
        px = slot.pixel_x;
        py = slot.pixel_y;
        pl = slot.layer;
    } else {
        if (next_x_ >= ATLAS_SIZE) {
            next_x_ = 0;
            next_y_ += SPRITE_SIZE;
        }

        if (next_y_ >= ATLAS_SIZE) {
            if (!addLayer()) {
                return std::nullopt;
            }
        }

        px = next_x_;
        py = next_y_;
        pl = current_layer_;

        next_x_ += SPRITE_SIZE;
    }

    if (!rgba_data) {
        spdlog::error("TextureAtlas: Null pixel data provided");
        return std::nullopt;
    }

    VkDeviceSize imageSize = SPRITE_SIZE * SPRITE_SIZE * 4;
    VulkanBuffer stagingBuffer(vkContext_->GetAllocator(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(vkContext_->GetAllocator(), stagingBuffer.GetAllocation(), &data);
    memcpy(data, rgba_data, static_cast<size_t>(imageSize));
    vmaUnmapMemory(vkContext_->GetAllocator(), stagingBuffer.GetAllocation());

    CopyBufferToImage(stagingBuffer.GetBuffer(), texture_id_->GetImage(), static_cast<uint32_t>(SPRITE_SIZE), static_cast<uint32_t>(SPRITE_SIZE), px, py, pl);

    AtlasRegion region;
    region.atlas_index = pl;
    region.pixel_x = px;
    region.pixel_y = py;
    region.u_min = static_cast<float>(px) / ATLAS_SIZE;
    region.v_min = static_cast<float>(py) / ATLAS_SIZE;
    region.u_max = static_cast<float>(px + SPRITE_SIZE) / ATLAS_SIZE;
    region.v_max = static_cast<float>(py + SPRITE_SIZE) / ATLAS_SIZE;

    total_sprite_count_++;
    return region;
}

void TextureAtlas::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, int x, int y, int layer) {
    VkCommandBuffer cmdBuffer = vkContext_->BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layer to SHADER_READ_ONLY_OPTIMAL for the specific layer and copy block?
    // Doing it globally is too slow for 1 sprite at a time, keeping it optimal requires more tracking.
    // For simplicity of migration, we'll assume it's kept in TRANSFER_DST or transitioned after all uploads.
    // Actually, let's keep it simple: Transfer block to READ_ONLY for reading.
    // In a real optimized system, this would be batched.

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = layer;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkContext_->EndSingleTimeCommands(cmdBuffer);
}

void TextureAtlas::freeSlot(const AtlasRegion& region) {
    if (region.atlas_index == AtlasRegion::INVALID_SENTINEL) {
        return;
    }

    // In OpenGL version, it zeroes out the texture using PBO.
    // In Vulkan, we just mark it as free.
    free_slots_.push_back({region.pixel_x, region.pixel_y, static_cast<int>(region.atlas_index)});
    total_sprite_count_--;
}

void TextureAtlas::release() {
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(vkContext_->GetDevice(), sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    texture_id_.reset();
    layer_count_ = 0;
    allocated_layers_ = 0;
    total_sprite_count_ = 0;
    free_slots_.clear();
}
