#include "item_definitions/formats/dat/dat_item_parser.h"

#include "rendering/core/game_sprite.h"
#include "rendering/core/graphics.h"

#include <format>

bool DatItemParser::parse(const ItemDefinitionLoadInput& input, ItemDefinitionFragments& fragments, wxString& error, std::vector<std::string>& warnings) const {
	if (input.graphics == nullptr) {
		error = "DAT item parser requires loaded graphics metadata.";
		return false;
	}

	const uint16_t item_count = input.graphics->getItemSpriteMaxID();
	if (item_count < 100) {
		error = "Graphics metadata does not contain any item sprites.";
		return false;
	}

	for (uint16_t id = 100; id <= item_count; ++id) {
		Sprite* sprite = input.graphics->getSprite(id);
		if (sprite == nullptr) {
			warnings.push_back(std::format("Missing loaded DAT metadata for client item id {}.", id));
			continue;
		}

		GameSprite* game_sprite = dynamic_cast<GameSprite*>(sprite);
		if (game_sprite == nullptr) {
			warnings.push_back(std::format("Loaded DAT metadata for client item id {} is not a GameSprite.", id));
			continue;
		}

		DatItemFragment fragment;
		fragment.client_id = static_cast<ClientItemId>(id);
		fragment.flags = game_sprite->item_definition_flags;
		fragments.dat[fragment.client_id] = fragment;
	}

	if (fragments.dat.empty()) {
		error = "No item definitions were derived from loaded DAT metadata.";
		return false;
	}

	return true;
}
