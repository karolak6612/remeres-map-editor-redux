#include "rendering/core/texture_array.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstring>

TextureArray::TextureArray(VulkanContext* vkContext) : vkContext_(vkContext) {}

TextureArray::~TextureArray() {
    cleanup();
}

bool TextureArray::initialize(int width, int height, int maxLayers) {
    if (initialized_) return true;

    width_ = width;
    height_ = height;
    maxLayers_ = maxLayers;
    allocatedLayers_ = 0;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = maxLayers;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    texture_ = std::make_unique<VulkanImage>(vkContext_->GetAllocator(), vkContext_->GetDevice(), imageInfo, VMA_MEMORY_USAGE_GPU_ONLY);
    texture_->CreateView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, maxLayers);

    // Transition image to TRANSFER_DST_OPTIMAL
    VkCommandBuffer cmdBuffer = vkContext_->BeginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture_->GetImage();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = maxLayers;
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

    initialized_ = true;
    return true;
}

void TextureArray::cleanup() {
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(vkContext_->GetDevice(), sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    texture_.reset();
    initialized_ = false;
    allocatedLayers_ = 0;
}

int TextureArray::allocateLayer() {
    if (!initialized_) return -1;
    if (allocatedLayers_ >= maxLayers_) {
        spdlog::warn("TextureArray is full! Cannot allocate layer.");
        return -1;
    }
    return allocatedLayers_++;
}

bool TextureArray::uploadLayer(int layer, const uint8_t* rgbaData) {
    if (!initialized_ || layer < 0 || layer >= maxLayers_ || !rgbaData) return false;

    VkDeviceSize imageSize = width_ * height_ * 4;
    VulkanBuffer stagingBuffer(vkContext_->GetAllocator(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(vkContext_->GetAllocator(), stagingBuffer.GetAllocation(), &data);
    memcpy(data, rgbaData, static_cast<size_t>(imageSize));
    vmaUnmapMemory(vkContext_->GetAllocator(), stagingBuffer.GetAllocation());

    CopyBufferToImage(stagingBuffer.GetBuffer(), texture_->GetImage(), static_cast<uint32_t>(width_), static_cast<uint32_t>(height_), layer);
    return true;
}

void TextureArray::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, int layer) {
    VkCommandBuffer cmdBuffer = vkContext_->BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layer to SHADER_READ_ONLY_OPTIMAL
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
