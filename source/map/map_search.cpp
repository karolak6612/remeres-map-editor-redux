#include "app/main.h"
#include "map/map_search.h"
#include "map/map.h"
#include "map/tile.h"
#include "map/map_region.h" // For MapNode, Floor
#include "map/spatial_hash_grid.h"
#include "ui/gui.h"
#include "editor/editor.h"
#include "editor/operations/search_operations.h"
#include <algorithm>
#include <thread>
#include <future>
#include <mutex>
#include <vector>
#include <cmath>

std::vector<SearchResult> MapSearchUtility::SearchItems(Map& map, bool unique, bool action, bool container, bool writable, bool onSelection) {
	// Prepare search configuration
	EditorOperations::MapSearcher prototype_searcher;
	prototype_searcher.search_unique = unique;
	prototype_searcher.search_action = action;
	prototype_searcher.search_container = container;
	prototype_searcher.search_writeable = writable;

	// Helper to convert results
	auto convertToResults = [&](EditorOperations::MapSearcher& searcher) {
		std::vector<SearchResult> results;
		results.reserve(searcher.found.size());
		for (auto& p : searcher.found) {
			results.push_back({ p.first, p.second, std::string(searcher.desc(p.second).c_str()) });
		}
		return results;
	};

	// Use parallel search if not searching on selection (which is usually small)
	if (!onSelection) {
		auto cells = map.getGrid().getSortedCells();
		size_t num_cells = cells.size();
		if (num_cells == 0) return {};

		unsigned int num_threads = std::thread::hardware_concurrency();
		if (num_threads == 0) num_threads = 2;

		// Don't spawn threads for tiny maps
		if (num_cells < 100) {
			num_threads = 1;
		}

		size_t chunk_size = (num_cells + num_threads - 1) / num_threads;
		std::vector<std::future<std::vector<std::pair<Tile*, Item*>>>> futures;

		// Lambda for processing a chunk of cells
		auto process_chunk = [&](size_t start_idx, size_t end_idx) -> std::vector<std::pair<Tile*, Item*>> {
			EditorOperations::MapSearcher local_searcher = prototype_searcher;
			long long dummy_done = 0;
			std::vector<Container*> containers;
			containers.reserve(64);

			for (size_t i = start_idx; i < end_idx; ++i) {
				SpatialHashGrid::GridCell* cell = cells[i].cell;
				if (!cell) continue;

				for (const auto& node_ptr : cell->nodes) {
					MapNode* node = node_ptr.get();
					if (!node) continue;

					// Iterate all floors (layers)
					for (int z = 0; z <= MAP_MAX_LAYER; ++z) {
						if (!node->hasFloor(z)) continue;

						Floor* floor = node->getFloor(z);
						if (!floor) continue;

						for (int t = 0; t < SpatialHashGrid::TILES_PER_NODE; ++t) {
							TileLocation& loc = floor->locs[t];
							Tile* tile = loc.get();
							if (!tile) continue;

							// Process Ground
							if (tile->ground) {
								local_searcher(map, tile, tile->ground, dummy_done);
							}

							// Process Items
							for (auto* item : tile->items) {
								local_searcher(map, tile, item, dummy_done);

								// Deep search in containers
								if (Container* c = item->asContainer()) {
									containers.clear();
									containers.push_back(c);

									size_t idx = 0;
									while (idx < containers.size()) {
										Container* current = containers[idx++];
										ItemVector& v = current->getVector();
										for (auto* inner_item : v) {
											local_searcher(map, tile, inner_item, dummy_done);
											if (Container* inner_c = inner_item->asContainer()) {
												containers.push_back(inner_c);
											}
										}
									}
								}
							}
						}
					}
				}
			}
			return local_searcher.found;
		};

		// Launch threads
		for (unsigned int t = 0; t < num_threads; ++t) {
			size_t start = t * chunk_size;
			size_t end = std::min(start + chunk_size, num_cells);

			if (start >= end) break;

			futures.push_back(std::async(std::launch::async, process_chunk, start, end));
		}

		// Collect results
		std::vector<std::pair<Tile*, Item*>> all_found;
		for (auto& f : futures) {
			auto chunk_results = f.get();
			all_found.insert(all_found.end(), chunk_results.begin(), chunk_results.end());
		}

		// Sort results
		prototype_searcher.found = std::move(all_found);
		prototype_searcher.sort();

		return convertToResults(prototype_searcher);
	}

	// Fallback to original implementation for selection-based search
	foreach_ItemOnMap(map, prototype_searcher, onSelection);
	prototype_searcher.sort();

	return convertToResults(prototype_searcher);
}
