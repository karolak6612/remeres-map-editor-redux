#include "app/main.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/tiles/tile_draw_plan.h"

#include "brushes/waypoint/waypoint_brush.h"
#include "game/complexitem.h"
#include "game/item.h"
#include "game/waypoints.h"
#include "map/map.h"
#include "map/map_region.h"
#include "map/tile.h"

#include "app/definitions.h"
#include "game/sprites.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/frame_accumulators.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/map_access.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_resolver.h"
#include "rendering/core/tile_render_snapshot_builder.h"
#include "rendering/drawers/tiles/tile_color_calculator.h"

#include "rendering/core/light_buffer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/ui/tooltip_data_extractor.h"
#include "rendering/utilities/pattern_calculator.h"

TileRenderer::TileRenderer(const TileRenderDeps& deps) :
    item_drawer(deps.item_drawer),
    sprite_drawer(deps.sprite_drawer),
    creature_drawer(deps.creature_drawer),
    marker_drawer(deps.marker_drawer),
    map_access(deps.map_access),
    sprite_resolver(deps.sprite_resolver)
{
    // Pre-reserve for typical tile item counts to avoid per-tile allocations
    reusable_plan_.reserve(16);
    preload_queue_.reserve(256);
}

namespace {
    // Accumulate a door indicator request into the frame accumulators.
    void AccumulateDoorIndicator(FrameAccumulators& accumulators, Item* item, const ItemDefinitionView& it, const Position& pos)
    {
        bool locked = item->isLocked();
        auto border = static_cast<BorderType>(it.attribute(ItemAttributeKey::BorderAlignment));

        // Door orientation: horizontal wall -> West border (south=true), vertical wall -> North border (east=true)
        if (border == WALL_HORIZONTAL) {
            accumulators.doors.push_back({pos, locked, true, false});
        } else if (border == WALL_VERTICAL) {
            accumulators.doors.push_back({pos, locked, false, true});
        } else {
            // Center case for non-aligned doors
            accumulators.doors.push_back({pos, locked, false, false});
        }
    }

    // Accumulate a hook indicator request into the frame accumulators.
    void AccumulateHookIndicator(FrameAccumulators& accumulators, const ItemDefinitionView& it, const Position& pos)
    {
        accumulators.hooks.push_back({pos, it.hasFlag(ItemFlag::HookSouth), it.hasFlag(ItemFlag::HookEast)});
    }
} // namespace

void TileRenderer::DrawTile(
    const DrawContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x, int in_draw_y, bool draw_lights
)
{
    reusable_plan_.clear();
    PlanTile(ctx, location, current_house_id, in_draw_x, in_draw_y, draw_lights, reusable_plan_);
    if (reusable_plan_.valid) {
        auto& mutable_ctx = const_cast<DrawContext&>(ctx);
        MergePlanSideEffects(reusable_plan_, mutable_ctx);
        ExecutePlan(ctx, reusable_plan_);
    }
}

void TileRenderer::PlanTile(
    const DrawContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x, int in_draw_y, bool draw_lights,
    TileDrawPlan& plan
)
{
    const FramePlanContext plan_ctx {ctx.view, ctx.settings, ctx.frame};
    PlanTile(plan_ctx, location, current_house_id, in_draw_x, in_draw_y, draw_lights, plan);
}

void TileRenderer::PlanTile(
    const FramePlanContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x, int in_draw_y, bool draw_lights,
    TileDrawPlan& plan
)
{
    if (!location) {
        return;
    }
    int draw_x = in_draw_x;
    int draw_y = in_draw_y;
    if (draw_x == -1 || draw_y == -1) {
        auto vis = ctx.view.IsTileVisible(location->getX(), location->getY(), location->getZ());
        if (!vis) {
            return;
        }
        draw_x = vis->x;
        draw_y = vis->y;
    }

    const Map* map = map_access ? &map_access->getMap() : nullptr;
    auto snapshot = TileRenderSnapshotBuilder::Build(*location, ctx.view, ctx.settings, ctx.frame, map, current_house_id, draw_x, draw_y);
    if (!snapshot) {
        return;
    }

    PlanTile(ctx, *snapshot, draw_lights, plan);
}

