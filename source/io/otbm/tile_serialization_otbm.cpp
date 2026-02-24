#include "tile_serialization_otbm.h"

#include "map/map.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/house.h"
#include "io/otbm/item_serialization_otbm.h"
#include "ui/gui.h"
#include <spdlog/spdlog.h>
#include <ranges>
#include <functional>

void TileSerializationOTBM::readTileArea(IOMapOTBM& iomap, Map& map, BinaryNode* mapNode) {
	uint16_t base_x, base_y;
	uint8_t base_z;
	if (!mapNode->getU16(base_x) || !mapNode->getU16(base_y) || !mapNode->getU8(base_z)) {
		spdlog::warn("Invalid tile area node: missing base coordinates");
		return;
	}
	spdlog::debug("Reading OTBM_TILE_AREA at base_x={}, base_y={}, base_z={}", base_x, base_y, base_z);

	for (BinaryNode* tileNode : mapNode->children()) {
		uint8_t tile_type;
		if (!tileNode->getByte(tile_type)) {
			spdlog::warn("Invalid tile node: failed to read type byte");
			continue;
		}

		if (tile_type != OTBM_TILE && tile_type != OTBM_HOUSETILE) {
			spdlog::warn("Unknown tile type: {}", static_cast<int>(tile_type));
			continue;
		}

		uint8_t x_offset, y_offset;
		if (!tileNode->getU8(x_offset) || !tileNode->getU8(y_offset)) {
			spdlog::warn("Failed to read tile offset in area {},{},{}", base_x, base_y, base_z);
			continue;
		}

		const Position pos(base_x + x_offset, base_y + y_offset, base_z);

		if (map.getTile(pos)) {
			spdlog::warn("Duplicate tile at {},{},{}, discarding duplicate", pos.x, pos.y, pos.z);
			continue;
		}

		Tile* tile = map.createTile(pos.x, pos.y, pos.z);
		House* house = nullptr;

		if (tile_type == OTBM_HOUSETILE) {
			uint32_t house_id;
			if (!tileNode->getU32(house_id)) {
				spdlog::warn("Failed to read house ID for tile {},{},{}", pos.x, pos.y, pos.z);
			} else if (house_id == 0) {
				spdlog::warn("House ID is zero for tile {},{},{}", pos.x, pos.y, pos.z);
			} else {
				house = map.houses.getHouse(house_id);
				if (!house) {
					auto new_house = std::make_unique<House>(map);
					house = new_house.get();
					new_house->setID(house_id);
					map.houses.addHouse(std::move(new_house));
				}
			}
		}

		uint8_t attribute;
		bool stop_attributes = false;
		while (!stop_attributes && tileNode->getU8(attribute)) {
			switch (attribute) {
				case OTBM_ATTR_TILE_FLAGS: {
					uint32_t flags = 0;
					if (tileNode->getU32(flags)) {
						tile->setMapFlags(flags);
					} else {
						spdlog::warn("Invalid tile flags at {},{},{}", pos.x, pos.y, pos.z);
					}
					break;
				}
				case OTBM_ATTR_ITEM: {
					auto item = ItemSerializationOTBM::createFromStream(iomap, tileNode);
					if (item) {
						tile->addItem(std::move(item));
					} else {
						spdlog::warn("Failed to read item attribute at {},{},{}", pos.x, pos.y, pos.z);
					}
					break;
				}
				default:
					spdlog::warn("Unknown tile attribute {} at {},{},{}", static_cast<int>(attribute), pos.x, pos.y, pos.z);
					stop_attributes = true;
					break;
			}
		}

		for (BinaryNode* itemNode : tileNode->children()) {
			uint8_t item_type;
			if (!itemNode->getByte(item_type)) {
				spdlog::warn("Failed to read item type at {},{},{}", pos.x, pos.y, pos.z);
				continue;
			}

			if (item_type == OTBM_ITEM) {
				auto item = ItemSerializationOTBM::createFromStream(iomap, itemNode);
				if (item) {
					if (!ItemSerializationOTBM::unserializeItemNode(iomap, itemNode, *item)) {
						spdlog::warn("Failed to unserialize item at {},{},{}", pos.x, pos.y, pos.z);
					} else {
						tile->addItem(std::move(item));
					}
				}
			} else {
				spdlog::warn("Unknown tile child node type {} at {},{},{}", static_cast<int>(item_type), pos.x, pos.y, pos.z);
			}
		}

		tile->update();
		if (house) {
			house->addTile(tile);
		}
	}
}

