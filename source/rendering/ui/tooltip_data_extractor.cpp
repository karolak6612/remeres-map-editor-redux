#include "rendering/ui/tooltip_data_extractor.h"

#include <algorithm>

#include "game/complexitem.h"
#include "game/item.h"
#include "game/items.h"
#include "rendering/ui/tooltip_collector.h"

namespace rme {

constexpr uint16_t MIN_TOOLTIP_ITEM_ID = 100;
constexpr float MAX_ZOOM_FOR_CONTAINER_TOOLTIP = 1.5f;
constexpr size_t MAX_CONTAINER_ITEMS_IN_TOOLTIP = 32;

static void ExtractComplexItemData(TooltipData& data, const Item* item) {
  if (item->isComplex()) {
    data.uniqueId = item->getUniqueID();
    data.actionId = item->getActionID();
    data.text = item->getText();
    data.description = item->getDescription();
  }
}

static void ExtractDoorData(TooltipData& data, const Item* item, bool isHouseTile) {
  if (isHouseTile && item->isDoor()) {
    if (const Door* door = item->asDoor()) {
      if (door->isRealDoor()) {
        data.doorId = door->getDoorID();
      }
    }
  }
}

static void ExtractTeleportData(TooltipData& data, const Item* item) {
  if (item->isTeleport()) {
    if (const Teleport* tp = item->asTeleport()) {
      if (tp->hasDestination()) {
        data.destination = tp->getDestination();
        data.has_destination = true;
      }
    }
  }
}

static void ExtractContainerData(TooltipData& data, const Item* item, float zoom) {
  if (zoom <= MAX_ZOOM_FOR_CONTAINER_TOOLTIP) {
    if (const Container* container = item->asContainer()) {
      data.containerCapacity = static_cast<uint8_t>(container->getVolume());

      const auto& items = container->getVector();
      data.containerItems.clear();
      data.containerItems.reserve(std::min(items.size(), MAX_CONTAINER_ITEMS_IN_TOOLTIP));
      
      for (const auto& subItem : items) {
        if (subItem) {
          ContainerItem ci;
          ci.id = subItem->getID();
          ci.subtype = subItem->getSubtype();
          ci.count = subItem->getCount();
          if (ci.count == 0) {
            ci.count = 1;
          }

          data.containerItems.push_back(ci);

          if (data.containerItems.size() >= MAX_CONTAINER_ITEMS_IN_TOOLTIP) {
            break;
          }
        }
      }
    }
  }
}

bool FillItemTooltipData(TooltipData& data, Item* item, const ItemType& it, const Position& pos, bool isHouseTile, float zoom) {
  if (!item) {
    return false;
  }

  const uint16_t id = item->getID();
  if (id < MIN_TOOLTIP_ITEM_ID) {
    return false;
  }

  // Initialize data defaults for the checks
  data.uniqueId = 0;
  data.actionId = 0;
  data.doorId = 0;
  data.text = std::string_view();
  data.description = std::string_view();
  data.has_destination = false;

  bool is_complex = item->isComplex();
  if (!is_complex && !it.isTooltipable()) {
    return false;
  }

  ExtractComplexItemData(data, item);
  ExtractDoorData(data, item, isHouseTile);
  ExtractTeleportData(data, item);

  bool is_container = it.isContainer();

  // Only create tooltip if there's something to show (or if it's a container)
  if (data.uniqueId == 0 && data.actionId == 0 && data.doorId == 0 && 
      data.text.empty() && data.description.empty() && !data.has_destination && !is_container) {
    return false;
  }

  std::string_view itemName = it.name;
  if (itemName.empty()) {
    itemName = "Item";
  }

  data.pos = pos;
  data.itemId = id;
  data.itemName = itemName;

  if (is_container) {
    ExtractContainerData(data, item, zoom);
  }

  data.updateCategory();

  return true;
}

} // namespace rme