void TileRenderer::PlanTile(const DrawContext& ctx, const TileRenderSnapshot& tile, bool draw_lights, TileDrawPlan& plan)
{
    const FramePlanContext plan_ctx {ctx.view, ctx.settings, ctx.frame};
    PlanTile(plan_ctx, tile, draw_lights, plan);
}

void TileRenderer::PlanTile(const FramePlanContext& ctx, const TileRenderSnapshot& tile, bool draw_lights, TileDrawPlan& plan)
{
    const auto& view = ctx.view;
    const auto& settings = ctx.settings;
    const auto& frame = ctx.frame;
    auto& accumulators = plan.accumulators;
    auto* light_buffer = draw_lights ? &plan.lights : nullptr;

    plan.valid = true;
    plan.draw_x = tile.draw_x;
    plan.draw_y = tile.draw_y;

    for (const auto& tooltip : tile.tooltips) {
        accumulators.tooltips.addItemTooltip(tooltip);
    }
    accumulators.hooks.insert(accumulators.hooks.end(), tile.hooks.begin(), tile.hooks.end());
    accumulators.doors.insert(accumulators.doors.end(), tile.doors.begin(), tile.doors.end());
    if (tile.creature_label) {
        accumulators.creature_names.push_back(*tile.creature_label);
    }

    bool as_minimap = settings.show_as_minimap;
    bool only_colors = as_minimap || settings.show_only_colors;

    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    if (!as_minimap) {
        TileColorCalculator::Calculate(tile, settings, r, g, b, frame.highlight_pulse);
    }

    if (only_colors) {
        if (as_minimap) {
            TileColorCalculator::GetMinimapColor(tile, r, g, b);
            plan.color_square = TileDrawPlan::ColorSquare {DrawColor(r, g, b, 255)};
        } else if (r != 255 || g != 255 || b != 255) {
            plan.color_square = TileDrawPlan::ColorSquare {DrawColor(r, g, b, 128)};
        }
    } else {
        if (tile.ground) {
            PlanGroundItem(ctx, tile, *tile.ground, r, g, b, plan);
        }
        if (plan.items.empty() && settings.always_show_zones && (r != 255 || g != 255 || b != 255)) {
            plan.zone_brush = TileDrawPlan::ZoneBrush {SPRITE_ZONE, r, g, b, 60};
        }
    }

    if (settings.show_houses && tile.isHouseTile() && tile.is_current_house_tile && tile.pos.z == view.floor) {
        uint8_t hr = 255;
        uint8_t hg = 255;
        uint8_t hb = 255;
        TileColorCalculator::GetHouseColor(tile.house_id, hr, hg, hb);
        const float intensity = 0.5f + (0.5f * frame.highlight_pulse);
        const int alpha = static_cast<int>(intensity * 255.0f);
        plan.house_border = TileDrawPlan::HouseBorder {DrawColor(hr, hg, hb, static_cast<uint8_t>(alpha))};
    }

    if (!only_colors) {
        if (view.zoom < 10.0 || !settings.hide_items_when_zoomed) {
            PlanStackedItems(ctx, tile, light_buffer, r, g, b, plan);
        }

        if (view.zoom < 10.0 && tile.marker) {
            plan.marker = TileDrawPlan::MarkerCmd {.marker = *tile.marker};
        }
    }

    if (tile.creature && settings.show_creatures) {
        plan.creature = TileDrawPlan::CreatureCmd {
            .creature = *tile.creature,
            .options = CreatureDrawOptions {.map_pos = tile.pos, .transient_selection_bounds = frame.transient_selection_bounds}
        };
    }
}

void TileRenderer::PlanGroundItem(
    const FramePlanContext& ctx, const TileRenderSnapshot& tile, const ItemRenderSnapshot& ground, uint8_t r, uint8_t g, uint8_t b, TileDrawPlan& plan
)
{
    if (!ground.definition || !sprite_resolver) {
        return;
    }

    const int client_id = ground.definition.clientId();
    const SpriteMetadata* meta = sprite_resolver->getSpriteMetadata(client_id);
    if (!meta) {
        return;
    }

    SpritePatterns patterns = PatternCalculator::Calculate(*meta, sprite_resolver->getSpriteAnimation(client_id), ground, tile);

    if (!sprite_resolver->isSpriteSimpleAndLoaded(client_id)) {
        plan.preload_requests.push_back({client_id, patterns.x, patterns.y, patterns.z, patterns.frame});
    }

    plan.items.push_back(TileDrawPlan::ItemCmd {
        .item = ground,
        .patterns = patterns,
        .red = r,
        .green = g,
        .blue = b,
        .alpha = 255,
    });

    if (plan.lights.size() == 0 && ground.has_light && ctx.settings.isDrawLight()) {
        plan.lights.AddLight(tile.pos.x, tile.pos.y, tile.pos.z, ground.light);
    }
}

