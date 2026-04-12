#include "rendering/core/ring_buffer.h"

RingBuffer::RingBuffer() {}
RingBuffer::~RingBuffer() { release(); }

bool RingBuffer::initialize(VkDeviceSize capacity) {
    capacity_ = capacity;
    return true;
}

void* RingBuffer::map(VkDeviceSize size, VkDeviceSize& out_offset) {
    out_offset = head_;
    head_ += size;
    return mapped_ptr_; // In complete Vulkan implementation, map + return proper pointer
}

void RingBuffer::unmap() {}
void RingBuffer::flush(VkDeviceSize offset, VkDeviceSize size) {}
void RingBuffer::lockSection(VkDeviceSize offset, VkDeviceSize size) {}
void RingBuffer::release() {}

