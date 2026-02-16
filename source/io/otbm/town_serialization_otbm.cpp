#include "town_serialization_otbm.h"

#include "map/map.h"
#include "game/town.h"
#include <spdlog/spdlog.h>
#include <format>

void TownSerializationOTBM::readTowns(Map& map, BinaryNode* mapNode) {
	spdlog::debug("Reading OTBM_TOWNS...");
	for (BinaryNode* townNode = mapNode->getChild(); townNode != nullptr; townNode = townNode->advance()) {
		uint8_t town_type;
		if (!townNode->getByte(town_type)) {
			spdlog::warn("Invalid town node: failed to read type byte");
			continue;
		}
		if (town_type != OTBM_TOWN) {
			spdlog::warn("Invalid town node type: {} (expected {})", static_cast<int>(town_type), static_cast<int>(OTBM_TOWN));
			continue;
		}
		uint32_t town_id;
		if (!townNode->getU32(town_id)) {
			spdlog::warn("Failed to read town ID");
			continue;
		}

		if (const auto* existing_town = map.towns.getTown(town_id)) {
			spdlog::warn("Duplicate town ID {}, discarding duplicate", town_id);
			continue;
		}

		auto new_town = std::make_unique<Town>(town_id);
		Town* town = new_town.get();
		if (!map.towns.addTown(std::move(new_town))) {
			spdlog::error("Failed to add town {} to map", town_id);
			continue;
		}

		std::string town_name;
		if (!townNode->getString(town_name) || town_name.empty()) {
			spdlog::warn("Failed to read valid town name for ID {}", town_id);
			continue;
		}
		town->setName(town_name);

		Position pos;
		uint16_t x, y;
		uint8_t z;
		if (!townNode->getU16(x) || !townNode->getU16(y) || !townNode->getU8(z)) {
			spdlog::warn("Invalid temple position for town '{}' (ID {})", town_name, town_id);
			continue;
		}
		pos = { x, y, z };
		if (pos.x == 0 || pos.y == 0) {
			spdlog::warn("Invalid temple position {}:{}:{} for town '{}' (ID {})", pos.x, pos.y, pos.z, town_name, town_id);
			continue;
		}
		town->setTemplePosition(pos);

		if (auto* tile = map.getOrCreateTile(pos)) {
			if (auto* location = tile->getLocation()) {
				location->increaseTownCount();
			}
		}
	}
}

void TownSerializationOTBM::writeTowns(const Map& map, NodeFileWriteHandle& f) {
	f.addNode(OTBM_TOWNS);
	for (const auto& [town_id, town_ptr] : map.towns) {
		const Town* town = town_ptr.get();
		const Position& pos = town->getTemplePosition();

		f.addNode(OTBM_TOWN);
		f.addU32(town->getID());
		f.addString(town->getName());
		f.addU16(pos.x);
		f.addU16(pos.y);
		f.addU8(pos.z);
		f.endNode();
	}
	f.endNode();
}
