#ifndef RME_MAP_SPATIAL_HASH_GRID_H
#define RME_MAP_SPATIAL_HASH_GRID_H

#include "app/main.h"
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <vector>
#include <utility>
#include <ranges>
#include <algorithm>
#include <tuple>
#include <limits>

class MapNode;
class BaseMap;

class SpatialHashGrid {
public:
	static constexpr int CELL_SHIFT = 6; // 64 tiles
	static constexpr int CELL_SIZE = 1 << CELL_SHIFT;
	static constexpr int NODE_SHIFT = 2; // 4 tiles
	static constexpr int NODES_PER_CELL_SHIFT = CELL_SHIFT - NODE_SHIFT; // 4
	static constexpr int NODES_PER_CELL = 1 << NODES_PER_CELL_SHIFT; // 16 nodes (4x4 tiles each)
	static constexpr int TILES_PER_NODE = 16; // 4x4 tiles per node
	static constexpr int NODES_IN_CELL = NODES_PER_CELL * NODES_PER_CELL; // 256 nodes per cell

	struct GridCell {
		std::unique_ptr<MapNode> nodes[NODES_IN_CELL];
		GridCell();
		~GridCell();
	};

	struct SortedGridCell {
		uint64_t key;
		int cx, cy;
		GridCell* cell;
	};

	SpatialHashGrid(BaseMap& map);
	~SpatialHashGrid();

	// Returns observer pointer (non-owning)
	MapNode* getLeaf(int x, int y);
	// Forces leaf creation. Throws std::bad_alloc on memory failure.
	MapNode* getLeafForce(int x, int y);

	void clear();
	void clearVisible(uint32_t mask);

	const std::vector<SortedGridCell>& getSortedCells() const;
	void updateSortedCells() const;

	template <typename Func>
	void visitLeaves(int min_x, int min_y, int max_x, int max_y, Func&& func) {
		int start_nx = min_x >> NODE_SHIFT;
		int start_ny = min_y >> NODE_SHIFT;
		int end_nx = (max_x - 1) >> NODE_SHIFT;
		int end_ny = (max_y - 1) >> NODE_SHIFT;

		int start_cx = start_nx >> NODES_PER_CELL_SHIFT;
		int start_cy = start_ny >> NODES_PER_CELL_SHIFT;
		int end_cx = end_nx >> NODES_PER_CELL_SHIFT;
		int end_cy = end_ny >> NODES_PER_CELL_SHIFT;

		// Cast operands to long long to avoid subtraction overflow before the final multiplication.
		// (static_cast<long long>(end_cx) - start_cx + 1) ensures the expression is evaluated in 64-bit.
		long long num_viewport_cells = (static_cast<long long>(end_cx) - start_cx + 1) * (static_cast<long long>(end_cy) - start_cy + 1);

		// Strategy selection heuristic:
		// If the number of cells in the viewport is greater than the total number of allocated cells,
		// it's more efficient to iterate over allocated cells and check if they are within the viewport.
		// Otherwise, we iterate over the viewport cells and look them up in the hash map.
		// Mixing signed long long and unsigned size_t is safe here as both are non-negative.
		if (num_viewport_cells > static_cast<long long>(cells.size())) {
			visitLeavesByCells(start_nx, start_ny, end_nx, end_ny, start_cx, start_cy, end_cx, end_cy, std::forward<Func>(func));
		} else {
			visitLeavesByViewport(start_nx, start_ny, end_nx, end_ny, start_cx, start_cy, end_cx, end_cy, std::forward<Func>(func));
		}
	}

	auto begin() {
		return cells.begin();
	}
	auto end() {
		return cells.end();
	}

protected:
	BaseMap& map;
	std::unordered_map<uint64_t, std::unique_ptr<GridCell>> cells;
	mutable std::vector<SortedGridCell> sorted_cells_cache;
	mutable bool sorted_cells_dirty;

	mutable uint64_t last_key = 0;
	mutable GridCell* last_cell = nullptr;

