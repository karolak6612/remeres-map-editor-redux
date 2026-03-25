#ifndef RME_RENDERING_CORE_TILE_RENDER_SNAPSHOT_BUILDER_H_
#define RME_RENDERING_CORE_TILE_RENDER_SNAPSHOT_BUILDER_H_

#include "rendering/core/chunk_source_snapshot.h"
#include "rendering/core/tile_render_snapshot.h"

class Map;
class TileLocation;
struct FrameOptions;
struct RenderSettings;
struct ViewState;

class TileRenderSnapshotBuilder {
public:
    [[nodiscard]] static TileRenderSnapshot* BuildInto(
        ChunkSourceSnapshot& chunk_snapshot, TileLocation& location, const ViewState& view, const RenderSettings& settings,
        const FrameOptions& frame, const Map* map, uint32_t current_house_id, int draw_x, int draw_y
    );
};

#endif