void TileSerializationOTBM::writeTileData(const IOMapOTBM& iomap, const Map& map, NodeFileWriteHandle& f, const std::function<void(int)>& progressCb) {
	uint32_t tiles_saved = 0;
	bool first = true;
	int local_x = -1, local_y = -1, local_z = -1;

	const uint64_t total_tiles = map.getTileCount();

	// Use modern ranges for iteration if possible, but the spatial hash grid iterator is already custom
	auto sorted_cells = map.getGrid().getSortedCells();
	for (const auto& sorted_cell : sorted_cells) {
		SpatialHashGrid::GridCell* cell = sorted_cell.cell;
		if (!cell) {
			continue;
		}

		for (int i = 0; i < SpatialHashGrid::NODES_IN_CELL; ++i) {
			MapNode* node = cell->nodes[i].get();
			if (!node) {
				continue;
			}

			for (int j = 0; j < MAP_LAYERS; ++j) {
				Floor* floor = node->getFloor(j);
				if (!floor) {
					continue;
				}

				for (int k = 0; k < SpatialHashGrid::TILES_PER_NODE; ++k) {
					Tile* save_tile = floor->locs[k].get();
					if (!save_tile || save_tile->size() == 0) {
						continue;
					}

					++tiles_saved;
					if (tiles_saved % 8192 == 0 && total_tiles > 0 && progressCb) {
						progressCb(std::min(100, static_cast<int>(100.0 * tiles_saved / total_tiles)));
					}

					const Position& pos = save_tile->getPosition();

					// Decide if new node should be created
					if (first || pos.x < local_x || pos.x >= local_x + 256 || pos.y < local_y || pos.y >= local_y + 256 || pos.z != local_z) {
						if (!first) {
							f.endNode();
						}
						first = false;

						f.addNode(OTBM_TILE_AREA);
						f.addU16(local_x = pos.x & 0xFF00);
						f.addU16(local_y = pos.y & 0xFF00);
						f.addU8(local_z = pos.z);
					}

					serializeTile(iomap, save_tile, f);
				}
			}
		}
	}

	if (!first) {
		f.endNode();
	}
}

void TileSerializationOTBM::serializeTile(const IOMapOTBM& iomap, const Tile* save_tile, NodeFileWriteHandle& f) {
	f.addNode(save_tile->isHouseTile() ? OTBM_HOUSETILE : OTBM_TILE);

	f.addU8(save_tile->getX() & 0xFF);
	f.addU8(save_tile->getY() & 0xFF);

	if (save_tile->isHouseTile()) {
		f.addU32(save_tile->getHouseID());
	}

	if (save_tile->getMapFlags()) {
		f.addU8(OTBM_ATTR_TILE_FLAGS);
		f.addU32(save_tile->getMapFlags());
	}

	if (save_tile->ground) {
		Item* ground = save_tile->ground.get();
		if (!ground->isMetaItem()) {
			if (ground->hasBorderEquivalent()) {
				bool found = std::ranges::any_of(save_tile->items, [&](const auto& item) {
					return item->getGroundEquivalent() == ground->getID();
				});

				if (!found) {
					ItemSerializationOTBM::serializeItemNode(iomap, f, *ground);
				}
			} else if (ground->isComplex()) {
				ItemSerializationOTBM::serializeItemNode(iomap, f, *ground);
			} else {
				f.addU8(OTBM_ATTR_ITEM);
				ItemSerializationOTBM::serializeItemCompact(iomap, f, *ground);
			}
		}
	}

	for (const auto& item : save_tile->items) {
		if (!item->isMetaItem()) {
			ItemSerializationOTBM::serializeItemNode(iomap, f, *item);
		}
	}

	f.endNode();
}
