#ifndef RME_RENDERING_SYSTEMS_TOOLTIP_SYSTEM_H_
#define RME_RENDERING_SYSTEMS_TOOLTIP_SYSTEM_H_

#include "map/position.h"
#include <string_view>

class Item;
class ItemType;
struct TooltipData;

class TooltipSystem {
public:
	static bool FillItemTooltipData(TooltipData& data, Item* item, const ItemType& it, const Position& pos, bool isHouseTile, float zoom);
};

#endif
