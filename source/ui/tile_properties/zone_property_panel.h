//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_ZONE_PROPERTY_PANEL_H_
#define RME_UI_ZONE_PROPERTY_PANEL_H_

#include <memory>
#include <wx/combobox.h>
#include <wx/listbox.h>
#include <wx/panel.h>

class Tile;
class Map;
class wxButton;

class ZonePropertyPanel : public wxPanel {
public:
	explicit ZonePropertyPanel(wxWindow* parent);
	~ZonePropertyPanel() override;

	void SetTile(Tile* tile, Map* map);

private:
	void ApplyTileChange(std::unique_ptr<Tile> new_tile, bool refresh_palettes = false);
	void RefreshZoneLists();
	void OnAddZone(wxCommandEvent& event);
	void OnRemoveZone(wxCommandEvent& event);
	void OnClearZones(wxCommandEvent& event);
	void OnAssignedZoneSelected(wxCommandEvent& event);

	Tile* current_tile = nullptr;
	Map* current_map = nullptr;
	wxListBox* assigned_zones = nullptr;
	wxComboBox* zone_input = nullptr;
	wxButton* add_zone_button = nullptr;
	wxButton* remove_zone_button = nullptr;
	wxButton* clear_zones_button = nullptr;
};

#endif
