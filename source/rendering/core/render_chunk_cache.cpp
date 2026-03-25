#include "app/main.h"

#include "rendering/core/render_chunk_cache.h"

#include <algorithm>

void RenderChunkCache::setCapacity(size_t capacity)
{
    std::lock_guard lock(mutex_);
    capacity_ = std::max<size_t>(capacity, 1);
    evictIfNeededLocked();
}

std::shared_ptr<const PreparedRenderChunk> RenderChunkCache::find(const RenderChunkKey& key, const RenderVariantKey& variant) const
{
    std::lock_guard lock(mutex_);
    auto it = entries_.find(RenderChunkCacheKey {.chunk = key, .variant = variant});
    if (it == entries_.end()) {
        return nullptr;
    }
    it->second.last_used = ++access_clock_;
    return it->second.chunk;
}

void RenderChunkCache::store(std::shared_ptr<const PreparedRenderChunk> chunk)
{
    if (!chunk) {
        return;
    }

    std::lock_guard lock(mutex_);
    const RenderChunkCacheKey cache_key {.chunk = chunk->key, .variant = chunk->variant};
    const auto last_used = ++access_clock_;
    entries_[cache_key] = Entry {
        .chunk = std::move(chunk),
        .last_used = last_used,
    };
    evictIfNeededLocked();
}

void RenderChunkCache::invalidateAll()
{
    std::lock_guard lock(mutex_);
    entries_.clear();
    access_clock_ = 0;
}

size_t RenderChunkCache::size() const
{
    std::lock_guard lock(mutex_);
    return entries_.size();
}

void RenderChunkCache::evictIfNeededLocked()
{
    while (entries_.size() > capacity_) {
        const auto victim = std::min_element(entries_.begin(), entries_.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.second.last_used < rhs.second.last_used;
        });
        if (victim == entries_.end()) {
            return;
        }
        entries_.erase(victim);
    }
}
