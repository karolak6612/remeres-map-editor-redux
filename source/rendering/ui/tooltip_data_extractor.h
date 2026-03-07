#ifndef RME_RENDERING_UI_TOOLTIP_DATA_EXTRACTOR_H_
#define RME_RENDERING_UI_TOOLTIP_DATA_EXTRACTOR_H_

struct TooltipData;

class Item;
class ItemType;
class Position;

namespace rme {

/**
 * @brief Populates TooltipData structure with information from a specific map item.
 *
 * @param data Output structure to be filled with item data.
 * @param item Pointer to the item instance on the map.
 * @param it Item type definition from the database.
 * @param pos Position of the item on the map.
 * @param isHouseTile Whether the item is located on a house tile.
 * @param zoom Current rendering zoom level (used for filtering container previews).
 * @return true if the item has relevant tooltip data to display, false otherwise.
 */
bool FillItemTooltipData(TooltipData& data, Item* item, const ItemType& it, const Position& pos, bool isHouseTile, float zoom);

} // namespace rme

#endif
