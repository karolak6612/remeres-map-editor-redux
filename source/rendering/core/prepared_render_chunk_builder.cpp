#include "app/main.h"

#include "rendering/core/prepared_render_chunk_builder.h"

#include <algorithm>

#include "rendering/core/draw_context.h"
#include "rendering/drawers/tiles/tile_draw_plan.h"
#include "rendering/drawers/tiles/tile_renderer.h"

namespace {

void MergeChunkSideEffects(const ChunkSourceSnapshot& chunk_snapshot, PreparedRenderChunk& prepared_chunk)
{
    prepared_chunk.accumulators.reserve(
        chunk_snapshot.hooks.size(), chunk_snapshot.doors.size(), chunk_snapshot.creature_names.size(), chunk_snapshot.tooltips.size()
    );
    for (const auto& tooltip : chunk_snapshot.tooltips) {
        prepared_chunk.accumulators.tooltips.addItemTooltip(tooltip);
    }
    prepared_chunk.accumulators.hooks.insert(
        prepared_chunk.accumulators.hooks.end(), chunk_snapshot.hooks.begin(), chunk_snapshot.hooks.end()
    );
    prepared_chunk.accumulators.doors.insert(
        prepared_chunk.accumulators.doors.end(), chunk_snapshot.doors.begin(), chunk_snapshot.doors.end()
    );
    prepared_chunk.accumulators.creature_names.insert(
        prepared_chunk.accumulators.creature_names.end(), chunk_snapshot.creature_names.begin(), chunk_snapshot.creature_names.end()
    );
}

void MergePlanOutputs(const TileDrawPlan& plan, PreparedRenderChunk& prepared_chunk)
{
    prepared_chunk.lights.map_x.insert(prepared_chunk.lights.map_x.end(), plan.lights.map_x.begin(), plan.lights.map_x.end());
    prepared_chunk.lights.map_y.insert(prepared_chunk.lights.map_y.end(), plan.lights.map_y.begin(), plan.lights.map_y.end());
    prepared_chunk.lights.color.insert(prepared_chunk.lights.color.end(), plan.lights.color.begin(), plan.lights.color.end());
    prepared_chunk.lights.intensity.insert(
        prepared_chunk.lights.intensity.end(), plan.lights.intensity.begin(), plan.lights.intensity.end()
    );
    prepared_chunk.preload_requests.insert(
        prepared_chunk.preload_requests.end(), plan.preload_requests.begin(), plan.preload_requests.end()
    );
}

} // namespace

std::shared_ptr<const PreparedRenderChunk> PreparedRenderChunkBuilder::Build(
    TileRenderer& tile_renderer, const FramePlanContext& ctx, const ChunkSourceSnapshot& chunk_snapshot, uint64_t generation
)
{
    auto prepared_chunk = std::make_shared<PreparedRenderChunk>();
    prepared_chunk->key = chunk_snapshot.key;
    prepared_chunk->variant = chunk_snapshot.variant;
    prepared_chunk->generation = generation;
    prepared_chunk->clearTransientData();
    prepared_chunk->reserve(
        chunk_snapshot.estimatedPreloadCount(),
        chunk_snapshot.hooks.size(),
        chunk_snapshot.doors.size(),
        chunk_snapshot.creature_names.size(),
        chunk_snapshot.estimatedCommandCount()
    );
    MergeChunkSideEffects(chunk_snapshot, *prepared_chunk);

    for (const auto& placeholder : chunk_snapshot.loading_placeholders) {
        prepared_chunk->commands.emplaceFilledRect(placeholder.pos, placeholder.width, placeholder.height, placeholder.color);
    }

    static thread_local std::vector<TileDrawPlan> reusable_plans;
    if (reusable_plans.size() < chunk_snapshot.tiles.size()) {
        reusable_plans.resize(chunk_snapshot.tiles.size());
    }

    const size_t estimated_item_capacity = chunk_snapshot.tiles.empty() ?
        8 :
        std::max<size_t>(8, (chunk_snapshot.estimatedPreloadCount() / chunk_snapshot.tiles.size()) + 2);
    for (size_t i = 0; i < chunk_snapshot.tiles.size(); ++i) {
        auto& plan = reusable_plans[i];
        plan.reserve(estimated_item_capacity);
        plan.clear();
        tile_renderer.PlanTile(ctx, chunk_snapshot.tiles[i], ctx.settings.isDrawLight() && ctx.view.zoom <= 10.0f, plan);
        if (!plan.valid) {
            continue;
        }
        MergePlanOutputs(plan, *prepared_chunk);
        tile_renderer.QueuePlanCommands(plan, prepared_chunk->commands);
    }

    return prepared_chunk;
}
