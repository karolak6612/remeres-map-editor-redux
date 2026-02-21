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
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/sprite_preloader.h"
#include "rendering/core/render_chunk.h"
#include "ui/gui.h"
#include <glm/gtc/matrix_transform.hpp>

MapLayerDrawer::MapLayerDrawer(TileRenderer* tile_renderer, GridDrawer* grid_drawer, Editor* editor) :
	tile_renderer(tile_renderer),
	grid_drawer(grid_drawer),
	editor(editor) {
}

MapLayerDrawer::~MapLayerDrawer() {
}

void MapLayerDrawer::Draw(SpriteBatch& sprite_batch, int map_z, bool live_client, const RenderView& view, const DrawingOptions& options, LightBuffer& light_buffer) {
	int nd_start_x = view.start_x & ~3;
	int nd_start_y = view.start_y & ~3;
	int nd_end_x = (view.end_x & ~3) + 4;
	int nd_end_y = (view.end_y & ~3) + 4;

	// Optimization: Pre-calculate offset and base coordinates
	int offset = (map_z <= GROUND_LAYER)
		? (GROUND_LAYER - map_z) * TILE_SIZE
		: TILE_SIZE * (view.floor - map_z);

	int base_screen_x = -view.view_scroll_x - offset;
	int base_screen_y = -view.view_scroll_y - offset;

	// Matrix to transform World Coordinates to Screen Coordinates
	glm::mat4 world_matrix = view.projectionMatrix * glm::translate(glm::mat4(1.0f), glm::vec3((float)base_screen_x, (float)base_screen_y, 0.0f));
	glm::mat4 original_matrix = view.projectionMatrix;

	// Check for option changes that require cache invalidation
	uint64_t current_hash = 0;
	current_hash |= (uint64_t)options.show_houses << 0;
	current_hash |= (uint64_t)options.show_blocking << 1;
	current_hash |= (uint64_t)options.show_spawns << 2;
	current_hash |= (uint64_t)options.show_items << 3;
	current_hash |= (uint64_t)options.transparent_items << 4;
	current_hash |= (uint64_t)options.show_special_tiles << 5;
	current_hash |= (uint64_t)options.isDrawLight() << 6;
	current_hash |= (uint64_t)options.show_creatures << 7;
	current_hash |= (uint64_t)options.highlight_items << 8;
	current_hash ^= (uint64_t)options.current_house_id << 10;

	if (current_hash != last_options_hash) {
		chunk_manager.Clear();
		last_options_hash = current_hash;
	}

	// Switch to World Matrix for chunk rendering
	if (g_gui.gfx.hasAtlasManager()) {
		sprite_batch.end(*g_gui.gfx.getAtlasManager());
	}
	sprite_batch.begin(world_matrix);

	bool draw_lights = options.isDrawLight() && view.zoom <= 10.0;

	// ND visibility
	auto collectSpriteWithPattern = [&](GameSprite* spr, int tx, int ty) {
		if (spr && !spr->isSimpleAndLoaded()) {
			int pattern_x = (spr->pattern_x > 1) ? tx % spr->pattern_x : 0;
			int pattern_y = (spr->pattern_y > 1) ? ty % spr->pattern_y : 0;
			int pattern_z = (spr->pattern_z > 1) ? map_z % spr->pattern_z : 0;
			rme::collectTileSprites(spr, pattern_x, pattern_y, pattern_z, 0);
		}
	};

	// Common lambda to draw a node
	auto drawNode = [&](MapNode* nd, int nd_map_x, int nd_map_y, bool live) {
		int node_draw_x = nd_map_x * TILE_SIZE + base_screen_x;
		int node_draw_y = nd_map_y * TILE_SIZE + base_screen_y;

		// Node level culling
		if (!view.IsRectVisible(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
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

			// Switch to Screen Matrix for GridDrawer
			if (g_gui.gfx.hasAtlasManager()) {
				sprite_batch.end(*g_gui.gfx.getAtlasManager());
				sprite_batch.begin(original_matrix);
				grid_drawer->DrawNodeLoadingPlaceholder(sprite_batch, nd_map_x, nd_map_y, view);
				sprite_batch.end(*g_gui.gfx.getAtlasManager());
				sprite_batch.begin(world_matrix);
			}
			return;
		}

		RenderChunk* chunk = chunk_manager.GetChunk(nd, map_z);
		bool rebuild = false;

		if (chunk->getLastRendered() < nd->last_modified.load(std::memory_order_relaxed) || chunk->isDynamic()) {
			rebuild = true;
		}

		if (rebuild) {
			chunk->Rebuild(nd, map_z, tile_renderer, view, options, options.current_house_id);
		}

		sprite_batch.append(chunk->getInstances());

		if (draw_lights) {
			const auto& cached_lights = chunk->getLights();
			if (!cached_lights.empty()) {
				light_buffer.lights.insert(light_buffer.lights.end(), cached_lights.begin(), cached_lights.end());
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

	// Restore original matrix for subsequent drawers
	if (g_gui.gfx.hasAtlasManager()) {
		sprite_batch.end(*g_gui.gfx.getAtlasManager());
	}
	sprite_batch.begin(original_matrix);
}

