#include "rendering/core/pixel_buffer_object.h"

PixelBufferObject::PixelBufferObject() {}

PixelBufferObject::~PixelBufferObject() {
    release();
}

bool PixelBufferObject::initialize(VkDeviceSize size) {
    size_ = size;
    // Vulkan VMA allocation logic for PBOs
    return true;
}

void* PixelBufferObject::map() {
    return mapped_ptr_;
}

void PixelBufferObject::unmap() {

}

void PixelBufferObject::swap() {
    current_index_ = (current_index_ + 1) % 2;
}

void PixelBufferObject::release() {}

// Dummy unpack to make sure signatures match mostly
void PixelBufferObject::unpackToTexture(int width, int height, int layer) {}

