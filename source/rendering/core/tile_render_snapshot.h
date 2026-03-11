#ifndef RME_RENDERING_CORE_TILE_RENDER_SNAPSHOT_H_
#define RME_RENDERING_CORE_TILE_RENDER_SNAPSHOT_H_

#include "game/creature.h"
#include "game/outfit.h"
#include "app/definitions.h"
#include "map/position.h"
#include "rendering/core/sprite_light.h"
#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include "rendering/ui/tooltip_data.h"
#include "map/tile.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct PodiumRenderSnapshot {
    Outfit outfit;
    Direction direction = SOUTH;
    bool show_outfit = true;
    bool show_mount = true;
    bool show_platform = true;
};

struct ItemRenderSnapshot {
    Position pos;
    uint16_t server_id = 0;
    uint16_t client_id = 0;
    uint16_t subtype = 0;
    bool selected = false;
    bool locked = false;
    bool has_light = false;
    bool is_ground_tile = false;
    bool is_splash = false;
    bool is_fluid_container = false;
    bool is_hangable = false;
    bool is_stackable = false;
    bool is_pickupable = false;
    bool is_meta_item = false;
    bool is_border = false;
    bool is_door = false;
    bool has_hook_south = false;
    bool has_hook_east = false;
    BorderType border_alignment = BORDER_NONE;
    SpriteLight light;
    std::optional<PodiumRenderSnapshot> podium;
};

struct CreatureRenderSnapshot {
    Outfit outfit;
    Direction direction = SOUTH;
    Position map_pos;
    std::string name;
    bool selected = false;
};

struct MarkerRenderSnapshot {
    bool has_waypoint = false;
    bool is_house_exit = false;
    bool is_current_house_exit = false;
    bool is_town_exit = false;
    bool has_spawn = false;
    bool spawn_selected = false;
};

struct LoadingPlaceholderSnapshot {
    int draw_x = 0;
    int draw_y = 0;
    int width = 0;
    int height = 0;
    DrawColor color;
};

struct TileRenderSnapshot {
    Position pos;
    int draw_x = 0;
    int draw_y = 0;
    uint16_t mapflags = 0;
    uint16_t statflags = 0;
    uint32_t house_id = 0;
    uint8_t minimap_color = 0;
    int spawn_count = 0;
    int stacked_item_count = 0;
    bool modified = false;
    bool top_item_is_border = false;
    bool is_current_house_tile = false;

    std::optional<ItemRenderSnapshot> ground;
    std::vector<ItemRenderSnapshot> items;
    std::optional<CreatureRenderSnapshot> creature;
    std::optional<MarkerRenderSnapshot> marker;
    std::vector<TooltipData> tooltips;
    std::vector<HookIndicatorDrawer::HookRequest> hooks;
    std::vector<DoorIndicatorDrawer::DoorRequest> doors;
    std::optional<CreatureLabel> creature_label;

    [[nodiscard]] bool hasHookSouth() const
    {
        return (statflags & TILESTATE_HOOK_SOUTH) != 0;
    }

    [[nodiscard]] bool hasHookEast() const
    {
        return (statflags & TILESTATE_HOOK_EAST) != 0;
    }

    [[nodiscard]] bool isHouseTile() const
    {
        return house_id != 0;
    }

    [[nodiscard]] bool isPZ() const
    {
        return (mapflags & TILESTATE_PROTECTIONZONE) != 0;
    }

    [[nodiscard]] bool isBlocking() const
    {
        return (statflags & TILESTATE_BLOCKING) != 0;
    }
};

struct VisibleFloorSnapshot {
    int map_z = 0;
    std::vector<TileRenderSnapshot> tiles;
    std::vector<LoadingPlaceholderSnapshot> loading_placeholders;

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
