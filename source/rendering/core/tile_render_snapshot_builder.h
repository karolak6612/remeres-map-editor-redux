#ifndef RME_RENDERING_CORE_TILE_RENDER_SNAPSHOT_BUILDER_H_
#define RME_RENDERING_CORE_TILE_RENDER_SNAPSHOT_BUILDER_H_

#include "rendering/core/tile_render_snapshot.h"

#include <optional>

class Map;
class TileLocation;
struct FrameOptions;
struct RenderSettings;
struct ViewState;

class TileRenderSnapshotBuilder {
public:
    [[nodiscard]] static std::optional<TileRenderSnapshot> Build(
        TileLocation& location, const ViewState& view, const RenderSettings& settings, const FrameOptions& frame, const Map* map,
        uint32_t current_house_id, int draw_x, int draw_y
    );
};

#endif
