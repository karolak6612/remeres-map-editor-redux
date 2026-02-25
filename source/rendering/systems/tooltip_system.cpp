#include "rendering/systems/tooltip_system.h"
#include "game/item.h"
#include "game/items.h"
#include "game/complexitem.h"
#include "rendering/ui/tooltip_drawer.h"
#include "app/definitions.h"
#include <boost/noncopyable.hpp>

// Helper function to populate tooltip data from an item (in-place)
bool TooltipSystem::FillItemTooltipData(TooltipData& data, Item* item, const ItemType& it, const Position& pos, bool isHouseTile, float zoom) {
	if (!item) {
		return false;
	}

	const uint16_t id = item->getID();
	if (id < 100) {
		return false;
	}

	uint16_t unique = 0;
	uint16_t action = 0;
	std::string_view text;
	std::string_view description;
	uint8_t doorId = 0;
	Position destination;
	bool hasContent = false;

	bool is_complex = item->isComplex();
	// Early exit for simple items
	// isTooltipable is cached (isContainer || isDoor || isTeleport)
	if (!is_complex && !it.isTooltipable()) {
		return false;
	}

	bool is_container = it.isContainer();
	bool is_door = isHouseTile && item->isDoor();
	bool is_teleport = item->isTeleport();

	if (is_complex) {
		unique = item->getUniqueID();
		action = item->getActionID();
		text = item->getText();
		description = item->getDescription();
	}

	// Check if it's a door
	if (is_door) {
		if (const Door* door = item->asDoor()) {
			if (door->isRealDoor()) {
				doorId = door->getDoorID();
			}
		}
	}

	// Check if it's a teleport
	if (is_teleport) {
		Teleport* tp = static_cast<Teleport*>(item);
		if (tp->hasDestination()) {
			destination = tp->getDestination();
		}
	}

	// Check if container has content
	if (is_container) {
		if (const Container* container = item->asContainer()) {
			hasContent = container->getItemCount() > 0;
		}
	}

	// Only create tooltip if there's something to show
	if (unique == 0 && action == 0 && doorId == 0 && text.empty() && description.empty() && destination.x == 0 && !hasContent) {
		return false;
	}

	// Get item name from database
	std::string_view itemName = it.name;
	if (itemName.empty()) {
		itemName = "Item";
	}

	data.pos = pos;
	data.itemId = id;
	data.itemName = itemName; // Assign string_view to string_view (no copy)

	data.actionId = action;
	data.uniqueId = unique;
	data.doorId = doorId;
	data.text = text;
	data.description = description;
	data.destination = destination;

	// Populate container items
	if (it.isContainer() && zoom <= 1.5f) {
		if (const Container* container = item->asContainer()) {
			// Set capacity for rendering empty slots
			data.containerCapacity = static_cast<uint8_t>(container->getVolume());

			const auto& items = container->getVector();
			data.containerItems.clear();
			// Reserve only what we need (capped at 32)
			data.containerItems.reserve(std::min(items.size(), size_t(32)));
			for (const auto& subItem : items) {
				if (subItem) {
					ContainerItem ci;
					ci.id = subItem->getID();
					ci.subtype = subItem->getSubtype();
					ci.count = subItem->getCount();
					// Sanity check for count
					if (ci.count == 0) {
						ci.count = 1;
					}

					data.containerItems.push_back(ci);

					// Limit preview items to avoid massive tooltips
					if (data.containerItems.size() >= 32) {
						break;
					}
				}
			}
		}
	}

	data.updateCategory();

	return true;
}