void TileRenderer::PlanStackedItems(
    const FramePlanContext& ctx, const TileRenderSnapshot& tile, LightBuffer* light_buffer, uint8_t r, uint8_t g, uint8_t b, TileDrawPlan& plan
)
{
    const auto& settings = ctx.settings;
    const auto& frame = ctx.frame;
    const int map_z = tile.pos.z;

    // Hoist house color calculation out of item loop
    uint8_t house_r = 255, house_g = 255, house_b = 255;
    bool calculate_house_color = settings.extended_house_shader && settings.show_houses && tile.isHouseTile();
    bool should_pulse = calculate_house_color && tile.is_current_house_tile && (frame.highlight_pulse > 0.0f);
    float boost = 0.0f;

    if (calculate_house_color) {
        TileColorCalculator::GetHouseColor(tile.house_id, house_r, house_g, house_b);
        if (should_pulse) {
            boost = frame.highlight_pulse * 0.6f;
        }
    }

    // items on tile
    for (const auto& item : tile.items) {
        if (light_buffer && item.has_light) {
            light_buffer->AddLight(tile.pos.x, tile.pos.y, tile.pos.z, item.light);
        }

        const auto& definition = item.definition;
        const int client_id = definition ? definition.clientId() : 0;
        if (!definition || !sprite_resolver || client_id <= 0) {
            continue;
        }

        const SpriteMetadata* meta = sprite_resolver->getSpriteMetadata(client_id);
        if (!meta) {
            continue;
        }

        SpritePatterns patterns = PatternCalculator::Calculate(*meta, sprite_resolver->getSpriteAnimation(client_id), item, tile);
        if (!sprite_resolver->isSpriteSimpleAndLoaded(client_id)) {
            plan.preload_requests.push_back({client_id, patterns.x, patterns.y, patterns.z, patterns.frame});
        }

        uint8_t item_red = 255;
        uint8_t item_green = 255;
        uint8_t item_blue = 255;
        if (definition.hasFlag(ItemFlag::IsBorder)) {
            item_red = r;
            item_green = g;
            item_blue = b;
        } else if (calculate_house_color) {
            item_red = static_cast<uint8_t>(item_red * house_r / 255);
            item_green = static_cast<uint8_t>(item_green * house_g / 255);
            item_blue = static_cast<uint8_t>(item_blue * house_b / 255);

            if (should_pulse) {
                item_red = static_cast<uint8_t>(std::min(255, static_cast<int>(item_red + (255 - item_red) * boost)));
                item_green = static_cast<uint8_t>(std::min(255, static_cast<int>(item_green + (255 - item_green) * boost)));
                item_blue = static_cast<uint8_t>(std::min(255, static_cast<int>(item_blue + (255 - item_blue) * boost)));
            }
        }

        plan.items.push_back(TileDrawPlan::ItemCmd {
            .item = item,
            .patterns = patterns,
            .red = item_red,
            .green = item_green,
            .blue = item_blue,
            .alpha = 255,
        });
    }
}

