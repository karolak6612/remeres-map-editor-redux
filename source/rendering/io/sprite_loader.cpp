#include "rendering/io/sprite_loader.h"
#include "rendering/core/sprite_database.h"
#include "rendering/io/editor_sprite_loader.h"
#include "rendering/io/game_sprite_loader.h"

void SpriteLoader::clear() {
	std::lock_guard<std::mutex> lock(mutex_);
	client_version = nullptr;
	spritefile = "";
	unloaded = true;
	dat_format = DAT_FORMAT_UNKNOWN;
	is_extended = false;
	has_transparency = false;
	has_frame_durations = false;
	has_frame_groups = false;
}

bool SpriteLoader::loadEditorSprites(SpriteDatabase& db) {
	return EditorSpriteLoader::Load(this, db); 
}

bool SpriteLoader::loadSpriteMetadata(SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings) {
	return GameSpriteLoader::LoadSpriteMetadata(this, db, datafile, error, warnings);
}

bool SpriteLoader::loadSpriteData(SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings) {
	return GameSpriteLoader::LoadSpriteData(this, db, datafile, error, warnings);
}

bool SpriteLoader::loadSpriteDump(std::unique_ptr<uint8_t[]>& target, uint16_t& size, int sprite_id) {
	return GameSpriteLoader::LoadSpriteDump(this, target, size, sprite_id);
}
