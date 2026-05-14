#include "editor/persistence/tileset_exporter.h"

#include "ui/dialog_util.h"

void TilesetExporter::exportTilesets(const FileName& directory, const std::string& filename) {
	(void)directory;
	(void)filename;
	DialogUtil::PopupDialog("Export Tilesets", "Tileset export targets the removed legacy palette model and is disabled for the modular data runtime.", wxOK);
}
