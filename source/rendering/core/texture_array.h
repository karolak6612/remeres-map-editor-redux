#ifndef RME_RENDERING_CORE_TEXTURE_ARRAY_H_
#define RME_RENDERING_CORE_TEXTURE_ARRAY_H_

#include "app/main.h"
#include "rendering/core/vulkan_context.h"
#include "rendering/core/vulkan_resources.h"
#include <vector>
#include <memory>

class TextureArray {
public:
    TextureArray(VulkanContext* vkContext);
    ~TextureArray();

    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;

    bool initialize(int width, int height, int maxLayers);
    bool uploadLayer(int layer, const uint8_t* rgbaData);
    int allocateLayer();

    VkImageView GetView() const { return texture_ ? texture_->GetView() : VK_NULL_HANDLE; }
    VkSampler GetSampler() const { return sampler_; }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    int getMaxLayers() const { return maxLayers_; }
    int getAllocatedLayers() const { return allocatedLayers_; }
    bool isInitialized() const { return initialized_; }

    void cleanup();

private:
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, int layer);

    VulkanContext* vkContext_ = nullptr;
    std::unique_ptr<VulkanImage> texture_;
    VkSampler sampler_ = VK_NULL_HANDLE;
    int width_ = 0;
    int height_ = 0;
    int maxLayers_ = 0;
    int allocatedLayers_ = 0;
    bool initialized_ = false;
};

#endif
