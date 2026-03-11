#ifndef RME_RENDERING_CORE_PREPARED_FRAME_BUFFER_H_
#define RME_RENDERING_CORE_PREPARED_FRAME_BUFFER_H_

#include <cstdint>

#include "rendering/core/draw_command_queue.h"
#include "rendering/core/draw_frame.h"
#include "rendering/core/frame_accumulators.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/sprite_preload_queue.h"
#include <vector>

struct PreparedFloorRange {
    int map_z = 0;
    size_t command_start = 0;
    size_t command_count = 0;
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
    DrawCommandQueue commands;
    std::vector<SpritePreloadQueue::Request> preload_requests;
    std::vector<PreparedFloorRange> floor_ranges;

    void clearTransientData()
    {
        accumulators.clear();
        lights.Clear();
        commands.clear();
        preload_requests.clear();
        floor_ranges.clear();
    }

    void reserve(size_t light_capacity, size_t hook_capacity, size_t door_capacity, size_t creature_capacity, size_t command_capacity)
    {
        lights.reserve(light_capacity);
        accumulators.reserve(hook_capacity, door_capacity, creature_capacity);
        commands.reserve(command_capacity);
        preload_requests.reserve(command_capacity);
        floor_ranges.reserve(16);
    }
};

#endif
