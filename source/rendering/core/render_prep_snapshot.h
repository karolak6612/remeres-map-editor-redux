#ifndef RME_RENDERING_CORE_RENDER_PREP_SNAPSHOT_H_
#define RME_RENDERING_CORE_RENDER_PREP_SNAPSHOT_H_

#include <cstdint>
#include <vector>

#include "rendering/core/chunk_source_snapshot.h"
#include "rendering/core/draw_frame.h"
#include "rendering/core/render_variant_key.h"

struct RenderPrepSnapshot {
    uint64_t generation = 0;
    uint32_t atlas_version = 0;
    DrawFrame frame;
    RenderVariantKey variant;
    std::vector<VisibleChunkList> floors;
    std::vector<ChunkSourceSnapshot> dirty_chunks;
};

#endif
