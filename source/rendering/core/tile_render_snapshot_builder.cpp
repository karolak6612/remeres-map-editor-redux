#include "app/main.h"

#include "rendering/core/tile_render_snapshot_builder.h"

#include "game/complexitem.h"
#include "game/creature.h"
#include "game/item.h"
#include "game/spawn.h"
#include "game/waypoints.h"
#include "map/map.h"
#include "map/tile.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
#include "rendering/ui/tooltip_data_extractor.h"

namespace {

void appendDoorIndicator(std::vector<DoorIndicatorDrawer::DoorRequest>& doors, const Item& item, const ItemDefinitionView& definition, const Position& pos)
{
    if (!definition.isDoor()) {
        return;
    }

    const bool locked = item.isLocked();
    const auto border = static_cast<BorderType>(definition.attribute(ItemAttributeKey::BorderAlignment));
    if (border == WALL_HORIZONTAL) {
        doors.push_back({pos, locked, true, false});
    } else if (border == WALL_VERTICAL) {
        doors.push_back({pos, locked, false, true});
    } else {
        doors.push_back({pos, locked, false, false});
    }
}

void appendHookIndicator(std::vector<HookIndicatorDrawer::HookRequest>& hooks, const ItemDefinitionView& definition, const Position& pos)
{
    hooks.push_back({pos, definition.hasFlag(ItemFlag::HookSouth), definition.hasFlag(ItemFlag::HookEast)});
}

ItemRenderSnapshot snapshotItem(const Item& item, const ItemDefinitionView& definition, const Position& pos)
{
    ItemRenderSnapshot snapshot;
    snapshot.pos = pos;
    snapshot.server_id = item.getID();
    snapshot.client_id = definition ? definition.clientId() : 0;
    snapshot.subtype = item.getSubtype();
    snapshot.selected = item.isSelected();
    snapshot.locked = item.isLocked();
    snapshot.has_light = item.hasLight();
    snapshot.is_ground_tile = definition.isGroundTile();
    snapshot.is_splash = definition.isSplash();
    snapshot.is_fluid_container = definition.isFluidContainer();
    snapshot.is_hangable = definition.hasFlag(ItemFlag::IsHangable);
    snapshot.is_stackable = definition.hasFlag(ItemFlag::Stackable);
    snapshot.is_pickupable = definition.hasFlag(ItemFlag::Pickupable);
    snapshot.is_meta_item = definition.isMetaItem();
    snapshot.is_border = definition.hasFlag(ItemFlag::IsBorder);
    snapshot.is_door = definition.isDoor();
    snapshot.has_hook_south = definition.hasFlag(ItemFlag::HookSouth);
    snapshot.has_hook_east = definition.hasFlag(ItemFlag::HookEast);
    snapshot.border_alignment = static_cast<BorderType>(definition.attribute(ItemAttributeKey::BorderAlignment));
    if (snapshot.has_light) {
        snapshot.light = item.getLight();
    }

    if (const auto* podium = item.asPodium()) {
        snapshot.podium = PodiumRenderSnapshot {
            .outfit = podium->getOutfit(),
            .direction = static_cast<Direction>(podium->getDirection()),
            .show_outfit = podium->getShowOutfit(),
            .show_mount = podium->getShowMount(),
            .show_platform = podium->getShowPlatform(),
        };
    }

    return snapshot;
}

} // namespace

