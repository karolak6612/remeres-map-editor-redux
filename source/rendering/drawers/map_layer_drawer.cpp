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

#include "app/main.h"
#include "app/definitions.h"
#include "rendering/drawers/map_layer_drawer.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/drawers/overlays/grid_drawer.h"
#include "editor/editor.h"
#include "live/live_client.h"
#include "map/map.h"
#include "map/map_region.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/floor_view_params.h"
#include "rendering/core/view_state.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/sprite_preloader.h"

MapLayerDrawer::MapLayerDrawer(TileRenderer* tile_renderer, GridDrawer* grid_drawer, Editor* editor) :
	tile_renderer(tile_renderer),
	grid_drawer(grid_drawer),
	editor(editor) {
}

MapLayerDrawer::~MapLayerDrawer() {
}

void MapLayerDrawer::Draw(const DrawContext& ctx, const FloorViewParams& floor_params, bool live_client) {
	int map_z = floor_params.current_z;
	int nd_start_x = floor_params.start_x & ~3;
	int nd_start_y = floor_params.start_y & ~3;
	int nd_end_x = (floor_params.end_x & ~3) + 4;
	int nd_end_y = (floor_params.end_y & ~3) + 4;

	// Optimization: Pre-calculate offset and base coordinates
	// IsTileVisible does this for every tile, but it's constant per layer/frame.
	// We also skip IsTileVisible because visitLeaves already bounds us to the visible area (with 4-tile alignment),
	// which is well within IsTileVisible's 6-tile margin.
	int offset = (map_z <= GROUND_LAYER)
		? (GROUND_LAYER - map_z) * TILE_SIZE
		: TILE_SIZE * (ctx.view.floor - map_z);

	int base_screen_x = -ctx.view.view_scroll_x - offset;
	int base_screen_y = -ctx.view.view_scroll_y - offset;

	bool draw_lights = ctx.options.isDrawLight() && ctx.view.zoom <= 10.0;

	// Common lambda to draw a node
	auto drawNode = [&](MapNode* nd, int nd_map_x, int nd_map_y, bool live) {
		int node_draw_x = nd_map_x * TILE_SIZE + base_screen_x;
		int node_draw_y = nd_map_y * TILE_SIZE + base_screen_y;

		// Node level culling
		if (!ctx.view.IsRectVisible(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
			return;
		}

		if (live && !nd->isVisible(map_z > GROUND_LAYER)) {
			if (!nd->isRequested(map_z > GROUND_LAYER)) {
				// Request the node
				if (editor->live_manager.GetClient()) {
					editor->live_manager.GetClient()->queryNode(nd_map_x, nd_map_y, map_z > GROUND_LAYER);
				}
				nd->setRequested(map_z > GROUND_LAYER, true);
			}
			grid_drawer->DrawNodeLoadingPlaceholder(ctx, nd_map_x, nd_map_y);
			return;
		}

		bool fully_inside = ctx.view.IsRectFullyInside(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE);

		Floor* floor = nd->getFloor(map_z);
		if (!floor) {
			return;
		}

		TileLocation* location = floor->locs.data();
		int draw_x_base = node_draw_x;
		for (int map_x = 0; map_x < 4; ++map_x, draw_x_base += TILE_SIZE) {
			int draw_y = node_draw_y;
			for (int map_y = 0; map_y < 4; ++map_y, ++location, draw_y += TILE_SIZE) {
				// Culling: Skip tiles that are far outside the viewport.
				if (!fully_inside && !ctx.view.IsPixelVisible(draw_x_base, draw_y, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
					continue;
				}

				tile_renderer->DrawTile(ctx, location, ctx.options.current_house_id, draw_x_base, draw_y, draw_lights);
			}
		}
	};

	if (live_client) {
		for (int nd_map_x = nd_start_x; nd_map_x <= nd_end_x; nd_map_x += 4) {
			for (int nd_map_y = nd_start_y; nd_map_y <= nd_end_y; nd_map_y += 4) {
				MapNode* nd = editor->map.getLeaf(nd_map_x, nd_map_y);
				if (!nd) {
					nd = editor->map.createLeaf(nd_map_x, nd_map_y);
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

		editor->map.visitLeaves(safe_start_x, safe_start_y, safe_end_x, safe_end_y, [&](MapNode* nd, int nd_map_x, int nd_map_y) {
			drawNode(nd, nd_map_x, nd_map_y, false);
		});
	}
}
