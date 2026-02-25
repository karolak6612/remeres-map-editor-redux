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
#include "map/tile.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/sprite_preloader.h"

MapLayerDrawer::MapLayerDrawer(TileRenderer* tile_renderer, GridDrawer* grid_drawer, Editor* editor) :
	tile_renderer(tile_renderer),
	grid_drawer(grid_drawer),
	editor(editor) {
}

MapLayerDrawer::~MapLayerDrawer() {
}

void MapLayerDrawer::Extract(int map_z, bool live_client, const RenderView& view, const DrawingOptions& options) {
	int nd_start_x = view.start_x & ~3;
	int nd_start_y = view.start_y & ~3;
	int nd_end_x = (view.end_x & ~3) + 4;
	int nd_end_y = (view.end_y & ~3) + 4;

	auto extractNode = [&](MapNode* nd, int nd_map_x, int nd_map_y, bool live) {
		Floor* floor = nd->getFloor(map_z);
		if (!floor) {
			return;
		}

		// Only re-evaluate if the map data in this chunk changed.
		RenderList* list = render_cache.Get(floor);
		if (!floor->is_render_dirty && list) {
			return; // Cache is still warm and valid.
		}

		if (!list) {
			list = render_cache.GetOrCreate(floor);
		}
		list->clear();

		TileLocation* location = floor->locs.data();
		int map_base_x = nd_map_x * TILE_SIZE;
		int draw_x_base = map_base_x;

		for (int map_x = 0; map_x < 4; ++map_x, draw_x_base += TILE_SIZE) {
			int draw_y = nd_map_y * TILE_SIZE;
			for (int map_y = 0; map_y < 4; ++map_y, ++location, draw_y += TILE_SIZE) {

				MarkerFlags marker_flags;
				// Markers are usually dynamic or zoom-dependent, but for now we bake them into the Extract phase using the current view.
				// In a truly decoupled system, markers might be drawn in a separate dynamic pass.
				if (view.zoom < 10.0) {
					if (location->getWaypointCount() > 0) {
						marker_flags.has_waypoint = true;
					}
					Tile* tile = location->get();
					if (tile) {
						marker_flags.is_house_exit = tile->isHouseExit();
						marker_flags.has_house_exit_match = tile->hasHouseExit(options.current_house_id);
						marker_flags.is_town_exit = tile->isTownExit(editor->map);
						marker_flags.has_spawn = tile->spawn != nullptr;
						if (tile->spawn) {
							marker_flags.is_spawn_selected = tile->spawn->isSelected();
						}
					}
				}

				// The TileRenderer now appends to the RenderList instead of immediately drawing!
				tile_renderer->DrawTile(*list, location, view, options, options.current_house_id, marker_flags, draw_x_base, draw_y);
			}
		}

		floor->is_render_dirty = false;
	};

	if (live_client) {
		for (int nd_map_x = nd_start_x; nd_map_x <= nd_end_x; nd_map_x += 4) {
			for (int nd_map_y = nd_start_y; nd_map_y <= nd_end_y; nd_map_y += 4) {
				MapNode* nd = editor->map.getLeaf(nd_map_x, nd_map_y);
				if (!nd) {
					// We don't create nodes during extraction if they don't exist
					continue;
				}
				extractNode(nd, nd_map_x, nd_map_y, true);
			}
		}
	} else {
		int safe_start_x = nd_start_x - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
		int safe_start_y = nd_start_y - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
		int safe_end_x = nd_end_x + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
		int safe_end_y = nd_end_y + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;

		editor->map.visitLeaves(safe_start_x, safe_start_y, safe_end_x, safe_end_y, [&](MapNode* nd, int nd_map_x, int nd_map_y) {
			extractNode(nd, nd_map_x, nd_map_y, false);
		});
	}
}

void MapLayerDrawer::Submit(SpriteBatch& sprite_batch, int map_z, bool live_client, const RenderView& view, const DrawingOptions& options, LightBuffer& light_buffer) {
	int nd_start_x = view.start_x & ~3;
	int nd_start_y = view.start_y & ~3;
	int nd_end_x = (view.end_x & ~3) + 4;
	int nd_end_y = (view.end_y & ~3) + 4;

	int offset = (map_z <= GROUND_LAYER)
		? (GROUND_LAYER - map_z) * TILE_SIZE
		: TILE_SIZE * (view.floor - map_z);

	int base_screen_x = -view.view_scroll_x - offset;
	int base_screen_y = -view.view_scroll_y - offset;

	bool draw_lights = options.isDrawLight() && view.zoom <= 10.0;

	auto submitNode = [&](MapNode* nd, int nd_map_x, int nd_map_y, bool live) {
		int node_draw_x = nd_map_x * TILE_SIZE + base_screen_x;
		int node_draw_y = nd_map_y * TILE_SIZE + base_screen_y;

		// Node level culling: skip chunks entirely outside the viewport
		if (!view.IsRectVisible(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
			return;
		}

		if (live && !nd->isVisible(map_z > GROUND_LAYER)) {
			grid_drawer->DrawNodeLoadingPlaceholder(sprite_batch, nd_map_x, nd_map_y, view);
			return;
		}

		Floor* floor = nd->getFloor(map_z);
		if (!floor) {
			return;
		}

		RenderList* list = render_cache.Get(floor);
		if (!list) {
			return;
		}

		// Fast path: Just blit the pre-calculated display list directly into the hardware batch.
		// All heavy memory lookups and branch logic were already executed in the Extract phase!
		list->submit(sprite_batch, base_screen_x, base_screen_y);

		// Lights are evaluated dynamically for now based on the locations array
		if (draw_lights) {
			TileLocation* location = floor->locs.data();
			for (int map_x = 0; map_x < 4; ++map_x) {
				for (int map_y = 0; map_y < 4; ++map_y, ++location) {
					tile_renderer->AddLight(location, view, options, light_buffer);
				}
			}
		}
	};

	if (live_client) {
		for (int nd_map_x = nd_start_x; nd_map_x <= nd_end_x; nd_map_x += 4) {
			for (int nd_map_y = nd_start_y; nd_map_y <= nd_end_y; nd_map_y += 4) {
				MapNode* nd = editor->map.getLeaf(nd_map_x, nd_map_y);
				if (nd) {
					submitNode(nd, nd_map_x, nd_map_y, true);
				}
			}
		}
	} else {
		int safe_start_x = nd_start_x - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
		int safe_start_y = nd_start_y - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
		int safe_end_x = nd_end_x + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
		int safe_end_y = nd_end_y + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;

		editor->map.visitLeaves(safe_start_x, safe_start_y, safe_end_x, safe_end_y, [&](MapNode* nd, int nd_map_x, int nd_map_y) {
			submitNode(nd, nd_map_x, nd_map_y, false);
		});
	}
}

void MapLayerDrawer::Draw(SpriteBatch& sprite_batch, int map_z, bool live_client, const RenderView& view, const DrawingOptions& options, LightBuffer& light_buffer) {
	Extract(map_z, live_client, view, options);
	Submit(sprite_batch, map_z, live_client, view, options, light_buffer);
}
