#ifndef RME_DAT_METADATA_DECODER_H_
#define RME_DAT_METADATA_DECODER_H_

#include "app/client_version.h"

#include <wx/filename.h>
#include <wx/string.h>

#include <string>
#include <vector>

class GraphicManager;
class FileReadHandle;
class GameSprite;

class DatMetadataDecoder {
public:
	static bool decodeIntoGraphics(GraphicManager* manager, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings);

private:
	static bool readFlagPayload(DatFormat format, FileReadHandle& file, GameSprite* sprite, uint8_t flag, uint8_t previous_flag, std::vector<std::string>& warnings);
	static bool readFlags(DatFormat format, FileReadHandle& file, GameSprite* sprite, uint32_t sprite_id, std::vector<std::string>& warnings);
	static bool readSpriteGroup(GraphicManager* manager, FileReadHandle& file, GameSprite* sprite, uint32_t group_index);
};

#endif
