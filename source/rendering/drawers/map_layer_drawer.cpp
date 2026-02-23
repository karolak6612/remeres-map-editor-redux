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
#include "rendering/utilities/render_benchmark.h"
#include <thread>
#include <future>
#include <mutex>

MapLayerDrawer::MapLayerDrawer(TileRenderer* tile_renderer, GridDrawer* grid_drawer, Editor* editor) :
	tile_renderer(tile_renderer),
	grid_drawer(grid_drawer),
	editor(editor) {
}

MapLayerDrawer::~MapLayerDrawer() {
}

struct ParallelDrawTask {
	MapNode* node;
	int map_x;
	int map_y;
	bool live;
};

void MapLayerDrawer::Draw(SpriteBatch& sprite_batch, int map_z, bool live_client, const RenderView& view, const DrawingOptions& options, LightBuffer& light_buffer) {
	auto start = std::chrono::high_resolution_clock::now();

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

	bool draw_lights = options.isDrawLight() && view.zoom <= 10.0;

	// Collect visible nodes
	std::vector<ParallelDrawTask> visible_nodes;
	// Heuristic reservation
	visible_nodes.reserve(2000);

	auto collectNode = [&](MapNode* nd, int nd_map_x, int nd_map_y, bool live) {
		int node_draw_x = nd_map_x * TILE_SIZE + base_screen_x;
		int node_draw_y = nd_map_y * TILE_SIZE + base_screen_y;

		// Node level culling
		if (!view.IsRectVisible(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
			RenderBenchmark::Get().IncrementMetric(RenderBenchmark::Metric::VisibleNodesCulled);
			return;
		}

		// If live and loading, draw placeholder immediately (uses GL, must be on main thread)
		if (live && !nd->isVisible(map_z > GROUND_LAYER)) {
			RenderBenchmark::Get().IncrementMetric(RenderBenchmark::Metric::VisibleNodesVisited);
			if (!nd->isRequested(map_z > GROUND_LAYER)) {
				if (editor->live_manager.GetClient()) {
					editor->live_manager.GetClient()->queryNode(nd_map_x, nd_map_y, map_z > GROUND_LAYER);
				}
				nd->setRequested(map_z > GROUND_LAYER, true);
			}
			grid_drawer->DrawNodeLoadingPlaceholder(sprite_batch, nd_map_x, nd_map_y, view);
			return;
		}

		visible_nodes.push_back({nd, nd_map_x, nd_map_y, live});
	};

	if (live_client) {
		for (int nd_map_x = nd_start_x; nd_map_x <= nd_end_x; nd_map_x += 4) {
			for (int nd_map_y = nd_start_y; nd_map_y <= nd_end_y; nd_map_y += 4) {
				MapNode* nd = editor->map.getLeaf(nd_map_x, nd_map_y);
				if (!nd) {
					nd = editor->map.createLeaf(nd_map_x, nd_map_y);
					nd->setVisible(false, false);
				}
				collectNode(nd, nd_map_x, nd_map_y, true);
			}
		}
	} else {
		int safe_start_x = nd_start_x - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
		int safe_start_y = nd_start_y - PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
		int safe_end_x = nd_end_x + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;
		int safe_end_y = nd_end_y + PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS / TILE_SIZE;

		editor->map.visitLeaves(safe_start_x, safe_start_y, safe_end_x, safe_end_y, [&](MapNode* nd, int nd_map_x, int nd_map_y) {
			collectNode(nd, nd_map_x, nd_map_y, false);
		});
	}

	RenderBenchmark::Get().IncrementMetric(RenderBenchmark::Metric::VisibleNodesVisited, visible_nodes.size());

	// Parallel Execution
	unsigned int num_threads = std::thread::hardware_concurrency();
	if (num_threads == 0) num_threads = 2;

	// Don't spawn threads for tiny workloads
	if (visible_nodes.size() < 16) {
		num_threads = 1;
	}

	std::vector<std::future<void>> futures;
	std::vector<SpriteBatch> thread_batches(num_threads);
	// We also need thread-local light buffers if we want to parallelize lights
	// LightBuffer is simple vector push, so we need a local one.
	// But TileRenderer::AddLight is simple.
	// For now, let's process lights serially or add locking? Locking LightBuffer is slow.
	// Let's defer light parallelization or create LightBuffer per thread.
	// LightBuffer is just a vector of lights.
	std::vector<LightBuffer> thread_lights(num_threads);

	size_t chunk_size = (visible_nodes.size() + num_threads - 1) / num_threads;

	auto work_lambda = [&](int thread_id, size_t start_idx, size_t end_idx) {
		SpriteBatch& local_batch = thread_batches[thread_id];
		LightBuffer& local_light = thread_lights[thread_id];

		// Initialize local batch state (projection matrix needed for culling? No, view is passed)
		// Local batch doesn't need GL init, it just acts as a container.

		for (size_t i = start_idx; i < end_idx; ++i) {
			const auto& task = visible_nodes[i];

			int node_draw_x = task.map_x * TILE_SIZE + base_screen_x;
			int node_draw_y = task.map_y * TILE_SIZE + base_screen_y;

			bool fully_inside = view.IsRectFullyInside(node_draw_x, node_draw_y, 4 * TILE_SIZE, 4 * TILE_SIZE);

			Floor* floor = task.node->getFloor(map_z);
			if (!floor) continue;

			TileLocation* location = floor->locs.data();
			int draw_x_base = node_draw_x;

			for (int map_x = 0; map_x < 4; ++map_x, draw_x_base += TILE_SIZE) {
				int draw_y = node_draw_y;
				for (int map_y = 0; map_y < 4; ++map_y, ++location, draw_y += TILE_SIZE) {
					if (!fully_inside && !view.IsPixelVisible(draw_x_base, draw_y, PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS)) {
						continue;
					}

					// Use local batch
					tile_renderer->DrawTile(local_batch, location, view, options, options.current_house_id, draw_x_base, draw_y);

					if (draw_lights) {
						tile_renderer->AddLight(location, view, options, local_light);
					}
				}
			}
		}
	};

	if (num_threads > 1) {
		for (unsigned int i = 0; i < num_threads; ++i) {
			size_t start_i = i * chunk_size;
			size_t end_i = std::min(start_i + chunk_size, visible_nodes.size());

			if (start_i < end_i) {
				futures.push_back(std::async(std::launch::async, work_lambda, i, start_i, end_i));
			}
		}

		for (auto& f : futures) {
			f.wait();
		}
	} else {
		// Run on main thread
		work_lambda(0, 0, visible_nodes.size());
	}

	// Merge results
	for (unsigned int i = 0; i < num_threads; ++i) {
		sprite_batch.append(thread_batches[i].getPendingSprites());

		if (draw_lights) {
			// LightBuffer interface doesn't have append? It's a struct with vector?
			// LightBuffer::lights is std::vector<Light>.
			// We need to access it. LightBuffer definition:
			/*
			struct LightBuffer {
				struct Light { int x, y, z, color, radius; };
				std::vector<Light> lights;
				void AddLight(...) ...
				void Clear() ...
			};
			*/
			// Assuming we can access lights vector.
			// LightBuffer is struct, defaults public.
			const auto& local_vec = thread_lights[i].lights;
			light_buffer.lights.insert(light_buffer.lights.end(), local_vec.begin(), local_vec.end());
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	RenderBenchmark::Get().IncrementMetric(RenderBenchmark::Metric::MapTraversalTime, duration);
}
