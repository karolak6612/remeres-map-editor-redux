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

#ifndef RME_TOOLTIP_DATA_H_
#define RME_TOOLTIP_DATA_H_

#include <cstdint>
#include <string_view>
#include <vector>
#include "map/position.h"

struct ContainerItem {
	uint16_t id;
	uint8_t count;
	uint8_t subtype;
};

// Tooltip category determines header color and icon
enum class TooltipCategory {
	WAYPOINT, // Green - landmark
	ITEM, // Dark charcoal - generic item
	DOOR, // Brown - door with door ID
	TELEPORT, // Purple - teleporter
	TEXT // Gold - readable text (signs, books)
};

// Structured tooltip data for card-based rendering
struct TooltipData {
	Position pos;
	TooltipCategory category = TooltipCategory::ITEM;

	// Header info
	uint16_t itemId = 0;
	std::string_view itemName;

	// Optional fields (0 or empty = not shown)
	uint16_t actionId = 0;
	uint16_t uniqueId = 0;
	uint8_t doorId = 0;
	std::string_view text;
	std::string_view description;
	Position destination; // For teleports (check if valid via destination.x > 0)

	// Waypoint-specific
	std::string_view waypointName;

	// Container contents
	std::vector<ContainerItem> containerItems;
	uint8_t containerCapacity = 0;

	TooltipData() = default;

	// Constructor for waypoint
	TooltipData(Position p, std::string_view wpName) :
		pos(p), category(TooltipCategory::WAYPOINT), waypointName(wpName) { }

	// Constructor for item
	TooltipData(Position p, uint16_t id, std::string_view name) :
		pos(p), category(TooltipCategory::ITEM), itemId(id), itemName(name) { }

	// Determine category based on fields
	void updateCategory() {
		if (!waypointName.empty()) {
			category = TooltipCategory::WAYPOINT;
		} else if (destination.x > 0) {
			category = TooltipCategory::TELEPORT;
		} else if (doorId > 0) {
			category = TooltipCategory::DOOR;
		} else if (!text.empty()) {
			category = TooltipCategory::TEXT;
		} else {
			category = TooltipCategory::ITEM;
		}
	}

	// Check if this tooltip has any visible fields
	bool hasVisibleFields() const {
		return !waypointName.empty() || actionId > 0 || uniqueId > 0 || doorId > 0 || !text.empty() || !description.empty() || destination.x > 0 || !containerItems.empty();
	}

	void clear() {
		pos = Position();
		category = TooltipCategory::ITEM;
		itemId = 0;
		itemName = {};
		actionId = 0;
		uniqueId = 0;
		doorId = 0;
		text = {};
		description = {};
		destination = Position();
		waypointName = {};
		containerItems.clear();
		containerCapacity = 0;
	}
};

#endif
