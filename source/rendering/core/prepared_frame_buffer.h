#ifndef RME_RENDERING_CORE_PREPARED_FRAME_BUFFER_H_
#define RME_RENDERING_CORE_PREPARED_FRAME_BUFFER_H_

#include <cstdint>
#include <memory>

#include "rendering/core/draw_frame.h"
#include "rendering/core/frame_accumulators.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/prepared_render_chunk.h"
#include "rendering/core/sprite_preload_queue.h"
#include <vector>

struct PreparedVisibleFloor {
    int map_z = 0;
    std::vector<std::shared_ptr<const PreparedRenderChunk>> chunks;
};

// Cross-thread handoff buffer for prepared rendering work.
// Today it primarily carries the built DrawFrame, while the remaining
// per-frame buffers are reserved for the snapshot-driven planning
// pipeline that will populate them off the main thread.
struct PreparedFrameBuffer {
    uint64_t generation = 0;
    uint32_t atlas_version = 0;
    DrawFrame frame;
    FrameAccumulators accumulators;
    LightBuffer lights;
    std::vector<SpritePreloadQueue::Request> preload_requests;
    std::vector<PreparedVisibleFloor> floors;

    void clearTransientData()
    {
        accumulators.clear();
        lights.Clear();
        preload_requests.clear();
        floors.clear();
    }

    void reserve(size_t light_capacity, size_t hook_capacity, size_t door_capacity, size_t creature_capacity, size_t command_capacity)
    {
        lights.reserve(light_capacity);
        accumulators.reserve(hook_capacity, door_capacity, creature_capacity);
        preload_requests.reserve(command_capacity);
        floors.reserve(16);
    }
};

#endif
