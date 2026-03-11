//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "rendering/drawers/map_layer_drawer.h"
#include "app/definitions.h"
#include "app/main.h"
#include "map/map.h"
#include "map/map_region.h"
#include "rendering/core/tile_render_snapshot.h"
#include "rendering/core/tile_render_snapshot_builder.h"
#include "rendering/core/map_access.h"
#include "rendering/core/draw_command_queue.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/prepared_frame_buffer.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/pending_node_requests.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/tile_planning_pool.h"
#include "rendering/core/sprite_preloader.h"
#include "rendering/drawers/overlays/grid_drawer.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/drawers/tiles/tile_draw_plan.h"

#include <vector>

MapLayerDrawer::MapLayerDrawer(
    TileRenderer* tile_renderer, GridDrawer* grid_drawer, IMapAccess* map_access, PendingNodeRequests* pending_requests
) :
    tile_renderer(tile_renderer), grid_drawer(grid_drawer), map_access(map_access), pending_requests_(pending_requests)
{
}

MapLayerDrawer::~MapLayerDrawer() { }

void MapLayerDrawer::MergePlans(std::span<const TileDrawPlan> plans, PreparedFrameBuffer& prepared)
{
    for (const auto& plan : plans) {
        if (!plan.valid) {
            continue;
        }
        tile_renderer->MergePlanSideEffects(plan, prepared.accumulators, prepared.lights);
        prepared.preload_requests.insert(prepared.preload_requests.end(), plan.preload_requests.begin(), plan.preload_requests.end());
    }
}

void MapLayerDrawer::PlanTilesParallel(
    const FramePlanContext& ctx, bool draw_lights, std::span<const TileRenderSnapshot> tiles, std::span<TileDrawPlan> plans
)
{
    if (tiles.empty()) {
        return;
    }

    if (planning_pool_) {
        planning_pool_->Plan(*tile_renderer, ctx, draw_lights, tiles, plans);
        return;
    }

    for (size_t i = 0; i < tiles.size(); ++i) {
        plans[i].clear();
        tile_renderer->PlanTile(ctx, tiles[i], draw_lights, plans[i]);
    }
}

VisibleFloorSnapshot MapLayerDrawer::BuildVisibleFloorSnapshot(
    const FramePlanContext& ctx, int map_z, bool live_client, const FloorViewParams& floor_params, uint32_t current_house_id
)
{
    const auto& view = ctx.view;
    const auto& settings = ctx.settings;
    int nd_start_x = floor_params.start_x & ~3;
    int nd_start_y = floor_params.start_y & ~3;
    int nd_end_x = (floor_params.end_x & ~3) + 4;
    int nd_end_y = (floor_params.end_y & ~3) + 4;

    const int offset = (map_z <= GROUND_LAYER) ? (GROUND_LAYER - map_z) * TILE_SIZE : TILE_SIZE * (view.floor - map_z);
    const int base_screen_x = -view.view_scroll_x - offset;
    const int base_screen_y = -view.view_scroll_y - offset;

    VisibleFloorSnapshot floor_snapshot {.map_z = map_z};
    floor_snapshot.tiles.reserve(2048);

    auto collectNode = [&](MapNode* nd, int nd_map_x, int nd_map_y, bool live) {
        const int node_draw_x = nd_map_x * TILE_SIZE + base_screen_x;
        const int node_draw_y = nd_map_y * TILE_SIZE + base_screen_y;

        if (!view.IsRectVisible(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
            return;
        }

        if (live && !nd->isVisible(map_z > GROUND_LAYER)) {
            if (!nd->isRequested(map_z > GROUND_LAYER)) {
                if (pending_requests_) {
                    pending_requests_->enqueue(nd_map_x, nd_map_y, map_z > GROUND_LAYER);
                }
                nd->setRequested(map_z > GROUND_LAYER, true);
            }
            floor_snapshot.loading_placeholders.push_back(LoadingPlaceholderSnapshot {
                .draw_x = nd_map_x * TILE_SIZE + base_screen_x,
                .draw_y = nd_map_y * TILE_SIZE + base_screen_y,
                .width = TILE_SIZE * 4,
                .height = TILE_SIZE * 4,
                .color = DrawColor(255, 0, 255, 128),
            });
            return;
        }

        const bool fully_inside = view.IsRectFullyInside(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE);
        Floor* floor = nd->getFloor(map_z);
        if (!floor) {
            return;
        }

        TileLocation* location = floor->locs.data();
        int draw_x_base = node_draw_x;
        for (int map_x = 0; map_x < 4; ++map_x, draw_x_base += TILE_SIZE) {
            int draw_y = node_draw_y;
            for (int map_y = 0; map_y < 4; ++map_y, ++location, draw_y += TILE_SIZE) {
                if (!location->get()) [[likely]] {
                    continue;
                }
                if (!fully_inside && !view.IsPixelVisible(draw_x_base, draw_y, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
                    continue;
                }

                const Map* map = map_access ? &map_access->getMap() : nullptr;
                if (auto snapshot = TileRenderSnapshotBuilder::Build(*location, ctx.view, settings, ctx.frame, map, current_house_id, draw_x_base, draw_y)) {
                    floor_snapshot.tiles.push_back(std::move(*snapshot));
                }
            }
        }
    };

    if (live_client) {
        for (int nd_map_x = nd_start_x; nd_map_x <= nd_end_x; nd_map_x += 4) {
            for (int nd_map_y = nd_start_y; nd_map_y <= nd_end_y; nd_map_y += 4) {
                MapNode* nd = map_access->getMap().getLeaf(nd_map_x, nd_map_y);
                if (!nd) {
                    nd = map_access->getMap().createLeaf(nd_map_x, nd_map_y);
                    nd->setVisible(false, false);
                }
                collectNode(nd, nd_map_x, nd_map_y, true);
            }
        }
    } else {
        const int safe_start_x = nd_start_x - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
        const int safe_start_y = nd_start_y - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
        const int safe_end_x = nd_end_x + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
        const int safe_end_y = nd_end_y + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;

        map_access->getMap().visitLeaves(safe_start_x, safe_start_y, safe_end_x, safe_end_y, [&](MapNode* nd, int nd_map_x, int nd_map_y) {
            collectNode(nd, nd_map_x, nd_map_y, false);
        });
    }

    return floor_snapshot;
}

PreparedFloorRange MapLayerDrawer::PrepareFloor(const FramePlanContext& ctx, const VisibleFloorSnapshot& floor_snapshot, PreparedFrameBuffer& prepared)
{
    const bool draw_lights = ctx.settings.isDrawLight() && ctx.view.zoom <= 10.0;
    std::vector<TileDrawPlan> plans(floor_snapshot.tiles.size());
    for (auto& plan : plans) {
        plan.reserve();
    }

    PlanTilesParallel(ctx, draw_lights, floor_snapshot.tiles, plans);
    MergePlans(plans, prepared);

    const size_t command_start = prepared.commands.size();
    for (const auto& placeholder : floor_snapshot.loading_placeholders) {
        prepared.commands.push(DrawFilledRectCmd {
            .draw_x = placeholder.draw_x,
            .draw_y = placeholder.draw_y,
            .width = placeholder.width,
            .height = placeholder.height,
            .color = placeholder.color,
        });
    }

    for (auto& plan : plans) {
        if (!plan.valid) {
            continue;
        }
        tile_renderer->QueuePlanCommands(plan, prepared.commands);
    }

    return PreparedFloorRange {
        .map_z = floor_snapshot.map_z,
        .command_start = command_start,
        .command_count = prepared.commands.size() - command_start,
    };
}