	// Traverses cells by iterating over the viewport coordinates.
	// Efficient for small or dense viewports.
	template <typename Func>
	void visitLeavesByViewport(int start_nx, int start_ny, int end_nx, int end_ny, int start_cx, int start_cy, int end_cx, int end_cy, Func&& func) {
		struct RowCellInfo {
			GridCell* cell;
			int start_nx;
		};
		std::vector<RowCellInfo> row_cells;
		row_cells.reserve(end_cx - start_cx + 1);

		for (int cy = start_cy; cy <= end_cy; ++cy) {
			row_cells.clear();

			for (int cx = start_cx; cx <= end_cx; ++cx) {
				uint64_t key = makeKeyFromCell(cx, cy);
				auto it = cells.find(key);
				if (it != cells.end()) {
					row_cells.push_back({ .cell = it->second.get(), .start_nx = (cx << NODES_PER_CELL_SHIFT) });
				}
			}

			if (row_cells.empty()) {
				continue;
			}

			int row_start_ny = std::max(start_ny, cy << NODES_PER_CELL_SHIFT);
			int row_end_ny = std::min(end_ny, ((cy + 1) << NODES_PER_CELL_SHIFT) - 1);

			for (int ny : std::views::iota(row_start_ny, row_end_ny + 1)) {
				int local_ny = ny & (NODES_PER_CELL - 1);

				for (const auto& [cell, cell_start_nx] : row_cells) {
					int local_start_nx = std::max(start_nx, cell_start_nx) - cell_start_nx;
					int local_end_nx = std::min(end_nx, cell_start_nx + NODES_PER_CELL - 1) - cell_start_nx;

					for (int lnx = local_start_nx; lnx <= local_end_nx; ++lnx) {
						int idx = local_ny * NODES_PER_CELL + lnx;
						if (MapNode* node = cell->nodes[idx].get()) {
							func(node, (cell_start_nx + lnx) << NODE_SHIFT, ny << NODE_SHIFT);
						}
					}
				}
			}
		}
	}

	// Traverses cells by iterating over pre-filtered allocated cells.
	// Efficient for huge or sparse viewports.
	template <typename Func>
	void visitLeavesByCells(int start_nx, int start_ny, int end_nx, int end_ny, int start_cx, int start_cy, int end_cx, int end_cy, Func&& func) {
		updateSortedCells();

		if (sorted_cells_cache.empty()) {
			return;
		}

		SortedGridCell search_val { .key = 0, .cx = std::numeric_limits<int>::min(), .cy = start_cy, .cell = nullptr };

		// Find the first cell that could potentially be in the viewport
		auto it = std::lower_bound(sorted_cells_cache.begin(), sorted_cells_cache.end(), search_val, [](const SortedGridCell& a, const SortedGridCell& b) {
			return std::tie(a.cy, a.cx) < std::tie(b.cy, b.cx);
		});

		int prev_cy = -1;

		std::vector<const SortedGridCell*> row_cells;
		row_cells.reserve(end_cx - start_cx + 1);

		for (int ny : std::views::iota(start_ny, end_ny + 1)) {
			int current_cy = ny >> NODES_PER_CELL_SHIFT;
			int local_ny = ny & (NODES_PER_CELL - 1);

			if (current_cy != prev_cy) {
				row_cells.clear();
				// Advance 'it' to start of current_cy
				while (it != sorted_cells_cache.end() && it->cy < current_cy) {
					++it;
				}

				// Collect cells for this row
				auto cell_it = it;
				while (cell_it != sorted_cells_cache.end() && cell_it->cy == current_cy) {
					if (cell_it->cx >= start_cx && cell_it->cx <= end_cx) {
						row_cells.push_back(&(*cell_it));
					} else if (cell_it->cx > end_cx) {
						break;
					}
					++cell_it;
				}
				prev_cy = current_cy;
			}

			for (const auto* cell_data : row_cells) {
				GridCell* cell = cell_data->cell;
				int cell_start_nx = cell_data->cx << NODES_PER_CELL_SHIFT;

				int local_start_nx = std::max(start_nx, cell_start_nx) - cell_start_nx;
				int local_end_nx = std::min(end_nx, cell_start_nx + NODES_PER_CELL - 1) - cell_start_nx;

				for (int lnx = local_start_nx; lnx <= local_end_nx; ++lnx) {
					int idx = local_ny * NODES_PER_CELL + lnx;
					if (MapNode* node = cell->nodes[idx].get()) {
						func(node, (cell_start_nx + lnx) << NODE_SHIFT, ny << NODE_SHIFT);
					}
				}
			}
		}
	}

	static uint64_t makeKeyFromCell(int cx, int cy) {
		static_assert(sizeof(int) == 4, "Key packing assumes exactly 32-bit integers");
		return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) | static_cast<uint32_t>(cy);
	}

	static void getCellCoordsFromKey(uint64_t key, int& cx, int& cy) {
		cx = static_cast<int32_t>(key >> 32);
		cy = static_cast<int32_t>(key);
	}

	static uint64_t makeKey(int x, int y) {
		return makeKeyFromCell(x >> CELL_SHIFT, y >> CELL_SHIFT);
	}

	friend class BaseMap;
	friend class MapIterator;
};

#endif
