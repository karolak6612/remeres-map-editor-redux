#include "ingame_preview/floor_visibility_calculator.h"
#include "map/basemap.h"
#include "map/tile.h"
#include <algorithm>

namespace IngamePreview {

	FloorVisibilityCalculator::FloorVisibilityCalculator() = default;

	bool FloorVisibilityCalculator::TileLimitsFloorsView(const Tile* tile, bool is_free_view) const {
		if (!tile) {
			return false;
		}

		const Item* first_thing = tile->ground.get();
		if (!first_thing && !tile->items.empty()) {
			first_thing = tile->items.front().get();
		}

		if (!first_thing) {
			return false;
		}

		const auto definition = first_thing->getDefinition();
		if (definition.hasFlag(ItemFlag::IgnoreLook)) {
			return false;
		}

		if (is_free_view) {
			return first_thing->isGroundTile() || first_thing->isAlwaysOnBottom();
		}

		return first_thing->isGroundTile() || (first_thing->isAlwaysOnBottom() && first_thing->hasProperty(BLOCKPROJECTILE));
	}

	bool FloorVisibilityCalculator::IsLookPossible(const Tile* tile) const {
		if (!tile) {
			return false;
		}

		if (tile->ground && tile->ground->hasProperty(BLOCKPROJECTILE)) {
			return false;
		}

		return std::ranges::none_of(tile->items, [](const auto& item) {
			return item->hasProperty(BLOCKPROJECTILE);
		});
	}

	int FloorVisibilityCalculator::CalcFirstVisibleFloor(const BaseMap& map, int camera_x, int camera_y, int camera_z) const {
		int first_floor = 0;
		if (camera_z > GROUND_LAYER) {
			first_floor = std::max(camera_z - AWARE_UNDERGROUND_FLOOR_RANGE, static_cast<int>(GROUND_LAYER) + 1);
		}

		for (int ix = -1; ix <= 1 && first_floor < camera_z; ++ix) {
			for (int iy = -1; iy <= 1 && first_floor < camera_z; ++iy) {
				const int pos_x = camera_x + ix;
				const int pos_y = camera_y + iy;
				const bool is_center = ix == 0 && iy == 0;
				const bool is_straight_neighbor = std::abs(ix) != std::abs(iy);
				const Tile* position_tile = map.getTile(pos_x, pos_y, camera_z);
				const bool look_possible = IsLookPossible(position_tile);

				if (!is_center && (!is_straight_neighbor && !look_possible)) {
					continue;
				}

				int upper_x = pos_x;
				int upper_y = pos_y;
				int upper_z = camera_z;
				int covered_x = pos_x;
				int covered_y = pos_y;
				int covered_z = camera_z;

				while (upper_z > 0 && covered_z > 0) {
					--upper_z;
					--covered_z;
					++covered_x;
					++covered_y;
					if (upper_z < first_floor) {
						break;
					}

					if (const Tile* upper_tile = map.getTile(upper_x, upper_y, upper_z); TileLimitsFloorsView(upper_tile, !look_possible)) {
						// This inversion is intentional: vertical sight above the camera uses the complementary free-view rule,
						// while the diagonal covered-tile march below keeps the original rule to match OTClient's asymmetric floor stepping.
						first_floor = upper_z + 1;
						break;
					}

					if (const Tile* covered_tile = map.getTile(covered_x, covered_y, covered_z); TileLimitsFloorsView(covered_tile, look_possible)) {
						first_floor = covered_z + 1;
						break;
					}
				}
			}
		}

		return std::clamp(first_floor, 0, static_cast<int>(MAP_MAX_LAYER));
	}

	int FloorVisibilityCalculator::CalcLastVisibleFloor(int camera_z) const {
		int z;
		if (camera_z > GROUND_LAYER) {
			z = camera_z + AWARE_UNDERGROUND_FLOOR_RANGE;
		} else {
			z = GROUND_LAYER;
		}
		return std::clamp(z, 0, static_cast<int>(MAP_MAX_LAYER));
	}

} // namespace IngamePreview
