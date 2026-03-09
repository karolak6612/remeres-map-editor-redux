//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TOOLTIP_DATA_H_
#define RME_TOOLTIP_DATA_H_

#include "map/position.h"
#include <vector>
#include <string>
#include <string_view>

struct ContainerItem {
	uint16_t id;
	uint8_t count;
	uint8_t subtype;
};

// Tooltip category determines header color and icon
enum class TooltipCategory {
	WAYPOINT, // Green - landmark
	ITEM,     // Dark charcoal - generic item
	DOOR,     // Brown - door with door ID
	TELEPORT, // Purple - teleporter
	TEXT      // Gold - readable text (signs, books)
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
		return !waypointName.empty() || actionId > 0 || uniqueId > 0 ||
			   doorId > 0 || !text.empty() || !description.empty() ||
			   destination.x > 0 || !containerItems.empty();
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
