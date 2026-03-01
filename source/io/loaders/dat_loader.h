#ifndef RME_IO_LOADERS_DAT_LOADER_H_
#define RME_IO_LOADERS_DAT_LOADER_H_

#include "app/client_version.h"
#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/string.h>


class SpriteDatabase;
class FileReadHandle;
class GameSprite;

class DatLoader {
public:
  static bool LoadMetadata(SpriteDatabase *database, const wxFileName &datafile,
                           wxString &error, std::vector<std::string> &warnings);

private:
  static bool ReadSpriteGroup(SpriteDatabase *database, FileReadHandle &file,
                              GameSprite *sType, uint32_t group_index,
                              std::vector<std::string> &warnings);
};

#endif
