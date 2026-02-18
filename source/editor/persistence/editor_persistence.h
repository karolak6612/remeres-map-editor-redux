#ifndef RME_EDITOR_PERSISTENCE_H
#define RME_EDITOR_PERSISTENCE_H

#include "util/common.h"
#include "app/rme_forward_declarations.h"
#include "io/iomap.h"
#include <wx/string.h>
#include <unordered_map>
#include <map>

class Editor;
class Map;
class Spawn;
struct Position;

class EditorPersistence {
public:
	static void saveMap(Editor& editor, FileName filename, bool showdialog);
	static void loadMap(Editor& editor, const FileName& filename);
	static bool importMap(Editor& editor, FileName filename, int import_x_offset, int import_y_offset, ImportType house_import_type, ImportType spawn_import_type);
	static bool importMiniMap(Editor& editor, FileName filename, int import, int import_x_offset, int import_y_offset, int import_z_offset);

private:
	static void importTowns(Editor& editor, Map& imported_map, const Position& offset, ImportType house_import_type, std::unordered_map<uint32_t, uint32_t>& town_id_map);
	static void importHouses(Editor& editor, Map& imported_map, const Position& offset, ImportType house_import_type, const std::unordered_map<uint32_t, uint32_t>& town_id_map, std::unordered_map<uint32_t, uint32_t>& house_id_map);
	static void importSpawns(Editor& editor, Map& imported_map, const Position& offset, ImportType spawn_import_type, std::map<Position, std::unique_ptr<Spawn>>& spawn_map);
};

#endif
