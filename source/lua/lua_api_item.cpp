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
#include "lua_api_item.h"
#include "game/item.h"
#include "game/items.h"

#include <algorithm>
#include <cctype>

namespace LuaAPI {

	// Helper: convert string to lowercase
	static std::string toLower(const std::string& str) {
		std::string result = str;
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
		return result;
	}

	// Helper: check if string contains another (case-insensitive)
	static bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
		std::string lowerHaystack = toLower(haystack);
		std::string lowerNeedle = toLower(needle);
		return lowerHaystack.find(lowerNeedle) != std::string::npos;
	}

	void registerItem(sol::state& lua) {
		// Register Item usertype
		lua.new_usertype<Item>(
			"Item",
			// No public constructor - items are created via Tile:addItem() or obtained from tiles
			sol::no_constructor,

			// Read-only properties
			"getID", &Item::getID,
			"id", sol::property(&Item::getID),
			"getClientID", &Item::getClientID,
			"clientId", sol::property(&Item::getClientID),
			"getName", &Item::getName,
			"name", sol::property(&Item::getName),
			"getFullName", &Item::getFullName,
			"fullName", sol::property(&Item::getFullName),

			// Read/write properties
			"getCount", &Item::getCount,
			"setCount", [](Item& item, int count) {
				item.setSubtype(static_cast<uint16_t>(count));
			},
			"count", sol::property(&Item::getCount, [](Item& item, int count) {
				item.setSubtype(static_cast<uint16_t>(count));
			}),
			"getSubtype", [](const Item& item) -> int { return item.getSubtype(); },
			"setSubtype", [](Item& item, int subtype) { item.setSubtype(static_cast<uint16_t>(subtype)); },
			"subtype", sol::property([](const Item& item) -> int { return item.getSubtype(); }, [](Item& item, int subtype) { item.setSubtype(static_cast<uint16_t>(subtype)); }),
			"getActionID", [](const Item& item) -> int { return item.getActionID(); },
			"setActionID", [](Item& item, int aid) { item.setActionID(static_cast<uint16_t>(aid)); },
			"actionId", sol::property([](const Item& item) -> int { return item.getActionID(); }, [](Item& item, int aid) { item.setActionID(static_cast<uint16_t>(aid)); }),
			"getUniqueID", [](const Item& item) -> int { return item.getUniqueID(); },
			"setUniqueID", [](Item& item, int uid) { item.setUniqueID(static_cast<uint16_t>(uid)); },
			"uniqueId", sol::property([](const Item& item) -> int { return item.getUniqueID(); }, [](Item& item, int uid) { item.setUniqueID(static_cast<uint16_t>(uid)); }),
			"getTier", [](const Item& item) -> int { return item.getTier(); },
			"setTier", [](Item& item, int tier) { item.setTier(static_cast<uint16_t>(tier)); },
			"tier", sol::property([](const Item& item) -> int { return item.getTier(); }, [](Item& item, int tier) { item.setTier(static_cast<uint16_t>(tier)); }),
			"getText", &Item::getText,
			"setText", &Item::setText,
			"text", sol::property(&Item::getText, &Item::setText),
			"getDescription", &Item::getDescription,
			"setDescription", &Item::setDescription,
			"description", sol::property(&Item::getDescription, &Item::setDescription),

			// Selection
			"isSelected", sol::property(&Item::isSelected),
			"select", &Item::select,
			"deselect", &Item::deselect,

			// Type checks (read-only)
			"isStackable", &Item::isStackable,
			"isMoveable", &Item::isMoveable,
			"isPickupable", &Item::isPickupable,
			"isBlocking", &Item::isBlocking,
			"isGroundTile", &Item::isGroundTile,
			"isBorder", &Item::isBorder,
			"isWall", &Item::isWall,
			"isDoor", &Item::isDoor,
			"isTable", &Item::isTable,
			"isCarpet", &Item::isCarpet,
			"isHangable", &Item::isHangable,
			"isRoteable", &Item::isRoteable,
			"isFluidContainer", &Item::isFluidContainer,
			"isSplash", &Item::isSplash,
			"hasCharges", &Item::hasCharges,
			"isContainer", [](const Item& item) { return item.asContainer() != nullptr; },
			"hasElevation", sol::property([](const Item& item) {
				return g_items[item.getID()].hasElevation;
			}),
			"zOrder", sol::property(&Item::getTopOrder),

			// Methods
			"setAttribute", [](Item& item, const std::string& key, sol::object value) {
				if (value.is<std::string>()) {
					item.setAttribute(key, value.as<std::string>());
				} else if (value.is<int>()) {
					item.setAttribute(key, value.as<int>());
				} else if (value.is<double>()) {
					item.setAttribute(key, value.as<double>());
				} else if (value.is<bool>()) {
					item.setAttribute(key, value.as<bool>());
				}
			},
			"getAttribute", [](const Item& item, const std::string& key, sol::this_state ts) -> sol::object {
				sol::state_view lua(ts);
				if (const std::string* s = item.getStringAttribute(key)) return sol::make_object(lua, *s);
				if (const int32_t* i = item.getIntegerAttribute(key)) return sol::make_object(lua, *i);
				if (const double* d = item.getFloatAttribute(key)) return sol::make_object(lua, *d);
				if (const bool* b = item.getBooleanAttribute(key)) return sol::make_object(lua, *b);
				return sol::make_object(lua, sol::nil);
			},
			"clone", [](const Item& item) -> std::unique_ptr<Item> { return item.deepCopy(); },
			"rotate", &Item::doRotate,

			// String representation
			sol::meta_function::to_string, [](const Item& item) { return "Item(id=" + std::to_string(item.getID()) + ", name=\"" + std::string(item.getName()) + "\")"; }
		);

		// Register Items namespace for item lookup
		sol::table items = lua.create_named_table("Items");

		// Get item info by ID - returns a table with item properties
		items["getInfo"] = [](sol::this_state ts, int id) -> sol::object {
			sol::state_view lua(ts);
			if (!g_items.typeExists(id)) {
				return sol::nil;
			}
			ItemType& it = g_items.getItemType(id);
			sol::table info = lua.create_table();
			info["id"] = it.id;
			info["clientId"] = it.clientID;
			info["name"] = it.name;
			info["description"] = it.description;
			info["isStackable"] = it.stackable;
			info["isMoveable"] = it.moveable;
			info["isPickupable"] = it.pickupable;
			info["isGroundTile"] = it.isGroundTile();
			info["isBorder"] = it.isBorder;
			info["isWall"] = it.isWall;
			info["isDoor"] = it.isDoor();
			info["isTable"] = it.isTable;
			info["isCarpet"] = it.isCarpet;
			info["hasElevation"] = it.hasElevation;
			return info;
		};

		// Check if item ID exists
		items["exists"] = [](int id) -> bool {
			return g_items.typeExists(id);
		};

		// Get max item ID
		items["getMaxId"] = []() -> int {
			return g_items.getMaxID();
		};

		// Find items by name (returns array of {id, name} tables)
		// Searches for items whose name contains the search string (case-insensitive)
		items["findByName"] = [](sol::this_state ts, const std::string& searchName, sol::optional<int> maxResults) -> sol::table {
			sol::state_view lua(ts);
			sol::table results = lua.create_table();

			int limit = maxResults.value_or(50); // Default max 50 results
			int count = 0;
			int maxId = g_items.getMaxID();

			for (int id = 1; id <= maxId && count < limit; ++id) {
				if (!g_items.typeExists(id)) {
					continue;
				}

				ItemType& it = g_items.getItemType(id);
				if (it.name.empty()) {
					continue;
				}

				// Check if name contains search string
				if (containsIgnoreCase(it.name, searchName)) {
					sol::table item = lua.create_table();
					item["id"] = it.id;
					item["name"] = it.name;
					results[++count] = item;
				}
			}

			return results;
		};

		// Find first item matching name exactly (case-insensitive)
		// Returns item ID or nil
		items["findIdByName"] = [](const std::string& searchName) -> sol::optional<int> {
			std::string searchLower = toLower(searchName);
			int maxId = g_items.getMaxID();

			// First pass: exact match
			for (int id = 1; id <= maxId; ++id) {
				if (!g_items.typeExists(id)) {
					continue;
				}

				ItemType& it = g_items.getItemType(id);
				if (it.name.empty()) {
					continue;
				}

				if (toLower(it.name) == searchLower) {
					return id;
				}
			}

			// Second pass: contains match (return first)
			for (int id = 1; id <= maxId; ++id) {
				if (!g_items.typeExists(id)) {
					continue;
				}

				ItemType& it = g_items.getItemType(id);
				if (it.name.empty()) {
					continue;
				}

				if (containsIgnoreCase(it.name, searchName)) {
					return id;
				}
			}

			return sol::nullopt;
		};

		// Get item name by ID
		items["getName"] = [](int id) -> std::string {
			if (g_items.typeExists(id)) {
				return g_items.getItemType(id).name;
			}
			return "";
		};
	}

} // namespace LuaAPI
