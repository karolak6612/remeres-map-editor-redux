//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "map/operations/map_processor.h"
#include "editor/editor.h"
#include "map/map.h"
#include "map/map_region.h"
#include "map/spatial_hash_grid.h"
#include "map/tile_operations.h"
#include "ui/gui.h"
#include "brushes/ground/ground_brush.h"

#include <atomic>
#include <thread>
#include <future>
#include <vector>
#include <algorithm>

namespace {
	void ProcessTilesParallel(Editor& editor, bool showdialog, const std::function<void(Tile*)>& tileFunc) {
		auto cells = editor.map.getGrid().getSortedCells();
		size_t num_cells = cells.size();
		if (num_cells == 0) {
			return;
		}

		unsigned int num_threads = std::thread::hardware_concurrency();
		if (num_threads == 0) {
			num_threads = 2;
		}
		if (num_threads > 16) {
			num_threads = 16;
		}

		if (num_cells < 100) {
			num_threads = 1;
		}

		std::atomic<uint64_t> tiles_done{0};
		uint64_t total_tiles = editor.map.getTileCount();

		auto worker = [&](size_t start_idx, size_t end_idx) {
			for (size_t i = start_idx; i < end_idx; ++i) {
				SpatialHashGrid::GridCell* cell = cells[i].cell;
				if (!cell) {
					continue;
				}

				for (const auto& node_ptr : cell->nodes) {
					MapNode* node = node_ptr.get();
					if (!node) {
						continue;
					}

					for (int z = 0; z <= MAP_MAX_LAYER; ++z) {
						if (!node->hasFloor(z)) {
							continue;
						}

						Floor* floor = node->getFloor(z);
						if (!floor) {
							continue;
						}

						for (TileLocation& loc : floor->locs) {
							Tile* tile = loc.get();
							if (!tile) {
								continue;
							}

							tileFunc(tile);

							uint64_t done = tiles_done.fetch_add(1, std::memory_order_relaxed);
							if (showdialog && (done & 0xFFF) == 0) { // Modulo 4096
								wxGetApp().CallAfter([done, total_tiles]() {
									g_gui.SetLoadDone(static_cast<int32_t>(done / static_cast<double>(total_tiles) * 100.0));
								});
							}
						}
					}
				}
			}
		};

		size_t chunk_size = (num_cells + num_threads - 1) / num_threads;
		std::vector<std::future<void>> futures;

		for (unsigned int t = 0; t < num_threads; ++t) {
			size_t start = t * chunk_size;
			size_t end = std::min(start + chunk_size, num_cells);

			if (start >= end) {
				break;
			}

			futures.push_back(std::async(std::launch::async, worker, start, end));
		}

		for (auto& f : futures) {
			f.get();
		}
	}
}

void MapProcessor::borderizeMap(Editor& editor, bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Borderizing map...");
	}

	ProcessTilesParallel(editor, showdialog, [&editor](Tile* tile) {
		TileOperations::borderize(tile, &editor.map);
	});

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}

void MapProcessor::randomizeMap(Editor& editor, bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Randomizing map...");
	}

	ProcessTilesParallel(editor, showdialog, [&editor](Tile* tile) {
		GroundBrush* groundBrush = tile->getGroundBrush();
		if (groundBrush) {
			Item* oldGround = tile->ground.get();

			uint16_t actionId, uniqueId;
			if (oldGround) {
				actionId = oldGround->getActionID();
				uniqueId = oldGround->getUniqueID();
			} else {
				actionId = 0;
				uniqueId = 0;
			}
			groundBrush->draw(&editor.map, tile, nullptr);

			Item* newGround = tile->ground.get();
			if (newGround) {
				newGround->setActionID(actionId);
				newGround->setUniqueID(uniqueId);
			}
			TileOperations::update(tile);
		}
	});

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}

void MapProcessor::clearInvalidHouseTiles(Editor& editor, bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Clearing invalid house tiles...");
	}

	Houses& houses = editor.map.houses;

	HouseMap::iterator iter = houses.begin();
	while (iter != houses.end()) {
		House* h = iter->second.get();
		if (editor.map.towns.getTown(h->townid) == nullptr) {
			iter = houses.erase(iter);
		} else {
			++iter;
		}
	}

	ProcessTilesParallel(editor, showdialog, [&houses](Tile* tile) {
		if (tile->isHouseTile()) {
			if (houses.getHouse(tile->getHouseID()) == nullptr) {
				tile->setHouse(nullptr);
			}
		}
	});

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}

void MapProcessor::clearModifiedTileState(Editor& editor, bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Clearing modified state from all tiles...");
	}

	ProcessTilesParallel(editor, showdialog, [](Tile* tile) {
		tile->unmodify();
	});

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}
