#ifndef RME_RENDERING_CORE_PREPARED_RENDER_CHUNK_BUILDER_H_
#define RME_RENDERING_CORE_PREPARED_RENDER_CHUNK_BUILDER_H_

#include <memory>

#include "rendering/core/chunk_source_snapshot.h"
#include "rendering/core/prepared_render_chunk.h"

class TileRenderer;
struct FramePlanContext;

class PreparedRenderChunkBuilder {
public:
    [[nodiscard]] static std::shared_ptr<const PreparedRenderChunk> Build(
        TileRenderer& tile_renderer, const FramePlanContext& ctx, const ChunkSourceSnapshot& chunk_snapshot, uint64_t generation
    );
};

#endif