void TileRenderer::ExecutePlan(const DrawContext& ctx, TileDrawPlan& plan)
{
    auto& sprite_batch = ctx.sprite_batch;

    int draw_x = plan.draw_x;
    int draw_y = plan.draw_y;

    // Only-colors mode: colored square
    if (plan.color_square) {
        sprite_drawer->glBlitSquare(sprite_batch, ctx.atlas, draw_x, draw_y, plan.color_square->color);
    }

    // Zone brush fallback
    if (plan.zone_brush) {
        const auto& zb = *plan.zone_brush;
        item_drawer->DrawRawBrush(sprite_batch, sprite_drawer, draw_x, draw_y, zb.sprite_id, zb.r, zb.g, zb.b, zb.a);
    }

    // House border highlight
    if (plan.house_border) {
        sprite_drawer->glDrawBox(sprite_batch, ctx.atlas, draw_x, draw_y, 32, 32, plan.house_border->color);
    }

    // Item draw commands (ground + stacked items in order)
    for (auto& cmd : plan.items) {
        item_drawer->BlitItemSnapshot(
            sprite_batch, ctx.atlas, sprite_drawer, creature_drawer, draw_x, draw_y, cmd.item, ctx.settings, ctx.frame, cmd.patterns, cmd.red,
            cmd.green, cmd.blue, cmd.alpha
        );
    }

    // Creature draw
    if (plan.creature) {
        creature_drawer->BlitCreature(sprite_batch, sprite_drawer, draw_x, draw_y, plan.creature->creature, plan.creature->options);
    }

    // Marker draw
    if (plan.marker) {
        marker_drawer->draw(sprite_batch, sprite_drawer, draw_x, draw_y, plan.marker->marker, ctx.settings);
    }
}

void TileRenderer::QueuePlanCommands(const TileDrawPlan& plan, DrawCommandQueue& queue) const
{
    const int draw_x = plan.draw_x;
    const int draw_y = plan.draw_y;

    if (plan.color_square) {
        queue.push(DrawColorSquareCmd {.draw_x = draw_x, .draw_y = draw_y, .color = plan.color_square->color});
    }

    if (plan.zone_brush) {
        queue.push(DrawZoneBrushCmd {
            .draw_x = draw_x,
            .draw_y = draw_y,
            .sprite_id = plan.zone_brush->sprite_id,
            .r = plan.zone_brush->r,
            .g = plan.zone_brush->g,
            .b = plan.zone_brush->b,
            .a = plan.zone_brush->a,
        });
    }

    if (plan.house_border) {
        queue.push(DrawHouseBorderCmd {.draw_x = draw_x, .draw_y = draw_y, .color = plan.house_border->color});
    }

    for (const auto& item : plan.items) {
        queue.push(DrawItemCmd {draw_x, draw_y, item.item, item.patterns, item.red, item.green, item.blue, item.alpha});
    }

    if (plan.creature) {
        queue.push(DrawCreatureCmd {
            .draw_x = draw_x,
            .draw_y = draw_y,
            .creature = plan.creature->creature,
            .options = plan.creature->options,
        });
    }

    if (plan.marker) {
        queue.push(DrawMarkerCmd {
            .draw_x = draw_x,
            .draw_y = draw_y,
            .marker = plan.marker->marker,
        });
    }
}

void TileRenderer::MergePlanSideEffects(const TileDrawPlan& plan, DrawContext& ctx)
{
    MergePlanSideEffects(plan, ctx.accumulators, ctx.light_buffer);
}

void TileRenderer::MergePlanSideEffects(const TileDrawPlan& plan, FrameAccumulators& accumulators, LightBuffer& light_buffer)
{
    for (const auto& tooltip : plan.accumulators.tooltips.getTooltips()) {
        accumulators.tooltips.addItemTooltip(tooltip);
    }

    accumulators.hooks.insert(accumulators.hooks.end(), plan.accumulators.hooks.begin(), plan.accumulators.hooks.end());
    accumulators.doors.insert(accumulators.doors.end(), plan.accumulators.doors.begin(), plan.accumulators.doors.end());
    accumulators.creature_names.insert(
        accumulators.creature_names.end(), plan.accumulators.creature_names.begin(), plan.accumulators.creature_names.end()
    );

    for (size_t i = 0; i < plan.lights.size(); ++i) {
        light_buffer.AddLight(plan.lights.map_x[i], plan.lights.map_y[i], GROUND_LAYER, SpriteLight {plan.lights.intensity[i], plan.lights.color[i]});
    }
}

void TileRenderer::QueuePreloadRequests(std::span<const SpritePreloadQueue::Request> requests)
{
    for (const auto& request : requests) {
        preload_queue_.enqueue(request.client_id, request.pattern_x, request.pattern_y, request.pattern_z, request.frame);
    }
}
