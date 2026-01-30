//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_GROUND_BORDER_CALCULATOR_H
#define RME_GROUND_BORDER_CALCULATOR_H

#include "app/main.h"
#include "brushes/ground/ground_brush.h"
#include <vector>

class BaseMap;
class Tile;

/**
 * @brief Handles the calculation of ground borders.
 */
class GroundBorderCalculator {
public:
	/**
	 * @brief Calculates and applies borders for a specific tile.
	 *
	 * @param map The map where the tile resides.
	 * @param tile The tile to calculate borders for.
	 */
	static void calculate(BaseMap* map, Tile* tile);

private:
	struct NeighborInfo {
		bool visited;
		GroundBrush* brush;
	};

	struct CalculationContext {
		BaseMap* map;
		Tile* tile;
		GroundBrush* borderBrush;
		NeighborInfo neighbours[8];
		std::vector<GroundBrush::BorderCluster> borderList;
		std::vector<const GroundBrush::BorderBlock*> specificList;

		CalculationContext(BaseMap* m, Tile* t) :
			map(m), tile(t), borderBrush(nullptr) {
			for (auto& n : neighbours) {
				n = { false, nullptr };
			}
		}
	};

	static void gatherNeighbors(CalculationContext& ctx);
	static void processNeighbors(CalculationContext& ctx);
	static void applyBorders(CalculationContext& ctx);
	static void applySpecificCases(CalculationContext& ctx);
};

#endif // RME_GROUND_BORDER_CALCULATOR_H
