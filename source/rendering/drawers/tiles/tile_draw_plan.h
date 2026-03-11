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
#include "map/position.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/sprite_preload_queue.h"
#include "rendering/core/tile_render_snapshot.h"
#include "rendering/core/draw_command_queue.h"
#include "rendering/core/frame_accumulators.h"

// Intermediate representation produced by PlanTile() and consumed by ExecutePlan().
// Separates data-gathering (tile traversal, color calculation, accumulator writes)
// from GPU submission (BlitItem, BlitCreature, marker draw calls).
//
// All accumulator side-effects (tooltips, hooks, doors, lights, creature names)
// happen during PlanTile(). ExecutePlan() only issues draw calls based on
// the commands stored here.

struct TileDrawPlan {
    bool valid = false;
    Position pos;
    int draw_x = 0;
    int draw_y = 0;

    // Only-colors mode: single colored square instead of individual items
    struct ColorSquare {
        DrawColor color;
        bool apply_highlight_pulse = false;
    };
    std::optional<ColorSquare> color_square;

    // Zone brush fallback (always_show_zones with no ground tile)
    struct ZoneBrush {
        ServerItemId sprite_id;
        uint8_t r, g, b, a;
        bool apply_highlight_pulse = false;
    };
    std::optional<ZoneBrush> zone_brush;

    // House border highlight overlay
    struct HouseBorder {
        uint32_t house_id = 0;
    };
    std::optional<HouseBorder> house_border;

    // Item draw commands (ground item + stacked items, in render order)
    std::vector<DrawItemCmd> items;
    std::optional<DrawCreatureCmd> creature;
    std::optional<DrawMarkerCmd> marker;

    // Per-tile side effects collected during planning and merged later.
    FrameAccumulators accumulators;
    LightBuffer lights;
    std::vector<SpritePreloadQueue::Request> preload_requests;

    void clear()
    {
        valid = false;
        pos = Position();
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
        accumulators.reserve(item_count, item_count, 2, item_count);
        lights.reserve(item_count);
    }
};

#endif
