//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_IO_GAME_SPRITE_LOADER_H_
#define RME_RENDERING_IO_GAME_SPRITE_LOADER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <wx/arrstr.h>   // For wxArrayString
#include <wx/filename.h> // For FileName if defined, or wxFileName
#include <wx/string.h>


class SpriteDatabase;
class FileReadHandle;
class GameSprite;

class GameSpriteLoader {
public:
  static bool LoadSpriteMetadata(SpriteDatabase *database,
                                 const wxFileName &datafile, wxString &error,
                                 std::vector<std::string> &warnings);
  static bool LoadSpriteData(SpriteDatabase *database,
                             const wxFileName &datafile, wxString &error,
                             std::vector<std::string> &warnings);
  static bool LoadSpriteDump(SpriteDatabase *database,
                             std::unique_ptr<uint8_t[]> &target, uint16_t &size,
                             int sprite_id);
};

#endif
