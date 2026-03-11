#ifndef RME_RENDERING_CORE_PREPARED_RENDER_CHUNK_H_
#define RME_RENDERING_CORE_PREPARED_RENDER_CHUNK_H_

#include <cstdint>

#include "rendering/core/draw_command_queue.h"
#include "rendering/core/frame_accumulators.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/render_chunk_key.h"
#include "rendering/core/render_variant_key.h"
#include "rendering/core/sprite_preload_queue.h"

struct PreparedRenderChunk {
    RenderChunkKey key;
    RenderVariantKey variant;
    uint64_t generation = 0;
    FrameAccumulators accumulators;
    LightBuffer lights;
    DrawCommandQueue commands;
    std::vector<SpritePreloadQueue::Request> preload_requests;

    void clearTransientData()
    {
        accumulators.clear();
        lights.Clear();
        commands.clear();
        preload_requests.clear();
    }

    void reserve(size_t light_capacity, size_t hook_capacity, size_t door_capacity, size_t creature_capacity, size_t command_capacity)
    {
        lights.reserve(light_capacity);
        accumulators.reserve(hook_capacity, door_capacity, creature_capacity);
        commands.reserve(command_capacity);
        preload_requests.reserve(command_capacity);
    }
};

#endif
