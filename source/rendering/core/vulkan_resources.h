#ifndef RME_VULKAN_RESOURCES_H_
#define RME_VULKAN_RESOURCES_H_

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <utility>
#include <spdlog/spdlog.h>

class VulkanBuffer {
public:
    VulkanBuffer() = default;

    VulkanBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
        : allocator_(allocator) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;

        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer_, &allocation_, nullptr) != VK_SUCCESS) {
            spdlog::error("Failed to create VulkanBuffer!");
        }
    }

    ~VulkanBuffer() {
        Cleanup();
    }

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VulkanBuffer(VulkanBuffer&& other) noexcept
        : allocator_(other.allocator_),
          buffer_(std::exchange(other.buffer_, VK_NULL_HANDLE)),
          allocation_(std::exchange(other.allocation_, VK_NULL_HANDLE)) {}

    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept {
        if (this != &other) {
            Cleanup();
            allocator_ = other.allocator_;
            buffer_ = std::exchange(other.buffer_, VK_NULL_HANDLE);
            allocation_ = std::exchange(other.allocation_, VK_NULL_HANDLE);
        }
        return *this;
    }

    void Cleanup() {
        if (buffer_ != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator_, buffer_, allocation_);
            buffer_ = VK_NULL_HANDLE;
            allocation_ = VK_NULL_HANDLE;
        }
    }

    VkBuffer GetBuffer() const { return buffer_; }
    VmaAllocation GetAllocation() const { return allocation_; }
    bool IsValid() const { return buffer_ != VK_NULL_HANDLE; }

private:
    VmaAllocator allocator_ = VK_NULL_HANDLE;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
};

class VulkanImage {
public:
    VulkanImage() = default;

    VulkanImage(VmaAllocator allocator, VkDevice device, const VkImageCreateInfo& imageInfo, VmaMemoryUsage memoryUsage)
        : allocator_(allocator), device_(device) {

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;

        if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image_, &allocation_, nullptr) != VK_SUCCESS) {
            spdlog::error("Failed to create VulkanImage!");
        }
    }

    ~VulkanImage() {
        Cleanup();
    }

    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;

    VulkanImage(VulkanImage&& other) noexcept
        : allocator_(other.allocator_),
          device_(other.device_),
          image_(std::exchange(other.image_, VK_NULL_HANDLE)),
          allocation_(std::exchange(other.allocation_, VK_NULL_HANDLE)),
          view_(std::exchange(other.view_, VK_NULL_HANDLE)) {}

    VulkanImage& operator=(VulkanImage&& other) noexcept {
        if (this != &other) {
            Cleanup();
            allocator_ = other.allocator_;
            device_ = other.device_;
            image_ = std::exchange(other.image_, VK_NULL_HANDLE);
            allocation_ = std::exchange(other.allocation_, VK_NULL_HANDLE);
            view_ = std::exchange(other.view_, VK_NULL_HANDLE);
        }
        return *this;
    }

    void Cleanup() {
        if (view_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, view_, nullptr);
            view_ = VK_NULL_HANDLE;
        }
        if (image_ != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator_, image_, allocation_);
            image_ = VK_NULL_HANDLE;
            allocation_ = VK_NULL_HANDLE;
        }
    }

    void CreateView(VkFormat format, VkImageAspectFlags aspectFlags, uint32_t layerCount = 1) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image_;
        viewInfo.viewType = layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = layerCount;

        if (vkCreateImageView(device_, &viewInfo, nullptr, &view_) != VK_SUCCESS) {
            spdlog::error("Failed to create texture image view!");
        }
    }

    VkImage GetImage() const { return image_; }
    VkImageView GetView() const { return view_; }
    bool IsValid() const { return image_ != VK_NULL_HANDLE; }

private:
    VmaAllocator allocator_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkImage image_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
};

#endif
