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

#include <algorithm>
#include "brushes/ground/ground_brush.h"
#include "brushes/ground/auto_border.h"
#include "brushes/ground/ground_brush_loader.h"
#include "brushes/ground/ground_border_calculator.h"
#include "game/items.h"
#include "map/basemap.h"

uint32_t GroundBrush::border_types[256];

GroundBrush::GroundBrush() :
	z_order(0),
	has_zilch_outer_border(false),
	has_zilch_inner_border(false),
	has_outer_border(false),
	has_inner_border(false),
	optional_border(nullptr),
	use_only_optional(false),
	randomize(true),
	total_chance(0) {
	////
}

GroundBrush::~GroundBrush() {
}

bool GroundBrush::load(pugi::xml_node node, std::vector<std::string>& warnings) {
	return GroundBrushLoader::load(*this, node, warnings);
}

void GroundBrush::undraw(BaseMap* map, Tile* tile) {
	ASSERT(tile);
	if (tile->hasGround() && tile->ground->getGroundBrush() == this) {
		tile->ground = nullptr;
	}
}

void GroundBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	ASSERT(tile);
	if (border_items.empty()) {
		return;
	}

	if (parameter != nullptr) {
		std::pair<bool, GroundBrush*>& param = *reinterpret_cast<std::pair<bool, GroundBrush*>*>(parameter);
		GroundBrush* other = tile->getGroundBrush();
		if (param.first) { // Volatile? :)
			if (other != nullptr) {
				return;
			}
		} else if (other != param.second) {
			return;
		}
	}
	int chance = random(1, total_chance);
	uint16_t id = 0;
	for (const auto& item_block : border_items) {
		if (chance < item_block.chance) {
			id = item_block.id;
			break;
		}
	}
	if (id == 0) {
		id = border_items.front().id;
	}

	tile->addItem(Item::Create(id));
}

const GroundBrush::BorderBlock* GroundBrush::getBrushTo(GroundBrush* first, GroundBrush* second) {
	if (first) {
		if (second) {
			if (first->getZ() < second->getZ() && second->hasOuterBorder()) {
				if (first->hasInnerBorder()) {
					auto it = std::ranges::find_if(first->borders, [&](const auto& bb) {
						return !bb->outer && (bb->to == second->getID() || bb->to == 0xFFFFFFFF);
					});
					if (it != first->borders.end()) {
						return it->get();
					}
				}
				auto it = std::ranges::find_if(second->borders, [&](const auto& bb) {
					return bb->outer && (bb->to == first->getID() || bb->to == 0xFFFFFFFF);
				});
				if (it != second->borders.end()) {
					return it->get();
				}
			} else if (first->hasInnerBorder()) {
				auto it = std::ranges::find_if(first->borders, [&](const auto& bb) {
					return !bb->outer && (bb->to == second->getID() || bb->to == 0xFFFFFFFF);
				});
				if (it != first->borders.end()) {
					return it->get();
				}
			}
		} else if (first->hasInnerZilchBorder()) {
			auto it = std::ranges::find_if(first->borders, [](const auto& bb) {
				return !bb->outer && bb->to == 0;
			});
			if (it != first->borders.end()) {
				return it->get();
			}
		}
	} else if (second && second->hasOuterZilchBorder()) {
		auto it = std::ranges::find_if(second->borders, [](const auto& bb) {
			return bb->outer && bb->to == 0;
		});
		if (it != second->borders.end()) {
			return it->get();
		}
	}
	return nullptr;
}

inline GroundBrush* extractGroundBrushFromTile(BaseMap* map, uint32_t x, uint32_t y, uint32_t z) {
	Tile* t = map->getTile(x, y, z);
	return t ? t->getGroundBrush() : nullptr;
}

void GroundBrush::doBorders(BaseMap* map, Tile* tile) {
	GroundBorderCalculator::calculate(map, tile);
}
void GroundBrush::getRelatedItems(std::vector<uint16_t>& items) {
	std::ranges::for_each(border_items, [&](const auto& item_block) {
		if (item_block.id != 0) {
			items.push_back(item_block.id);
		}
	});

	std::ranges::for_each(borders, [&](const auto& bb) {
		if (bb->autoborder) {
			std::ranges::for_each(bb->autoborder->tiles, [&](uint32_t tile_id) {
				if (tile_id != 0) {
					items.push_back(static_cast<uint16_t>(tile_id));
				}
			});
		}
		std::ranges::for_each(bb->specific_cases, [&](const auto& sc) {
			if (sc->to_replace_id != 0) {
				items.push_back(sc->to_replace_id);
			}
			if (sc->with_id != 0) {
				items.push_back(sc->with_id);
			}
		});
	});
}
