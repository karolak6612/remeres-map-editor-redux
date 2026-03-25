#ifndef RME_RENDERING_CORE_RENDER_CHUNK_CACHE_H_
#define RME_RENDERING_CORE_RENDER_CHUNK_CACHE_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "rendering/core/prepared_render_chunk.h"
#include "rendering/core/render_chunk_key.h"
#include "rendering/core/render_variant_key.h"

struct RenderChunkCacheKey {
    RenderChunkKey chunk;
    RenderVariantKey variant;

    [[nodiscard]] bool operator==(const RenderChunkCacheKey& other) const = default;
};

struct RenderChunkCacheKeyHash {
    [[nodiscard]] size_t operator()(const RenderChunkCacheKey& key) const noexcept
    {
        return RenderChunkKeyHash {}(key.chunk) ^ (RenderVariantKeyHash {}(key.variant) << 1U);
    }
};

class RenderChunkCache {
public:
    void setCapacity(size_t capacity);
    [[nodiscard]] std::shared_ptr<const PreparedRenderChunk> find(const RenderChunkKey& key, const RenderVariantKey& variant) const;
    void store(std::shared_ptr<const PreparedRenderChunk> chunk);
    void invalidateAll();
    [[nodiscard]] size_t size() const;

private:
    struct Entry {
        std::shared_ptr<const PreparedRenderChunk> chunk;
        uint64_t last_used = 0;
    };

    void evictIfNeededLocked();

    mutable std::mutex mutex_;
    mutable std::unordered_map<RenderChunkCacheKey, Entry, RenderChunkCacheKeyHash> entries_;
    size_t capacity_ = 512;
    mutable uint64_t access_clock_ = 0;
};

#endif
