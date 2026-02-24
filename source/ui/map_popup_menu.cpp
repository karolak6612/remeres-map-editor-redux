//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "ui/map_popup_menu.h"

#include "editor/editor.h"
#include "ui/gui.h"
#include "brushes/brush.h"
#include "game/sprites.h"
#include "map/map.h"
#include "map/tile.h"
#include "util/image_manager.h"
#include "ui/properties/properties_window.h"
#include "ui/properties/old_properties_window.h"
#include "ui/tileset_window.h"
#include "ui/browse_tile_window.h"

#include "brushes/doodad/doodad_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "brushes/house/house_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "brushes/raw/raw_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include "brushes/table/table_brush.h"

// Helper: create a menu item with bitmap set BEFORE appending (required for GTK3)
static wxMenuItem* AppendWithBitmap(wxMenu* menu, int id, const wxString& text, const wxString& help, std::string_view icon) {
	wxMenuItem* item = new wxMenuItem(menu, id, text, help);
	item->SetBitmap(IMAGE_MANAGER.GetBitmap(icon, wxSize(16, 16)));
	menu->Append(item);
	return item;
}

MapPopupMenu::MapPopupMenu(Editor& editor) :
	wxMenu(""), editor(editor) {
	////
}

MapPopupMenu::~MapPopupMenu() {
	////
}

