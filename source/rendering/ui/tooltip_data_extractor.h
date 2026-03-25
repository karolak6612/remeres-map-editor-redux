#ifndef RME_RENDERING_TOOLTIP_DATA_EXTRACTOR_H_
#define RME_RENDERING_TOOLTIP_DATA_EXTRACTOR_H_

struct TooltipData;
class Item;
class ItemDefinitionView;
struct Position;

namespace TooltipDataExtractor {

// Populates tooltip data from an item. Returns true if the item has tooltip-worthy data.
bool Fill(TooltipData& data, Item* item, const ItemDefinitionView& it, const Position& pos, bool isHouseTile, float zoom);

} // namespace TooltipDataExtractor

#endif
