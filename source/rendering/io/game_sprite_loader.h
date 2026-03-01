//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_IO_GAME_SPRITE_LOADER_H_
#define RME_RENDERING_IO_GAME_SPRITE_LOADER_H_

#include <wx/string.h>
#include <wx/arrstr.h> // For wxArrayString
#include <wx/filename.h> // For FileName if defined, or wxFileName
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

class SpriteLoader;
class SpriteDatabase;
class FileReadHandle;
class GameSprite;

class GameSpriteLoader {
public:
	static bool LoadSpriteMetadata(SpriteLoader* loader, SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings);
	static bool LoadSpriteData(SpriteLoader* loader, SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings);
	static bool LoadSpriteDump(SpriteLoader* loader, std::unique_ptr<uint8_t[]>& target, uint16_t& size, int sprite_id);
};

#endif