void MapPopupMenu::Update() {
	// Clear the menu of all items
	while (GetMenuItemCount() != 0) {
		wxMenuItem* m_item = FindItemByPosition(0);
		// If you add a submenu, this won't delete it.
		Delete(m_item);
	}

	bool anything_selected = editor.selection.size() != 0;

	wxMenuItem* cutItem = AppendWithBitmap(this, MAP_POPUP_MENU_CUT, "&Cut\tCTRL+X", "Cut out all selected items", ICON_CUT);
	cutItem->Enable(anything_selected);

	wxMenuItem* copyItem = AppendWithBitmap(this, MAP_POPUP_MENU_COPY, "&Copy\tCTRL+C", "Copy all selected items", ICON_COPY);
	copyItem->Enable(anything_selected);

	wxMenuItem* copyPositionItem = AppendWithBitmap(this, MAP_POPUP_MENU_COPY_POSITION, "&Copy Position", "Copy the position as a lua table", ICON_LOCATION);
	copyPositionItem->Enable(anything_selected);

	wxMenuItem* pasteItem = AppendWithBitmap(this, MAP_POPUP_MENU_PASTE, "&Paste\tCTRL+V", "Paste items in the copybuffer here", ICON_PASTE);
	pasteItem->Enable(editor.copybuffer.canPaste());

	wxMenuItem* deleteItem = AppendWithBitmap(this, MAP_POPUP_MENU_DELETE, "&Delete\tDEL", "Removes all seleceted items", ICON_TRASH_CAN);
	deleteItem->Enable(anything_selected);

	if (anything_selected) {
		AppendWithBitmap(this, MAP_POPUP_MENU_ADVANCED_REPLACE, "Replace tiles...", "Open Advanced Replace Tool for selected items", ICON_WAND_MAGIC);
	}

	if (anything_selected) {
		if (editor.selection.size() == 1) {
			Tile* tile = editor.selection.getSelectedTile();
			ItemVector selected_items = tile->getSelectedItems();

			bool hasWall = false;
			bool hasCarpet = false;
			bool hasTable = false;
			bool hasCollection = false;
			Item* topItem = nullptr;
			Item* topSelectedItem = (selected_items.size() == 1 ? selected_items.back() : nullptr);
			Creature* topCreature = tile->creature.get();
			Spawn* topSpawn = tile->spawn.get();

			for (const auto& item : tile->items) {
				if (item->isWall()) {
					Brush* wb = item->getWallBrush();
					if (wb && wb->visibleInPalette()) {
						hasWall = true;
						hasCollection = hasCollection || wb->hasCollection();
					}
				}
				if (item->isTable()) {
					Brush* tb = item->getTableBrush();
					if (tb && tb->visibleInPalette()) {
						hasTable = true;
						hasCollection = hasCollection || tb->hasCollection();
					}
				}
				if (item->isCarpet()) {
					Brush* cb = item->getCarpetBrush();
					if (cb && cb->visibleInPalette()) {
						hasCarpet = true;
						hasCollection = hasCollection || cb->hasCollection();
					}
				}
				if (Brush* db = item->getDoodadBrush()) {
					hasCollection = hasCollection || db->hasCollection();
				}
				if (item->isSelected()) {
					topItem = item.get();
				}
			}
			if (!topItem) {
				topItem = tile->ground.get();
			}

			AppendSeparator();

			if (topSelectedItem) {
				AppendWithBitmap(this, MAP_POPUP_MENU_COPY_SERVER_ID, "Copy Item Server Id", "Copy the server id of this item", ICON_SERVER);
				AppendWithBitmap(this, MAP_POPUP_MENU_COPY_CLIENT_ID, "Copy Item Client Id", "Copy the client id of this item", ICON_DESKTOP);
				AppendWithBitmap(this, MAP_POPUP_MENU_COPY_NAME, "Copy Item Name", "Copy the name of this item", ICON_TAG);
				AppendSeparator();
			}

			if (topSelectedItem || topCreature || topItem) {
				Teleport* teleport = dynamic_cast<Teleport*>(topSelectedItem);
				if (topSelectedItem && (topSelectedItem->isBrushDoor() || topSelectedItem->isRoteable() || teleport)) {

					if (topSelectedItem->isRoteable()) {
						AppendWithBitmap(this, MAP_POPUP_MENU_ROTATE, "&Rotate item", "Rotate this item", ICON_ROTATE);
					}

					if (teleport && teleport->hasDestination()) {
						AppendWithBitmap(this, MAP_POPUP_MENU_GOTO, "&Go To Destination", "Go to the destination of this teleport", ICON_ARROW_RIGHT_TO_BRACKET);
					}

					if (topSelectedItem->isDoor()) {
						if (topSelectedItem->isOpen()) {
							AppendWithBitmap(this, MAP_POPUP_MENU_SWITCH_DOOR, "&Close door", "Close this door", ICON_DOOR_CLOSED);
						} else {
							AppendWithBitmap(this, MAP_POPUP_MENU_SWITCH_DOOR, "&Open door", "Open this door", ICON_DOOR_OPEN);
						}
						AppendSeparator();
					}
				}

				if (topCreature) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush", ICON_DRAGON);
				}

				if (topSpawn) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, "Select Spawn", "Select the spawn brush", ICON_FIRE);
				}

				AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_RAW_BRUSH, "Select RAW", "Uses the top item as a RAW brush", ICON_CUBE);

				if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR)) {
					AppendWithBitmap(this, MAP_POPUP_MENU_MOVE_TO_TILESET, "Move To Tileset", "Move this item to any tileset", ICON_SHARE_FROM_SQUARE);
				}

				if (hasWall) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush", ICON_DUNGEON);
				}

				if (hasCarpet) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_CARPET_BRUSH, "Select Carpetbrush", "Uses the current item as a carpetbrush", ICON_RUG);
				}

				if (hasTable) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_TABLE_BRUSH, "Select Tablebrush", "Uses the current item as a tablebrush", ICON_TABLE);
				}

				if (topSelectedItem && topSelectedItem->getDoodadBrush() && topSelectedItem->getDoodadBrush()->visibleInPalette()) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, "Select Doodadbrush", "Use this doodad brush", ICON_TREE);
				}

				if (topSelectedItem && topSelectedItem->isBrushDoor() && topSelectedItem->getDoorBrush()) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_DOOR_BRUSH, "Select Doorbrush", "Use this door brush", ICON_DOOR_CLOSED);
				}

				if (tile->hasGround() && tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current item as a groundbrush", ICON_LAYER_GROUP);
				}

				if (hasCollection || topSelectedItem && topSelectedItem->hasCollectionBrush() || tile->getGroundBrush() && tile->getGroundBrush()->hasCollection()) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, "Select Collection", "Use this collection", ICON_LAYER_GROUP);
				}

				if (tile->isHouseTile()) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, "Select House", "Draw with the house on this tile.", ICON_HOUSE);
				}

				AppendSeparator();
				AppendWithBitmap(this, MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object", ICON_GEAR);
			} else {

				if (topCreature) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush", ICON_DRAGON);
				}

				if (topSpawn) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, "Select Spawn", "Select the spawn brush", ICON_FIRE);
				}

				AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_RAW_BRUSH, "Select RAW", "Uses the top item as a RAW brush", ICON_CUBE);
				if (hasWall) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush", ICON_DUNGEON);
				}
				if (tile->hasGround() && tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current tile as a groundbrush", ICON_LAYER_GROUP);
				}

				if (hasCollection || tile->getGroundBrush() && tile->getGroundBrush()->hasCollection()) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, "Select Collection", "Use this collection", ICON_LAYER_GROUP);
				}

				if (tile->isHouseTile()) {
					AppendWithBitmap(this, MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, "Select House", "Draw with the house on this tile.", ICON_HOUSE);
				}

				if (tile->hasGround() || topCreature || topSpawn) {
					AppendSeparator();
					AppendWithBitmap(this, MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object", ICON_GEAR);
				}
			}

			AppendSeparator();

			wxMenuItem* browseTile = AppendWithBitmap(this, MAP_POPUP_MENU_BROWSE_TILE, "Browse Field", "Navigate from tile items", ICON_SEARCH);
			browseTile->Enable(anything_selected);

			wxMenuItem* tileProps = AppendWithBitmap(this, MAP_POPUP_MENU_TILE_PROPERTIES, "Tile Properties", "Show tile properties panel", ICON_LIST);
			tileProps->Enable(anything_selected);
		}
	}
}
