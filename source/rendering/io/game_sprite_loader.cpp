//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "io/loaders/dat_loader.h"
#include "io/loaders/spr_loader.h"
#include "rendering/io/game_sprite_loader.h"


bool GameSpriteLoader::LoadSpriteMetadata(SpriteDatabase *database,
                                          const wxFileName &datafile,
                                          wxString &error,
                                          std::vector<std::string> &warnings) {
  return DatLoader::LoadMetadata(database, datafile, error, warnings);
}

bool GameSpriteLoader::LoadSpriteData(SpriteDatabase *database,
                                      const wxFileName &datafile,
                                      wxString &error,
                                      std::vector<std::string> &warnings) {
  return SprLoader::LoadData(database, datafile, error, warnings);
}

bool GameSpriteLoader::LoadSpriteDump(SpriteDatabase *database,
                                      std::unique_ptr<uint8_t[]> &target,
                                      uint16_t &size, int sprite_id) {
  return SprLoader::LoadDump(database, target, size, sprite_id);
}
