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
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"

MapLayerDrawer::MapLayerDrawer(TileRenderer* tile_renderer, GridDrawer* grid_drawer, Editor* editor) :
	tile_renderer(tile_renderer),
	grid_drawer(grid_drawer),
	editor(editor) {
}

MapLayerDrawer::~MapLayerDrawer() {
}

void MapLayerDrawer::Draw(SpriteBatch& sprite_batch, PrimitiveRenderer& primitive_renderer, int map_z, bool live_client, const RenderView& view, const DrawingOptions& options, LightBuffer& light_buffer) {
	// Optimization: Calculate exact visible bounds for this layer
	// This accounts for the parallax/depth offset relative to the floor layer
	const int TileSize = 32;
	int offset = (map_z <= GROUND_LAYER)
		? (GROUND_LAYER - map_z) * TileSize
		: TileSize * (view.floor - map_z);

	// Calculate visible map coordinates based on viewport and offset
	// view_scroll_x + offset <= map_x * TileSize <= view_scroll_x + screensize + offset
	// Add 2 tiles margin to be safe (matching RenderView::Setup logic)
	int margin_tiles = 2;
	int start_map_x = (view.view_scroll_x + offset) / TileSize - margin_tiles;
	int start_map_y = (view.view_scroll_y + offset) / TileSize - margin_tiles;

	int end_map_x = (view.view_scroll_x + (int)(view.screensize_x / view.zoom) + offset) / TileSize + margin_tiles;
	int end_map_y = (view.view_scroll_y + (int)(view.screensize_y / view.zoom) + offset) / TileSize + margin_tiles;

	// Align to node boundaries (4 tiles per node)
	int nd_start_x = start_map_x & ~3;
	int nd_start_y = start_map_y & ~3;
	int nd_end_x = (end_map_x & ~3) + 4;
	int nd_end_y = (end_map_y & ~3) + 4;

	// Precompute draw base position to avoid recomputing in loop
	int draw_x_base = -view.view_scroll_x - offset;
	int draw_y_base = -view.view_scroll_y - offset;

	// ND visibility

	if (live_client) {
		for (int nd_map_x = nd_start_x; nd_map_x <= nd_end_x; nd_map_x += 4) {
			for (int nd_map_y = nd_start_y; nd_map_y <= nd_end_y; nd_map_y += 4) {
				MapNode* nd = editor->map.getLeaf(nd_map_x, nd_map_y);
				if (!nd) {
					nd = editor->map.createLeaf(nd_map_x, nd_map_y);
					nd->setVisible(false, false);
				}

				if (nd->isVisible(map_z > GROUND_LAYER)) {
					for (int map_x = 0; map_x < 4; ++map_x) {
						for (int map_y = 0; map_y < 4; ++map_y) {
							// Optimization: Skip IsTileVisible call, rely on calculated bounds
							// Manually calculate draw position
							int tile_map_x = nd_map_x + map_x;
							int tile_map_y = nd_map_y + map_y;

							int draw_x = tile_map_x * TileSize + draw_x_base;
							int draw_y = tile_map_y * TileSize + draw_y_base;

							TileLocation* location = nd->getTile(map_x, map_y, map_z);

							tile_renderer->DrawTile(sprite_batch, primitive_renderer, location, view, options, options.current_house_id, draw_x, draw_y);
							// draw light, but only if not zoomed too far
							if (location && options.isDrawLight() && view.zoom <= 10.0) {
								tile_renderer->AddLight(location, view, options, light_buffer);
							}
						}
					}
				} else {
					if (!nd->isRequested(map_z > GROUND_LAYER)) {
						// Request the node
						if (editor->live_manager.GetClient()) {
							editor->live_manager.GetClient()->queryNode(nd_map_x, nd_map_y, map_z > GROUND_LAYER);
						}
						nd->setRequested(map_z > GROUND_LAYER, true);
					}
					grid_drawer->DrawNodeLoadingPlaceholder(sprite_batch, nd_map_x, nd_map_y, view);
				}
			}
		}
	} else {
		editor->map.visitLeaves(nd_start_x, nd_start_y, nd_end_x, nd_end_y, [&](MapNode* nd, int nd_map_x, int nd_map_y) {
			for (int map_x = 0; map_x < 4; ++map_x) {
				for (int map_y = 0; map_y < 4; ++map_y) {
					// Optimization: Skip IsTileVisible call, rely on calculated bounds
					// Manually calculate draw position
					int tile_map_x = nd_map_x + map_x;
					int tile_map_y = nd_map_y + map_y;

					int draw_x = tile_map_x * TileSize + draw_x_base;
					int draw_y = tile_map_y * TileSize + draw_y_base;

					TileLocation* location = nd->getTile(map_x, map_y, map_z);

					tile_renderer->DrawTile(sprite_batch, primitive_renderer, location, view, options, options.current_house_id, draw_x, draw_y);
					// draw light, but only if not zoomed too far
					if (location && options.isDrawLight() && view.zoom <= 10.0) {
						tile_renderer->AddLight(location, view, options, light_buffer);
					}
				}
			}
		});
	}
}
