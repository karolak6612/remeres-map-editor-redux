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
#include <array>

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
		std::array<std::unique_ptr<MapNode>, NODES_IN_CELL> nodes;
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
	const MapNode* getLeaf(int x, int y) const;
	// Forces leaf creation. Throws std::bad_alloc on memory failure.
	MapNode* getLeafForce(int x, int y);

	void clear();
	void clearVisible(uint32_t mask);

	const std::vector<SortedGridCell>& getSortedCells() const;
	void updateSortedCells() const;
	static void getCellCoordsFromKey(uint64_t key, int& cx, int& cy);

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
	uint32_t generation_id = 0;
	uint64_t instance_id = 0;

	// Traverses cells by checking every potential cell in the viewport.
	// Efficient for small viewports on dense maps.
	template <typename Func>
	void visitLeavesByViewport(int start_nx, int start_ny, int end_nx, int end_ny, int start_cx, int start_cy, int end_cx, int end_cy, Func&& func) {
		visitLeavesByViewportImpl(start_nx, start_ny, end_nx, end_ny, start_cx, start_cy, end_cx, end_cy, std::forward<Func>(func));
	}

	struct RowCellInfo {
		GridCell* cell;
		int cell_start_nx;
		int local_start_nx;
		int local_end_nx;
	};

	// Traverses cells by checking every potential cell in the viewport.
	// Efficient for small viewports on dense maps.
	template <typename Func>
	void visitLeavesByViewportImpl(int start_nx, int start_ny, int end_nx, int end_ny, int start_cx, int start_cy, int end_cx, int end_cy, Func&& func) {
		std::vector<RowCellInfo> row_cells;
		row_cells.reserve(end_cx - start_cx + 1);

		for (int cy = start_cy; cy <= end_cy; ++cy) {
			row_cells.clear();

			for (int cx = start_cx; cx <= end_cx; ++cx) {
				uint64_t key = makeKeyFromCell(cx, cy);
				auto it = cells.find(key);
				if (it != cells.end()) {
					int cell_start_nx = cx << NODES_PER_CELL_SHIFT;
					row_cells.push_back({ .cell = it->second.get(), .cell_start_nx = cell_start_nx, .local_start_nx = std::max(start_nx, cell_start_nx) - cell_start_nx, .local_end_nx = std::min(end_nx, cell_start_nx + NODES_PER_CELL - 1) - cell_start_nx });
				}
			}

			if (row_cells.empty()) {
				continue;
			}

			int row_start_ny = std::max(start_ny, cy << NODES_PER_CELL_SHIFT);
			int row_end_ny = std::min(end_ny, ((cy + 1) << NODES_PER_CELL_SHIFT) - 1);

			for (int ny = row_start_ny; ny <= row_end_ny; ++ny) {
				int local_ny = ny & (NODES_PER_CELL - 1);
				int idx_base = local_ny * NODES_PER_CELL;

				for (const auto& row_cell : row_cells) {
					for (int lnx = row_cell.local_start_nx; lnx <= row_cell.local_end_nx; ++lnx) {
						if (MapNode* node = row_cell.cell->nodes[idx_base + lnx].get()) {
							func(node, (row_cell.cell_start_nx + lnx) << NODE_SHIFT, ny << NODE_SHIFT);
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

		// INT_MIN for cx produces the minimum biased key for row start_cy,
		// so lower_bound lands at the first cell for that row (or later).
		SortedGridCell search_val { .key = makeKeyFromCell(std::numeric_limits<int>::min(), start_cy), .cx = std::numeric_limits<int>::min(), .cy = start_cy, .cell = nullptr };

		// Comparator matches the sort order established in updateSortedCells.
		auto it = std::lower_bound(sorted_cells_cache.begin(), sorted_cells_cache.end(), search_val, [](const SortedGridCell& a, const SortedGridCell& b) {
			return a.key < b.key;
		});

		int prev_cy = std::numeric_limits<int>::min();

		std::vector<RowCellInfo> row_cells;
		row_cells.reserve(end_cx - start_cx + 1);

		for (int ny = start_ny; ny <= end_ny; ++ny) {
			int current_cy = ny >> NODES_PER_CELL_SHIFT;
			int local_ny = ny & (NODES_PER_CELL - 1);

			if (current_cy != prev_cy) {
				row_cells.clear();
				// Advance 'it' to start of current_cy
				while (it != sorted_cells_cache.end() && it->cy < current_cy) {
					++it;
				}

				// Collect and hoist calculations for this row
				auto cell_it = it;
				uint64_t end_key = makeKeyFromCell(end_cx, current_cy);
				while (cell_it != sorted_cells_cache.end() && cell_it->cy == current_cy) {
					if (cell_it->cx >= start_cx && cell_it->cx <= end_cx) {
						int cell_start_nx = cell_it->cx << NODES_PER_CELL_SHIFT;
						row_cells.push_back({ .cell = cell_it->cell, .cell_start_nx = cell_start_nx, .local_start_nx = std::max(start_nx, cell_start_nx) - cell_start_nx, .local_end_nx = std::min(end_nx, cell_start_nx + NODES_PER_CELL - 1) - cell_start_nx });
					} else if (cell_it->key > end_key) {
						break;
					}
					++cell_it;
				}
				prev_cy = current_cy;
			}

			int idx_base = local_ny * NODES_PER_CELL;
			for (const auto& row_cell : row_cells) {
				for (int lnx = row_cell.local_start_nx; lnx <= row_cell.local_end_nx; ++lnx) {
					if (MapNode* node = row_cell.cell->nodes[idx_base + lnx].get()) {
						func(node, (row_cell.cell_start_nx + lnx) << NODE_SHIFT, ny << NODE_SHIFT);
					}
				}
			}
		}
	}

	static uint64_t makeKeyFromCell(int cx, int cy) {
		static_assert(sizeof(int) == 4, "Key packing assumes exactly 32-bit integers");
		return (static_cast<uint64_t>(static_cast<uint32_t>(cy) ^ 0x80000000u) << 32) | (static_cast<uint32_t>(cx) ^ 0x80000000u);
	}

	static uint64_t makeKey(int x, int y) {
		return makeKeyFromCell(x >> CELL_SHIFT, y >> CELL_SHIFT);
	}

	friend class BaseMap;
	friend class MapIterator;
};

#endif
