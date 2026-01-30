//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "brushes/ground/ground_border_calculator.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/ground/auto_border.h"
#include "map/basemap.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/items.h"
#include <array>
#include <algorithm>
#include <vector>

namespace {
GroundBrush* extractGroundBrushFromTile(BaseMap* map, uint32_t x, uint32_t y, uint32_t z) {
	Tile* tile = map->getTile(x, y, z);
	if (tile) {
		return tile->getGroundBrush();
	}
	return nullptr;
}
}

void GroundBorderCalculator::gatherNeighbors(CalculationContext& ctx) {
	const Position& position = ctx.tile->getPosition();
	uint32_t x = position.x;
	uint32_t y = position.y;
	uint32_t z = position.z;

	static constexpr std::array<std::pair<int32_t, int32_t>, 8> offsets = { { { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } } };

	for (size_t i = 0; i < offsets.size(); ++i) {
		const auto& [dx, dy] = offsets[i];
		if ((x == 0 && dx < 0) || (y == 0 && dy < 0)) {
			continue;
		}
		ctx.neighbours[i] = { false, extractGroundBrushFromTile(ctx.map, x + dx, y + dy, z) };
	}
}

void GroundBorderCalculator::processNeighbors(CalculationContext& ctx) {
	for (int32_t i = 0; i < 8; ++i) {
		auto& [visited, other] = ctx.neighbours[i];
		if (visited) {
			continue;
		}

		if (ctx.borderBrush) {
			if (other) {
				if (other->getID() == ctx.borderBrush->getID()) {
					continue;
				}

				if (other->hasOuterBorder() || ctx.borderBrush->hasInnerBorder()) {
					bool only_mountain = false;
					if ((other->friendOf(ctx.borderBrush) || ctx.borderBrush->friendOf(other))) {
						if (!other->hasOptionalBorder()) {
							continue;
						}
						only_mountain = true;
					}

					uint32_t tiledata = 0;
					for (int32_t j = i; j < 8; ++j) {
						auto& [other_visited, other_brush] = ctx.neighbours[j];
						if (!other_visited && other_brush && other_brush->getID() == other->getID()) {
							other_visited = true;
							tiledata |= 1 << j;
						}
					}

					if (tiledata != 0) {
						if (other->hasOptionalBorder() && ctx.tile->hasOptionalBorder()) {
							GroundBrush::BorderCluster borderCluster;
							borderCluster.alignment = tiledata;
							borderCluster.z = 0x7FFFFFFF;
							borderCluster.border = other->optional_border;

							ctx.borderList.push_back(borderCluster);
							if (other->useSoloOptionalBorder()) {
								only_mountain = true;
							}
						}

						if (!only_mountain) {
							const GroundBrush::BorderBlock* borderBlock = GroundBrush::getBrushTo(ctx.borderBrush, other);
							if (borderBlock) {
								bool found = false;
								for (GroundBrush::BorderCluster& borderCluster : ctx.borderList) {
									if (borderCluster.border == borderBlock->autoborder) {
										borderCluster.alignment |= tiledata;
										if (borderCluster.z < other->getZ()) {
											borderCluster.z = other->getZ();
										}

										if (!borderBlock->specific_cases.empty()) {
											ctx.specificList.push_back(borderBlock);
										}

										found = true;
										break;
									}
								}

								if (!found) {
									GroundBrush::BorderCluster borderCluster;
									borderCluster.alignment = tiledata;
									borderCluster.z = other->getZ();
									borderCluster.border = borderBlock->autoborder;

									ctx.borderList.push_back(borderCluster);
									if (!borderBlock->specific_cases.empty()) {
										ctx.specificList.push_back(borderBlock);
									}
								}
							}
						}
					}
				}

				// Border against nothing
				uint32_t tiledata = 0;
				for (int32_t j = i; j < 8; ++j) {
					auto& [other_visited, other_brush] = ctx.neighbours[j];
					if (!other_visited && !other_brush) {
						other_visited = true;
						tiledata |= 1 << j;
					}
				}

				if (tiledata != 0) {
					const GroundBrush::BorderBlock* borderBlock = GroundBrush::getBrushTo(ctx.borderBrush, nullptr);
					if (borderBlock) {
						if (borderBlock->autoborder) {
							bool found = false;
							for (GroundBrush::BorderCluster& borderCluster : ctx.borderList) {
								if (borderCluster.border == borderBlock->autoborder) {
									borderCluster.alignment |= tiledata;
									borderCluster.z = -1000;
									found = true;
									break;
								}
							}

							if (!found) {
								GroundBrush::BorderCluster borderCluster;
								borderCluster.alignment = tiledata;
								borderCluster.z = -1000;
								borderCluster.border = borderBlock->autoborder;
								ctx.borderList.push_back(borderCluster);
							}
						}

						if (!borderBlock->specific_cases.empty()) {
							ctx.specificList.push_back(borderBlock);
						}
					}
				}
				continue;
			} else {
				uint32_t tiledata = 0;
				for (int32_t j = i; j < 8; ++j) {
					auto& [other_visited, other_brush] = ctx.neighbours[j];
					if (!other_visited && !other_brush) {
						other_visited = true;
						tiledata |= 1 << j;
					}
				}

				if (tiledata != 0) {
					const GroundBrush::BorderBlock* borderBlock = GroundBrush::getBrushTo(ctx.borderBrush, nullptr);
					if (borderBlock) {
						if (borderBlock->autoborder) {
							bool found = false;
							for (GroundBrush::BorderCluster& borderCluster : ctx.borderList) {
								if (borderCluster.border == borderBlock->autoborder) {
									borderCluster.alignment |= tiledata;
									borderCluster.z = -1000;
									found = true;
									break;
								}
							}

							if (!found) {
								GroundBrush::BorderCluster borderCluster;
								borderCluster.alignment = tiledata;
								borderCluster.z = -1000;
								borderCluster.border = borderBlock->autoborder;
								ctx.borderList.push_back(borderCluster);
							}
						}

						if (!borderBlock->specific_cases.empty()) {
							ctx.specificList.push_back(borderBlock);
						}
					}
				}
				continue;
			}
		} else if (other && other->hasOuterZilchBorder()) {
			uint32_t tiledata = 0;
			for (int32_t j = i; j < 8; ++j) {
				auto& [other_visited, other_brush] = ctx.neighbours[j];
				if (!other_visited && other_brush && other_brush->getID() == other->getID()) {
					other_visited = true;
					tiledata |= 1 << j;
				}
			}

			if (tiledata != 0) {
				const GroundBrush::BorderBlock* borderBlock = GroundBrush::getBrushTo(nullptr, other);
				if (borderBlock) {
					if (borderBlock->autoborder) {
						bool found = false;
						for (GroundBrush::BorderCluster& borderCluster : ctx.borderList) {
							if (borderCluster.border == borderBlock->autoborder) {
								borderCluster.alignment |= tiledata;
								if (borderCluster.z < other->getZ()) {
									borderCluster.z = other->getZ();
								}
								found = true;
								break;
							}
						}

						if (!found) {
							GroundBrush::BorderCluster borderCluster;
							borderCluster.alignment = tiledata;
							borderCluster.z = other->getZ();
							borderCluster.border = borderBlock->autoborder;
							ctx.borderList.push_back(borderCluster);
						}
					}

					if (!borderBlock->specific_cases.empty()) {
						ctx.specificList.push_back(borderBlock);
					}
				}

				if (other->hasOptionalBorder() && ctx.tile->hasOptionalBorder()) {
					GroundBrush::BorderCluster borderCluster;
					borderCluster.alignment = tiledata;
					borderCluster.z = 0x7FFFFFFF;
					borderCluster.border = other->optional_border;

					ctx.borderList.push_back(borderCluster);
				} else {
					ctx.tile->setOptionalBorder(false);
				}
			}
		}
		visited = true;
	}
}

