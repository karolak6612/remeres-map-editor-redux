#include "io/loaders/dat_loader.h"

#include "item_definitions/formats/dat/dat_metadata_decoder.h"

bool DatLoader::LoadMetadata(GraphicManager* manager, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings) {
	return DatMetadataDecoder::decodeIntoGraphics(manager, datafile, error, warnings);
}
