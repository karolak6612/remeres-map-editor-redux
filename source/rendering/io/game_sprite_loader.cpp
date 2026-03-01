//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "rendering/io/game_sprite_loader.h"
#include "io/loaders/dat_loader.h"
#include "io/loaders/spr_loader.h"

bool GameSpriteLoader::LoadSpriteMetadata(SpriteLoader* loader, SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings) {
	if (!loader) {
		error = "SpriteLoader is null in LoadSpriteMetadata";
		return false;
	}
	return DatLoader::LoadMetadata(*loader, db, datafile, error, warnings);
}

bool GameSpriteLoader::LoadSpriteData(SpriteLoader* loader, SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings) {
	if (!loader) {
		error = "SpriteLoader is null in LoadSpriteData";
		return false;
	}
	return SprLoader::LoadData(*loader, db, datafile, error, warnings);
}

bool GameSpriteLoader::LoadSpriteDump(SpriteLoader* loader, std::unique_ptr<uint8_t[]>& target, uint16_t& size, int sprite_id) {
	if (!loader) {
		target = nullptr;
		size = 0;
		return false;
	}
	return SprLoader::LoadDump(*loader, target, size, sprite_id);
}