void GroundBorderCalculator::applyBorders(CalculationContext& ctx) {
	// Clean current borders
	std::erase_if(ctx.tile->items, [](Item* item) {
		if (item->isBorder()) {
			delete item;
			return true;
		}
		return false;
	});

	// Sort borders
	std::ranges::sort(ctx.borderList, [](const GroundBrush::BorderCluster& a, const GroundBrush::BorderCluster& b) {
		return a.z < b.z;
	});

	ctx.tile->cleanBorders();

	while (!ctx.borderList.empty()) {
		GroundBrush::BorderCluster& borderCluster = ctx.borderList.back();
		if (!borderCluster.border) {
			ctx.borderList.pop_back();
			continue;
		}

		uint32_t border_alignment = borderCluster.alignment;

		BorderType directions[4] = {
			static_cast<BorderType>((GroundBrush::border_types[border_alignment] & 0x000000FF) >> 0),
			static_cast<BorderType>((GroundBrush::border_types[border_alignment] & 0x0000FF00) >> 8),
			static_cast<BorderType>((GroundBrush::border_types[border_alignment] & 0x00FF0000) >> 16),
			static_cast<BorderType>((GroundBrush::border_types[border_alignment] & 0xFF000000) >> 24)
		};

		for (int32_t i = 0; i < 4; ++i) {
			BorderType direction = directions[i];
			if (direction == BORDER_NONE) {
				break;
			}

			if (borderCluster.border->tiles[direction] != 0) {
				ctx.tile->addBorderItem(Item::Create(borderCluster.border->tiles[direction]));
			} else {
				if (direction == NORTHWEST_DIAGONAL) {
					if (borderCluster.border->tiles[WEST_HORIZONTAL] != 0 && borderCluster.border->tiles[NORTH_HORIZONTAL] != 0) {
						ctx.tile->addBorderItem(Item::Create(borderCluster.border->tiles[WEST_HORIZONTAL]));
						ctx.tile->addBorderItem(Item::Create(borderCluster.border->tiles[NORTH_HORIZONTAL]));
					}
				} else if (direction == NORTHEAST_DIAGONAL) {
					if (borderCluster.border->tiles[EAST_HORIZONTAL] != 0 && borderCluster.border->tiles[NORTH_HORIZONTAL] != 0) {
						ctx.tile->addBorderItem(Item::Create(borderCluster.border->tiles[EAST_HORIZONTAL]));
						ctx.tile->addBorderItem(Item::Create(borderCluster.border->tiles[NORTH_HORIZONTAL]));
					}
				} else if (direction == SOUTHWEST_DIAGONAL) {
					if (borderCluster.border->tiles[SOUTH_HORIZONTAL] != 0 && borderCluster.border->tiles[WEST_HORIZONTAL] != 0) {
						ctx.tile->addBorderItem(Item::Create(borderCluster.border->tiles[SOUTH_HORIZONTAL]));
						ctx.tile->addBorderItem(Item::Create(borderCluster.border->tiles[WEST_HORIZONTAL]));
					}
				} else if (direction == SOUTHEAST_DIAGONAL) {
					if (borderCluster.border->tiles[SOUTH_HORIZONTAL] != 0 && borderCluster.border->tiles[EAST_HORIZONTAL] != 0) {
						ctx.tile->addBorderItem(Item::Create(borderCluster.border->tiles[SOUTH_HORIZONTAL]));
						ctx.tile->addBorderItem(Item::Create(borderCluster.border->tiles[EAST_HORIZONTAL]));
					}
				}
			}
		}

		ctx.borderList.pop_back();
	}
}