std::optional<TileRenderSnapshot> TileRenderSnapshotBuilder::Build(
    TileLocation& location, const ViewState& view, const RenderSettings& settings, const FrameOptions& frame, const Map* map, uint32_t current_house_id,
    int draw_x, int draw_y
)
{
    Tile* tile = location.get();
    if (!tile) {
        return std::nullopt;
    }

    if (settings.show_only_modified && !tile->isModified()) {
        return std::nullopt;
    }

    TileRenderSnapshot snapshot;
    snapshot.pos = location.getPosition();
    snapshot.draw_x = draw_x;
    snapshot.draw_y = draw_y;
    snapshot.mapflags = tile->getMapFlags();
    snapshot.statflags = tile->getStatFlags();
    snapshot.house_id = tile->getHouseID();
    snapshot.minimap_color = tile->getMiniMapColor();
    snapshot.spawn_count = location.getSpawnCount();
    snapshot.stacked_item_count = static_cast<int>(tile->items.size());
    snapshot.modified = tile->isModified();
    snapshot.top_item_is_border = !tile->items.empty() && tile->items.back()->isBorder();
    snapshot.is_current_house_tile = snapshot.house_id != 0 && static_cast<int>(snapshot.house_id) == static_cast<int>(current_house_id);

    Waypoint* waypoint = nullptr;
    if (map && location.getWaypointCount() > 0) {
        waypoint = const_cast<Map*>(map)->waypoints.getWaypoint(&location);
    }

    if (!settings.ingame) {
        MarkerRenderSnapshot marker;
        marker.has_waypoint = waypoint != nullptr;
        marker.is_house_exit = tile->isHouseExit();
        marker.is_current_house_exit = marker.is_house_exit && tile->hasHouseExit(current_house_id);
        marker.is_town_exit = map ? tile->isTownExit(const_cast<Map&>(*map)) : false;
        marker.has_spawn = tile->spawn != nullptr;
        marker.spawn_selected = tile->spawn && tile->spawn->isSelected();
        if (marker.has_waypoint || marker.is_house_exit || marker.is_town_exit || marker.has_spawn) {
            snapshot.marker = marker;
        }
    }

    if (settings.show_tooltips && snapshot.pos.z == view.floor) {
        if (waypoint && !waypoint->name.empty()) {
            snapshot.tooltips.reserve(4);
            snapshot.tooltips.emplace_back(snapshot.pos, std::string(waypoint->name));
        }
    }

    if (tile->ground) {
        const auto ground_definition = tile->ground->getDefinition();
        snapshot.ground = snapshotItem(*tile->ground, ground_definition, snapshot.pos);

        if (!settings.ingame && settings.highlight_locked_doors) {
            appendDoorIndicator(snapshot.doors, *tile->ground, ground_definition, snapshot.pos);
        }

        if (settings.show_tooltips && snapshot.pos.z == view.floor) {
            TooltipData tooltip;
            if (TooltipDataExtractor::Fill(tooltip, tile->ground.get(), ground_definition, snapshot.pos, tile->isHouseTile(), view.zoom)) {
                snapshot.tooltips.push_back(std::move(tooltip));
            }
        }
    }

    snapshot.items.reserve(tile->items.size());
    snapshot.hooks.reserve(tile->items.size());
    snapshot.doors.reserve(tile->items.size());
    for (const auto& item : tile->items) {
        if (!item) {
            continue;
        }

        const auto definition = item->getDefinition();
        snapshot.items.push_back(snapshotItem(*item, definition, snapshot.pos));

        if (!settings.ingame && settings.highlight_locked_doors) {
            appendDoorIndicator(snapshot.doors, *item, definition, snapshot.pos);
        }
        if (!settings.ingame && settings.show_hooks && (definition.hasFlag(ItemFlag::HookSouth) || definition.hasFlag(ItemFlag::HookEast))) {
            appendHookIndicator(snapshot.hooks, definition, snapshot.pos);
        }
        if (settings.show_tooltips && snapshot.pos.z == view.floor) {
            TooltipData tooltip;
            if (TooltipDataExtractor::Fill(tooltip, item.get(), definition, snapshot.pos, tile->isHouseTile(), view.zoom)) {
                snapshot.tooltips.push_back(std::move(tooltip));
            }
        }
    }

    if (tile->creature) {
        snapshot.creature = CreatureRenderSnapshot {
            .outfit = tile->creature->getLookType(),
            .direction = tile->creature->getDirection(),
            .map_pos = snapshot.pos,
            .name = tile->creature->getName(),
            .selected = tile->creature->isSelected(),
        };

        if (settings.show_creatures && !snapshot.creature->name.empty()) {
            snapshot.creature_label = CreatureLabel {
                .pos = snapshot.pos,
                .name = snapshot.creature->name,
            };
        }
    }

    static_cast<void>(frame);
    return snapshot;
}
