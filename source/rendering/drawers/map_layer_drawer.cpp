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
#include "rendering/core/map_access.h"
#include "rendering/core/draw_command_queue.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/pending_node_requests.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/sprite_preloader.h"
#include "rendering/drawers/overlays/grid_drawer.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/drawers/tiles/tile_draw_plan.h"

#include <algorithm>
#include <thread>
#include <vector>

MapLayerDrawer::MapLayerDrawer(
    TileRenderer* tile_renderer, GridDrawer* grid_drawer, IMapAccess* map_access, PendingNodeRequests* pending_requests
) :
    tile_renderer(tile_renderer), grid_drawer(grid_drawer), map_access(map_access), pending_requests_(pending_requests)
{
}

MapLayerDrawer::~MapLayerDrawer() { }

void MapLayerDrawer::MergePlans(std::span<const TileDrawPlan> plans, DrawContext& ctx)
{
    for (const auto& plan : plans) {
        if (!plan.valid) {
            continue;
        }
        tile_renderer->MergePlanSideEffects(plan, ctx);
    }
}

void MapLayerDrawer::PlanTilesParallel(
    const DrawContext& ctx, uint32_t current_house_id, bool draw_lights, std::span<const TilePlanInput> inputs, std::span<TileDrawPlan> plans
)
{
    if (inputs.empty()) {
        return;
    }

    const size_t worker_count = std::clamp<size_t>(std::thread::hardware_concurrency(), 1, inputs.size());
    const size_t chunk_size = (inputs.size() + worker_count - 1) / worker_count;
    std::vector<std::jthread> workers;
    workers.reserve(worker_count);

    for (size_t worker_index = 0; worker_index < worker_count; ++worker_index) {
        const size_t begin = worker_index * chunk_size;
        if (begin >= inputs.size()) {
            break;
        }
        const size_t end = std::min(inputs.size(), begin + chunk_size);
        workers.emplace_back([&, begin, end](std::stop_token) {
            for (size_t i = begin; i < end; ++i) {
                plans[i].clear();
                tile_renderer->PlanTile(ctx, inputs[i].location, current_house_id, inputs[i].draw_x, inputs[i].draw_y, draw_lights, plans[i]);
            }
        });
    }
}

void MapLayerDrawer::Draw(const DrawContext& ctx, int map_z, bool live_client, const FloorViewParams& floor_params, DrawCommandQueue& command_queue)
{
    auto& sprite_batch = ctx.sprite_batch;
    const auto& view = ctx.view;
    const auto& settings = ctx.settings;
    const auto& frame = ctx.frame;
    auto& light_buffer = ctx.light_buffer;
    int nd_start_x = floor_params.start_x & ~3;
    int nd_start_y = floor_params.start_y & ~3;
    int nd_end_x = (floor_params.end_x & ~3) + 4;
    int nd_end_y = (floor_params.end_y & ~3) + 4;

    // Optimization: Pre-calculate offset and base coordinates
    // IsTileVisible does this for every tile, but it's constant per layer/frame.
    // We also skip IsTileVisible because visitLeaves already bounds us to the visible area (with 4-tile alignment),
    // which is well within IsTileVisible's 6-tile margin.
    int offset = (map_z <= GROUND_LAYER) ? (GROUND_LAYER - map_z) * TILE_SIZE : TILE_SIZE * (view.floor - map_z);

    int base_screen_x = -view.view_scroll_x - offset;
    int base_screen_y = -view.view_scroll_y - offset;

    bool draw_lights = settings.isDrawLight() && view.zoom <= 10.0;
    std::vector<TilePlanInput> tile_inputs;
    tile_inputs.reserve(2048);

    // Common lambda to draw a node
    auto drawNode = [&](MapNode* nd, int nd_map_x, int nd_map_y, bool live) {
        int node_draw_x = nd_map_x * TILE_SIZE + base_screen_x;
        int node_draw_y = nd_map_y * TILE_SIZE + base_screen_y;

        // Node level culling
        if (!view.IsRectVisible(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
            return;
        }

        if (live && !nd->isVisible(map_z > GROUND_LAYER)) {
            if (!nd->isRequested(map_z > GROUND_LAYER)) {
                // Enqueue node request for deferred dispatch (after frame submission)
                if (pending_requests_) {
                    pending_requests_->enqueue(nd_map_x, nd_map_y, map_z > GROUND_LAYER);
                }
                nd->setRequested(map_z > GROUND_LAYER, true);
            }
            grid_drawer->DrawNodeLoadingPlaceholder(sprite_batch, ctx.atlas, nd_map_x, nd_map_y, view);
            return;
        }

        bool fully_inside = view.IsRectFullyInside(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE);

        Floor* floor = nd->getFloor(map_z);
        if (!floor) {
            return;
        }

        TileLocation* location = floor->locs.data();

        int draw_x_base = node_draw_x;
        for (int map_x = 0; map_x < 4; ++map_x, draw_x_base += TILE_SIZE) {
            int draw_y = node_draw_y;
            for (int map_y = 0; map_y < 4; ++map_y, ++location, draw_y += TILE_SIZE) {
                // Early-out: skip empty tile locations before function call overhead
                if (!location->get()) [[likely]] {
                    continue;
                }

                // Culling: Skip tiles that are far outside the viewport.
                if (!fully_inside && !view.IsPixelVisible(draw_x_base, draw_y, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
                    continue;
                }

                tile_inputs.push_back({location, draw_x_base, draw_y});
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
                drawNode(nd, nd_map_x, nd_map_y, true);
            }
        }
    } else {
        // Use SpatialHashGrid::visitLeaves which handles O(1) viewport query internally
        // Expand the query range slightly to handle the 4-tile alignment and safety margin
        int safe_start_x = nd_start_x - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
        int safe_start_y = nd_start_y - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
        int safe_end_x = nd_end_x + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
        int safe_end_y = nd_end_y + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;

        map_access->getMap().visitLeaves(safe_start_x, safe_start_y, safe_end_x, safe_end_y, [&](MapNode* nd, int nd_map_x, int nd_map_y) {
            drawNode(nd, nd_map_x, nd_map_y, false);
        });
    }

    std::vector<TileDrawPlan> plans(tile_inputs.size());
    for (auto& plan : plans) {
        plan.reserve();
    }

    auto& mutable_ctx = const_cast<DrawContext&>(ctx);
    PlanTilesParallel(ctx, frame.current_house_id, draw_lights, tile_inputs, plans);
    MergePlans(plans, mutable_ctx);
    command_queue.clear();
    command_queue.reserve(tile_inputs.size() * 4);

    for (auto& plan : plans) {
        if (!plan.valid) {
            continue;
        }
        tile_renderer->QueuePlanCommands(plan, command_queue);
    }
}