void GroundBorderCalculator::applySpecificCases(CalculationContext& ctx) {
	std::ranges::sort(ctx.specificList);
	auto [first, last] = std::ranges::unique(ctx.specificList);
	ctx.specificList.erase(first, last);

	for (const GroundBrush::BorderBlock* borderBlock : ctx.specificList) {
		for (const GroundBrush::SpecificCaseBlock* specificCaseBlock : borderBlock->specific_cases) {
			uint32_t matches = 0;
			for (Item* item : ctx.tile->items) {
				if (!item->isBorder()) {
					break;
				}

				if (specificCaseBlock->match_group > 0) {
					if (item->getBorderGroup() == specificCaseBlock->match_group && item->getBorderAlignment() == specificCaseBlock->group_match_alignment) {
						++matches;
						continue;
					}
				}

				for (uint16_t matchId : specificCaseBlock->items_to_match) {
					if (item->getID() == matchId) {
						++matches;
					}
				}
			}

			if (matches >= specificCaseBlock->items_to_match.size()) {
				auto& tileItems = ctx.tile->items;
				auto it = tileItems.begin();

				// if delete_all mode, consider the border replaced
				bool replaced = specificCaseBlock->delete_all;

				while (it != tileItems.end()) {
					Item* item = *it;
					if (!item->isBorder()) {
						++it;
						continue;
					}

					bool inc = true;
					for (uint16_t matchId : specificCaseBlock->items_to_match) {
						if (item->getID() == matchId) {
							if (!replaced && item->getID() == specificCaseBlock->to_replace_id) {
								// replace the matching border, delete everything else
								item->setID(specificCaseBlock->with_id);
								replaced = true;
							} else {
								if (specificCaseBlock->delete_all || !specificCaseBlock->keepBorder) {
									delete item;
									it = tileItems.erase(it);
									inc = false;
									break;
								}
							}
						}
					}

					if (inc) {
						++it;
					}
				}
			}
		}
	}
}

void GroundBorderCalculator::calculate(BaseMap* map, Tile* tile) {
	ASSERT(tile);
	CalculationContext ctx(map, tile);

	if (tile->ground) {
		ctx.borderBrush = tile->ground->getGroundBrush();
	}

	gatherNeighbors(ctx);
	processNeighbors(ctx);
	applyBorders(ctx);
	applySpecificCases(ctx);
}
