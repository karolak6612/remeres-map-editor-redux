#ifndef RME_RENDERING_CORE_RENDER_PREP_SNAPSHOT_H_
#define RME_RENDERING_CORE_RENDER_PREP_SNAPSHOT_H_

#include <cstdint>
#include <vector>

#include "rendering/core/draw_frame.h"
#include "rendering/core/tile_render_snapshot.h"

struct RenderPrepSnapshot {
    uint64_t generation = 0;
    uint32_t atlas_version = 0;
    DrawFrame frame;
    std::vector<VisibleFloorSnapshot> floors;
};

#endif
