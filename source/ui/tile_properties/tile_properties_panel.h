//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_TILE_PROPERTIES_PANEL_H_
#define RME_UI_TILE_PROPERTIES_PANEL_H_

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/splitter.h>

class Tile;
class Map;
class BrowseFieldList;
class SpawnCreaturePanel;
class MapFlagsPanel;
class ItemPropertyPanel;
class ContainerPropertyPanel;
class DepotPropertyPanel;
class TeleportPropertyPanel;
class DoorPropertyPanel;
class SpawnPropertyPanel;
class CreaturePropertyPanel;

class TilePropertiesPanel : public wxPanel {
public:
	TilePropertiesPanel(wxWindow* parent);
	virtual ~TilePropertiesPanel();

	void SetTile(Tile* tile, Map* map);
	void SelectItem(Item* item);
	void OnItemSelected(Item* item);
	void OnSpawnSelected();
	void OnCreatureSelected();

protected:
	wxSplitterWindow* splitter;
	wxPanel* left_panel;
	wxBoxSizer* left_sizer;
	wxPanel* right_panel;
	wxBoxSizer* right_sizer;

	BrowseFieldList* browse_field_list;
	SpawnCreaturePanel* spawn_creature_panel;
	MapFlagsPanel* map_flags_panel;
	ItemPropertyPanel* item_property_panel;
	ContainerPropertyPanel* container_property_panel;
	DepotPropertyPanel* depot_property_panel;
	TeleportPropertyPanel* teleport_property_panel;
	DoorPropertyPanel* door_property_panel;
	SpawnPropertyPanel* spawn_property_panel;
	CreaturePropertyPanel* creature_property_panel;
	wxStaticText* placeholder_text;

	Tile* current_tile;
	Map* current_map;
};

#endif
