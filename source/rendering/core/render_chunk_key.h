#ifndef RME_RENDERING_CORE_RENDER_CHUNK_KEY_H_
#define RME_RENDERING_CORE_RENDER_CHUNK_KEY_H_

#include <cstddef>

constexpr int RENDER_CHUNK_SIZE = 32;

struct RenderChunkKey {
    int map_z = 0;
    int chunk_x = 0;
    int chunk_y = 0;

    [[nodiscard]] int startX() const
    {
        return chunk_x * RENDER_CHUNK_SIZE;
    }

    [[nodiscard]] int startY() const
    {
        return chunk_y * RENDER_CHUNK_SIZE;
    }

    [[nodiscard]] bool operator==(const RenderChunkKey& other) const = default;
};

struct RenderChunkKeyHash {
    [[nodiscard]] size_t operator()(const RenderChunkKey& key) const noexcept
    {
        const auto z = static_cast<size_t>(static_cast<unsigned int>(key.map_z));
        const auto x = static_cast<size_t>(static_cast<unsigned int>(key.chunk_x));
        const auto y = static_cast<size_t>(static_cast<unsigned int>(key.chunk_y));
        return (z * 73856093U) ^ (x * 19349663U) ^ (y * 83492791U);
    }
};

#endif
