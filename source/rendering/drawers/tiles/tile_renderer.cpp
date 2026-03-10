#include "rendering/drawers/tiles/tile_renderer.h"
#include "app/main.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/tiles/tile_draw_plan.h"

#include "brushes/waypoint/waypoint_brush.h"
#include "editor/editor.h"
#include "game/complexitem.h"
#include "game/item.h"
#include "map/tile.h"

#include "app/definitions.h"
#include "game/sprites.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/frame_accumulators.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
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
    editor(deps.editor)
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
        ExecutePlan(ctx, reusable_plan_);
    }
}

void TileRenderer::PlanTile(
    const DrawContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x, int in_draw_y, bool draw_lights,
    TileDrawPlan& plan
)
{
    const auto& view = ctx.view;
    const auto& settings = ctx.settings;
    const auto& frame = ctx.frame;
    auto& accumulators = ctx.accumulators;
    auto* light_buffer = draw_lights ? &ctx.light_buffer : nullptr;

    plan.valid = false;

    if (!location) {
        return;
    }
    Tile* tile = location->get();

    if (!tile) {
        return;
    }

    if (settings.show_only_modified && !tile->isModified()) {
        return;
    }

    int map_x = location->getX();
    int map_y = location->getY();
    int map_z = location->getZ();

    int draw_x, draw_y;
    if (in_draw_x != -1 && in_draw_y != -1) {
        draw_x = in_draw_x;
        draw_y = in_draw_y;
    } else {
        // Early viewport culling - skip tiles that are completely off-screen
        auto vis = view.IsTileVisible(map_x, map_y, map_z);
        if (!vis) {
            return;
        }
        draw_x = vis->x;
        draw_y = vis->y;
    }

    plan.valid = true;
    plan.draw_x = draw_x;
    plan.draw_y = draw_y;

    const auto& position = location->getPosition();

    // Light Processing (Ground)
    if (light_buffer && tile->hasLight()) {
        if (tile->ground && tile->ground->hasLight()) {
            light_buffer->AddLight(position.x, position.y, position.z, tile->ground->getLight());
        }
    }

    Waypoint* waypoint = nullptr;
    if (location->getWaypointCount() > 0) {
        waypoint = editor->map.waypoints.getWaypoint(location);
    }

    // Waypoint tooltip (one per waypoint)
    if (settings.show_tooltips && waypoint && map_z == view.floor) {
        accumulators.tooltips.addWaypointTooltip(position, waypoint->name);
    }

    bool as_minimap = settings.show_as_minimap;
    bool only_colors = as_minimap || settings.show_only_colors;

    uint8_t r = 255, g = 255, b = 255;

    // begin filters for ground tile
    if (!as_minimap) {
        TileColorCalculator::Calculate(tile, settings, current_house_id, location->getSpawnCount(), r, g, b, frame.highlight_pulse);
    }

    ItemDefinitionView ground_it;
    if (tile->ground) {
        ground_it = tile->ground->getDefinition();
    }

    if (only_colors) {
        if (as_minimap) {
            TileColorCalculator::GetMinimapColor(tile, r, g, b);
            plan.color_square = TileDrawPlan::ColorSquare {DrawColor(r, g, b, 255)};
        } else if (r != 255 || g != 255 || b != 255) {
            plan.color_square = TileDrawPlan::ColorSquare {DrawColor(r, g, b, 128)};
        }
    } else {
        if (tile->ground && ground_it) {
            if (GameSprite* ground_sprite = tile->ground->getSprite()) {
                SpritePatterns patterns = PatternCalculator::Calculate(ground_sprite, ground_it, tile->ground.get(), tile, position);

                // Inline preload check - skip function call when sprite is simple and loaded (95%+ case)
                if (!ground_sprite->isSimpleAndLoaded()) {
                    preload_queue_.enqueue(ground_sprite, patterns.x, patterns.y, patterns.z, patterns.frame);
                }

                BlitItemParams params(position, tile->ground.get(), settings, frame);
                params.tile = tile;
                params.item_definition = ground_it;
                params.red = r;
                params.green = g;
                params.blue = b;
                // patterns pointer will be set in ExecutePlan
                plan.items.push_back({std::move(params), patterns});

                // Accumulate door indicator for ground items (doors can be ground items)
                if (!settings.ingame && settings.highlight_locked_doors && ground_it.isDoor()) {
                    AccumulateDoorIndicator(accumulators, tile->ground.get(), ground_it, position);
                }
            }
        } else if (settings.always_show_zones && (r != 255 || g != 255 || b != 255)) {
            plan.zone_brush = TileDrawPlan::ZoneBrush {SPRITE_ZONE, r, g, b, 60};
        }
    }

    // Cache isHouseTile - used multiple times below
    const bool is_house_tile = tile->isHouseTile();

    // Ground tooltip (one per item)
    if (settings.show_tooltips && map_z == view.floor && tile->ground && ground_it) {
        TooltipData& groundData = accumulators.tooltips.requestTooltipData();
        if (TooltipDataExtractor::Fill(groundData, tile->ground.get(), ground_it, position, is_house_tile, view.zoom)) {
            if (groundData.hasVisibleFields()) {
                accumulators.tooltips.commitTooltip();
            }
        }
    }

    // end filters for ground tile

    // House border highlight for selected house tiles on current floor
    if (settings.show_houses && is_house_tile && static_cast<int>(tile->getHouseID()) == current_house_id && map_z == view.floor) {
        uint8_t hr, hg, hb;
        TileColorCalculator::GetHouseColor(tile->getHouseID(), hr, hg, hb);

        float intensity = 0.5f + (0.5f * frame.highlight_pulse);
        int ba = static_cast<int>(intensity * 255.0f);
        plan.house_border = TileDrawPlan::HouseBorder {DrawColor(hr, hg, hb, static_cast<uint8_t>(ba))};
    }

    if (!only_colors) {
        if (view.zoom < 10.0 || !settings.hide_items_when_zoomed) {
            // Hoist house color calculation out of item loop
            uint8_t house_r = 255, house_g = 255, house_b = 255;
            bool calculate_house_color = settings.extended_house_shader && settings.show_houses && is_house_tile;
            bool should_pulse
                = calculate_house_color && (static_cast<int>(tile->getHouseID()) == current_house_id) && (frame.highlight_pulse > 0.0f);
            float boost = 0.0f;

            if (calculate_house_color) {
                TileColorCalculator::GetHouseColor(tile->getHouseID(), house_r, house_g, house_b);
                if (should_pulse) {
                    boost = frame.highlight_pulse * 0.6f;
                }
            }

            bool process_tooltips = settings.show_tooltips && map_z == view.floor;

            // items on tile
            for (const auto& item : tile->items) {
                if (light_buffer && item->hasLight()) {
                    light_buffer->AddLight(position.x, position.y, position.z, item->getLight());
                }

                const ItemDefinitionView it = item->getDefinition();

                // item tooltip (one per item)
                if (process_tooltips) {
                    TooltipData& itemData = accumulators.tooltips.requestTooltipData();
                    if (TooltipDataExtractor::Fill(itemData, item.get(), it, position, is_house_tile, view.zoom)) {
                        if (itemData.hasVisibleFields()) {
                            accumulators.tooltips.commitTooltip();
                        }
                    }
                }

                if (GameSprite* sprite = item->getSprite()) {
                    SpritePatterns patterns = PatternCalculator::Calculate(sprite, it, item.get(), tile, position);

                    // Inline preload check - skip function call when sprite is simple and loaded
                    if (!sprite->isSimpleAndLoaded()) {
                        preload_queue_.enqueue(sprite, patterns.x, patterns.y, patterns.z, patterns.frame);
                    }

                    BlitItemParams params(position, item.get(), settings, frame);
                    params.tile = tile;
                    params.item_definition = it;

                    // item sprite
                    if (item->isBorder()) {
                        params.red = r;
                        params.green = g;
                        params.blue = b;
                    } else {
                        uint8_t ir = 255, ig = 255, ib = 255;

                        if (calculate_house_color) {
                            // Apply house color tint
                            ir = static_cast<uint8_t>(ir * house_r / 255);
                            ig = static_cast<uint8_t>(ig * house_g / 255);
                            ib = static_cast<uint8_t>(ib * house_b / 255);

                            if (should_pulse) {
                                // Pulse effect matching the tile pulse
                                ir = static_cast<uint8_t>(std::min(255, static_cast<int>(ir + (255 - ir) * boost)));
                                ig = static_cast<uint8_t>(std::min(255, static_cast<int>(ig + (255 - ig) * boost)));
                                ib = static_cast<uint8_t>(std::min(255, static_cast<int>(ib + (255 - ib) * boost)));
                            }
                        }
                        params.red = ir;
                        params.green = ig;
                        params.blue = ib;
                    }
                    // patterns pointer will be set in ExecutePlan
                    plan.items.push_back({std::move(params), patterns});
                }

                // Accumulate door indicator for this item
                if (!settings.ingame && settings.highlight_locked_doors && it.isDoor()) {
                    AccumulateDoorIndicator(accumulators, item.get(), it, position);
                }

                // Accumulate hook indicator for this item
                if (!settings.ingame && settings.show_hooks && (it.hasFlag(ItemFlag::HookSouth) || it.hasFlag(ItemFlag::HookEast))) {
                    AccumulateHookIndicator(accumulators, it, position);
                }
            }

            // monster/npc on tile
            if (tile->creature && settings.show_creatures) {
                plan.creature = TileDrawPlan::CreatureCmd {
                    tile->creature.get(),
                    CreatureDrawOptions {.map_pos = position, .transient_selection_bounds = frame.transient_selection_bounds}
                };
                if (!tile->creature->getName().empty()) {
                    accumulators.creature_names.push_back({position, tile->creature->getName(), tile->creature.get()});
                }
            }
        }

        if (view.zoom < 10.0) {
            // markers (waypoint, house exit, town temple, spawn)
            plan.marker = TileDrawPlan::MarkerCmd {tile, waypoint, current_house_id};
        }
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
        // Fix up patterns pointer to point to the command's owned copy
        cmd.params.patterns = &cmd.patterns;
        item_drawer->BlitItem(sprite_batch, ctx.atlas, sprite_drawer, creature_drawer, draw_x, draw_y, cmd.params);
    }

    // Creature draw
    if (plan.creature) {
        creature_drawer->BlitCreature(sprite_batch, sprite_drawer, draw_x, draw_y, plan.creature->creature, plan.creature->options);
    }

    // Marker draw
    if (plan.marker) {
        marker_drawer->draw(
            sprite_batch, sprite_drawer, draw_x, draw_y, plan.marker->tile, plan.marker->waypoint, plan.marker->current_house_id, *editor,
            ctx.settings
        );
    }
}
