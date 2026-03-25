//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/drawers/map_layer_drawer.h"

#include <algorithm>

#include "app/definitions.h"
#include "map/map.h"
#include "map/map_region.h"
#include "rendering/core/chunk_source_snapshot.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/map_access.h"
#include "rendering/core/pending_node_requests.h"
#include "rendering/core/render_chunk_key.h"
#include "rendering/core/render_view.h"
#include "rendering/core/tile_planning_pool.h"
#include "rendering/core/tile_render_snapshot_builder.h"
#include "rendering/drawers/overlays/grid_drawer.h"
#include "rendering/drawers/tiles/tile_renderer.h"

namespace {

[[nodiscard]] int floorDiv(int value, int divisor)
{
    const int quotient = value / divisor;
    const int remainder = value % divisor;
    return remainder < 0 ? quotient - 1 : quotient;
}

[[nodiscard]] size_t estimateChunkTileCapacity()
{
    return static_cast<size_t>(RENDER_CHUNK_SIZE) * static_cast<size_t>(RENDER_CHUNK_SIZE);
}

[[nodiscard]] size_t estimateChunkNodeCapacity()
{
    constexpr int nodes_per_side = RENDER_CHUNK_SIZE / 4;
    return static_cast<size_t>(nodes_per_side) * static_cast<size_t>(nodes_per_side);
}

[[nodiscard]] size_t estimateSideEffectCapacity(size_t tile_capacity, bool enabled, size_t divisor, size_t minimum_capacity)
{
    if (!enabled) {
        return 0;
    }
    return std::max(minimum_capacity, tile_capacity / divisor);
}

} // namespace

MapLayerDrawer::MapLayerDrawer(
    TileRenderer* tile_renderer, GridDrawer* grid_drawer, IMapAccess* map_access, PendingNodeRequests* pending_requests
) :
    tile_renderer(tile_renderer), grid_drawer(grid_drawer), map_access(map_access), pending_requests_(pending_requests)
{
}

MapLayerDrawer::~MapLayerDrawer() { }

VisibleChunkList MapLayerDrawer::BuildVisibleChunkList(int map_z, const FloorViewParams& floor_params) const
{
    constexpr int safety_margin_tiles = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
    const int start_chunk_x = floorDiv(floor_params.start_x - safety_margin_tiles, RENDER_CHUNK_SIZE);
    const int start_chunk_y = floorDiv(floor_params.start_y - safety_margin_tiles, RENDER_CHUNK_SIZE);
    const int end_chunk_x = floorDiv(floor_params.end_x + safety_margin_tiles, RENDER_CHUNK_SIZE);
    const int end_chunk_y = floorDiv(floor_params.end_y + safety_margin_tiles, RENDER_CHUNK_SIZE);

    VisibleChunkList visible_chunks {.map_z = map_z};
    visible_chunks.chunks.reserve(
        static_cast<size_t>(std::max(0, end_chunk_x - start_chunk_x + 1)) * static_cast<size_t>(std::max(0, end_chunk_y - start_chunk_y + 1))
    );

    for (int chunk_x = start_chunk_x; chunk_x <= end_chunk_x; ++chunk_x) {
        for (int chunk_y = start_chunk_y; chunk_y <= end_chunk_y; ++chunk_y) {
            visible_chunks.chunks.push_back(RenderChunkKey {.map_z = map_z, .chunk_x = chunk_x, .chunk_y = chunk_y});
        }
    }

    return visible_chunks;
}

ChunkSourceSnapshot MapLayerDrawer::BuildChunkSourceSnapshot(
    const FramePlanContext& ctx, const RenderChunkKey& key, bool live_client, uint32_t current_house_id
)
{
    const auto& view = ctx.view;
    const auto& settings = ctx.settings;
    const int chunk_start_x = key.startX();
    const int chunk_start_y = key.startY();
    const int node_start_x = chunk_start_x;
    const int node_start_y = chunk_start_y;
    const int node_end_x = chunk_start_x + RENDER_CHUNK_SIZE - 4;
    const int node_end_y = chunk_start_y + RENDER_CHUNK_SIZE - 4;

    ChunkSourceSnapshot chunk_snapshot {.key = key};
    chunk_snapshot.variant = RenderVariantKey::From(ctx.view, ctx.settings, ctx.frame);
    chunk_snapshot.reserve(
        estimateChunkTileCapacity(),
        estimateChunkNodeCapacity(),
        estimateSideEffectCapacity(estimateChunkTileCapacity(), settings.show_tooltips, 8, 16),
        estimateSideEffectCapacity(estimateChunkTileCapacity(), settings.show_hooks, 16, 8),
        estimateSideEffectCapacity(estimateChunkTileCapacity(), settings.highlight_locked_doors, 16, 8),
        estimateSideEffectCapacity(estimateChunkTileCapacity(), settings.show_creatures, 32, 4)
    );

    auto collectNode = [&](MapNode* node, int nd_map_x, int nd_map_y, bool live) {
        if (!node) {
            return;
        }

        if (live && !node->isVisible(key.map_z > GROUND_LAYER)) {
            if (!node->isRequested(key.map_z > GROUND_LAYER)) {
                if (pending_requests_) {
                    pending_requests_->enqueue(nd_map_x, nd_map_y, key.map_z > GROUND_LAYER);
                }
                node->setRequested(key.map_z > GROUND_LAYER, true);
            }
            chunk_snapshot.loading_placeholders.push_back(LoadingPlaceholderSnapshot {
                .pos = Position(nd_map_x, nd_map_y, key.map_z),
                .width = TILE_SIZE * 4,
                .height = TILE_SIZE * 4,
                .color = DrawColor(255, 0, 255, 128),
            });
            return;
        }

        Floor* floor = node->getFloor(key.map_z);
        if (!floor) {
            return;
        }

        TileLocation* location = floor->locs.data();
        for (int local_x = 0; local_x < 4; ++local_x) {
            for (int local_y = 0; local_y < 4; ++local_y, ++location) {
                if (!location->get()) [[likely]] {
                    continue;
                }

                const int tile_map_x = nd_map_x + local_x;
                const int tile_map_y = nd_map_y + local_y;
                const int local_draw_x = (tile_map_x - chunk_start_x) * TILE_SIZE;
                const int local_draw_y = (tile_map_y - chunk_start_y) * TILE_SIZE;
                const Map* map = map_access ? &map_access->getMap() : nullptr;
                static_cast<void>(TileRenderSnapshotBuilder::BuildInto(
                    chunk_snapshot, *location, view, settings, ctx.frame, map, current_house_id, local_draw_x, local_draw_y
                ));
            }
        }
    };

    if (live_client) {
        for (int nd_map_x = node_start_x; nd_map_x <= node_end_x; nd_map_x += 4) {
            for (int nd_map_y = node_start_y; nd_map_y <= node_end_y; nd_map_y += 4) {
                MapNode* node = map_access->getMap().getLeaf(nd_map_x, nd_map_y);
                if (!node) {
                    node = map_access->getMap().createLeaf(nd_map_x, nd_map_y);
                    node->setVisible(false, false);
                }
                collectNode(node, nd_map_x, nd_map_y, true);
            }
        }
    } else {
        for (int nd_map_x = node_start_x; nd_map_x <= node_end_x; nd_map_x += 4) {
            for (int nd_map_y = node_start_y; nd_map_y <= node_end_y; nd_map_y += 4) {
                collectNode(map_access->getMap().getLeaf(nd_map_x, nd_map_y), nd_map_x, nd_map_y, false);
            }
        }
    }

    return chunk_snapshot;
}
