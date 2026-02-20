//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_SPAWN_PROPERTY_PANEL_H_
#define RME_UI_SPAWN_PROPERTY_PANEL_H_

#include "ui/tile_properties/custom_property_panel.h"
#include <wx/spinctrl.h>

class Spawn;

class SpawnPropertyPanel : public CustomPropertyPanel {
public:
	SpawnPropertyPanel(wxWindow* parent);
	virtual ~SpawnPropertyPanel();

	void SetItem(Item* item, Tile* tile, Map* map) override;
	void SetSpawn(Spawn* spawn, Tile* tile, Map* map);

	void OnRadiusChange(wxSpinEvent& event);

protected:
	Spawn* current_spawn;
	Tile* current_tile;
	Map* current_map;

	wxStaticBoxSizer* radius_sizer;
	wxSpinCtrl* radius_spin;
};

#endif
