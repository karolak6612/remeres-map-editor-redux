#ifndef RME_RENDERING_UI_TOOLTIP_DATA_EXTRACTOR_H_
#define RME_RENDERING_UI_TOOLTIP_DATA_EXTRACTOR_H_

#include "rendering/ui/tooltip_drawer.h"

class Item;
class ItemType;
class Position;

namespace rme {

bool FillItemTooltipData(TooltipData& data, Item* item, const ItemType& it, const Position& pos, bool isHouseTile, float zoom);

} // namespace rme

#endif
