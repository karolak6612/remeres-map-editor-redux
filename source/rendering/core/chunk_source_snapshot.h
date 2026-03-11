#ifndef RME_RENDERING_CORE_CHUNK_SOURCE_SNAPSHOT_H_
#define RME_RENDERING_CORE_CHUNK_SOURCE_SNAPSHOT_H_

#include <vector>

#include "rendering/core/render_chunk_key.h"
#include "rendering/core/render_variant_key.h"
#include "rendering/core/tile_render_snapshot.h"
#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include "rendering/ui/tooltip_data.h"

struct VisibleChunkList {
    int map_z = 0;
    std::vector<RenderChunkKey> chunks;
};

struct ChunkSourceSnapshot {
    RenderChunkKey key;
    RenderVariantKey variant;
    std::vector<TileRenderSnapshot> tiles;
    std::vector<LoadingPlaceholderSnapshot> loading_placeholders;
    std::vector<TooltipData> tooltips;
    std::vector<HookIndicatorDrawer::HookRequest> hooks;
    std::vector<DoorIndicatorDrawer::DoorRequest> doors;
    std::vector<CreatureLabel> creature_names;

    void reserve(
        size_t tile_capacity, size_t placeholder_capacity, size_t tooltip_capacity, size_t hook_capacity, size_t door_capacity,
        size_t creature_capacity
    )
    {
        tiles.reserve(tile_capacity);
        loading_placeholders.reserve(placeholder_capacity);
        tooltips.reserve(tooltip_capacity);
        hooks.reserve(hook_capacity);
        doors.reserve(door_capacity);
        creature_names.reserve(creature_capacity);
    }

    [[nodiscard]] size_t estimatedPreloadCount() const
    {
        size_t requests = 0;
        for (const auto& tile : tiles) {
            requests += tile.items.size();
            requests += tile.ground ? 1U : 0U;
        }
        return requests;
    }

    [[nodiscard]] size_t estimatedCommandCount() const
    {
        size_t commands = loading_placeholders.size();
        for (const auto& tile : tiles) {
            commands += tile.items.size();
            commands += tile.ground ? 1U : 0U;
            commands += tile.creature ? 1U : 0U;
            commands += tile.marker ? 1U : 0U;
            commands += 3U;
        }
        return commands;
    }
};

#endif
