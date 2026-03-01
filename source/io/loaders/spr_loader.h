#ifndef RME_IO_LOADERS_SPR_LOADER_H_
#define RME_IO_LOADERS_SPR_LOADER_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/string.h>

class SpriteDatabase;
class FileReadHandle;

class SprLoader {
public:
  static bool LoadData(SpriteDatabase *database, const wxFileName &datafile,
                       wxString &error, std::vector<std::string> &warnings);
  static bool LoadDump(SpriteDatabase *database,
                       std::unique_ptr<uint8_t[]> &target, uint16_t &size,
                       int sprite_id);
  static bool LoadDump(const std::string &filename, bool extended,
                       std::unique_ptr<uint8_t[]> &target, uint16_t &size,
                       int sprite_id);

  static bool ReadSprites(SpriteDatabase *database, FileReadHandle &fh,
                          const std::vector<uint32_t> &sprite_indexes,
                          std::vector<std::string> &warnings, wxString &error);

private:
  static std::vector<uint32_t>
  ReadSpriteIndexes(FileReadHandle &fh, uint32_t total_pics, wxString &error);
};

#endif
