//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "brushes/flag/flag_brush.h"

#include "map/map.h"
#include "map/tile.h"
#include "game/sprites.h"

#include <array>
#include <string_view>
#include <algorithm>

FlagBrush::FlagBrush(uint32_t flag) :
	flag(flag) {
	//
}

std::string FlagBrush::getName() const {
	static constexpr std::array<std::pair<uint32_t, std::string_view>, 4> flagNames = { { { static_cast<uint16_t>(TileState::PROTECTIONZONE), "PZ brush (0x01)" },
																						  { static_cast<uint16_t>(TileState::NOPVP), "No combat zone brush (0x04)" },
																						  { static_cast<uint16_t>(TileState::NOLOGOUT), "No logout zone brush (0x08)" },
																						  { static_cast<uint16_t>(TileState::PVPZONE), "PVP Zone brush (0x10)" } } };

	auto it = std::ranges::find_if(flagNames, [this](const auto& p) { return p.first == flag; });
	if (it != flagNames.end()) {
		return std::string(it->second);
	}
	return "Unknown flag brush";
}

int FlagBrush::getLookID() const {
	static constexpr std::array<std::pair<uint32_t, int>, 4> flagSprites = { { { static_cast<uint16_t>(TileState::PROTECTIONZONE), EDITOR_SPRITE_PZ_TOOL },
																			   { static_cast<uint16_t>(TileState::NOPVP), EDITOR_SPRITE_NOPVP_TOOL },
																			   { static_cast<uint16_t>(TileState::NOLOGOUT), EDITOR_SPRITE_NOLOG_TOOL },
																			   { static_cast<uint16_t>(TileState::PVPZONE), EDITOR_SPRITE_PVPZ_TOOL } } };

	auto it = std::ranges::find_if(flagSprites, [this](const auto& p) { return p.first == flag; });
	if (it != flagSprites.end()) {
		return it->second;
	}
	return 0;
}

bool FlagBrush::canDraw(BaseMap* map, const Position& position) const {
	if (Tile* tile = map->getTile(position)) {
		return tile->hasGround();
	}
	return false;
}

void FlagBrush::undraw(BaseMap* /*map*/, Tile* tile) {
	tile->unsetMapFlags(static_cast<uint16_t>(flag));
}

void FlagBrush::draw(BaseMap* /*map*/, Tile* tile, void* /*parameter*/) {
	if (tile->hasGround()) {
		tile->setMapFlags(static_cast<uint16_t>(flag));
	}
}
