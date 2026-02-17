#ifndef RME_MAP_SEARCH_H_
#define RME_MAP_SEARCH_H_

#include "app/main.h"
#include <vector>
#include <utility>
#include <string>

class Map;
class Tile;
class Item;

/**
 * @file map_search.h
 * @brief Utilities for searching the map for items or tiles.
 *
 * Provides functionality to scan the entire map or a selection
 * for tiles matching specific criteria.
 */

/**
 * @brief Structure holding the result of a map search.
 */
struct SearchResult {
	Tile* tile; ///< The tile where the match was found.
	Item* item; ///< The specific item that matched (if any).
	std::string description; ///< Text description of the match.
};

/**
 * @brief Static utility class for performing map searches.
 */
class MapSearchUtility {
public:
	/**
	 * @brief Searches the map for items matching the specified criteria.
	 *
	 * Scans all tiles (or selected ones) for items that match the flags provided.
	 *
	 * @param map The map to search.
	 * @param unique Find items with unique IDs.
	 * @param action Find items with action IDs.
	 * @param container Find container items.
	 * @param writable Find writable items (signs, books).
	 * @param onSelection If true, restricts search to current selection.
	 * @return A vector of SearchResult objects.
	 */
	static std::vector<SearchResult> SearchItems(Map& map, bool unique, bool action, bool container, bool writable, bool onSelection);
};

#endif
