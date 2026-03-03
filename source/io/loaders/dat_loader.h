#ifndef RME_IO_LOADERS_DAT_LOADER_H_
#define RME_IO_LOADERS_DAT_LOADER_H_

#include "app/client_version.h"
#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/string.h>

class SpriteLoader;
class SpriteDatabase;
class FileReadHandle;
class GameSprite;

class DatLoader {
public:
    static bool
    LoadMetadata(SpriteLoader& loader, SpriteDatabase& db, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings);

private:
    static bool ReadSpriteGroup(
        SpriteLoader& loader, SpriteDatabase& db, FileReadHandle& file, uint32_t id, uint32_t group_index,
        std::vector<std::string>& warnings
    );
};

#endif
