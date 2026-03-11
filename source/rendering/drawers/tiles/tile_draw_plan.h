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

#ifndef RME_RENDERING_TILE_DRAW_PLAN_H_
#define RME_RENDERING_TILE_DRAW_PLAN_H_

#include <optional>
#include <vector>

#include "app/definitions.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/sprite_preload_queue.h"
#include "rendering/core/frame_accumulators.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/utilities/pattern_calculator.h"

class Tile;
class Waypoint;
class Creature;

// Intermediate representation produced by PlanTile() and consumed by ExecutePlan().
// Separates data-gathering (tile traversal, color calculation, accumulator writes)
// from GPU submission (BlitItem, BlitCreature, marker draw calls).
//
// All accumulator side-effects (tooltips, hooks, doors, lights, creature names)
// happen during PlanTile(). ExecutePlan() only issues draw calls based on
// the commands stored here.

struct TileDrawPlan {
    bool valid = false;
    int draw_x = 0;
    int draw_y = 0;

    // Only-colors mode: single colored square instead of individual items
    struct ColorSquare {
        DrawColor color;
    };
    std::optional<ColorSquare> color_square;

    // Zone brush fallback (always_show_zones with no ground tile)
    struct ZoneBrush {
        ServerItemId sprite_id;
        uint8_t r, g, b, a;
    };
    std::optional<ZoneBrush> zone_brush;

    // House border highlight overlay
    struct HouseBorder {
        DrawColor color;
    };
    std::optional<HouseBorder> house_border;

    // Item draw commands (ground item + stacked items, in render order)
    struct ItemCmd {
        BlitItemParams params;
        SpritePatterns patterns;
    };
    std::vector<ItemCmd> items;

    // Creature draw command
    struct CreatureCmd {
        Creature* creature;
        CreatureDrawOptions options;
    };
    std::optional<CreatureCmd> creature;

    // Marker draw command (waypoints, house exits, spawns, etc.)
    struct MarkerCmd {
        Tile* tile;
        Waypoint* waypoint;
        uint32_t current_house_id;
    };
    std::optional<MarkerCmd> marker;

    // Per-tile side effects collected during planning and merged later.
    FrameAccumulators accumulators;
    LightBuffer lights;
    std::vector<SpritePreloadQueue::Request> preload_requests;

    void clear()
    {
        valid = false;
        color_square.reset();
        zone_brush.reset();
        house_border.reset();
        items.clear();
        creature.reset();
        marker.reset();
        accumulators.clear();
        lights.Clear();
        preload_requests.clear();
    }

    void reserve(size_t item_count = 16)
    {
        items.reserve(item_count);
        preload_requests.reserve(item_count);
    }
};

#endif
